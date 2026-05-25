#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <gl/GL.h>
#include <mmsystem.h>

#ifdef OVTR_HAS_OPENVR_SDK
#include <openvr.h>
#endif

#include "data/SessionTypes.h"
#include "export/FbxAsciiExporter.h"
#include "export/GltfExporter.h"
#include "recording/RecordingController.h"
#include "recording/SamplingScheduler.h"
#include "util/SteamVRRuntime.h"
#include "vr/OpenVRProvider.h"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

constexpr UINT_PTR kStatusTimerId = 1;
constexpr UINT kStatusIntervalMs = 1000;
constexpr double kTargetViewportFps = 90.0;
constexpr double kFbxExportSampleRate = 60.0;
constexpr float kMinimumCameraDistance = 0.08f;
constexpr float kViewportFovDegrees = 48.0f;
constexpr float kRenderModelOutlinePixels = 2.6f;
constexpr std::uint32_t kNoSelectedRuntimeIndex = 0xffffffffu;
constexpr const wchar_t* kMainWindowClassName = L"OpenVRTrackerRecorderBootstrapWindow";
constexpr const wchar_t* kViewportWindowClassName = L"OpenVRTrackerRecorderGLViewport";

enum class ExportFormat {
    Fbx,
    Glb,
};

struct RenderModelVertex {
    std::array<float, 3> position{0.0f, 0.0f, 0.0f};
    std::array<float, 3> normal{0.0f, 1.0f, 0.0f};
    std::array<float, 2> texCoord{0.0f, 0.0f};
};

struct RenderModelMesh {
    enum class LoadState {
        Pending,
        Ready,
        Failed,
    };

    LoadState state = LoadState::Pending;
    std::vector<RenderModelVertex> vertices;
    std::vector<std::uint16_t> indices;
    int diffuseTextureId = -1;
    GLuint textureId = 0;
    bool textureAvailable = false;
    bool textureLoadFailed = false;
};

struct AppWindowState {
    ovtr::SteamVRRuntime runtime;
    ovtr::SteamVRRuntimeStatus status;
    ovtr::OpenVRProvider provider;
    std::vector<ovtr::DeviceDescriptor> devices;
    ovtr::PosePollResult poses;
    std::string providerError;
    HWND glWindow = nullptr;
    HDC glDeviceContext = nullptr;
    HGLRC glContext = nullptr;
    GLuint glLabelFontBase = 0;
    std::chrono::steady_clock::time_point lastFpsUpdate = std::chrono::steady_clock::now();
    std::uint64_t posePollFrames = 0;
    std::uint64_t renderFrames = 0;
    double posePollFps = 0.0;
    double renderFps = 0.0;
    std::chrono::steady_clock::time_point lastDeviceEnumeration{};
    double targetViewportFps = kTargetViewportFps;
    float cameraYawDegrees = 42.0f;
    float cameraPitchDegrees = 28.0f;
    float cameraDistance = 5.5f;
    float cameraPanX = 0.0f;
    float cameraPanY = 0.0f;
    float cameraPanZ = 0.0f;
    bool orbitDragging = false;
    bool panDragging = false;
    int lastMouseX = 0;
    int lastMouseY = 0;
    ovtr::RecordingController recorder;
    ovtr::SamplingScheduler recordingScheduler{kTargetViewportFps};
    std::chrono::steady_clock::time_point recordingStart = std::chrono::steady_clock::now();
    std::uint64_t recordingDroppedFrames = 0;
    std::filesystem::path currentSessionFolder;
    std::string recordingError;
    std::string exportStatusMessage;
    bool originEnabled = false;
    std::array<float, 3> originOffset{0.0f, 0.0f, 0.0f};
    std::uint32_t selectedOriginRuntimeIndex = kNoSelectedRuntimeIndex;
    std::string originStatusMessage;
    std::unordered_map<std::string, RenderModelMesh> renderModelCache;
};

std::wstring widen(const std::string& value)
{
    if (value.empty()) {
        return {};
    }

    const int required = MultiByteToWideChar(
        CP_UTF8,
        0,
        value.data(),
        static_cast<int>(value.size()),
        nullptr,
        0
    );

    if (required <= 0) {
        return {};
    }

    std::wstring output(static_cast<std::size_t>(required), L'\0');
    MultiByteToWideChar(
        CP_UTF8,
        0,
        value.data(),
        static_cast<int>(value.size()),
        output.data(),
        required
    );
    return output;
}

std::wstring yesNo(const bool value)
{
    return value ? L"Yes" : L"No";
}

std::wstring recorderStateText(const ovtr::RecorderState state)
{
    switch (state) {
    case ovtr::RecorderState::Idle:
        return L"Idle";
    case ovtr::RecorderState::Starting:
        return L"Starting";
    case ovtr::RecorderState::Recording:
        return L"Recording";
    case ovtr::RecorderState::Paused:
        return L"Paused";
    case ovtr::RecorderState::Stopping:
        return L"Stopping";
    case ovtr::RecorderState::Finalizing:
        return L"Finalizing";
    case ovtr::RecorderState::Error:
        return L"Error";
    }

    return L"Unknown";
}

std::string localTimestampForPath()
{
    const std::time_t currentTime = std::time(nullptr);
    std::tm localTime{};
    localtime_s(&localTime, &currentTime);

    std::ostringstream stream;
    stream << std::put_time(&localTime, "%Y%m%d_%H%M%S");
    return stream.str();
}

std::string utcTimestampIso()
{
    const std::time_t currentTime = std::time(nullptr);
    std::tm utcTime{};
    gmtime_s(&utcTime, &currentTime);

    std::ostringstream stream;
    stream << std::put_time(&utcTime, "%Y-%m-%dT%H:%M:%SZ");
    return stream.str();
}

std::string poseSummaryForDevice(
    const ovtr::PosePollResult& poses,
    const std::uint32_t runtimeIndex,
    const std::array<float, 3>& originOffset
)
{
    for (const ovtr::PoseSample& pose : poses.poses) {
        if (pose.runtimeIndex != runtimeIndex) {
            continue;
        }

        const std::array<float, 3> position{
            pose.position[0] - originOffset[0],
            pose.position[1] - originOffset[1],
            pose.position[2] - originOffset[2],
        };

        std::ostringstream stream;
        stream << std::fixed << std::setprecision(3)
               << "pos=(" << position[0] << ", " << position[1] << ", " << position[2] << ") "
               << "rot=(" << pose.rotation[0] << ", " << pose.rotation[1] << ", "
               << pose.rotation[2] << ", " << pose.rotation[3] << ")";
        if ((pose.flags & ovtr::PoseFlagPoseValid) == 0) {
            stream << " invalid";
        }
        return stream.str();
    }

    return "pose unavailable";
}

std::vector<std::wstring> makeStatusLines(const AppWindowState& state)
{
    const ovtr::SteamVRRuntimeStatus& status = state.status;
    std::vector<std::wstring> lines;
    lines.emplace_back(L"OpenVR Tracker Recorder");
    lines.emplace_back(L"");
    lines.emplace_back(L"OpenVR device monitor");
    lines.emplace_back(L"");
    lines.emplace_back(L"SteamVR runtime DLL loaded: " + yesNo(status.dllLoaded));
    lines.emplace_back(L"OpenVR runtime installed: " + yesNo(status.runtimeInstalled));
    lines.emplace_back(L"HMD present: " + yesNo(status.hmdPresent));
    lines.emplace_back(L"OpenVR provider initialized: " + yesNo(state.provider.isInitialized()));
    {
        std::wostringstream stream;
        stream << std::fixed << std::setprecision(1)
               << L"Pose polling FPS: " << state.posePollFps
               << L"   Viewport FPS: " << state.renderFps
               << L"   Target: " << state.targetViewportFps;
        lines.emplace_back(stream.str());
    }
    lines.emplace_back(L"");
    lines.emplace_back(L"DLL path: " + widen(status.dllPath));
    lines.emplace_back(L"Runtime path: " + widen(status.runtimePath));
    lines.emplace_back(L"");

    if (!status.error.empty()) {
        lines.emplace_back(L"Status: " + widen(status.error));
    } else if (!status.runtimeInstalled) {
        lines.emplace_back(L"Status: SteamVR runtime was not detected.");
    } else if (!status.hmdPresent) {
        lines.emplace_back(L"Status: Runtime detected. No HMD currently reported.");
    } else {
        lines.emplace_back(L"Status: Runtime and HMD detected.");
    }

    if (!state.providerError.empty()) {
        lines.emplace_back(L"OpenVR provider: " + widen(state.providerError));
    }

    lines.emplace_back(L"");
    {
        std::wostringstream stream;
        stream << L"Recording: " << recorderStateText(state.recorder.state())
               << L"   Frames: " << state.recorder.frameCount()
               << L"   Dropped: " << state.recordingDroppedFrames;
        lines.emplace_back(stream.str());
    }
    if (!state.currentSessionFolder.empty()) {
        lines.emplace_back(L"Session: " + widen(state.currentSessionFolder.string()));
    }
    if (!state.recordingError.empty()) {
        lines.emplace_back(L"Recording error: " + widen(state.recordingError));
    }
    if (!state.exportStatusMessage.empty()) {
        lines.emplace_back(L"Export: " + widen(state.exportStatusMessage));
    }

    {
        std::wostringstream stream;
        stream << L"Origin: " << (state.originEnabled ? L"Enabled" : L"Disabled");
        const ovtr::DeviceDescriptor* selected = nullptr;
        for (const ovtr::DeviceDescriptor& device : state.devices) {
            if (device.runtimeIndex == state.selectedOriginRuntimeIndex) {
                selected = &device;
                break;
            }
        }
        if (selected != nullptr) {
            std::ostringstream name;
            name << "#" << selected->runtimeIndex << " " << ovtr::toString(selected->deviceClass);
            if (!selected->serial.empty()) {
                name << " " << selected->serial;
            }
            stream << L"   Selected: " << widen(name.str());
        }
        if (state.originEnabled) {
            stream << std::fixed << std::setprecision(3)
                   << L"   Offset: (" << state.originOffset[0]
                   << L", " << state.originOffset[1]
                   << L", " << state.originOffset[2] << L")";
        }
        lines.emplace_back(stream.str());
    }
    if (!state.originStatusMessage.empty()) {
        lines.emplace_back(L"Origin status: " + widen(state.originStatusMessage));
    }

    lines.emplace_back(L"");
    lines.emplace_back(L"Tracked devices: " + std::to_wstring(state.devices.size()));
    const std::array<float, 3> activeOriginOffset = state.originEnabled
        ? state.originOffset
        : std::array<float, 3>{0.0f, 0.0f, 0.0f};
    for (const ovtr::DeviceDescriptor& device : state.devices) {
        std::ostringstream stream;
        stream << "#" << device.runtimeIndex
               << " " << ovtr::toString(device.deviceClass)
               << " serial=" << (device.serial.empty() ? "(none)" : device.serial)
               << " model=" << (device.modelName.empty() ? "(unknown)" : device.modelName)
               << " " << poseSummaryForDevice(state.poses, device.runtimeIndex, activeOriginOffset);
        lines.emplace_back(L"  " + widen(stream.str()));
    }

    lines.emplace_back(L"");
    lines.emplace_back(L"Viewport: perspective 3D, X red, Y green, Z blue.");
    lines.emplace_back(L"Left drag orbit, middle drag pan, wheel dolly, Tab select origin, O set, C clear, R record, E export FBX, G export GLB, Esc exit.");
    return lines;
}

float clampFloat(const float value, const float minValue, const float maxValue)
{
    if (value < minValue) {
        return minValue;
    }
    if (value > maxValue) {
        return maxValue;
    }
    return value;
}

float positiveCameraDistance(const float distance)
{
    return distance < kMinimumCameraDistance ? kMinimumCameraDistance : distance;
}

struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

Vec3 normalizeVec3(const Vec3 value)
{
    const float length = std::sqrt(value.x * value.x + value.y * value.y + value.z * value.z);
    if (length <= 0.000001f) {
        return {};
    }
    return {value.x / length, value.y / length, value.z / length};
}

Vec3 rotateByInverseViewRotation(const AppWindowState& state, const Vec3 cameraVector)
{
    const float yaw = state.cameraYawDegrees * 3.14159265359f / 180.0f;
    const float pitch = state.cameraPitchDegrees * 3.14159265359f / 180.0f;
    const float cosPitch = std::cos(-pitch);
    const float sinPitch = std::sin(-pitch);
    const float cosYaw = std::cos(-yaw);
    const float sinYaw = std::sin(-yaw);

    // The view transform is Rx(pitch) * Ry(yaw) * T(-pan).
    // A screen-space direction must be converted back to world space with
    // inverse(view rotation) = Ry(-yaw) * Rx(-pitch).
    const Vec3 afterPitch{
        cameraVector.x,
        cameraVector.y * cosPitch - cameraVector.z * sinPitch,
        cameraVector.y * sinPitch + cameraVector.z * cosPitch,
    };

    return normalizeVec3({
        afterPitch.x * cosYaw + afterPitch.z * sinYaw,
        afterPitch.y,
        -afterPitch.x * sinYaw + afterPitch.z * cosYaw,
    });
}

void applyScreenSpacePan(AppWindowState& state, const int dx, const int dy)
{
    const Vec3 right = rotateByInverseViewRotation(state, {1.0f, 0.0f, 0.0f});
    const Vec3 up = rotateByInverseViewRotation(state, {0.0f, 1.0f, 0.0f});

    const float panScale = 0.0018f * positiveCameraDistance(state.cameraDistance);
    const float moveRight = -static_cast<float>(dx) * panScale;
    const float moveUp = static_cast<float>(dy) * panScale;

    state.cameraPanX += right.x * moveRight + up.x * moveUp;
    state.cameraPanY += right.y * moveRight + up.y * moveUp;
    state.cameraPanZ += right.z * moveRight + up.z * moveUp;
}

void applyCameraDolly(AppWindowState& state, const float distance)
{
    const Vec3 forward = rotateByInverseViewRotation(state, {0.0f, 0.0f, -1.0f});
    state.cameraPanX += forward.x * distance;
    state.cameraPanY += forward.y * distance;
    state.cameraPanZ += forward.z * distance;
}

float cameraDepthForWorldPoint(const AppWindowState& state, const Vec3 point)
{
    const float yaw = state.cameraYawDegrees * 3.14159265359f / 180.0f;
    const float pitch = state.cameraPitchDegrees * 3.14159265359f / 180.0f;
    const float cosYaw = std::cos(yaw);
    const float sinYaw = std::sin(yaw);
    const float cosPitch = std::cos(pitch);
    const float sinPitch = std::sin(pitch);

    const Vec3 translated{
        point.x - state.cameraPanX,
        point.y - state.cameraPanY,
        point.z - state.cameraPanZ,
    };
    const Vec3 afterYaw{
        translated.x * cosYaw + translated.z * sinYaw,
        translated.y,
        -translated.x * sinYaw + translated.z * cosYaw,
    };
    const float cameraSpaceZ =
        afterYaw.y * sinPitch + afterYaw.z * cosPitch - positiveCameraDistance(state.cameraDistance);
    return cameraSpaceZ < -0.001f ? -cameraSpaceZ : 0.001f;
}

float outlineExpansionForDepth(const float cameraDepth, const int viewportHeight)
{
    if (viewportHeight <= 0) {
        return 0.0f;
    }

    const float fovRadians = kViewportFovDegrees * 3.14159265359f / 180.0f;
    const float worldHeight = 2.0f * cameraDepth * std::tan(fovRadians * 0.5f);
    return worldHeight * kRenderModelOutlinePixels / static_cast<float>(viewportHeight);
}

int leftPanelWidthForClient(const int clientWidth)
{
    const int proportional = static_cast<int>(static_cast<float>(clientWidth) * 0.36f);
    if (proportional < 520) {
        return 520;
    }
    if (proportional > 760) {
        return 760;
    }
    return proportional;
}

void perspectiveMatrix(float fovyDegrees, float aspect, float nearPlane, float farPlane, float* out)
{
    const float radians = fovyDegrees * 3.14159265359f / 180.0f;
    const float f = 1.0f / std::tan(radians * 0.5f);

    for (int i = 0; i < 16; ++i) {
        out[i] = 0.0f;
    }

    out[0] = f / aspect;
    out[5] = f;
    out[10] = (farPlane + nearPlane) / (nearPlane - farPlane);
    out[11] = -1.0f;
    out[14] = (2.0f * farPlane * nearPlane) / (nearPlane - farPlane);
}

void drawGroundGrid3D()
{
    glLineWidth(1.0f);
    glColor3f(0.78f, 0.79f, 0.82f);
    glBegin(GL_LINES);
    for (int i = -10; i <= 10; ++i) {
        const float value = static_cast<float>(i) * 0.5f;
        glVertex3f(value, 0.0f, -5.0f);
        glVertex3f(value, 0.0f, 5.0f);
        glVertex3f(-5.0f, 0.0f, value);
        glVertex3f(5.0f, 0.0f, value);
    }
    glEnd();
}

void drawAxes3D()
{
    constexpr float axisLength = 0.35f;

    glLineWidth(1.25f);
    glBegin(GL_LINES);
    glColor3f(0.55f, 0.16f, 0.16f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(axisLength, 0.0f, 0.0f);
    glColor3f(0.18f, 0.55f, 0.22f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, axisLength, 0.0f);
    glColor3f(0.20f, 0.30f, 0.65f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, axisLength);
    glEnd();
}

void multiplyOpenGLMatrixFromQuaternion(const std::array<float, 4>& q)
{
    const float x = q[0];
    const float y = q[1];
    const float z = q[2];
    const float w = q[3];

    const float xx = x * x;
    const float yy = y * y;
    const float zz = z * z;
    const float xy = x * y;
    const float xz = x * z;
    const float yz = y * z;
    const float wx = w * x;
    const float wy = w * y;
    const float wz = w * z;

    const float matrix[16] = {
        1.0f - 2.0f * (yy + zz), 2.0f * (xy + wz), 2.0f * (xz - wy), 0.0f,
        2.0f * (xy - wz), 1.0f - 2.0f * (xx + zz), 2.0f * (yz + wx), 0.0f,
        2.0f * (xz + wy), 2.0f * (yz - wx), 1.0f - 2.0f * (xx + yy), 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    };

    glMultMatrixf(matrix);
}

ovtr::DeviceClass deviceClassForRuntimeIndex(
    const std::vector<ovtr::DeviceDescriptor>& devices,
    const std::uint32_t runtimeIndex
)
{
    for (const ovtr::DeviceDescriptor& device : devices) {
        if (device.runtimeIndex == runtimeIndex) {
            return device.deviceClass;
        }
    }
    return ovtr::DeviceClass::Other;
}

const ovtr::DeviceDescriptor* deviceForRuntimeIndex(
    const std::vector<ovtr::DeviceDescriptor>& devices,
    const std::uint32_t runtimeIndex
)
{
    for (const ovtr::DeviceDescriptor& device : devices) {
        if (device.runtimeIndex == runtimeIndex) {
            return &device;
        }
    }
    return nullptr;
}

const ovtr::PoseSample* poseForRuntimeIndex(
    const ovtr::PosePollResult& poses,
    const std::uint32_t runtimeIndex
)
{
    for (const ovtr::PoseSample& pose : poses.poses) {
        if (pose.runtimeIndex == runtimeIndex) {
            return &pose;
        }
    }
    return nullptr;
}

bool isPoseValid(const ovtr::PoseSample& pose)
{
    return (pose.flags & ovtr::PoseFlagPoseValid) != 0;
}

ovtr::PoseSample applyOriginToPose(
    ovtr::PoseSample pose,
    const bool originEnabled,
    const std::array<float, 3>& originOffset
)
{
    if (originEnabled) {
        pose.position[0] -= originOffset[0];
        pose.position[1] -= originOffset[1];
        pose.position[2] -= originOffset[2];
    }
    return pose;
}

ovtr::PosePollResult applyOriginToPoses(
    ovtr::PosePollResult poses,
    const bool originEnabled,
    const std::array<float, 3>& originOffset
)
{
    if (!originEnabled) {
        return poses;
    }

    for (ovtr::PoseSample& pose : poses.poses) {
        pose = applyOriginToPose(pose, true, originOffset);
    }
    return poses;
}

std::string deviceDisplayName(const ovtr::DeviceDescriptor& device)
{
    std::ostringstream stream;
    stream << "#" << device.runtimeIndex << " " << ovtr::toString(device.deviceClass);
    if (!device.serial.empty()) {
        stream << " " << device.serial;
    }
    return stream.str();
}

const ovtr::DeviceDescriptor* selectedOriginDevice(const AppWindowState& state)
{
    return deviceForRuntimeIndex(state.devices, state.selectedOriginRuntimeIndex);
}

void ensureOriginSelection(AppWindowState& state)
{
    if (state.devices.empty()) {
        state.selectedOriginRuntimeIndex = kNoSelectedRuntimeIndex;
        return;
    }

    if (selectedOriginDevice(state) == nullptr) {
        state.selectedOriginRuntimeIndex = state.devices.front().runtimeIndex;
    }
}

void selectNextOriginDevice(AppWindowState& state)
{
    if (state.devices.empty()) {
        state.selectedOriginRuntimeIndex = kNoSelectedRuntimeIndex;
        state.originStatusMessage = "no devices to select as origin";
        return;
    }

    const ovtr::DeviceDescriptor* current = selectedOriginDevice(state);
    if (current == nullptr) {
        state.selectedOriginRuntimeIndex = state.devices.front().runtimeIndex;
    } else {
        const auto currentIndex = static_cast<std::size_t>(current - state.devices.data());
        const std::size_t nextIndex = (currentIndex + 1) % state.devices.size();
        state.selectedOriginRuntimeIndex = state.devices[nextIndex].runtimeIndex;
    }

    const ovtr::DeviceDescriptor* selected = selectedOriginDevice(state);
    state.originStatusMessage = selected
        ? "selected " + deviceDisplayName(*selected)
        : "origin selection unavailable";
}

void setOriginFromSelectedDevice(AppWindowState& state)
{
    ensureOriginSelection(state);
    const ovtr::DeviceDescriptor* selected = selectedOriginDevice(state);
    if (selected == nullptr) {
        state.originStatusMessage = "no device selected for origin";
        return;
    }

    const ovtr::PoseSample* pose = poseForRuntimeIndex(state.poses, selected->runtimeIndex);
    if (pose == nullptr || !isPoseValid(*pose)) {
        state.originStatusMessage = "selected device has no valid pose";
        return;
    }

    state.originEnabled = true;
    state.originOffset = pose->position;
    state.originStatusMessage = "origin set from " + deviceDisplayName(*selected);
}

void clearOrigin(AppWindowState& state)
{
    state.originEnabled = false;
    state.originOffset = {0.0f, 0.0f, 0.0f};
    state.originStatusMessage = "origin cleared";
}

std::string labelForDevice(const ovtr::DeviceDescriptor* device, const ovtr::PoseSample& pose)
{
    if (device && !device->serial.empty()) {
        return device->serial;
    }

    std::ostringstream stream;
    stream << "#" << pose.runtimeIndex;
    return stream.str();
}

void drawDeviceMarker3D(
    const ovtr::PoseSample& pose,
    const ovtr::DeviceClass deviceClass,
    const bool drawBody = true
)
{
    const float x = pose.position[0];
    const float y = pose.position[1];
    const float z = pose.position[2];
    const bool isTrackingReference = deviceClass == ovtr::DeviceClass::TrackingReference;
    const float radius = isTrackingReference ? 0.0325f : 0.02f;
    constexpr float axisLength = 0.08f;

    if ((pose.flags & ovtr::PoseFlagPoseValid) == 0) {
        glColor3f(0.55f, 0.55f, 0.55f);
    } else if (isTrackingReference) {
        glColor3f(0.72f, 0.42f, 1.0f);
    } else if (pose.runtimeIndex == 0) {
        glColor3f(0.25f, 0.95f, 0.95f);
    } else {
        glColor3f(1.0f, 0.72f, 0.24f);
    }

    glPushMatrix();
    glTranslatef(x, y, z);
    multiplyOpenGLMatrixFromQuaternion(pose.rotation);

    if (drawBody) {
        glBegin(GL_QUADS);
        glVertex3f(-radius, -radius, radius);
        glVertex3f(radius, -radius, radius);
        glVertex3f(radius, radius, radius);
        glVertex3f(-radius, radius, radius);

        glVertex3f(-radius, -radius, -radius);
        glVertex3f(-radius, radius, -radius);
        glVertex3f(radius, radius, -radius);
        glVertex3f(radius, -radius, -radius);

        glVertex3f(-radius, radius, -radius);
        glVertex3f(-radius, radius, radius);
        glVertex3f(radius, radius, radius);
        glVertex3f(radius, radius, -radius);

        glVertex3f(-radius, -radius, -radius);
        glVertex3f(radius, -radius, -radius);
        glVertex3f(radius, -radius, radius);
        glVertex3f(-radius, -radius, radius);

        glVertex3f(radius, -radius, -radius);
        glVertex3f(radius, radius, -radius);
        glVertex3f(radius, radius, radius);
        glVertex3f(radius, -radius, radius);

        glVertex3f(-radius, -radius, -radius);
        glVertex3f(-radius, -radius, radius);
        glVertex3f(-radius, radius, radius);
        glVertex3f(-radius, radius, -radius);
        glEnd();
    }

    glLineWidth(1.5f);
    glBegin(GL_LINES);
    glColor3f(1.0f, 0.20f, 0.20f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(axisLength, 0.0f, 0.0f);
    glColor3f(0.20f, 1.0f, 0.35f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, axisLength, 0.0f);
    glColor3f(0.35f, 0.55f, 1.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, axisLength);
    glEnd();

    glPopMatrix();
}

bool uploadRenderModelTexture(
    RenderModelMesh& mesh,
#ifdef OVTR_HAS_OPENVR_SDK
    vr::IVRRenderModels* renderModels,
    const vr::TextureID_t textureId
#else
    void*,
    const int
#endif
)
{
    if (mesh.textureId != 0 || mesh.textureAvailable) {
        return true;
    }
    if (mesh.textureLoadFailed) {
        return false;
    }

#ifdef OVTR_HAS_OPENVR_SDK
    if (renderModels == nullptr || textureId < 0) {
        mesh.textureLoadFailed = true;
        return false;
    }

    vr::RenderModel_TextureMap_t* texture = nullptr;
    const vr::EVRRenderModelError error = renderModels->LoadTexture_Async(textureId, &texture);
    if (error == vr::VRRenderModelError_Loading) {
        return false;
    }
    if (error != vr::VRRenderModelError_None || texture == nullptr || texture->rubTextureMapData == nullptr) {
        mesh.textureLoadFailed = true;
        return false;
    }
    if (texture->format != vr::VRRenderModelTextureFormat_RGBA8_SRGB ||
        texture->unWidth == 0 ||
        texture->unHeight == 0) {
        renderModels->FreeTexture(texture);
        mesh.textureLoadFailed = true;
        return false;
    }

    GLuint glTexture = 0;
    glGenTextures(1, &glTexture);
    if (glTexture == 0) {
        renderModels->FreeTexture(texture);
        mesh.textureLoadFailed = true;
        return false;
    }

    glBindTexture(GL_TEXTURE_2D, glTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        texture->unWidth,
        texture->unHeight,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        texture->rubTextureMapData
    );
    glBindTexture(GL_TEXTURE_2D, 0);
    renderModels->FreeTexture(texture);

    mesh.textureId = glTexture;
    mesh.textureAvailable = true;
    mesh.textureLoadFailed = false;
    return true;
#else
    mesh.textureLoadFailed = true;
    return false;
#endif
}

bool updateRenderModelMesh(RenderModelMesh& mesh, const std::string& renderModelName)
{
    if (mesh.state == RenderModelMesh::LoadState::Failed || renderModelName.empty()) {
        return false;
    }

#ifdef OVTR_HAS_OPENVR_SDK
    vr::IVRRenderModels* renderModels = vr::VRRenderModels();
    if (renderModels == nullptr) {
        if (mesh.state != RenderModelMesh::LoadState::Ready) {
            mesh.state = RenderModelMesh::LoadState::Failed;
            return false;
        }
        return true;
    }

    if (mesh.state == RenderModelMesh::LoadState::Ready) {
        if (!mesh.textureAvailable && mesh.diffuseTextureId >= 0) {
            uploadRenderModelTexture(mesh, renderModels, mesh.diffuseTextureId);
        }
        return true;
    }

    vr::RenderModel_t* openvrModel = nullptr;
    const vr::EVRRenderModelError error =
        renderModels->LoadRenderModel_Async(renderModelName.c_str(), &openvrModel);
    if (error == vr::VRRenderModelError_Loading) {
        return false;
    }
    if (error != vr::VRRenderModelError_None || openvrModel == nullptr) {
        mesh.state = RenderModelMesh::LoadState::Failed;
        return false;
    }

    mesh.vertices.clear();
    mesh.vertices.reserve(openvrModel->unVertexCount);
    for (std::uint32_t i = 0; i < openvrModel->unVertexCount; ++i) {
        const vr::RenderModel_Vertex_t& source = openvrModel->rVertexData[i];
        RenderModelVertex vertex;
        vertex.position = {source.vPosition.v[0], source.vPosition.v[1], source.vPosition.v[2]};
        vertex.normal = {source.vNormal.v[0], source.vNormal.v[1], source.vNormal.v[2]};
        vertex.texCoord = {source.rfTextureCoord[0], source.rfTextureCoord[1]};
        mesh.vertices.push_back(vertex);
    }

    const std::uint32_t indexCount = openvrModel->unTriangleCount * 3;
    mesh.indices.assign(openvrModel->rIndexData, openvrModel->rIndexData + indexCount);
    mesh.diffuseTextureId = openvrModel->diffuseTextureId;
    renderModels->FreeRenderModel(openvrModel);
    uploadRenderModelTexture(mesh, renderModels, mesh.diffuseTextureId);

    mesh.state = mesh.vertices.empty() || mesh.indices.empty()
        ? RenderModelMesh::LoadState::Failed
        : RenderModelMesh::LoadState::Ready;
    return mesh.state == RenderModelMesh::LoadState::Ready;
#else
    mesh.state = RenderModelMesh::LoadState::Failed;
    return false;
#endif
}

void drawRenderModelTriangles(const RenderModelMesh& mesh)
{
    glBegin(GL_TRIANGLES);
    for (const std::uint16_t index : mesh.indices) {
        if (index >= mesh.vertices.size()) {
            continue;
        }
        const RenderModelVertex& vertex = mesh.vertices[index];
        glNormal3f(vertex.normal[0], vertex.normal[1], vertex.normal[2]);
        glTexCoord2f(vertex.texCoord[0], vertex.texCoord[1]);
        glVertex3f(vertex.position[0], vertex.position[1], vertex.position[2]);
    }
    glEnd();
}

void drawRenderModelOutlineTriangles(const RenderModelMesh& mesh, const float expansion)
{
    glBegin(GL_TRIANGLES);
    for (const std::uint16_t index : mesh.indices) {
        if (index >= mesh.vertices.size()) {
            continue;
        }
        const RenderModelVertex& vertex = mesh.vertices[index];
        const Vec3 normal = normalizeVec3({vertex.normal[0], vertex.normal[1], vertex.normal[2]});
        glVertex3f(
            vertex.position[0] + normal.x * expansion,
            vertex.position[1] + normal.y * expansion,
            vertex.position[2] + normal.z * expansion
        );
    }
    glEnd();
}

bool drawSteamVRRenderModel3D(
    AppWindowState& state,
    const ovtr::PoseSample& pose,
    const ovtr::DeviceDescriptor* device,
    const int viewportHeight
)
{
    if (device == nullptr || device->renderModelName.empty()) {
        return false;
    }

    RenderModelMesh& mesh = state.renderModelCache[device->renderModelName];
    if (!updateRenderModelMesh(mesh, device->renderModelName)) {
        return false;
    }

    glPushMatrix();
    glTranslatef(pose.position[0], pose.position[1], pose.position[2]);
    multiplyOpenGLMatrixFromQuaternion(pose.rotation);

    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    if (device->deviceClass == ovtr::DeviceClass::TrackingReference) {
        glColor3f(0.72f, 0.42f, 1.0f);
    } else if (device->deviceClass == ovtr::DeviceClass::Hmd) {
        glColor3f(0.20f, 1.0f, 0.82f);
    } else {
        glColor3f(1.0f, 0.58f, 0.12f);
    }
    const float outlineExpansion = outlineExpansionForDepth(
        cameraDepthForWorldPoint(state, {pose.position[0], pose.position[1], pose.position[2]}),
        viewportHeight
    );
    drawRenderModelOutlineTriangles(mesh, outlineExpansion);
    glCullFace(GL_BACK);
    glDisable(GL_CULL_FACE);

    const GLfloat lightPosition[4] = {0.35f, 0.85f, 0.45f, 0.0f};
    const GLfloat lightDiffuse[4] = {0.85f, 0.88f, 0.92f, 1.0f};
    const GLfloat lightAmbient[4] = {0.24f, 0.26f, 0.30f, 1.0f};
    const GLfloat materialSpecular[4] = {0.92f, 0.96f, 1.0f, 1.0f};
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, materialSpecular);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 72.0f);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    if (mesh.textureAvailable && mesh.textureId != 0) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, mesh.textureId);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        glColor3f(1.0f, 1.0f, 1.0f);
    } else {
        glDisable(GL_TEXTURE_2D);
        glColor3f(0.72f, 0.76f, 0.82f);
    }
    drawRenderModelTriangles(mesh);

    if (mesh.textureAvailable && mesh.textureId != 0) {
        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_TEXTURE_2D);
    }
    glDisable(GL_COLOR_MATERIAL);
    glDisable(GL_LIGHT0);
    glDisable(GL_LIGHTING);

    glPopMatrix();
    return true;
}

void drawLabelText3D(const std::string& text, const GLuint fontBase)
{
    if (fontBase == 0 || text.empty()) {
        return;
    }

    std::string ascii;
    ascii.reserve(text.size());
    for (const char ch : text) {
        const unsigned char value = static_cast<unsigned char>(ch);
        ascii.push_back(value >= 32 && value < 128 ? ch : '?');
    }

    glListBase(fontBase);
    glCallLists(static_cast<GLsizei>(ascii.size()), GL_UNSIGNED_BYTE, ascii.data());
}

void drawDeviceLabel3D(
    const ovtr::PoseSample& pose,
    const ovtr::DeviceDescriptor* device,
    const GLuint fontBase
)
{
    if ((pose.flags & ovtr::PoseFlagPoseValid) == 0) {
        return;
    }

    const ovtr::DeviceClass deviceClass = device ? device->deviceClass : ovtr::DeviceClass::Other;
    const float labelLift = deviceClass == ovtr::DeviceClass::TrackingReference ? 0.13f : 0.10f;

    glRasterPos3f(pose.position[0], pose.position[1] + labelLift, pose.position[2]);
    drawLabelText3D(labelForDevice(device, pose), fontBase);
}

void renderViewport(HWND hwnd)
{
    auto* state = reinterpret_cast<AppWindowState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (!state || !state->glDeviceContext || !state->glContext) {
        return;
    }

    RECT rect;
    GetClientRect(hwnd, &rect);
    const int width = rect.right - rect.left;
    const int height = rect.bottom - rect.top;
    if (width <= 0 || height <= 0) {
        return;
    }

    wglMakeCurrent(state->glDeviceContext, state->glContext);
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float projection[16];
    perspectiveMatrix(kViewportFovDegrees, static_cast<float>(width) / static_cast<float>(height), 0.05f, 100.0f, projection);
    glLoadMatrixf(projection);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    glTranslatef(0.0f, -0.55f, -positiveCameraDistance(state->cameraDistance));
    glRotatef(state->cameraPitchDegrees, 1.0f, 0.0f, 0.0f);
    glRotatef(state->cameraYawDegrees, 0.0f, 1.0f, 0.0f);
    glTranslatef(-state->cameraPanX, -state->cameraPanY, -state->cameraPanZ);

    drawGroundGrid3D();
    drawAxes3D();

    for (const ovtr::PoseSample& pose : state->poses.poses) {
        const ovtr::PoseSample displayPose = applyOriginToPose(pose, state->originEnabled, state->originOffset);
        const ovtr::DeviceDescriptor* device = deviceForRuntimeIndex(state->devices, displayPose.runtimeIndex);
        const ovtr::DeviceClass deviceClass = device ? device->deviceClass : ovtr::DeviceClass::Other;
        const bool modelDrawn = drawSteamVRRenderModel3D(*state, displayPose, device, height);
        drawDeviceMarker3D(displayPose, deviceClass, !modelDrawn);
    }

    glDisable(GL_DEPTH_TEST);
    glColor3f(0.08f, 0.10f, 0.14f);
    for (const ovtr::PoseSample& pose : state->poses.poses) {
        const ovtr::PoseSample displayPose = applyOriginToPose(pose, state->originEnabled, state->originOffset);
        drawDeviceLabel3D(
            displayPose,
            deviceForRuntimeIndex(state->devices, displayPose.runtimeIndex),
            state->glLabelFontBase
        );
    }

    SwapBuffers(state->glDeviceContext);
    ++state->renderFrames;
}

LRESULT CALLBACK viewportProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    switch (message) {
    case WM_NCCREATE: {
        const auto* createStruct = reinterpret_cast<LPCREATESTRUCTW>(lparam);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(createStruct->lpCreateParams));
        return TRUE;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_SIZE:
        renderViewport(hwnd);
        return 0;
    case WM_PAINT: {
        PAINTSTRUCT paint;
        BeginPaint(hwnd, &paint);
        renderViewport(hwnd);
        EndPaint(hwnd, &paint);
        return 0;
    }
    case WM_LBUTTONDOWN: {
        auto* state = reinterpret_cast<AppWindowState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (state) {
            state->orbitDragging = true;
            state->lastMouseX = GET_X_LPARAM(lparam);
            state->lastMouseY = GET_Y_LPARAM(lparam);
            SetCapture(hwnd);
        }
        return 0;
    }
    case WM_LBUTTONUP: {
        auto* state = reinterpret_cast<AppWindowState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (state) {
            state->orbitDragging = false;
            if (!state->panDragging) {
                ReleaseCapture();
            }
        }
        return 0;
    }
    case WM_MBUTTONDOWN: {
        auto* state = reinterpret_cast<AppWindowState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (state) {
            state->panDragging = true;
            state->lastMouseX = GET_X_LPARAM(lparam);
            state->lastMouseY = GET_Y_LPARAM(lparam);
            SetCapture(hwnd);
        }
        return 0;
    }
    case WM_MBUTTONUP: {
        auto* state = reinterpret_cast<AppWindowState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (state) {
            state->panDragging = false;
            if (!state->orbitDragging) {
                ReleaseCapture();
            }
        }
        return 0;
    }
    case WM_MOUSEMOVE: {
        auto* state = reinterpret_cast<AppWindowState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (!state) {
            return 0;
        }

        const int x = GET_X_LPARAM(lparam);
        const int y = GET_Y_LPARAM(lparam);
        const int dx = x - state->lastMouseX;
        const int dy = y - state->lastMouseY;
        state->lastMouseX = x;
        state->lastMouseY = y;

        if (state->orbitDragging) {
            state->cameraYawDegrees += static_cast<float>(dx) * 0.35f;
            state->cameraPitchDegrees = clampFloat(
                state->cameraPitchDegrees + static_cast<float>(dy) * 0.25f,
                -10.0f,
                80.0f
            );
            renderViewport(hwnd);
        } else if (state->panDragging) {
            applyScreenSpacePan(*state, dx, dy);
            renderViewport(hwnd);
        }
        return 0;
    }
    case WM_MOUSEWHEEL: {
        auto* state = reinterpret_cast<AppWindowState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (state) {
            const int wheelDelta = GET_WHEEL_DELTA_WPARAM(wparam);
            const float zoomSteps = static_cast<float>(wheelDelta) / static_cast<float>(WHEEL_DELTA);
            applyCameraDolly(*state, zoomSteps * 0.45f);
            renderViewport(hwnd);
        }
        return 0;
    }
    default:
        break;
    }

    return DefWindowProcW(hwnd, message, wparam, lparam);
}

bool setupOpenGLForChild(HWND hwnd, AppWindowState& state)
{
    state.glDeviceContext = GetDC(hwnd);
    if (!state.glDeviceContext) {
        return false;
    }

    PIXELFORMATDESCRIPTOR descriptor{};
    descriptor.nSize = sizeof(descriptor);
    descriptor.nVersion = 1;
    descriptor.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    descriptor.iPixelType = PFD_TYPE_RGBA;
    descriptor.cColorBits = 32;
    descriptor.cDepthBits = 24;
    descriptor.cStencilBits = 8;
    descriptor.iLayerType = PFD_MAIN_PLANE;

    const int pixelFormat = ChoosePixelFormat(state.glDeviceContext, &descriptor);
    if (pixelFormat == 0) {
        return false;
    }

    if (!SetPixelFormat(state.glDeviceContext, pixelFormat, &descriptor)) {
        return false;
    }

    state.glContext = wglCreateContext(state.glDeviceContext);
    if (!state.glContext) {
        return false;
    }

    wglMakeCurrent(state.glDeviceContext, state.glContext);
    state.glLabelFontBase = glGenLists(128);
    if (state.glLabelFontBase != 0) {
        HFONT labelFont = CreateFontW(
            -13,
            0,
            0,
            0,
            FW_SEMIBOLD,
            FALSE,
            FALSE,
            FALSE,
            DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE,
            L"Segoe UI"
        );
        HGDIOBJ previousFont = nullptr;
        if (labelFont) {
            previousFont = SelectObject(state.glDeviceContext, labelFont);
        }
        wglUseFontBitmapsW(state.glDeviceContext, 0, 128, state.glLabelFontBase);
        if (previousFont) {
            SelectObject(state.glDeviceContext, previousFont);
        }
        if (labelFont) {
            DeleteObject(labelFont);
        }
    }

    return true;
}

void layoutChildWindows(HWND hwnd)
{
    auto* state = reinterpret_cast<AppWindowState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (!state || !state->glWindow) {
        return;
    }

    RECT rect;
    GetClientRect(hwnd, &rect);
    const int width = rect.right - rect.left;
    const int height = rect.bottom - rect.top;
    const int margin = 32;
    const int leftPanelWidth = leftPanelWidthForClient(width);
    const int glX = leftPanelWidth;
    const int glY = margin;
    const int glWidth = width - leftPanelWidth - margin;
    const int glHeight = height - margin * 2;

    if (glWidth > 160 && glHeight > 160) {
        MoveWindow(state->glWindow, glX, glY, glWidth, glHeight, TRUE);
        ShowWindow(state->glWindow, SW_SHOW);
    } else {
        ShowWindow(state->glWindow, SW_HIDE);
    }
}

void updateFpsCounters(AppWindowState& state)
{
    const auto now = std::chrono::steady_clock::now();
    const double elapsedSeconds = std::chrono::duration<double>(now - state.lastFpsUpdate).count();
    if (elapsedSeconds < 1.0) {
        return;
    }

    state.posePollFps = static_cast<double>(state.posePollFrames) / elapsedSeconds;
    state.renderFps = static_cast<double>(state.renderFrames) / elapsedSeconds;
    state.posePollFrames = 0;
    state.renderFrames = 0;
    state.lastFpsUpdate = now;
}

bool deviceListEventReceived(const std::vector<ovtr::VREvent>& events)
{
    for (const ovtr::VREvent& event : events) {
        if (event.type == ovtr::VREventType::DeviceActivated ||
            event.type == ovtr::VREventType::DeviceDeactivated ||
            event.type == ovtr::VREventType::DeviceUpdated) {
            return true;
        }
    }
    return false;
}

void invalidateStatusPanel(HWND hwnd)
{
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    const int leftPanelWidth = leftPanelWidthForClient(clientRect.right - clientRect.left);
    RECT statusRect{0, 0, leftPanelWidth, clientRect.bottom};
    InvalidateRect(hwnd, &statusRect, FALSE);
}

void refreshStatus(HWND hwnd, const bool forceDeviceEnumeration = false)
{
    auto* state = reinterpret_cast<AppWindowState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (!state) {
        return;
    }

    state->status = state->runtime.queryStatus();
    state->providerError.clear();

    if (!state->provider.isInitialized()) {
        state->provider.initialize();
    }

    if (state->provider.isInitialized()) {
        std::vector<ovtr::VREvent> events;
        state->provider.pollEvents(events);

        const auto now = std::chrono::steady_clock::now();
        const bool deviceRefreshDue =
            state->lastDeviceEnumeration.time_since_epoch().count() == 0 ||
            std::chrono::duration<double>(now - state->lastDeviceEnumeration).count() >= 10.0;
        if (forceDeviceEnumeration ||
            state->devices.empty() ||
            deviceRefreshDue ||
            deviceListEventReceived(events)) {
            state->devices = state->provider.enumerateDevices();
            state->lastDeviceEnumeration = now;
        }
        ensureOriginSelection(*state);
    } else {
        state->devices.clear();
        state->lastDeviceEnumeration = {};
        state->poses = {};
        ensureOriginSelection(*state);
        state->providerError = state->provider.lastError();
    }

    updateFpsCounters(*state);
    invalidateStatusPanel(hwnd);
}

void refreshPoseAndViewport(HWND hwnd)
{
    auto* state = reinterpret_cast<AppWindowState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (!state) {
        return;
    }

    if (state->provider.isInitialized()) {
        if (state->provider.pollPoses(state->poses)) {
            ++state->posePollFrames;
            if (state->recorder.state() == ovtr::RecorderState::Recording) {
                const auto now = std::chrono::steady_clock::now();
                if (state->recordingScheduler.shouldSample(now)) {
                    const ovtr::SampleTiming timing = state->recordingScheduler.markSampled(now);
                    state->recordingDroppedFrames += timing.droppedFrames;

                    ovtr::FrameSample frame;
                    frame.frameIndex = timing.frameIndex;
                    frame.timestampNs = static_cast<std::uint64_t>(
                        std::chrono::duration_cast<std::chrono::nanoseconds>(now - state->recordingStart).count()
                    );
                    frame.timeSeconds = static_cast<double>(frame.timestampNs) / 1'000'000'000.0;
                    frame.poses = applyOriginToPoses(
                        state->poses,
                        state->originEnabled,
                        state->originOffset
                    ).poses;

                    if (!state->recorder.appendFrame(frame)) {
                        state->recordingError = state->recorder.lastError();
                    }
                }
            }
        }
    }

    if (state->glWindow) {
        renderViewport(state->glWindow);
    }
}

void toggleRecording(HWND hwnd)
{
    auto* state = reinterpret_cast<AppWindowState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (!state) {
        return;
    }

    state->recordingError.clear();

    if (state->recorder.state() == ovtr::RecorderState::Recording ||
        state->recorder.state() == ovtr::RecorderState::Paused) {
        const auto now = std::chrono::steady_clock::now();
        const double durationSeconds = std::chrono::duration<double>(now - state->recordingStart).count();
        if (!state->recorder.stop(durationSeconds, state->recordingDroppedFrames)) {
            state->recordingError = state->recorder.lastError();
        }
        invalidateStatusPanel(hwnd);
        return;
    }

    if (state->recorder.state() != ovtr::RecorderState::Idle &&
        state->recorder.state() != ovtr::RecorderState::Error) {
        return;
    }

    ovtr::RecordingSession session;
    session.sessionId = "session_" + localTimestampForPath();
    session.sessionName = session.sessionId;
    session.createdAtUtc = utcTimestampIso();
    session.appVersion = "0.1.0";
    session.targetSampleRate = kTargetViewportFps;
    session.devices = state->devices;

    state->currentSessionFolder = std::filesystem::current_path() / "recordings" / session.sessionId;

    ovtr::RecordingStartOptions options;
    options.sessionFolder = state->currentSessionFolder;
    options.session = session;

    state->recordingStart = std::chrono::steady_clock::now();
    state->recordingDroppedFrames = 0;
    state->recordingScheduler = ovtr::SamplingScheduler(kTargetViewportFps);
    state->recordingScheduler.reset(state->recordingStart);

    if (!state->recorder.start(options)) {
        state->recordingError = state->recorder.lastError();
    }

    invalidateStatusPanel(hwnd);
}

void exportCurrentSession(HWND hwnd, const ExportFormat format)
{
    auto* state = reinterpret_cast<AppWindowState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (!state) {
        return;
    }

    state->exportStatusMessage.clear();

    if (state->recorder.state() == ovtr::RecorderState::Recording ||
        state->recorder.state() == ovtr::RecorderState::Paused ||
        state->recorder.state() == ovtr::RecorderState::Starting ||
        state->recorder.state() == ovtr::RecorderState::Stopping ||
        state->recorder.state() == ovtr::RecorderState::Finalizing) {
        state->exportStatusMessage = "stop recording before exporting";
        invalidateStatusPanel(hwnd);
        return;
    }

    if (state->currentSessionFolder.empty()) {
        state->exportStatusMessage = "no recorded session available";
        invalidateStatusPanel(hwnd);
        return;
    }

    ovtr::RecordingSession session = state->recorder.session();
    if (session.sessionId.empty()) {
        session.sessionId = state->currentSessionFolder.filename().string();
        session.sessionName = session.sessionId;
    }
    if (session.devices.empty()) {
        session.devices = state->devices;
    }
    if (session.framesPath.empty()) {
        session.framesPath = state->currentSessionFolder / "frames.bin";
    }
    if (session.frameIndexPath.empty()) {
        session.frameIndexPath = state->currentSessionFolder / "frame_index.bin";
    }

    ovtr::ExportResult result;
    if (format == ExportFormat::Fbx) {
        ovtr::FbxExportOptions options;
        options.outputPath = std::filesystem::current_path() / "exports" / (session.sessionId + ".fbx");
        options.includeGeometry = true;
        options.includeTrackingReference = true;
        options.exportSampleRate = kFbxExportSampleRate;
        result = ovtr::exportSessionToFbxAscii(session, options);
    } else {
        ovtr::GltfExportOptions options;
        options.outputPath = std::filesystem::current_path() / "exports" / (session.sessionId + ".glb");
        options.includeTrackingReference = true;
        options.exportSampleRate = kFbxExportSampleRate;
        options.format = ovtr::GltfExportFormat::Glb;
        result = ovtr::exportSessionToGltf(session, options);
    }

    if (result.success) {
        state->exportStatusMessage = (format == ExportFormat::Fbx ? "FBX saved to " : "GLB saved to ") +
            result.outputPath.string();
    } else {
        state->exportStatusMessage = (format == ExportFormat::Fbx ? "FBX export failed: " : "GLB export failed: ") +
            result.error;
    }

    invalidateStatusPanel(hwnd);
}

void paintWindow(HWND hwnd)
{
    PAINTSTRUCT paint;
    HDC paintDc = BeginPaint(hwnd, &paint);

    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    const int clientWidth = clientRect.right - clientRect.left;
    const int clientHeight = clientRect.bottom - clientRect.top;
    if (clientWidth <= 0 || clientHeight <= 0) {
        EndPaint(hwnd, &paint);
        return;
    }

    HDC bufferDc = CreateCompatibleDC(paintDc);
    HBITMAP bufferBitmap = bufferDc ? CreateCompatibleBitmap(paintDc, clientWidth, clientHeight) : nullptr;
    HGDIOBJ previousBitmap = nullptr;
    HDC drawDc = paintDc;
    if (bufferDc && bufferBitmap) {
        previousBitmap = SelectObject(bufferDc, bufferBitmap);
        drawDc = bufferDc;
    }

    HBRUSH background = CreateSolidBrush(RGB(24, 26, 30));
    FillRect(drawDc, &clientRect, background);
    DeleteObject(background);

    SetBkMode(drawDc, TRANSPARENT);
    SetTextColor(drawDc, RGB(230, 234, 240));

    HFONT titleFont = CreateFontW(
        26,
        0,
        0,
        0,
        FW_SEMIBOLD,
        FALSE,
        FALSE,
        FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        L"Segoe UI"
    );
    HFONT bodyFont = CreateFontW(
        17,
        0,
        0,
        0,
        FW_NORMAL,
        FALSE,
        FALSE,
        FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        L"Segoe UI"
    );

    auto* state = reinterpret_cast<AppWindowState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    const auto lines = state ? makeStatusLines(*state) : std::vector<std::wstring>{L"Loading..."};
    const int leftPanelWidth = leftPanelWidthForClient(clientRect.right - clientRect.left);
    const int textRight = leftPanelWidth - 24;

    int y = 28;
    for (std::size_t i = 0; i < lines.size(); ++i) {
        SelectObject(drawDc, i == 0 ? titleFont : bodyFont);
        const int lineHeight = i == 0 ? 34 : 25;
        RECT lineRect{32, y, textRight, y + lineHeight};
        DrawTextW(drawDc, lines[i].c_str(), static_cast<int>(lines[i].size()), &lineRect, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS);
        y += lineHeight;
    }

    if (drawDc == bufferDc) {
        BitBlt(paintDc, 0, 0, clientWidth, clientHeight, bufferDc, 0, 0, SRCCOPY);
    }

    if (previousBitmap) {
        SelectObject(bufferDc, previousBitmap);
    }
    if (bufferBitmap) {
        DeleteObject(bufferBitmap);
    }
    if (bufferDc) {
        DeleteDC(bufferDc);
    }
    DeleteObject(titleFont);
    DeleteObject(bodyFont);
    EndPaint(hwnd, &paint);
}

LRESULT CALLBACK windowProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    switch (message) {
    case WM_CREATE: {
        auto* state = new AppWindowState();
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));
        state->glWindow = CreateWindowExW(
            0,
            kViewportWindowClassName,
            nullptr,
            WS_CHILD | WS_VISIBLE | WS_BORDER | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
            760,
            32,
            400,
            400,
            hwnd,
            nullptr,
            reinterpret_cast<LPCREATESTRUCTW>(lparam)->hInstance,
            state
        );
        if (state->glWindow) {
            setupOpenGLForChild(state->glWindow, *state);
        }
        layoutChildWindows(hwnd);
        SetTimer(hwnd, kStatusTimerId, kStatusIntervalMs, nullptr);
        refreshStatus(hwnd);
        refreshPoseAndViewport(hwnd);
        return 0;
    }
    case WM_DESTROY: {
        KillTimer(hwnd, kStatusTimerId);
        auto* state = reinterpret_cast<AppWindowState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (state) {
            state->provider.shutdown();
            if (state->glContext) {
                wglMakeCurrent(state->glDeviceContext, state->glContext);
                for (auto& entry : state->renderModelCache) {
                    RenderModelMesh& mesh = entry.second;
                    if (mesh.textureId != 0) {
                        glDeleteTextures(1, &mesh.textureId);
                        mesh.textureId = 0;
                        mesh.textureAvailable = false;
                    }
                }
                if (state->glLabelFontBase != 0) {
                    glDeleteLists(state->glLabelFontBase, 128);
                    state->glLabelFontBase = 0;
                }
                wglMakeCurrent(nullptr, nullptr);
                wglDeleteContext(state->glContext);
                state->glContext = nullptr;
            }
            if (state->glWindow && state->glDeviceContext) {
                ReleaseDC(state->glWindow, state->glDeviceContext);
                state->glDeviceContext = nullptr;
            }
        }
        delete state;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
        PostQuitMessage(0);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_TIMER:
        if (wparam == kStatusTimerId) {
            refreshStatus(hwnd);
            return 0;
        }
        break;
    case WM_SIZE:
        layoutChildWindows(hwnd);
        return 0;
    case WM_KEYDOWN:
        if (wparam == VK_ESCAPE) {
            DestroyWindow(hwnd);
            return 0;
        }
        if (wparam == VK_LEFT) {
            auto* state = reinterpret_cast<AppWindowState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
            if (state) {
                state->cameraYawDegrees -= 5.0f;
                refreshPoseAndViewport(hwnd);
            }
            return 0;
        }
        if (wparam == VK_RIGHT) {
            auto* state = reinterpret_cast<AppWindowState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
            if (state) {
                state->cameraYawDegrees += 5.0f;
                refreshPoseAndViewport(hwnd);
            }
            return 0;
        }
        if (wparam == VK_UP) {
            auto* state = reinterpret_cast<AppWindowState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
            if (state) {
                state->cameraPitchDegrees = clampFloat(state->cameraPitchDegrees - 4.0f, -10.0f, 80.0f);
                refreshPoseAndViewport(hwnd);
            }
            return 0;
        }
        if (wparam == VK_DOWN) {
            auto* state = reinterpret_cast<AppWindowState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
            if (state) {
                state->cameraPitchDegrees = clampFloat(state->cameraPitchDegrees + 4.0f, -10.0f, 80.0f);
                refreshPoseAndViewport(hwnd);
            }
            return 0;
        }
        if (wparam == VK_OEM_PLUS || wparam == VK_ADD) {
            auto* state = reinterpret_cast<AppWindowState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
            if (state) {
                applyCameraDolly(*state, 0.35f);
                refreshPoseAndViewport(hwnd);
            }
            return 0;
        }
        if (wparam == VK_OEM_MINUS || wparam == VK_SUBTRACT) {
            auto* state = reinterpret_cast<AppWindowState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
            if (state) {
                applyCameraDolly(*state, -0.35f);
                refreshPoseAndViewport(hwnd);
            }
            return 0;
        }
        if (wparam == VK_HOME) {
            auto* state = reinterpret_cast<AppWindowState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
            if (state) {
                state->cameraYawDegrees = 42.0f;
                state->cameraPitchDegrees = 28.0f;
                state->cameraDistance = 5.5f;
                state->cameraPanX = 0.0f;
                state->cameraPanY = 0.0f;
                state->cameraPanZ = 0.0f;
                refreshPoseAndViewport(hwnd);
            }
            return 0;
        }
        if (wparam == VK_F5) {
            refreshStatus(hwnd, true);
            refreshPoseAndViewport(hwnd);
            return 0;
        }
        if (wparam == VK_TAB) {
            auto* state = reinterpret_cast<AppWindowState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
            if (state) {
                selectNextOriginDevice(*state);
                invalidateStatusPanel(hwnd);
            }
            return 0;
        }
        if (wparam == 'O') {
            auto* state = reinterpret_cast<AppWindowState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
            if (state) {
                setOriginFromSelectedDevice(*state);
                refreshPoseAndViewport(hwnd);
                invalidateStatusPanel(hwnd);
            }
            return 0;
        }
        if (wparam == 'C') {
            auto* state = reinterpret_cast<AppWindowState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
            if (state) {
                clearOrigin(*state);
                refreshPoseAndViewport(hwnd);
                invalidateStatusPanel(hwnd);
            }
            return 0;
        }
        if (wparam == 'R') {
            toggleRecording(hwnd);
            return 0;
        }
        if (wparam == 'E') {
            exportCurrentSession(hwnd, ExportFormat::Fbx);
            return 0;
        }
        if (wparam == 'G') {
            exportCurrentSession(hwnd, ExportFormat::Glb);
            return 0;
        }
        break;
    case WM_PAINT:
        paintWindow(hwnd);
        return 0;
    default:
        break;
    }

    return DefWindowProcW(hwnd, message, wparam, lparam);
}

int runMessageAndFrameLoop(HWND hwnd)
{
    timeBeginPeriod(1);

    MSG message{};
    bool running = true;
    auto nextFrame = std::chrono::steady_clock::now();
    const auto frameInterval = std::chrono::duration_cast<std::chrono::steady_clock::duration>(
        std::chrono::duration<double>(1.0 / kTargetViewportFps)
    );

    while (running && IsWindow(hwnd)) {
        while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
            if (message.message == WM_QUIT) {
                running = false;
                break;
            }

            TranslateMessage(&message);
            DispatchMessageW(&message);
        }

        if (!running || !IsWindow(hwnd)) {
            break;
        }

        const auto now = std::chrono::steady_clock::now();
        if (now >= nextFrame) {
            refreshPoseAndViewport(hwnd);

            do {
                nextFrame += frameInterval;
            } while (nextFrame <= now);
        }

        const auto afterWork = std::chrono::steady_clock::now();
        const auto timeUntilNextFrame = nextFrame - afterWork;
        if (timeUntilNextFrame > std::chrono::milliseconds(2)) {
            Sleep(1);
        } else {
            Sleep(0);
        }
    }

    timeEndPeriod(1);
    return static_cast<int>(message.wParam);
}

} // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int showCommand)
{
    WNDCLASSW windowClass{};
    windowClass.lpfnWndProc = windowProc;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    windowClass.lpszClassName = kMainWindowClassName;

    RegisterClassW(&windowClass);

    WNDCLASSW viewportClass{};
    viewportClass.style = CS_OWNDC;
    viewportClass.lpfnWndProc = viewportProc;
    viewportClass.hInstance = instance;
    viewportClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    viewportClass.lpszClassName = kViewportWindowClassName;

    RegisterClassW(&viewportClass);

    HWND hwnd = CreateWindowExW(
        0,
        kMainWindowClassName,
        L"OpenVR Tracker Recorder",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        1480,
        860,
        nullptr,
        nullptr,
        instance,
        nullptr
    );

    if (!hwnd) {
        return 1;
    }

    ShowWindow(hwnd, showCommand);
    UpdateWindow(hwnd);
    layoutChildWindows(hwnd);
    refreshStatus(hwnd, true);
    refreshPoseAndViewport(hwnd);
    RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);

    return runMessageAndFrameLoop(hwnd);
}
