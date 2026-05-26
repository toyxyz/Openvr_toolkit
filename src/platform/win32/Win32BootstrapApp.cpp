#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>
#include <shlobj.h>
#include <gl/GL.h>
#include <mmsystem.h>

#ifdef OVTR_HAS_OPENVR_SDK
#include <openvr.h>
#endif

#include "data/SessionTypes.h"
#include "export/FbxAsciiExporter.h"
#include "export/GltfExporter.h"
#include "import/GltfImporter.h"
#include "recording/RecordingController.h"
#include "recording/SamplingScheduler.h"
#include "util/SteamVRRuntime.h"
#include "vr/OpenVRProvider.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cctype>
#include <cwchar>
#include <cwctype>
#include <ctime>
#include <filesystem>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <system_error>
#include <utility>
#include <unordered_map>
#include <vector>

namespace {

constexpr UINT_PTR kStatusTimerId = 1;
constexpr UINT kStatusIntervalMs = 1000;
constexpr double kTargetViewportFps = 90.0;
constexpr double kDefaultExportSampleRate = 60.0;
constexpr float kMinimumCameraDistance = 0.08f;
constexpr float kViewportFovDegrees = 48.0f;
constexpr float kRenderModelOutlinePixels = 2.6f;
constexpr float kRecordingViewportBorderPixels = 5.0f;
constexpr int kDelayCountdownFontHeight = 54;
constexpr float kPi = 3.14159265359f;
constexpr int kContentMargin = 32;
constexpr int kStatusBarHeight = 36;
constexpr int kDebugMonitorHeight = 220;
constexpr int kDebugMonitorMinHeight = 120;
constexpr int kDebugResizeGripHeight = 8;
constexpr int kDebugButtonWidth = 76;
constexpr int kDebugButtonHeight = 24;
constexpr int kTopBarHeight = 32;
constexpr int kTopMenuSettingWidth = 92;
constexpr int kTopMenuFileWidth = 68;
constexpr int kTopMenuGap = 6;
constexpr int kViewportControlBarHeight = 48;
constexpr int kImportedAnimationBarHeight = 48;
constexpr int kViewportControlButtonSize = 30;
constexpr int kImportedAnimationButtonSize = 30;
constexpr int kImportedAnimationCloseButtonWidth = 70;
constexpr int kImportedAnimationFrameTextWidth = 142;
constexpr int kImportedAnimationTimelineMinWidth = 90;
constexpr int kDeviceToggleRailWidth = 32;
constexpr int kDeviceToggleButtonWidth = 24;
constexpr int kDeviceToggleButtonHeight = 96;
constexpr int kDebugPanelPaddingTop = 10;
constexpr int kDebugPanelLineHeight = 18;
constexpr int kDeviceListHeaderHeight = 28;
constexpr int kDeviceListItemHeight = 26;
constexpr int kDeviceListBoxPadding = 12;
constexpr int kOriginPanelHeight = 128;
constexpr int kOriginPanelGap = 14;
constexpr int kOriginPanelPadding = 12;
constexpr int kOriginStepperButtonSize = 18;
constexpr int kOriginStepperRowHeight = 30;
constexpr int kOriginStepperLabelWidth = 34;
constexpr float kOriginPositionStep = 0.001f;
constexpr float kOriginRotationStepDegrees = 0.1f;
constexpr double kImportedAnimationFrameRate = 60.0;
constexpr int kDeviceNameDialogWidth = 440;
constexpr int kDeviceNameDialogHeight = 172;
constexpr int kOriginDialogWidth = 480;
constexpr int kOriginDialogHeight = 250;
constexpr int kExportLocationDialogWidth = 580;
constexpr int kExportLocationDialogHeight = 260;
constexpr int kViewportColorDialogWidth = 560;
constexpr int kViewportColorDialogHeight = 304;
constexpr int kViewportColorCount = 4;
constexpr int kSplitterWidth = 8;
constexpr int kLeftPanelMinWidth = 320;
constexpr int kLeftPanelMaxWidth = 880;
constexpr float kDefaultLeftPanelWidthRatio = 0.28f;
constexpr int kDefaultLeftPanelMinWidth = 420;
constexpr int kDefaultLeftPanelMaxWidth = 620;
constexpr int kViewportMinWidth = 320;
constexpr std::size_t kDebugLogMaxLines = 80;
constexpr UINT kDeviceContextMenuSetOriginId = 1001;
constexpr UINT kDeviceContextMenuSetNameId = 1002;
constexpr UINT kSettingsMenuColorId = 1101;
constexpr UINT kSettingsMenuOriginId = 1102;
constexpr UINT kSettingsMenuLocationId = 1103;
constexpr UINT kFileMenuImportGlbId = 1201;
constexpr UINT_PTR kOriginEditControlId = 2001;
constexpr UINT_PTR kDeviceNameEditControlId = 3001;
constexpr UINT_PTR kViewportColorEditBaseControlId = 4000;
constexpr UINT_PTR kViewportColorPickBaseControlId = 4100;
constexpr UINT_PTR kViewportOutlineEditControlId = 4200;
constexpr UINT_PTR kViewportColorResetControlId = 4201;
constexpr UINT_PTR kOriginDialogEditBaseControlId = 4300;
constexpr UINT_PTR kOriginDialogEnabledControlId = 4400;
constexpr UINT_PTR kExportLocationEditControlId = 4500;
constexpr UINT_PTR kExportLocationBrowseControlId = 4501;
constexpr UINT_PTR kRecordDelayEditControlId = 4502;
constexpr UINT_PTR kRecordSaveFormatControlId = 4503;
constexpr UINT_PTR kRecordResampleFpsEditControlId = 4504;
constexpr std::uint32_t kNoSelectedRuntimeIndex = 0xffffffffu;
constexpr const wchar_t* kMainWindowClassName = L"OpenVRTrackerRecorderBootstrapWindow";
constexpr const wchar_t* kViewportWindowClassName = L"OpenVRTrackerRecorderGLViewport";
constexpr const wchar_t* kDeviceNameDialogClassName = L"OpenVRTrackerRecorderDeviceNameDialog";
constexpr const wchar_t* kOriginDialogClassName = L"OpenVRTrackerRecorderOriginDialog";
constexpr const wchar_t* kExportLocationDialogClassName = L"OpenVRTrackerRecorderExportLocationDialog";
constexpr const wchar_t* kViewportColorDialogClassName = L"OpenVRTrackerRecorderViewportColorDialog";
constexpr const char* kOriginConfigFileName = "openvr_tracker_recorder_origin.cfg";
constexpr const char* kDeviceNameConfigFileName = "openvr_tracker_recorder_device_names.cfg";
constexpr const char* kViewportSettingsConfigFileName = "openvr_tracker_recorder_viewport.cfg";
constexpr const char* kRecordSettingsConfigFileName = "openvr_tracker_recorder_record.cfg";
constexpr const char* kLegacyExportLocationConfigFileName = "openvr_tracker_recorder_export_location.cfg";

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

struct RgbColor {
    int r = 0;
    int g = 0;
    int b = 0;
};

struct ViewportSettings {
    RgbColor labelTextColor{20, 26, 36};
    RgbColor gridColor{199, 201, 209};
    RgbColor backgroundColor{198, 217, 225};
    RgbColor importedGlbColor{255, 255, 255};
    float outlineMultiplier = 1.0f;
};

struct DeviceListRow {
    std::uint32_t runtimeIndex = kNoSelectedRuntimeIndex;
    std::wstring customName;
    std::wstring model;
    std::wstring serial;
};

struct DeviceListLayout {
    RECT boxRect{0, 0, 0, 0};
    RECT headerRect{0, 0, 0, 0};
    RECT contentRect{0, 0, 0, 0};
    int visibleItemCount = 0;
    bool valid = false;
};

struct OriginPanelLayout {
    RECT boxRect{0, 0, 0, 0};
    RECT valueRect{0, 0, 0, 0};
    bool valid = false;
};

struct OriginStepperButton {
    RECT rect{0, 0, 0, 0};
    bool rotation = false;
    int axis = 0;
    float delta = 0.0f;
    bool valid = false;
};

struct ViewportControlLayout {
    RECT barRect{0, 0, 0, 0};
    RECT recordButtonRect{0, 0, 0, 0};
    RECT animationBarRect{0, 0, 0, 0};
    RECT firstFrameButtonRect{0, 0, 0, 0};
    RECT playPauseButtonRect{0, 0, 0, 0};
    RECT lastFrameButtonRect{0, 0, 0, 0};
    RECT timelineRect{0, 0, 0, 0};
    RECT frameTextRect{0, 0, 0, 0};
    RECT closeButtonRect{0, 0, 0, 0};
    bool valid = false;
    bool animationValid = false;
};

struct DeviceNameDialogState {
    HWND parent = nullptr;
    HWND editWindow = nullptr;
    std::wstring deviceLabel;
    std::wstring initialName;
    std::wstring resultName;
    bool accepted = false;
    bool done = false;
};

struct AppWindowState;

struct OriginDialogState {
    HWND parent = nullptr;
    AppWindowState* appState = nullptr;
    bool originalEnabled = false;
    std::array<float, 3> originalOffset{0.0f, 0.0f, 0.0f};
    std::array<float, 3> originalRotationDegrees{0.0f, 0.0f, 0.0f};
    bool workingEnabled = false;
    std::array<float, 3> workingOffset{0.0f, 0.0f, 0.0f};
    std::array<float, 3> workingRotationDegrees{0.0f, 0.0f, 0.0f};
    HWND enabledCheck = nullptr;
    std::array<HWND, 3> positionEdits{};
    std::array<HWND, 3> rotationEdits{};
    bool updatingControls = false;
    bool accepted = false;
    bool done = false;
};

struct PopupMenuItem {
    UINT commandId = 0;
    std::wstring label;
};

struct ExportLocationDialogState {
    HWND parent = nullptr;
    AppWindowState* appState = nullptr;
    HWND editWindow = nullptr;
    HWND delayEditWindow = nullptr;
    HWND resampleFpsEditWindow = nullptr;
    HWND saveFormatComboWindow = nullptr;
    std::filesystem::path initialDirectory;
    std::filesystem::path resultDirectory;
    float initialRecordDelaySeconds = 0.0f;
    float resultRecordDelaySeconds = 0.0f;
    float initialExportSampleRate = static_cast<float>(kDefaultExportSampleRate);
    float resultExportSampleRate = static_cast<float>(kDefaultExportSampleRate);
    ExportFormat initialSaveFormat = ExportFormat::Glb;
    ExportFormat resultSaveFormat = ExportFormat::Glb;
    bool accepted = false;
    bool done = false;
};

struct ColorEditControls {
    HWND red = nullptr;
    HWND green = nullptr;
    HWND blue = nullptr;
    HWND pick = nullptr;
};

struct ViewportColorDialogState {
    HWND parent = nullptr;
    AppWindowState* appState = nullptr;
    ViewportSettings workingSettings;
    std::array<ColorEditControls, kViewportColorCount> colorControls{};
    HWND outlineEdit = nullptr;
    std::array<COLORREF, 16> customColors{};
    bool accepted = false;
    bool done = false;
};

struct AppWindowState {
    ovtr::SteamVRRuntime runtime;
    ovtr::SteamVRRuntimeStatus status;
    ovtr::OpenVRProvider provider;
    std::vector<ovtr::DeviceDescriptor> devices;
    ovtr::PosePollResult poses;
    std::string providerError;
    std::string lastLoggedProviderError;
    std::size_t lastLoggedDeviceCount = 0;
    HWND glWindow = nullptr;
    HDC glDeviceContext = nullptr;
    HGLRC glContext = nullptr;
    GLuint glLabelFontBase = 0;
    GLuint glOverlayFontBase = 0;
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
    std::string importStatusMessage;
    std::filesystem::path exportDirectory;
    float recordDelaySeconds = 0.0f;
    float recordExportSampleRate = static_cast<float>(kDefaultExportSampleRate);
    ExportFormat recordSaveFormat = ExportFormat::Glb;
    bool recordingDelayActive = false;
    std::chrono::steady_clock::time_point recordingDelayDeadline{};
    bool originEnabled = false;
    std::array<float, 3> originOffset{0.0f, 0.0f, 0.0f};
    std::array<float, 3> originRotationDegrees{0.0f, 0.0f, 0.0f};
    std::uint32_t selectedOriginRuntimeIndex = kNoSelectedRuntimeIndex;
    std::string originStatusMessage;
    ViewportSettings viewportSettings;
    std::unordered_map<std::string, std::string> deviceCustomNames;
    std::unordered_map<std::string, RenderModelMesh> renderModelCache;
    ovtr::ImportedGltfScene importedScene;
    bool importedSceneLoaded = false;
    bool importedScenePlaying = false;
    bool importedSceneTimelineDragging = false;
    double importedScenePlaybackSeconds = 0.0;
    std::chrono::steady_clock::time_point importedSceneLastUpdate{};
    bool debugMonitorVisible = false;
    int debugLogScrollOffset = 0;
    int debugInfoScrollOffset = 0;
    int debugMonitorHeight = kDebugMonitorHeight;
    std::vector<std::wstring> debugLogLines;
    int deviceListScrollOffset = 0;
    std::uint32_t selectedDeviceRuntimeIndex = kNoSelectedRuntimeIndex;
    int leftPanelWidth = 0;
    bool devicePanelVisible = true;
    bool splitterDragging = false;
    bool debugResizeDragging = false;
    HWND originEditWindow = nullptr;
    WNDPROC originEditOriginalProc = nullptr;
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

std::string narrow(const std::wstring& value)
{
    if (value.empty()) {
        return {};
    }

    const int required = WideCharToMultiByte(
        CP_UTF8,
        0,
        value.data(),
        static_cast<int>(value.size()),
        nullptr,
        0,
        nullptr,
        nullptr
    );

    if (required <= 0) {
        return {};
    }

    std::string output(static_cast<std::size_t>(required), '\0');
    WideCharToMultiByte(
        CP_UTF8,
        0,
        value.data(),
        static_cast<int>(value.size()),
        output.data(),
        required,
        nullptr,
        nullptr
    );
    return output;
}

std::wstring yesNo(const bool value)
{
    return value ? L"Yes" : L"No";
}

int leftPanelWidthForClient(const AppWindowState* state, const int clientWidth);
int leftPanelContentBottomForClient(const AppWindowState* state, const int clientHeight);
void invalidateStatusPanel(HWND hwnd);
void layoutChildWindows(HWND hwnd);
void invalidateWindowLayout(HWND hwnd);
void exportCurrentSession(HWND hwnd, const ExportFormat format);
void importGlbFromFile(HWND hwnd, AppWindowState& state);
void refreshPoseAndViewport(HWND hwnd);
void refreshStatus(HWND hwnd, const bool forceDeviceEnumeration = false);

std::filesystem::path executableDirectoryPath()
{
    std::vector<wchar_t> buffer(MAX_PATH);
    while (true) {
        const DWORD length = GetModuleFileNameW(
            nullptr,
            buffer.data(),
            static_cast<DWORD>(buffer.size())
        );
        if (length == 0) {
            return std::filesystem::current_path();
        }
        if (length < buffer.size()) {
            return std::filesystem::path(std::wstring(buffer.data(), length)).parent_path();
        }
        buffer.resize(buffer.size() * 2);
        if (buffer.size() > 32768) {
            return std::filesystem::current_path();
        }
    }
}

std::filesystem::path configDirectoryPath()
{
    return executableDirectoryPath() / "config";
}

std::filesystem::path configFilePath(const char* fileName)
{
    return configDirectoryPath() / fileName;
}

std::filesystem::path legacyExecutableConfigPath(const char* fileName)
{
    return executableDirectoryPath() / fileName;
}

std::filesystem::path legacyWorkingDirectoryConfigPath(const char* fileName)
{
    return std::filesystem::current_path() / fileName;
}

std::filesystem::path readableConfigPath(const char* fileName)
{
    const std::filesystem::path preferredPath = configFilePath(fileName);
    std::error_code error;
    if (std::filesystem::exists(preferredPath, error)) {
        return preferredPath;
    }

    const std::filesystem::path executableLegacyPath = legacyExecutableConfigPath(fileName);
    error.clear();
    if (std::filesystem::exists(executableLegacyPath, error)) {
        return executableLegacyPath;
    }

    const std::filesystem::path workingDirectoryLegacyPath = legacyWorkingDirectoryConfigPath(fileName);
    error.clear();
    if (std::filesystem::exists(workingDirectoryLegacyPath, error)) {
        return workingDirectoryLegacyPath;
    }

    return preferredPath;
}

bool ensureConfigDirectory(std::string& error)
{
    std::error_code createError;
    const std::filesystem::path directory = configDirectoryPath();
    std::filesystem::create_directories(directory, createError);
    if (createError) {
        error = "could not create " + directory.string() + ": " + createError.message();
        return false;
    }
    return true;
}

std::filesystem::path originConfigPath()
{
    return configFilePath(kOriginConfigFileName);
}

std::filesystem::path deviceNameConfigPath()
{
    return configFilePath(kDeviceNameConfigFileName);
}

std::filesystem::path viewportSettingsConfigPath()
{
    return configFilePath(kViewportSettingsConfigFileName);
}

std::filesystem::path recordSettingsConfigPath()
{
    return configFilePath(kRecordSettingsConfigFileName);
}

std::filesystem::path defaultExportDirectoryPath()
{
    return std::filesystem::current_path() / "exports";
}

std::filesystem::path normalizedExportDirectoryPath(const std::filesystem::path& path)
{
    const std::filesystem::path requested = path.empty()
        ? defaultExportDirectoryPath()
        : path;
    if (requested.is_absolute()) {
        return requested.lexically_normal();
    }

    std::error_code error;
    const std::filesystem::path absolutePath = std::filesystem::absolute(requested, error);
    if (!error) {
        return absolutePath.lexically_normal();
    }
    return (std::filesystem::current_path() / requested).lexically_normal();
}

std::filesystem::path activeExportDirectoryPath(const AppWindowState& state)
{
    return normalizedExportDirectoryPath(state.exportDirectory);
}

float sanitizedRecordDelaySeconds(const float value)
{
    return std::isfinite(value) && value > 0.0f ? value : 0.0f;
}

float sanitizedExportSampleRate(const float value)
{
    return std::isfinite(value) && value > 0.0f
        ? value
        : static_cast<float>(kDefaultExportSampleRate);
}

const char* exportFormatConfigValue(const ExportFormat format)
{
    return format == ExportFormat::Fbx ? "fbx" : "glb";
}

std::wstring exportFormatDisplayText(const ExportFormat format)
{
    return format == ExportFormat::Fbx ? L"fbx" : L"glb";
}

void appendDebugLog(AppWindowState& state, const std::wstring& message)
{
    const std::time_t currentTime = std::time(nullptr);
    std::tm localTime{};
    localtime_s(&localTime, &currentTime);

    std::wostringstream stream;
    stream << L"["
           << std::setw(2) << std::setfill(L'0') << localTime.tm_hour << L":"
           << std::setw(2) << std::setfill(L'0') << localTime.tm_min << L":"
           << std::setw(2) << std::setfill(L'0') << localTime.tm_sec << L"] "
           << message;

    state.debugLogLines.push_back(stream.str());
    if (state.debugLogLines.size() > kDebugLogMaxLines) {
        state.debugLogLines.erase(state.debugLogLines.begin());
    }
    if (state.debugLogScrollOffset > static_cast<int>(state.debugLogLines.size())) {
        state.debugLogScrollOffset = static_cast<int>(state.debugLogLines.size());
    }
}

void appendDebugLog(AppWindowState& state, const std::string& message)
{
    appendDebugLog(state, widen(message));
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
    stream << std::put_time(&localTime, "%Y_%m_%d_%H%M%S");
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

std::string trimAscii(std::string value)
{
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front())) != 0) {
        value.erase(value.begin());
    }
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())) != 0) {
        value.pop_back();
    }
    return value;
}

std::wstring trimWide(std::wstring value)
{
    while (!value.empty() && std::iswspace(value.front()) != 0) {
        value.erase(value.begin());
    }
    while (!value.empty() && std::iswspace(value.back()) != 0) {
        value.pop_back();
    }
    return value;
}

std::string lowerAscii(std::string value)
{
    for (char& ch : value) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return value;
}

bool parseExportFormatConfigValue(const std::string& value, ExportFormat& out)
{
    const std::string lowered = lowerAscii(trimAscii(value));
    if (lowered == "fbx") {
        out = ExportFormat::Fbx;
        return true;
    }
    if (lowered == "glb" || lowered == "gltf") {
        out = ExportFormat::Glb;
        return true;
    }
    return false;
}

bool parseBoolConfigValue(const std::string& value, bool& out)
{
    const std::string trimmed = trimAscii(value);
    if (trimmed == "1" || trimmed == "true" || trimmed == "True" || trimmed == "yes" || trimmed == "Yes") {
        out = true;
        return true;
    }
    if (trimmed == "0" || trimmed == "false" || trimmed == "False" || trimmed == "no" || trimmed == "No") {
        out = false;
        return true;
    }
    return false;
}

bool parseFloatConfigValue(const std::string& value, float& out)
{
    std::istringstream stream(trimAscii(value));
    stream >> out;
    return !stream.fail();
}

bool parseIntConfigValue(const std::string& value, int& out)
{
    std::istringstream stream(trimAscii(value));
    stream >> out;
    return !stream.fail();
}

int clampColorComponent(const int value)
{
    return std::clamp(value, 0, 255);
}

RgbColor clampRgbColor(const RgbColor color)
{
    return {
        clampColorComponent(color.r),
        clampColorComponent(color.g),
        clampColorComponent(color.b),
    };
}

COLORREF colorRefFromRgb(const RgbColor color)
{
    const RgbColor clamped = clampRgbColor(color);
    return RGB(clamped.r, clamped.g, clamped.b);
}

RgbColor rgbFromColorRef(const COLORREF color)
{
    return {
        static_cast<int>(GetRValue(color)),
        static_cast<int>(GetGValue(color)),
        static_cast<int>(GetBValue(color)),
    };
}

void setGlColor(const RgbColor color)
{
    const RgbColor clamped = clampRgbColor(color);
    glColor3f(
        static_cast<float>(clamped.r) / 255.0f,
        static_cast<float>(clamped.g) / 255.0f,
        static_cast<float>(clamped.b) / 255.0f
    );
}

void setGlClearColor(const RgbColor color)
{
    const RgbColor clamped = clampRgbColor(color);
    glClearColor(
        static_cast<float>(clamped.r) / 255.0f,
        static_cast<float>(clamped.g) / 255.0f,
        static_cast<float>(clamped.b) / 255.0f,
        1.0f
    );
}

std::string deviceNameKeyForParts(const std::string& deviceClass, const std::string& serial)
{
    return deviceClass + '\x1f' + serial;
}

std::string deviceNameKeyForDevice(const ovtr::DeviceDescriptor& device)
{
    return deviceNameKeyForParts(ovtr::toString(device.deviceClass), device.serial);
}

bool splitDeviceNameKey(const std::string& key, std::string& deviceClass, std::string& serial)
{
    const std::size_t separator = key.find('\x1f');
    if (separator == std::string::npos) {
        return false;
    }

    deviceClass = key.substr(0, separator);
    serial = key.substr(separator + 1);
    return true;
}

std::string customNameForDevice(const AppWindowState& state, const ovtr::DeviceDescriptor& device)
{
    const auto entry = state.deviceCustomNames.find(deviceNameKeyForDevice(device));
    if (entry == state.deviceCustomNames.end()) {
        return {};
    }
    return entry->second;
}

void applyCustomNamesToExportDevices(
    const AppWindowState& state,
    std::vector<ovtr::DeviceDescriptor>& devices
)
{
    for (ovtr::DeviceDescriptor& device : devices) {
        device.displayName = customNameForDevice(state, device);
    }
}

bool writeDeviceNameConfigFile(const AppWindowState& state, std::string& error)
{
    const std::filesystem::path path = deviceNameConfigPath();
    if (!ensureConfigDirectory(error)) {
        return false;
    }

    std::ofstream output(path, std::ios::trunc);
    if (!output) {
        error = "could not open " + path.string();
        return false;
    }

    output << "# device_class serial custom_name\n";
    for (const auto& entry : state.deviceCustomNames) {
        if (entry.second.empty()) {
            continue;
        }

        std::string deviceClass;
        std::string serial;
        if (!splitDeviceNameKey(entry.first, deviceClass, serial)) {
            continue;
        }

        output << std::quoted(deviceClass) << " "
               << std::quoted(serial) << " "
               << std::quoted(entry.second) << "\n";
    }
    return true;
}

void saveDeviceNameConfig(AppWindowState& state)
{
    std::string error;
    if (!writeDeviceNameConfigFile(state, error)) {
        appendDebugLog(state, "Device name config save failed: " + error);
        return;
    }
    appendDebugLog(state, "Device name config saved: " + deviceNameConfigPath().string());
}

void loadDeviceNameConfig(AppWindowState& state)
{
    const std::filesystem::path path = readableConfigPath(kDeviceNameConfigFileName);
    std::ifstream input(path);
    if (!input) {
        appendDebugLog(state, "Device name config not found: " + path.string());
        return;
    }

    state.deviceCustomNames.clear();
    std::string line;
    while (std::getline(input, line)) {
        line = trimAscii(line);
        if (line.empty() || line.front() == '#') {
            continue;
        }

        std::istringstream stream(line);
        std::string deviceClass;
        std::string serial;
        std::string customName;
        if (!(stream >> std::quoted(deviceClass) >> std::quoted(serial) >> std::quoted(customName))) {
            appendDebugLog(state, "Device name config ignored invalid line");
            continue;
        }

        customName = trimAscii(customName);
        if (!customName.empty()) {
            state.deviceCustomNames[deviceNameKeyForParts(deviceClass, serial)] = customName;
        }
    }

    appendDebugLog(state, "Device name config loaded: " + path.string());
    if (path.lexically_normal() != deviceNameConfigPath().lexically_normal()) {
        saveDeviceNameConfig(state);
    }
}

bool writeRecordSettingsConfigFile(const AppWindowState& state, std::string& error)
{
    const std::filesystem::path path = recordSettingsConfigPath();
    if (!ensureConfigDirectory(error)) {
        return false;
    }

    std::ofstream output(path, std::ios::trunc);
    if (!output) {
        error = "could not open " + path.string();
        return false;
    }

    output << "directory=" << narrow(activeExportDirectoryPath(state).wstring()) << "\n";
    output << "record_delay_seconds=" << std::fixed << std::setprecision(3)
           << sanitizedRecordDelaySeconds(state.recordDelaySeconds) << "\n";
    output << "resample_fps=" << sanitizedExportSampleRate(state.recordExportSampleRate) << "\n";
    output << "save_format=" << exportFormatConfigValue(state.recordSaveFormat) << "\n";
    return true;
}

void saveRecordSettingsConfig(AppWindowState& state)
{
    state.exportDirectory = activeExportDirectoryPath(state);
    state.recordDelaySeconds = sanitizedRecordDelaySeconds(state.recordDelaySeconds);
    state.recordExportSampleRate = sanitizedExportSampleRate(state.recordExportSampleRate);
    std::string error;
    if (!writeRecordSettingsConfigFile(state, error)) {
        appendDebugLog(state, "Record settings config save failed: " + error);
        return;
    }
    appendDebugLog(state, "Record settings config saved: " + recordSettingsConfigPath().string());
}

void loadRecordSettingsConfig(AppWindowState& state)
{
    std::filesystem::path path = readableConfigPath(kRecordSettingsConfigFileName);
    std::ifstream input(path);
    if (!input) {
        path = readableConfigPath(kLegacyExportLocationConfigFileName);
        input.clear();
        input.open(path);
    }
    if (!input) {
        state.exportDirectory = defaultExportDirectoryPath();
        state.recordDelaySeconds = 0.0f;
        state.recordExportSampleRate = static_cast<float>(kDefaultExportSampleRate);
        state.recordSaveFormat = ExportFormat::Glb;
        appendDebugLog(state, "Record settings config not found: " + path.string());
        return;
    }

    std::filesystem::path loadedDirectory;
    float recordDelaySeconds = 0.0f;
    float exportSampleRate = static_cast<float>(kDefaultExportSampleRate);
    ExportFormat saveFormat = ExportFormat::Glb;
    std::string line;
    while (std::getline(input, line)) {
        const std::size_t separator = line.find('=');
        if (separator == std::string::npos) {
            const std::string value = trimAscii(line);
            if (!value.empty() && value.front() != '#') {
                loadedDirectory = std::filesystem::path(widen(value));
            }
            continue;
        }

        const std::string key = trimAscii(line.substr(0, separator));
        const std::string value = trimAscii(line.substr(separator + 1));
        if (key == "directory" || key == "path" || key == "export_directory") {
            loadedDirectory = std::filesystem::path(widen(value));
        } else if (key == "record_delay_seconds" || key == "record_delay" || key == "delay_seconds") {
            float parsedDelay = 0.0f;
            if (parseFloatConfigValue(value, parsedDelay)) {
                recordDelaySeconds = parsedDelay;
            }
        } else if (key == "resample_fps" || key == "export_sample_rate" || key == "export_fps") {
            float parsedSampleRate = 0.0f;
            if (parseFloatConfigValue(value, parsedSampleRate)) {
                exportSampleRate = parsedSampleRate;
            }
        } else if (key == "save_format" || key == "format" || key == "export_format") {
            parseExportFormatConfigValue(value, saveFormat);
        }
    }

    state.exportDirectory = normalizedExportDirectoryPath(loadedDirectory);
    state.recordDelaySeconds = sanitizedRecordDelaySeconds(recordDelaySeconds);
    state.recordExportSampleRate = sanitizedExportSampleRate(exportSampleRate);
    state.recordSaveFormat = saveFormat;
    appendDebugLog(state, "Record settings config loaded: " + path.string());
    if (path.lexically_normal() != recordSettingsConfigPath().lexically_normal()) {
        saveRecordSettingsConfig(state);
    }
}

bool writeOriginConfigFile(const AppWindowState& state, std::string& error)
{
    const std::filesystem::path path = originConfigPath();
    if (!ensureConfigDirectory(error)) {
        return false;
    }

    std::ofstream output(path, std::ios::trunc);
    if (!output) {
        error = "could not open " + path.string();
        return false;
    }

    output << "enabled=" << (state.originEnabled ? 1 : 0) << "\n";
    output << std::fixed << std::setprecision(9)
           << "x=" << state.originOffset[0] << "\n"
           << "y=" << state.originOffset[1] << "\n"
           << "z=" << state.originOffset[2] << "\n"
           << "rx=" << state.originRotationDegrees[0] << "\n"
           << "ry=" << state.originRotationDegrees[1] << "\n"
           << "rz=" << state.originRotationDegrees[2] << "\n";
    return true;
}

void saveOriginConfig(AppWindowState& state)
{
    std::string error;
    if (!writeOriginConfigFile(state, error)) {
        state.originStatusMessage = "origin config save failed: " + error;
        appendDebugLog(state, state.originStatusMessage);
        return;
    }
    appendDebugLog(state, "Origin config saved: " + originConfigPath().string());
}

void loadOriginConfig(AppWindowState& state)
{
    const std::filesystem::path path = readableConfigPath(kOriginConfigFileName);
    std::ifstream input(path);
    if (!input) {
        appendDebugLog(state, "Origin config not found: " + path.string());
        return;
    }

    bool enabled = false;
    bool hasEnabled = false;
    bool hasX = false;
    bool hasY = false;
    bool hasZ = false;
    std::array<float, 3> offset{0.0f, 0.0f, 0.0f};
    std::array<float, 3> rotation{0.0f, 0.0f, 0.0f};

    std::string line;
    while (std::getline(input, line)) {
        const std::size_t separator = line.find('=');
        if (separator == std::string::npos) {
            continue;
        }

        const std::string key = trimAscii(line.substr(0, separator));
        const std::string value = trimAscii(line.substr(separator + 1));
        if (key == "enabled") {
            hasEnabled = parseBoolConfigValue(value, enabled);
        } else if (key == "x") {
            hasX = parseFloatConfigValue(value, offset[0]);
        } else if (key == "y") {
            hasY = parseFloatConfigValue(value, offset[1]);
        } else if (key == "z") {
            hasZ = parseFloatConfigValue(value, offset[2]);
        } else if (key == "rx") {
            parseFloatConfigValue(value, rotation[0]);
        } else if (key == "ry") {
            parseFloatConfigValue(value, rotation[1]);
        } else if (key == "rz") {
            parseFloatConfigValue(value, rotation[2]);
        }
    }

    if (!hasEnabled) {
        appendDebugLog(state, "Origin config ignored: missing enabled flag");
        return;
    }
    if (!enabled) {
        state.originEnabled = false;
        state.originOffset = {0.0f, 0.0f, 0.0f};
        state.originRotationDegrees = {0.0f, 0.0f, 0.0f};
        state.originStatusMessage = "origin disabled by config";
        appendDebugLog(state, "Origin config loaded: disabled");
        return;
    }
    if (!hasX || !hasY || !hasZ) {
        appendDebugLog(state, "Origin config ignored: missing origin coordinates");
        return;
    }

    state.originEnabled = true;
    state.originOffset = offset;
    state.originRotationDegrees = rotation;
    state.originStatusMessage = "origin loaded from config";
    appendDebugLog(state, "Origin config loaded: " + path.string());
    if (path.lexically_normal() != originConfigPath().lexically_normal()) {
        saveOriginConfig(state);
    }
}

bool writeViewportSettingsConfigFile(const AppWindowState& state, std::string& error)
{
    const std::filesystem::path path = viewportSettingsConfigPath();
    if (!ensureConfigDirectory(error)) {
        return false;
    }

    const ViewportSettings& settings = state.viewportSettings;
    std::ofstream output(path, std::ios::trunc);
    if (!output) {
        error = "could not open " + path.string();
        return false;
    }

    output << "label_r=" << settings.labelTextColor.r << "\n"
           << "label_g=" << settings.labelTextColor.g << "\n"
           << "label_b=" << settings.labelTextColor.b << "\n"
           << "grid_r=" << settings.gridColor.r << "\n"
           << "grid_g=" << settings.gridColor.g << "\n"
           << "grid_b=" << settings.gridColor.b << "\n"
           << "background_r=" << settings.backgroundColor.r << "\n"
           << "background_g=" << settings.backgroundColor.g << "\n"
           << "background_b=" << settings.backgroundColor.b << "\n"
           << "imported_glb_r=" << settings.importedGlbColor.r << "\n"
           << "imported_glb_g=" << settings.importedGlbColor.g << "\n"
           << "imported_glb_b=" << settings.importedGlbColor.b << "\n"
           << std::fixed << std::setprecision(6)
           << "outline_multiplier=" << settings.outlineMultiplier << "\n";
    return true;
}

void saveViewportSettingsConfig(AppWindowState& state)
{
    state.viewportSettings.labelTextColor = clampRgbColor(state.viewportSettings.labelTextColor);
    state.viewportSettings.gridColor = clampRgbColor(state.viewportSettings.gridColor);
    state.viewportSettings.backgroundColor = clampRgbColor(state.viewportSettings.backgroundColor);
    state.viewportSettings.importedGlbColor = clampRgbColor(state.viewportSettings.importedGlbColor);
    state.viewportSettings.outlineMultiplier = std::clamp(state.viewportSettings.outlineMultiplier, 0.0f, 10.0f);

    std::string error;
    if (!writeViewportSettingsConfigFile(state, error)) {
        appendDebugLog(state, "Viewport settings save failed: " + error);
        return;
    }
    appendDebugLog(state, "Viewport settings saved: " + viewportSettingsConfigPath().string());
}

void loadViewportSettingsConfig(AppWindowState& state)
{
    const std::filesystem::path path = readableConfigPath(kViewportSettingsConfigFileName);
    std::ifstream input(path);
    if (!input) {
        appendDebugLog(state, "Viewport settings config not found: " + path.string());
        return;
    }

    ViewportSettings settings = state.viewportSettings;
    std::string line;
    while (std::getline(input, line)) {
        const std::size_t separator = line.find('=');
        if (separator == std::string::npos) {
            continue;
        }

        const std::string key = trimAscii(line.substr(0, separator));
        const std::string value = trimAscii(line.substr(separator + 1));
        int intValue = 0;
        float floatValue = 0.0f;
        if (key == "label_r" && parseIntConfigValue(value, intValue)) {
            settings.labelTextColor.r = intValue;
        } else if (key == "label_g" && parseIntConfigValue(value, intValue)) {
            settings.labelTextColor.g = intValue;
        } else if (key == "label_b" && parseIntConfigValue(value, intValue)) {
            settings.labelTextColor.b = intValue;
        } else if (key == "grid_r" && parseIntConfigValue(value, intValue)) {
            settings.gridColor.r = intValue;
        } else if (key == "grid_g" && parseIntConfigValue(value, intValue)) {
            settings.gridColor.g = intValue;
        } else if (key == "grid_b" && parseIntConfigValue(value, intValue)) {
            settings.gridColor.b = intValue;
        } else if (key == "background_r" && parseIntConfigValue(value, intValue)) {
            settings.backgroundColor.r = intValue;
        } else if (key == "background_g" && parseIntConfigValue(value, intValue)) {
            settings.backgroundColor.g = intValue;
        } else if (key == "background_b" && parseIntConfigValue(value, intValue)) {
            settings.backgroundColor.b = intValue;
        } else if ((key == "imported_glb_r" || key == "glb_r") && parseIntConfigValue(value, intValue)) {
            settings.importedGlbColor.r = intValue;
        } else if ((key == "imported_glb_g" || key == "glb_g") && parseIntConfigValue(value, intValue)) {
            settings.importedGlbColor.g = intValue;
        } else if ((key == "imported_glb_b" || key == "glb_b") && parseIntConfigValue(value, intValue)) {
            settings.importedGlbColor.b = intValue;
        } else if (key == "outline_multiplier" && parseFloatConfigValue(value, floatValue) && std::isfinite(floatValue)) {
            settings.outlineMultiplier = floatValue;
        }
    }

    settings.labelTextColor = clampRgbColor(settings.labelTextColor);
    settings.gridColor = clampRgbColor(settings.gridColor);
    settings.backgroundColor = clampRgbColor(settings.backgroundColor);
    settings.importedGlbColor = clampRgbColor(settings.importedGlbColor);
    settings.outlineMultiplier = std::clamp(settings.outlineMultiplier, 0.0f, 10.0f);
    state.viewportSettings = settings;
    appendDebugLog(state, "Viewport settings loaded: " + path.string());
    if (path.lexically_normal() != viewportSettingsConfigPath().lexically_normal()) {
        saveViewportSettingsConfig(state);
    }
}

std::vector<DeviceListRow> makeDeviceListRows(const AppWindowState& state)
{
    auto rankDevice = [](const ovtr::DeviceDescriptor& device) {
        const std::string modelName = lowerAscii(device.modelName);
        const bool isValveTrackingReference =
            modelName.find("valve sr imp") != std::string::npos ||
            modelName.find("valve sr") != std::string::npos;

        if (device.deviceClass == ovtr::DeviceClass::Hmd) {
            return 0;
        }
        if (device.deviceClass == ovtr::DeviceClass::TrackingReference || isValveTrackingReference) {
            return 1;
        }
        if (device.deviceClass == ovtr::DeviceClass::Controller) {
            return 2;
        }
        if (device.deviceClass == ovtr::DeviceClass::GenericTracker) {
            return 3;
        }
        return 4;
    };

    std::vector<const ovtr::DeviceDescriptor*> sortedDevices;
    sortedDevices.reserve(state.devices.size());
    for (const ovtr::DeviceDescriptor& device : state.devices) {
        sortedDevices.push_back(&device);
    }

    std::stable_sort(
        sortedDevices.begin(),
        sortedDevices.end(),
        [&](const ovtr::DeviceDescriptor* left, const ovtr::DeviceDescriptor* right) {
            return rankDevice(*left) < rankDevice(*right);
        }
    );

    std::vector<DeviceListRow> rows;
    rows.reserve(sortedDevices.size());
    for (const ovtr::DeviceDescriptor* device : sortedDevices) {
        rows.push_back(DeviceListRow{
            device->runtimeIndex,
            widen(customNameForDevice(state, *device)),
            widen(device->modelName.empty() ? ovtr::toString(device->deviceClass) : device->modelName),
            widen(device->serial.empty() ? "(none)" : device->serial),
        });
    }
    return rows;
}

int remainingRecordDelaySeconds(const AppWindowState& state);

std::vector<std::wstring> makeDebugMonitorLines(const AppWindowState& state)
{
    const ovtr::SteamVRRuntimeStatus& status = state.status;
    std::vector<std::wstring> lines;
    lines.emplace_back(L"Debug Monitor");
    lines.emplace_back(L"SteamVR runtime DLL loaded: " + yesNo(status.dllLoaded));
    lines.emplace_back(L"OpenVR runtime installed: " + yesNo(status.runtimeInstalled));
    lines.emplace_back(L"HMD present: " + yesNo(status.hmdPresent));
    lines.emplace_back(L"OpenVR provider initialized: " + yesNo(state.provider.isInitialized()));
    lines.emplace_back(L"DLL path: " + widen(status.dllPath));
    lines.emplace_back(L"Runtime path: " + widen(status.runtimePath));

    if (!status.error.empty()) {
        lines.emplace_back(L"Runtime status: " + widen(status.error));
    } else if (!status.runtimeInstalled) {
        lines.emplace_back(L"Runtime status: SteamVR runtime was not detected.");
    } else if (!status.hmdPresent) {
        lines.emplace_back(L"Runtime status: Runtime detected. No HMD currently reported.");
    } else {
        lines.emplace_back(L"Runtime status: Runtime and HMD detected.");
    }

    if (!state.providerError.empty()) {
        lines.emplace_back(L"Provider error: " + widen(state.providerError));
    }

    {
        std::wostringstream stream;
        stream << std::fixed << std::setprecision(1)
               << L"Pose FPS: " << state.posePollFps
               << L"   View FPS: " << state.renderFps
               << L"   Target: " << state.targetViewportFps
               << L"   Pose samples: " << state.poses.poses.size();
        lines.emplace_back(stream.str());
    }

    {
        std::wostringstream stream;
        stream << L"Recording: " << recorderStateText(state.recorder.state())
               << L"   Frames: " << state.recorder.frameCount()
               << L"   Dropped: " << state.recordingDroppedFrames
               << L"   Save: " << exportFormatDisplayText(state.recordSaveFormat)
               << L"   Resample: " << std::fixed << std::setprecision(3)
               << state.recordExportSampleRate << L"fps"
               << L"   Delay: " << std::fixed << std::setprecision(3)
               << state.recordDelaySeconds << L"s";
        if (state.recordingDelayActive) {
            stream << L"   Remaining: " << remainingRecordDelaySeconds(state) << L"s";
        }
        lines.emplace_back(stream.str());
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
                   << L", " << state.originOffset[2] << L")"
                   << L"   Rotation: (" << state.originRotationDegrees[0]
                   << L", " << state.originRotationDegrees[1]
                   << L", " << state.originRotationDegrees[2] << L")";
        }
        lines.emplace_back(stream.str());
    }
    if (!state.originStatusMessage.empty()) {
        lines.emplace_back(L"Origin status: " + widen(state.originStatusMessage));
    }

    if (!state.currentSessionFolder.empty()) {
        lines.emplace_back(L"Session folder: " + widen(state.currentSessionFolder.string()));
    }
    if (state.importedSceneLoaded) {
        std::wostringstream stream;
        stream << L"Imported GLB: " << widen(state.importedScene.sourcePath.filename().string())
               << L"   Nodes: " << state.importedScene.nodes.size()
               << L"   Meshes: " << state.importedScene.meshes.size()
               << L"   Duration: " << std::fixed << std::setprecision(3)
               << state.importedScene.durationSeconds << L"s";
        lines.emplace_back(stream.str());
    }
    if (!state.importStatusMessage.empty()) {
        lines.emplace_back(L"Import status: " + widen(state.importStatusMessage));
    }

    return lines;
}

int remainingRecordDelaySeconds(const AppWindowState& state)
{
    if (!state.recordingDelayActive) {
        return 0;
    }

    const auto now = std::chrono::steady_clock::now();
    const double remainingSeconds = std::chrono::duration<double>(
        state.recordingDelayDeadline - now
    ).count();
    return remainingSeconds > 0.0 ? static_cast<int>(std::ceil(remainingSeconds)) : 0;
}

std::wstring makeStatusBarMessage(const AppWindowState& state)
{
    const ovtr::SteamVRRuntimeStatus& status = state.status;

    if (!state.recordingError.empty()) {
        return L"Recording error: " + widen(state.recordingError);
    }
    if (state.recordingDelayActive) {
        return L"Recording starts in " + std::to_wstring(remainingRecordDelaySeconds(state)) + L"s";
    }
    if (!state.exportStatusMessage.empty()) {
        return L"Export: " + widen(state.exportStatusMessage);
    }
    if (!state.importStatusMessage.empty()) {
        return L"Import: " + widen(state.importStatusMessage);
    }
    if (!state.providerError.empty()) {
        return L"OpenVR provider: " + widen(state.providerError);
    }
    if (!status.error.empty()) {
        return L"SteamVR: " + widen(status.error);
    }
    if (state.recorder.state() == ovtr::RecorderState::Recording) {
        return L"Recording session";
    }
    if (state.recorder.state() == ovtr::RecorderState::Finalizing) {
        return L"Finalizing recording";
    }
    if (!status.runtimeInstalled) {
        return L"SteamVR runtime was not detected";
    }
    if (!status.hmdPresent) {
        return L"Runtime detected. No HMD currently reported";
    }
    if (!state.provider.isInitialized()) {
        return L"OpenVR provider initializing";
    }

    return L"Runtime and HMD detected";
}

std::wstring makeStatusBarMetrics(const AppWindowState& state)
{
    const std::wstring recordingState = state.recordingDelayActive
        ? L"Waiting"
        : recorderStateText(state.recorder.state());
    std::wostringstream stream;
    stream << std::fixed << std::setprecision(1)
           << L"Pose FPS " << state.posePollFps
           << L"   View FPS " << state.renderFps
           << L"   Target " << state.targetViewportFps
           << L"   Rec " << recordingState
           << L"   Frames " << state.recorder.frameCount()
           << L"   Dropped " << state.recordingDroppedFrames;
    return stream.str();
}

std::wstring formatOriginPanelPosition(const AppWindowState& state)
{
    std::wostringstream stream;
    stream << std::fixed << std::setprecision(3)
           << L"Position   X " << state.originOffset[0]
           << L"   Y " << state.originOffset[1]
           << L"   Z " << state.originOffset[2];
    return stream.str();
}

std::wstring formatOriginPanelRotation(const AppWindowState& state)
{
    std::wostringstream stream;
    stream << std::fixed << std::setprecision(3)
           << L"Rotation   X " << state.originRotationDegrees[0]
           << L"   Y " << state.originRotationDegrees[1]
           << L"   Z " << state.originRotationDegrees[2];
    return stream.str();
}

std::wstring formatOriginEditorText(const AppWindowState& state)
{
    std::wostringstream stream;
    stream << std::fixed << std::setprecision(6)
           << state.originOffset[0] << L" "
           << state.originOffset[1] << L" "
           << state.originOffset[2] << L" "
           << state.originRotationDegrees[0] << L" "
           << state.originRotationDegrees[1] << L" "
           << state.originRotationDegrees[2];
    return stream.str();
}

std::wstring formatOriginStepperValue(const float value)
{
    std::wostringstream stream;
    stream << std::fixed << std::setprecision(3) << value;
    return stream.str();
}

int maxDebugMonitorHeightForClient(const int clientHeight)
{
    const int maximumHeight = clientHeight - kStatusBarHeight - kContentMargin - 180;
    return maximumHeight > 0 ? maximumHeight : 0;
}

int clampDebugMonitorHeightForClient(const int requestedHeight, const int clientHeight)
{
    const int maximumHeight = maxDebugMonitorHeightForClient(clientHeight);
    if (maximumHeight <= 0) {
        return 0;
    }

    int minimumHeight = kDebugMonitorMinHeight;
    if (minimumHeight > maximumHeight) {
        minimumHeight = maximumHeight;
    }

    if (requestedHeight < minimumHeight) {
        return minimumHeight;
    }
    if (requestedHeight > maximumHeight) {
        return maximumHeight;
    }
    return requestedHeight;
}

int activeDebugMonitorHeight(const AppWindowState* state, const int clientHeight)
{
    if (!state || !state->debugMonitorVisible) {
        return 0;
    }

    return clampDebugMonitorHeightForClient(state->debugMonitorHeight, clientHeight);
}

RECT debugButtonRectForClient(const int clientWidth, const int clientHeight)
{
    const int statusBarTop = clientHeight > kStatusBarHeight
        ? clientHeight - kStatusBarHeight
        : 0;
    const int right = clientWidth > kContentMargin ? clientWidth - kContentMargin : clientWidth;
    const int left = right > kDebugButtonWidth ? right - kDebugButtonWidth : 0;
    const int top = statusBarTop + (kStatusBarHeight - kDebugButtonHeight) / 2;
    return RECT{left, top, right, top + kDebugButtonHeight};
}

RECT topBarSettingRectForClient(const int clientWidth, const int clientHeight)
{
    if (clientWidth <= 0 || clientHeight <= 0) {
        return RECT{0, 0, 0, 0};
    }

    const int top = 4;
    const int bottom = clientHeight < kTopBarHeight ? clientHeight : kTopBarHeight;
    const int buttonBottom = bottom > top ? bottom - 4 : bottom;
    const int left = 8 + kTopMenuFileWidth + kTopMenuGap;
    if (left >= clientWidth) {
        return RECT{0, 0, 0, 0};
    }
    const int right = left + kTopMenuSettingWidth < clientWidth
        ? left + kTopMenuSettingWidth
        : clientWidth;
    return RECT{left, top, right, buttonBottom};
}

RECT topBarFileRectForClient(const int clientWidth, const int clientHeight)
{
    if (clientWidth <= 8 || clientHeight <= 0) {
        return RECT{0, 0, 0, 0};
    }

    const int top = 4;
    const int bottom = clientHeight < kTopBarHeight ? clientHeight : kTopBarHeight;
    const int buttonBottom = bottom > top ? bottom - 4 : bottom;
    const int right = 8 + kTopMenuFileWidth < clientWidth
        ? 8 + kTopMenuFileWidth
        : clientWidth;
    return RECT{8, top, right, buttonBottom};
}

RECT deviceToggleButtonRectForClient(
    const AppWindowState* state,
    const int clientWidth,
    const int clientHeight
)
{
    if (!state || clientWidth <= 0 || clientHeight <= 0) {
        return RECT{0, 0, 0, 0};
    }

    const int contentBottom = leftPanelContentBottomForClient(state, clientHeight);
    const int top = kTopBarHeight + 12;
    if (contentBottom <= top + 48) {
        return RECT{0, 0, 0, 0};
    }

    int height = kDeviceToggleButtonHeight;
    const int availableHeight = contentBottom - top - 12;
    if (height > availableHeight) {
        height = availableHeight;
    }
    if (height < 48) {
        return RECT{0, 0, 0, 0};
    }

    const int left = (kDeviceToggleRailWidth - kDeviceToggleButtonWidth) / 2;
    return RECT{
        left,
        top,
        left + kDeviceToggleButtonWidth,
        top + height
    };
}

RECT splitterRectForClient(
    const AppWindowState* state,
    const int clientWidth,
    const int clientHeight
)
{
    const int leftPanelWidth = leftPanelWidthForClient(state, clientWidth);
    const int statusBarTop = clientHeight > kStatusBarHeight
        ? clientHeight - kStatusBarHeight
        : 0;
    const int debugMonitorHeight = activeDebugMonitorHeight(state, clientHeight);
    const int bottom = debugMonitorHeight > 0 && statusBarTop > debugMonitorHeight
        ? statusBarTop - debugMonitorHeight
        : statusBarTop;
    return RECT{leftPanelWidth, kTopBarHeight, leftPanelWidth + kSplitterWidth, bottom};
}

int leftPanelContentBottomForClient(const AppWindowState* state, const int clientHeight)
{
    const int statusBarTop = clientHeight > kStatusBarHeight
        ? clientHeight - kStatusBarHeight
        : 0;
    const int debugMonitorHeight = activeDebugMonitorHeight(state, clientHeight);
    if (debugMonitorHeight <= 0) {
        return statusBarTop;
    }
    return statusBarTop > debugMonitorHeight ? statusBarTop - debugMonitorHeight : statusBarTop;
}

ViewportControlLayout viewportControlLayoutForClient(
    const AppWindowState* state,
    const int clientWidth,
    const int clientHeight
)
{
    ViewportControlLayout layout;
    if (!state || clientWidth <= 0 || clientHeight <= 0) {
        return layout;
    }

    const int left = leftPanelWidthForClient(state, clientWidth) + kSplitterWidth;
    const int right = clientWidth;
    const int contentBottom = leftPanelContentBottomForClient(state, clientHeight);
    const int availableWidth = right - left;
    const int availableHeight = contentBottom - kTopBarHeight;
    const bool showAnimationControls = state->importedSceneLoaded;
    const int controlHeight = kViewportControlBarHeight +
        (showAnimationControls ? kImportedAnimationBarHeight : 0);
    if (availableWidth < 240 || availableHeight < 180 + controlHeight) {
        return layout;
    }

    layout.barRect = RECT{
        left,
        contentBottom - kViewportControlBarHeight,
        right,
        contentBottom
    };

    const int buttonY = layout.barRect.top + (kViewportControlBarHeight - kViewportControlButtonSize) / 2;
    const int buttonTotalWidth = kViewportControlButtonSize;
    const int buttonLeft = left + (availableWidth - buttonTotalWidth) / 2;
    layout.recordButtonRect = RECT{
        buttonLeft,
        buttonY,
        buttonLeft + kViewportControlButtonSize,
        buttonY + kViewportControlButtonSize
    };

    layout.valid = layout.recordButtonRect.right > layout.recordButtonRect.left &&
        layout.barRect.bottom > layout.barRect.top;

    if (showAnimationControls) {
        layout.animationBarRect = RECT{
            left,
            layout.barRect.top - kImportedAnimationBarHeight,
            right,
            layout.barRect.top
        };

        const int buttonTop = layout.animationBarRect.top +
            (kImportedAnimationBarHeight - kImportedAnimationButtonSize) / 2;
        int x = layout.animationBarRect.left + 14;
        layout.firstFrameButtonRect = RECT{
            x,
            buttonTop,
            x + kImportedAnimationButtonSize,
            buttonTop + kImportedAnimationButtonSize
        };
        x = layout.firstFrameButtonRect.right + 8;
        layout.playPauseButtonRect = RECT{
            x,
            buttonTop,
            x + kImportedAnimationButtonSize,
            buttonTop + kImportedAnimationButtonSize
        };
        x = layout.playPauseButtonRect.right + 8;
        layout.lastFrameButtonRect = RECT{
            x,
            buttonTop,
            x + kImportedAnimationButtonSize,
            buttonTop + kImportedAnimationButtonSize
        };

        layout.closeButtonRect = RECT{
            layout.animationBarRect.right - 14 - kImportedAnimationCloseButtonWidth,
            buttonTop,
            layout.animationBarRect.right - 14,
            buttonTop + kImportedAnimationButtonSize
        };
        layout.frameTextRect = RECT{
            layout.closeButtonRect.left - 12 - kImportedAnimationFrameTextWidth,
            layout.animationBarRect.top,
            layout.closeButtonRect.left - 12,
            layout.animationBarRect.bottom
        };
        layout.timelineRect = RECT{
            layout.lastFrameButtonRect.right + 18,
            layout.animationBarRect.top + 14,
            layout.frameTextRect.left - 18,
            layout.animationBarRect.bottom - 14
        };

        layout.animationValid =
            layout.animationBarRect.bottom > layout.animationBarRect.top &&
            layout.closeButtonRect.right > layout.closeButtonRect.left &&
            layout.timelineRect.right - layout.timelineRect.left >= kImportedAnimationTimelineMinWidth;
    }
    return layout;
}

RECT viewportRenderRectForClient(
    const AppWindowState* state,
    const int clientWidth,
    const int clientHeight
)
{
    if (!state || clientWidth <= 0 || clientHeight <= 0) {
        return RECT{0, 0, 0, 0};
    }

    const int left = leftPanelWidthForClient(state, clientWidth) + kSplitterWidth;
    const int contentBottom = leftPanelContentBottomForClient(state, clientHeight);
    const ViewportControlLayout controls = viewportControlLayoutForClient(
        state,
        clientWidth,
        clientHeight
    );
    const int bottom = controls.animationValid
        ? controls.animationBarRect.top
        : (controls.valid ? controls.barRect.top : contentBottom);
    return RECT{left, kTopBarHeight, clientWidth, bottom};
}

OriginPanelLayout originPanelLayoutForClient(
    const AppWindowState* state,
    const int clientWidth,
    const int clientHeight
)
{
    (void)state;
    (void)clientWidth;
    (void)clientHeight;
    return {};
}

RECT originEditorRectForClient(
    const AppWindowState* state,
    const int clientWidth,
    const int clientHeight
)
{
    const OriginPanelLayout layout = originPanelLayoutForClient(state, clientWidth, clientHeight);
    if (!layout.valid) {
        return RECT{0, 0, 0, 0};
    }

    RECT editRect = layout.valueRect;
    editRect.bottom = editRect.top + 26;
    if (editRect.bottom > layout.boxRect.bottom - kOriginPanelPadding) {
        editRect.bottom = layout.boxRect.bottom - kOriginPanelPadding;
    }
    return editRect;
}

RECT originStepperRowRect(const OriginPanelLayout& layout, const bool rotation)
{
    if (!layout.valid) {
        return RECT{0, 0, 0, 0};
    }

    const int top = layout.valueRect.top + (rotation ? kOriginStepperRowHeight : 0);
    const int bottom = top + kOriginStepperRowHeight;
    return RECT{
        layout.valueRect.left,
        top,
        layout.valueRect.right,
        bottom < layout.valueRect.bottom ? bottom : layout.valueRect.bottom
    };
}

std::vector<OriginStepperButton> originStepperButtonsForLayout(const OriginPanelLayout& layout)
{
    std::vector<OriginStepperButton> buttons;
    if (!layout.valid) {
        return buttons;
    }

    for (const bool rotation : {false, true}) {
        const RECT rowRect = originStepperRowRect(layout, rotation);
        const int rowHeight = rowRect.bottom - rowRect.top;
        const int rowWidth = rowRect.right - rowRect.left;
        if (rowHeight < kOriginStepperButtonSize || rowWidth <= kOriginStepperLabelWidth + 48) {
            continue;
        }

        const int columnsLeft = rowRect.left + kOriginStepperLabelWidth;
        const int columnWidth = (rowRect.right - columnsLeft) / 3;
        if (columnWidth < 56) {
            continue;
        }

        const int buttonTop = rowRect.top + (rowHeight - kOriginStepperButtonSize) / 2;
        for (int axis = 0; axis < 3; ++axis) {
            const int columnLeft = columnsLeft + axis * columnWidth;
            const int columnRight = axis == 2 ? rowRect.right : columnLeft + columnWidth;
            const int minusLeft = columnLeft + 14;
            const int plusRight = columnRight - 4;
            const float step = rotation ? kOriginRotationStepDegrees : kOriginPositionStep;

            buttons.push_back(OriginStepperButton{
                RECT{minusLeft, buttonTop, minusLeft + kOriginStepperButtonSize, buttonTop + kOriginStepperButtonSize},
                rotation,
                axis,
                -step,
                true,
            });
            buttons.push_back(OriginStepperButton{
                RECT{plusRight - kOriginStepperButtonSize, buttonTop, plusRight, buttonTop + kOriginStepperButtonSize},
                rotation,
                axis,
                step,
                true,
            });
        }
    }

    return buttons;
}

OriginStepperButton originStepperButtonFromPoint(
    const AppWindowState* state,
    const int clientWidth,
    const int clientHeight,
    const POINT point
)
{
    const OriginPanelLayout layout = originPanelLayoutForClient(state, clientWidth, clientHeight);
    if (!layout.valid) {
        return {};
    }

    const std::vector<OriginStepperButton> buttons = originStepperButtonsForLayout(layout);
    for (const OriginStepperButton& button : buttons) {
        if (button.valid && PtInRect(&button.rect, point)) {
            return button;
        }
    }
    return {};
}

DeviceListLayout deviceListLayoutForClient(
    const AppWindowState* state,
    const int clientWidth,
    const int clientHeight
)
{
    if (!state || !state->devicePanelVisible) {
        return {};
    }

    const int leftPanelWidth = leftPanelWidthForClient(state, clientWidth);
    const int textRight = leftPanelWidth - 24;
    const int contentBottom = leftPanelContentBottomForClient(state, clientHeight);
    const OriginPanelLayout originLayout = originPanelLayoutForClient(state, clientWidth, clientHeight);
    const int listBottom = originLayout.valid
        ? originLayout.boxRect.top - kOriginPanelGap
        : (contentBottom > 12 ? contentBottom - 12 : contentBottom);
    const int textBottom = listBottom > 0 ? listBottom : 0;

    int y = kTopBarHeight + 12;

    const int boxLeft = kDeviceToggleRailWidth + kContentMargin - 8;
    const int boxRight = textRight;
    const int boxTop = y;
    const int visibleBodyHeight = textBottom - boxTop - kDeviceListBoxPadding * 2 -
        kDeviceListHeaderHeight;
    const int visibleItemCount = visibleBodyHeight > 0
        ? visibleBodyHeight / kDeviceListItemHeight
        : 0;
    const int rowsForBox = state->devices.empty()
        ? 1
        : static_cast<int>(state->devices.size());
    const int drawnRows = rowsForBox < visibleItemCount ? rowsForBox : visibleItemCount;
    const int boxBottom = boxTop + kDeviceListBoxPadding + kDeviceListHeaderHeight +
        drawnRows * kDeviceListItemHeight + kDeviceListBoxPadding;

    DeviceListLayout layout;
    if (boxRight <= boxLeft + 80 ||
        drawnRows <= 0 ||
        boxBottom <= boxTop + kDeviceListBoxPadding ||
        boxTop >= textBottom) {
        return layout;
    }

    layout.boxRect = RECT{
        boxLeft,
        boxTop,
        boxRight,
        boxBottom < textBottom ? boxBottom : textBottom
    };
    layout.headerRect = RECT{
        boxLeft + kDeviceListBoxPadding,
        boxTop + kDeviceListBoxPadding,
        boxRight - kDeviceListBoxPadding,
        boxTop + kDeviceListBoxPadding + kDeviceListHeaderHeight
    };
    layout.contentRect = RECT{
        boxLeft + kDeviceListBoxPadding,
        boxTop + kDeviceListBoxPadding + kDeviceListHeaderHeight,
        boxRight - kDeviceListBoxPadding,
        layout.boxRect.bottom - kDeviceListBoxPadding
    };
    layout.visibleItemCount = visibleItemCount;
    layout.valid = layout.headerRect.right > layout.headerRect.left &&
        layout.headerRect.bottom > layout.headerRect.top &&
        layout.contentRect.right > layout.contentRect.left &&
        layout.contentRect.bottom > layout.contentRect.top;
    return layout;
}

RECT debugMessagesRectForClient(
    const AppWindowState* state,
    const int clientWidth,
    const int clientHeight
)
{
    const int statusBarTop = clientHeight > kStatusBarHeight
        ? clientHeight - kStatusBarHeight
        : 0;
    const int debugMonitorHeight = activeDebugMonitorHeight(state, clientHeight);
    if (debugMonitorHeight <= 0) {
        return RECT{0, 0, 0, 0};
    }

    const int debugMonitorTop = statusBarTop > debugMonitorHeight
        ? statusBarTop - debugMonitorHeight
        : statusBarTop;
    const int splitX = clientWidth > 900 ? (clientWidth * 48) / 100 : clientWidth / 2;
    const int rightColumnLeft = splitX + 18;
    return RECT{
        rightColumnLeft,
        debugMonitorTop + kDebugPanelPaddingTop + 28,
        clientWidth - kContentMargin,
        statusBarTop - 8
    };
}

RECT debugInfoRectForClient(
    const AppWindowState* state,
    const int clientWidth,
    const int clientHeight
)
{
    const int statusBarTop = clientHeight > kStatusBarHeight
        ? clientHeight - kStatusBarHeight
        : 0;
    const int debugMonitorHeight = activeDebugMonitorHeight(state, clientHeight);
    if (debugMonitorHeight <= 0) {
        return RECT{0, 0, 0, 0};
    }

    const int debugMonitorTop = statusBarTop > debugMonitorHeight
        ? statusBarTop - debugMonitorHeight
        : statusBarTop;
    const int splitX = clientWidth > 900 ? (clientWidth * 48) / 100 : clientWidth / 2;
    const int leftColumnRight = splitX - 18;
    return RECT{
        kContentMargin,
        debugMonitorTop + kDebugPanelPaddingTop + 28,
        leftColumnRight,
        statusBarTop - 8
    };
}

RECT debugResizeRectForClient(
    const AppWindowState* state,
    const int clientWidth,
    const int clientHeight
)
{
    const int statusBarTop = clientHeight > kStatusBarHeight
        ? clientHeight - kStatusBarHeight
        : 0;
    const int debugMonitorHeight = activeDebugMonitorHeight(state, clientHeight);
    if (debugMonitorHeight <= 0) {
        return RECT{0, 0, 0, 0};
    }

    const int debugMonitorTop = statusBarTop > debugMonitorHeight
        ? statusBarTop - debugMonitorHeight
        : statusBarTop;
    const int bottom = debugMonitorTop + kDebugResizeGripHeight < statusBarTop
        ? debugMonitorTop + kDebugResizeGripHeight
        : statusBarTop;
    return RECT{0, debugMonitorTop, clientWidth, bottom};
}

int visibleDebugLogLineCount(const RECT& messagesRect)
{
    const int height = messagesRect.bottom - messagesRect.top;
    if (height <= 0) {
        return 0;
    }
    return height / kDebugPanelLineHeight;
}

int maxDebugLogScrollOffset(const AppWindowState& state, const int visibleLineCount)
{
    if (visibleLineCount <= 0) {
        return 0;
    }
    const int totalLines = static_cast<int>(state.debugLogLines.size());
    return totalLines > visibleLineCount ? totalLines - visibleLineCount : 0;
}

void clampDebugLogScroll(AppWindowState& state, const int visibleLineCount)
{
    const int maxOffset = maxDebugLogScrollOffset(state, visibleLineCount);
    if (state.debugLogScrollOffset < 0) {
        state.debugLogScrollOffset = 0;
    } else if (state.debugLogScrollOffset > maxOffset) {
        state.debugLogScrollOffset = maxOffset;
    }
}

int maxDebugInfoScrollOffset(const AppWindowState& state, const int visibleLineCount)
{
    if (visibleLineCount <= 0) {
        return 0;
    }
    const int totalLines = static_cast<int>(makeDebugMonitorLines(state).size()) - 1;
    return totalLines > visibleLineCount ? totalLines - visibleLineCount : 0;
}

void clampDebugInfoScroll(AppWindowState& state, const int visibleLineCount)
{
    const int maxOffset = maxDebugInfoScrollOffset(state, visibleLineCount);
    if (state.debugInfoScrollOffset < 0) {
        state.debugInfoScrollOffset = 0;
    } else if (state.debugInfoScrollOffset > maxOffset) {
        state.debugInfoScrollOffset = maxOffset;
    }
}

int maxDeviceListScrollOffset(const AppWindowState& state, const int visibleItemCount)
{
    if (visibleItemCount <= 0) {
        return 0;
    }
    const int totalItems = static_cast<int>(state.devices.size());
    return totalItems > visibleItemCount ? totalItems - visibleItemCount : 0;
}

void clampDeviceListScroll(AppWindowState& state, const int visibleItemCount)
{
    const int maxOffset = maxDeviceListScrollOffset(state, visibleItemCount);
    if (state.deviceListScrollOffset < 0) {
        state.deviceListScrollOffset = 0;
    } else if (state.deviceListScrollOffset > maxOffset) {
        state.deviceListScrollOffset = maxOffset;
    }
}

std::uint32_t deviceRuntimeIndexFromListPoint(
    const AppWindowState& state,
    const DeviceListLayout& layout,
    const POINT point
)
{
    if (!layout.valid || state.devices.empty() || !PtInRect(&layout.contentRect, point)) {
        return kNoSelectedRuntimeIndex;
    }

    const int maxScrollOffset = maxDeviceListScrollOffset(state, layout.visibleItemCount);
    const int itemTextRight = maxScrollOffset > 0
        ? layout.contentRect.right - 14
        : layout.contentRect.right;
    if (point.x >= itemTextRight) {
        return kNoSelectedRuntimeIndex;
    }

    const int visibleRowIndex = (point.y - layout.contentRect.top) / kDeviceListItemHeight;
    if (visibleRowIndex < 0 || visibleRowIndex >= layout.visibleItemCount) {
        return kNoSelectedRuntimeIndex;
    }

    const int rowIndex = state.deviceListScrollOffset + visibleRowIndex;
    const std::vector<DeviceListRow> rows = makeDeviceListRows(state);
    if (rowIndex < 0 || rowIndex >= static_cast<int>(rows.size())) {
        return kNoSelectedRuntimeIndex;
    }
    return rows[static_cast<std::size_t>(rowIndex)].runtimeIndex;
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

float outlineExpansionForDepth(const float cameraDepth, const int viewportHeight, const float multiplier)
{
    if (viewportHeight <= 0) {
        return 0.0f;
    }

    const float fovRadians = kViewportFovDegrees * 3.14159265359f / 180.0f;
    const float worldHeight = 2.0f * cameraDepth * std::tan(fovRadians * 0.5f);
    return worldHeight * kRenderModelOutlinePixels * multiplier / static_cast<float>(viewportHeight);
}

int defaultLeftPanelWidthForClient(const int clientWidth)
{
    const int proportional = static_cast<int>(
        static_cast<float>(clientWidth) * kDefaultLeftPanelWidthRatio
    );
    if (proportional < kDefaultLeftPanelMinWidth) {
        return kDefaultLeftPanelMinWidth;
    }
    if (proportional > kDefaultLeftPanelMaxWidth) {
        return kDefaultLeftPanelMaxWidth;
    }
    return proportional;
}

int clampLeftPanelWidthForClient(const int requestedWidth, const int clientWidth)
{
    const int maxByViewport = clientWidth - kSplitterWidth - kContentMargin - kViewportMinWidth;
    if (maxByViewport <= 0) {
        return clientWidth > kSplitterWidth ? clientWidth - kSplitterWidth : 0;
    }

    int maxWidth = maxByViewport < kLeftPanelMaxWidth ? maxByViewport : kLeftPanelMaxWidth;
    if (maxWidth < kLeftPanelMinWidth) {
        maxWidth = maxByViewport;
    }

    int minWidth = kLeftPanelMinWidth;
    if (minWidth > maxWidth) {
        minWidth = maxWidth;
    }

    if (requestedWidth < minWidth) {
        return minWidth;
    }
    if (requestedWidth > maxWidth) {
        return maxWidth;
    }
    return requestedWidth;
}

int leftPanelWidthForClient(const AppWindowState* state, const int clientWidth)
{
    if (state && !state->devicePanelVisible) {
        return clientWidth > kDeviceToggleRailWidth ? kDeviceToggleRailWidth : clientWidth;
    }

    const int requestedWidth = state && state->leftPanelWidth > 0
        ? state->leftPanelWidth
        : defaultLeftPanelWidthForClient(clientWidth);
    return clampLeftPanelWidthForClient(requestedWidth, clientWidth);
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

void drawGroundGrid3D(const RgbColor gridColor)
{
    glLineWidth(1.0f);
    setGlColor(gridColor);
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

std::array<float, 4> normalizeQuaternion(const std::array<float, 4>& quaternion)
{
    const float length = std::sqrt(
        quaternion[0] * quaternion[0] +
        quaternion[1] * quaternion[1] +
        quaternion[2] * quaternion[2] +
        quaternion[3] * quaternion[3]
    );
    if (length <= 0.000001f) {
        return {0.0f, 0.0f, 0.0f, 1.0f};
    }
    return {
        quaternion[0] / length,
        quaternion[1] / length,
        quaternion[2] / length,
        quaternion[3] / length,
    };
}

std::array<float, 4> multiplyQuaternion(
    const std::array<float, 4>& left,
    const std::array<float, 4>& right
)
{
    const float lx = left[0];
    const float ly = left[1];
    const float lz = left[2];
    const float lw = left[3];
    const float rx = right[0];
    const float ry = right[1];
    const float rz = right[2];
    const float rw = right[3];
    return {
        lw * rx + lx * rw + ly * rz - lz * ry,
        lw * ry - lx * rz + ly * rw + lz * rx,
        lw * rz + lx * ry - ly * rx + lz * rw,
        lw * rw - lx * rx - ly * ry - lz * rz,
    };
}

std::array<float, 4> conjugateQuaternion(const std::array<float, 4>& quaternion)
{
    return {-quaternion[0], -quaternion[1], -quaternion[2], quaternion[3]};
}

std::array<float, 4> quaternionFromEulerDegrees(const std::array<float, 3>& degrees)
{
    const float halfX = degrees[0] * kPi / 360.0f;
    const float halfY = degrees[1] * kPi / 360.0f;
    const float halfZ = degrees[2] * kPi / 360.0f;

    const std::array<float, 4> qx{std::sin(halfX), 0.0f, 0.0f, std::cos(halfX)};
    const std::array<float, 4> qy{0.0f, std::sin(halfY), 0.0f, std::cos(halfY)};
    const std::array<float, 4> qz{0.0f, 0.0f, std::sin(halfZ), std::cos(halfZ)};
    return normalizeQuaternion(multiplyQuaternion(qz, multiplyQuaternion(qy, qx)));
}

std::array<float, 3> rotatePositionByQuaternion(
    const std::array<float, 4>& quaternion,
    const std::array<float, 3>& position
)
{
    const std::array<float, 4> vectorQuaternion{position[0], position[1], position[2], 0.0f};
    const std::array<float, 4> normalized = normalizeQuaternion(quaternion);
    const std::array<float, 4> rotated = multiplyQuaternion(
        multiplyQuaternion(normalized, vectorQuaternion),
        conjugateQuaternion(normalized)
    );
    return {rotated[0], rotated[1], rotated[2]};
}

float yawDegreesFromQuaternion(const std::array<float, 4>& quaternion)
{
    const std::array<float, 3> forward = rotatePositionByQuaternion(quaternion, {0.0f, 0.0f, 1.0f});
    if (std::sqrt(forward[0] * forward[0] + forward[2] * forward[2]) <= 0.000001f) {
        return 0.0f;
    }
    return std::atan2(forward[0], forward[2]) * 180.0f / kPi;
}

ovtr::PoseSample applyOriginToPose(
    ovtr::PoseSample pose,
    const bool originEnabled,
    const std::array<float, 3>& originOffset,
    const std::array<float, 3>& originRotationDegrees
)
{
    if (originEnabled) {
        const std::array<float, 4> inverseOriginRotation =
            conjugateQuaternion(quaternionFromEulerDegrees(originRotationDegrees));
        const std::array<float, 3> translatedPosition{
            pose.position[0] - originOffset[0],
            pose.position[1] - originOffset[1],
            pose.position[2] - originOffset[2],
        };
        pose.position = rotatePositionByQuaternion(inverseOriginRotation, translatedPosition);
        pose.rotation = normalizeQuaternion(multiplyQuaternion(inverseOriginRotation, pose.rotation));
    }
    return pose;
}

ovtr::PosePollResult applyOriginToPoses(
    ovtr::PosePollResult poses,
    const bool originEnabled,
    const std::array<float, 3>& originOffset,
    const std::array<float, 3>& originRotationDegrees
)
{
    if (!originEnabled) {
        return poses;
    }

    for (ovtr::PoseSample& pose : poses.poses) {
        pose = applyOriginToPose(pose, true, originOffset, originRotationDegrees);
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

const ovtr::DeviceDescriptor* selectedListDevice(const AppWindowState& state)
{
    return deviceForRuntimeIndex(state.devices, state.selectedDeviceRuntimeIndex);
}

bool confirmSetOrigin(HWND hwnd, AppWindowState& state, const ovtr::DeviceDescriptor& selected)
{
    std::wstring message = L"Set this device position and Y rotation as the origin?\n\n";
    message += widen(deviceDisplayName(selected));
    message += L"\n\nPosition and Y rotation will be used. X and Z rotation will be set to 0.";

    const int result = MessageBoxW(
        hwnd,
        message.c_str(),
        L"Set to Origin",
        MB_OKCANCEL | MB_ICONQUESTION | MB_DEFBUTTON2
    );
    if (result == IDOK) {
        return true;
    }

    state.originStatusMessage = "set origin canceled";
    appendDebugLog(state, state.originStatusMessage);
    return false;
}

void clearMissingDeviceSelection(AppWindowState& state)
{
    if (state.selectedDeviceRuntimeIndex == kNoSelectedRuntimeIndex) {
        return;
    }
    if (selectedListDevice(state) != nullptr) {
        return;
    }
    state.selectedDeviceRuntimeIndex = kNoSelectedRuntimeIndex;
    appendDebugLog(state, L"Device selection cleared: device unavailable");
}

void toggleListDeviceSelection(AppWindowState& state, const ovtr::DeviceDescriptor& device)
{
    if (state.selectedDeviceRuntimeIndex == device.runtimeIndex) {
        state.selectedDeviceRuntimeIndex = kNoSelectedRuntimeIndex;
        appendDebugLog(state, L"Device selection cleared");
        return;
    }

    state.selectedDeviceRuntimeIndex = device.runtimeIndex;
    appendDebugLog(state, L"Device selected: " + widen(deviceDisplayName(device)));
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
        appendDebugLog(state, state.originStatusMessage);
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
    appendDebugLog(state, state.originStatusMessage);
}

bool setOriginFromDevice(AppWindowState& state, const ovtr::DeviceDescriptor& selected)
{
    const ovtr::PoseSample* pose = poseForRuntimeIndex(state.poses, selected.runtimeIndex);
    if (pose == nullptr || !isPoseValid(*pose)) {
        state.originStatusMessage = "selected device has no valid pose";
        appendDebugLog(state, state.originStatusMessage);
        return false;
    }

    state.originEnabled = true;
    state.originOffset = pose->position;
    state.originRotationDegrees = {0.0f, yawDegreesFromQuaternion(pose->rotation), 0.0f};
    state.selectedOriginRuntimeIndex = selected.runtimeIndex;
    state.originStatusMessage = "origin position and Y rotation set from " + deviceDisplayName(selected);
    appendDebugLog(state, state.originStatusMessage);
    saveOriginConfig(state);
    return true;
}

bool setOriginFromSelectedDevice(AppWindowState& state)
{
    ensureOriginSelection(state);
    const ovtr::DeviceDescriptor* selected = selectedOriginDevice(state);
    if (selected == nullptr) {
        state.originStatusMessage = "no device selected for origin";
        appendDebugLog(state, state.originStatusMessage);
        return false;
    }

    return setOriginFromDevice(state, *selected);
}

bool setOriginFromSelectedListDevice(AppWindowState& state)
{
    const ovtr::DeviceDescriptor* selected = selectedListDevice(state);
    if (selected == nullptr) {
        state.originStatusMessage = "no device selected in list for origin";
        appendDebugLog(state, state.originStatusMessage);
        return false;
    }

    return setOriginFromDevice(state, *selected);
}

void clearOrigin(AppWindowState& state)
{
    state.originEnabled = false;
    state.originOffset = {0.0f, 0.0f, 0.0f};
    state.originRotationDegrees = {0.0f, 0.0f, 0.0f};
    state.originStatusMessage = "origin cleared";
    appendDebugLog(state, state.originStatusMessage);
    saveOriginConfig(state);
}

std::string labelForDevice(
    const AppWindowState& state,
    const ovtr::DeviceDescriptor* device,
    const ovtr::PoseSample& pose
)
{
    if (device) {
        const std::string customName = customNameForDevice(state, *device);
        if (!customName.empty()) {
            return customName;
        }
    }

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
    const bool drawBody = true,
    const bool selected = false
)
{
    const float x = pose.position[0];
    const float y = pose.position[1];
    const float z = pose.position[2];
    const bool isTrackingReference = deviceClass == ovtr::DeviceClass::TrackingReference;
    const float radius = isTrackingReference ? 0.0325f : 0.02f;
    constexpr float axisLength = 0.08f;

    if (selected) {
        glColor3f(1.0f, 0.12f, 0.10f);
    } else if ((pose.flags & ovtr::PoseFlagPoseValid) == 0) {
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
    const int viewportHeight,
    const bool selected
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
    if (selected) {
        glColor3f(1.0f, 0.12f, 0.10f);
    } else if (device->deviceClass == ovtr::DeviceClass::TrackingReference) {
        glColor3f(0.72f, 0.42f, 1.0f);
    } else if (device->deviceClass == ovtr::DeviceClass::Hmd) {
        glColor3f(0.20f, 1.0f, 0.82f);
    } else {
        glColor3f(1.0f, 0.58f, 0.12f);
    }
    const float outlineExpansion = outlineExpansionForDepth(
        cameraDepthForWorldPoint(state, {pose.position[0], pose.position[1], pose.position[2]}),
        viewportHeight,
        state.viewportSettings.outlineMultiplier
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

void drawImportedGltfMeshTriangles(const ovtr::RenderModelGeometry& mesh)
{
    glBegin(GL_TRIANGLES);
    for (const std::uint16_t index : mesh.indices) {
        if (index >= mesh.vertices.size()) {
            continue;
        }
        const ovtr::RenderModelVertex& vertex = mesh.vertices[index];
        glNormal3f(vertex.normal[0], vertex.normal[1], vertex.normal[2]);
        glVertex3f(vertex.position[0], vertex.position[1], vertex.position[2]);
    }
    glEnd();
}

void drawImportedFallbackMarker3D()
{
    constexpr float radius = 0.025f;
    constexpr float axisLength = 0.10f;

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

    glLineWidth(1.25f);
    glBegin(GL_LINES);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(axisLength, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, axisLength, 0.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, axisLength);
    glEnd();
}

double importedSceneDurationSeconds(const AppWindowState& state)
{
    if (!state.importedSceneLoaded || state.importedScene.durationSeconds <= 0.000001) {
        return 0.0;
    }
    return state.importedScene.durationSeconds;
}

int importedSceneTotalFrames(const AppWindowState& state)
{
    const double durationSeconds = importedSceneDurationSeconds(state);
    if (durationSeconds <= 0.0) {
        return state.importedSceneLoaded ? 1 : 0;
    }
    const int totalFrames = static_cast<int>(std::floor(durationSeconds * kImportedAnimationFrameRate)) + 1;
    return totalFrames > 1 ? totalFrames : 1;
}

int importedSceneCurrentFrame(const AppWindowState& state)
{
    const int totalFrames = importedSceneTotalFrames(state);
    if (totalFrames <= 0) {
        return 0;
    }

    const int frame = static_cast<int>(std::floor(
        std::clamp(state.importedScenePlaybackSeconds, 0.0, importedSceneDurationSeconds(state)) *
        kImportedAnimationFrameRate
    )) + 1;
    return std::clamp(frame, 1, totalFrames);
}

void setImportedScenePlaybackSeconds(AppWindowState& state, const double playbackSeconds)
{
    const double durationSeconds = importedSceneDurationSeconds(state);
    state.importedScenePlaybackSeconds = std::clamp(playbackSeconds, 0.0, durationSeconds);
    state.importedSceneLastUpdate = std::chrono::steady_clock::now();
}

void updateImportedScenePlayback(AppWindowState& state)
{
    if (!state.importedSceneLoaded) {
        return;
    }

    const auto now = std::chrono::steady_clock::now();
    if (state.importedSceneLastUpdate == std::chrono::steady_clock::time_point{}) {
        state.importedSceneLastUpdate = now;
    }
    if (!state.importedScenePlaying || state.importedSceneTimelineDragging) {
        state.importedSceneLastUpdate = now;
        return;
    }

    const double elapsedSeconds = std::chrono::duration<double>(now - state.importedSceneLastUpdate).count();
    state.importedSceneLastUpdate = now;
    const double durationSeconds = importedSceneDurationSeconds(state);
    state.importedScenePlaybackSeconds += elapsedSeconds > 0.0 ? elapsedSeconds : 0.0;
    if (state.importedScenePlaybackSeconds >= durationSeconds) {
        state.importedScenePlaybackSeconds = durationSeconds;
        state.importedScenePlaying = false;
    }
}

double importedScenePlaybackTime(const AppWindowState& state)
{
    return state.importedSceneLoaded
        ? std::clamp(state.importedScenePlaybackSeconds, 0.0, importedSceneDurationSeconds(state))
        : 0.0;
}

void drawImportedGltfScene3D(AppWindowState& state)
{
    if (!state.importedSceneLoaded) {
        return;
    }

    const double playbackTime = importedScenePlaybackTime(state);

    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_CULL_FACE);
    glDepthFunc(GL_LEQUAL);
    setGlColor(state.viewportSettings.importedGlbColor);

    for (const ovtr::ImportedGltfNode& node : state.importedScene.nodes) {
        std::array<float, 3> translation{};
        std::array<float, 4> rotation{};
        ovtr::sampleImportedGltfNodePose(node, playbackTime, translation, rotation);

        glPushMatrix();
        glTranslatef(translation[0], translation[1], translation[2]);
        multiplyOpenGLMatrixFromQuaternion(rotation);
        glScalef(node.scale[0], node.scale[1], node.scale[2]);
        setGlColor(state.viewportSettings.importedGlbColor);

        if (node.meshIndex >= 0 &&
            static_cast<std::size_t>(node.meshIndex) < state.importedScene.meshes.size()) {
            const ovtr::RenderModelGeometry& mesh = state.importedScene.meshes[static_cast<std::size_t>(node.meshIndex)];
            if (mesh.available) {
                drawImportedGltfMeshTriangles(mesh);
            } else {
                drawImportedFallbackMarker3D();
            }
        } else {
            drawImportedFallbackMarker3D();
        }
        glPopMatrix();
    }

    glDepthFunc(GL_LESS);
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
    const AppWindowState& state,
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
    drawLabelText3D(labelForDevice(state, device, pose), fontBase);
}

bool shouldDrawRecordingViewportBorder(const AppWindowState& state)
{
    return state.recorder.state() == ovtr::RecorderState::Recording;
}

void drawRecordingViewportBorder2D(const int width, const int height)
{
    if (width <= 0 || height <= 0) {
        return;
    }

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(
        0.0,
        static_cast<GLdouble>(width),
        static_cast<GLdouble>(height),
        0.0,
        -1.0,
        1.0
    );

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_CULL_FACE);
    glColor3f(1.0f, 0.04f, 0.02f);
    glLineWidth(kRecordingViewportBorderPixels);

    const float inset = kRecordingViewportBorderPixels * 0.5f;
    glBegin(GL_LINE_LOOP);
    glVertex2f(inset, inset);
    glVertex2f(static_cast<float>(width) - inset, inset);
    glVertex2f(static_cast<float>(width) - inset, static_cast<float>(height) - inset);
    glVertex2f(inset, static_cast<float>(height) - inset);
    glEnd();

    glLineWidth(1.0f);
    glPopMatrix();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void drawDelayCountdownOverlay2D(const AppWindowState& state, const int width, const int height)
{
    if (!state.recordingDelayActive || state.glOverlayFontBase == 0 || width <= 0 || height <= 0) {
        return;
    }

    const std::string text = std::to_string(remainingRecordDelaySeconds(state));

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(
        0.0,
        static_cast<GLdouble>(width),
        static_cast<GLdouble>(height),
        0.0,
        -1.0,
        1.0
    );

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_CULL_FACE);
    setGlColor(state.viewportSettings.labelTextColor);
    glRasterPos2f(24.0f, 64.0f);
    drawLabelText3D(text, state.glOverlayFontBase);

    glPopMatrix();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
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

    setGlClearColor(state->viewportSettings.backgroundColor);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    glTranslatef(0.0f, -0.55f, -positiveCameraDistance(state->cameraDistance));
    glRotatef(state->cameraPitchDegrees, 1.0f, 0.0f, 0.0f);
    glRotatef(state->cameraYawDegrees, 0.0f, 1.0f, 0.0f);
    glTranslatef(-state->cameraPanX, -state->cameraPanY, -state->cameraPanZ);

    drawGroundGrid3D(state->viewportSettings.gridColor);
    drawAxes3D();

    for (const ovtr::PoseSample& pose : state->poses.poses) {
        const ovtr::PoseSample displayPose = applyOriginToPose(
            pose,
            state->originEnabled,
            state->originOffset,
            state->originRotationDegrees
        );
        const ovtr::DeviceDescriptor* device = deviceForRuntimeIndex(state->devices, displayPose.runtimeIndex);
        const ovtr::DeviceClass deviceClass = device ? device->deviceClass : ovtr::DeviceClass::Other;
        const bool selected = displayPose.runtimeIndex == state->selectedDeviceRuntimeIndex;
        const bool modelDrawn = drawSteamVRRenderModel3D(*state, displayPose, device, height, selected);
        drawDeviceMarker3D(displayPose, deviceClass, !modelDrawn, selected);
    }

    drawImportedGltfScene3D(*state);

    glDisable(GL_DEPTH_TEST);
    setGlColor(state->viewportSettings.labelTextColor);
    for (const ovtr::PoseSample& pose : state->poses.poses) {
        const ovtr::PoseSample displayPose = applyOriginToPose(
            pose,
            state->originEnabled,
            state->originOffset,
            state->originRotationDegrees
        );
        drawDeviceLabel3D(
            *state,
            displayPose,
            deviceForRuntimeIndex(state->devices, displayPose.runtimeIndex),
            state->glLabelFontBase
        );
    }

    drawDelayCountdownOverlay2D(*state, width, height);

    if (shouldDrawRecordingViewportBorder(*state)) {
        drawRecordingViewportBorder2D(width, height);
    }

    SwapBuffers(state->glDeviceContext);
    ++state->renderFrames;
}

bool parseOriginEditorText(
    const std::wstring& text,
    std::array<float, 3>& offset,
    std::array<float, 3>& rotation
)
{
    std::vector<float> values;
    values.reserve(6);

    const wchar_t* cursor = text.c_str();
    while (*cursor != L'\0' && values.size() < 6) {
        wchar_t* end = nullptr;
        const float value = std::wcstof(cursor, &end);
        if (end == cursor) {
            ++cursor;
            continue;
        }
        if (!std::isfinite(value)) {
            return false;
        }
        values.push_back(value);
        cursor = end;
    }

    if (values.size() < 6) {
        return false;
    }

    offset = {values[0], values[1], values[2]};
    rotation = {values[3], values[4], values[5]};
    return true;
}

bool originValuesAreZero(
    const std::array<float, 3>& offset,
    const std::array<float, 3>& rotation
)
{
    constexpr float kOriginZeroThreshold = 0.000001f;
    for (const float value : offset) {
        if (std::fabs(value) > kOriginZeroThreshold) {
            return false;
        }
    }
    for (const float value : rotation) {
        if (std::fabs(value) > kOriginZeroThreshold) {
            return false;
        }
    }
    return true;
}

void closeOriginEditor(HWND hwnd, AppWindowState& state)
{
    HWND editWindow = state.originEditWindow;
    if (!editWindow) {
        return;
    }

    if (state.originEditOriginalProc) {
        SetWindowLongPtrW(editWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(state.originEditOriginalProc));
    }
    state.originEditWindow = nullptr;
    state.originEditOriginalProc = nullptr;
    DestroyWindow(editWindow);
    InvalidateRect(hwnd, nullptr, FALSE);
}

void applyOriginStepperButton(HWND hwnd, AppWindowState& state, const OriginStepperButton& button)
{
    if (!button.valid || button.axis < 0 || button.axis >= 3) {
        return;
    }

    if (state.originEditWindow && IsWindow(state.originEditWindow)) {
        closeOriginEditor(hwnd, state);
    }

    std::array<float, 3>& values = button.rotation
        ? state.originRotationDegrees
        : state.originOffset;
    values[static_cast<std::size_t>(button.axis)] += button.delta;
    if (std::fabs(values[static_cast<std::size_t>(button.axis)]) < 0.0000005f) {
        values[static_cast<std::size_t>(button.axis)] = 0.0f;
    }

    state.selectedOriginRuntimeIndex = kNoSelectedRuntimeIndex;
    state.originEnabled = !originValuesAreZero(state.originOffset, state.originRotationDegrees);

    static constexpr const char* kAxisNames[] = {"X", "Y", "Z"};
    std::ostringstream stream;
    stream << "origin "
           << (button.rotation ? "rotation " : "position ")
           << kAxisNames[button.axis]
           << " adjusted to "
           << std::fixed << std::setprecision(3)
           << values[static_cast<std::size_t>(button.axis)];
    state.originStatusMessage = stream.str();
    appendDebugLog(state, state.originStatusMessage);
    saveOriginConfig(state);

    if (state.glWindow) {
        renderViewport(state.glWindow);
    }
    invalidateStatusPanel(hwnd);
    InvalidateRect(hwnd, nullptr, FALSE);
}

bool applyOriginEditorText(HWND hwnd, AppWindowState& state)
{
    if (!state.originEditWindow || !IsWindow(state.originEditWindow)) {
        return false;
    }

    const int textLength = GetWindowTextLengthW(state.originEditWindow);
    std::wstring text(static_cast<std::size_t>(textLength) + 1, L'\0');
    GetWindowTextW(state.originEditWindow, text.data(), textLength + 1);
    text.resize(static_cast<std::size_t>(textLength));

    std::array<float, 3> offset{};
    std::array<float, 3> rotation{};
    if (!parseOriginEditorText(text, offset, rotation)) {
        state.originStatusMessage = "origin edit requires 6 values: x y z rx ry rz";
        appendDebugLog(state, state.originStatusMessage);
        InvalidateRect(hwnd, nullptr, FALSE);
        return false;
    }

    state.originOffset = offset;
    state.originRotationDegrees = rotation;
    state.selectedOriginRuntimeIndex = kNoSelectedRuntimeIndex;
    if (originValuesAreZero(offset, rotation)) {
        state.originEnabled = false;
        state.originStatusMessage = "origin reset from editor";
    } else {
        state.originEnabled = true;
        state.originStatusMessage = "origin manually edited";
    }

    appendDebugLog(state, state.originStatusMessage);
    saveOriginConfig(state);
    closeOriginEditor(hwnd, state);

    if (state.glWindow) {
        renderViewport(state.glWindow);
    }
    invalidateStatusPanel(hwnd);
    InvalidateRect(hwnd, nullptr, FALSE);
    return true;
}

LRESULT CALLBACK originEditProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    HWND parent = GetParent(hwnd);
    auto* state = parent
        ? reinterpret_cast<AppWindowState*>(GetWindowLongPtrW(parent, GWLP_USERDATA))
        : nullptr;

    if (message == WM_KEYDOWN && state) {
        if (wparam == VK_RETURN) {
            applyOriginEditorText(parent, *state);
            return 0;
        }
        if (wparam == VK_ESCAPE) {
            closeOriginEditor(parent, *state);
            return 0;
        }
    }

    if (state && state->originEditOriginalProc) {
        return CallWindowProcW(state->originEditOriginalProc, hwnd, message, wparam, lparam);
    }
    return DefWindowProcW(hwnd, message, wparam, lparam);
}

void showOriginEditor(HWND hwnd, AppWindowState& state)
{
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    const RECT editRect = originEditorRectForClient(
        &state,
        clientRect.right - clientRect.left,
        clientRect.bottom - clientRect.top
    );
    if (editRect.right <= editRect.left || editRect.bottom <= editRect.top) {
        return;
    }

    if (state.originEditWindow && IsWindow(state.originEditWindow)) {
        SetWindowTextW(state.originEditWindow, formatOriginEditorText(state).c_str());
        MoveWindow(
            state.originEditWindow,
            editRect.left,
            editRect.top,
            editRect.right - editRect.left,
            editRect.bottom - editRect.top,
            TRUE
        );
        ShowWindow(state.originEditWindow, SW_SHOW);
        SetFocus(state.originEditWindow);
        SendMessageW(state.originEditWindow, EM_SETSEL, 0, -1);
        return;
    }

    HINSTANCE instance = reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(hwnd, GWLP_HINSTANCE));
    state.originEditWindow = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        formatOriginEditorText(state).c_str(),
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        editRect.left,
        editRect.top,
        editRect.right - editRect.left,
        editRect.bottom - editRect.top,
        hwnd,
        reinterpret_cast<HMENU>(kOriginEditControlId),
        instance,
        nullptr
    );
    if (!state.originEditWindow) {
        return;
    }

    SendMessageW(
        state.originEditWindow,
        WM_SETFONT,
        reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),
        TRUE
    );
    state.originEditOriginalProc = reinterpret_cast<WNDPROC>(
        SetWindowLongPtrW(state.originEditWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(originEditProc))
    );
    SetFocus(state.originEditWindow);
    SendMessageW(state.originEditWindow, EM_SETSEL, 0, -1);
    appendDebugLog(state, L"Origin editor opened");
    InvalidateRect(hwnd, &editRect, FALSE);
}

void finishDeviceNameDialog(HWND hwnd, DeviceNameDialogState& dialog, const bool accepted)
{
    dialog.accepted = accepted;
    if (accepted && dialog.editWindow) {
        const int textLength = GetWindowTextLengthW(dialog.editWindow);
        std::wstring text(static_cast<std::size_t>(textLength) + 1, L'\0');
        GetWindowTextW(dialog.editWindow, text.data(), textLength + 1);
        text.resize(static_cast<std::size_t>(textLength));
        dialog.resultName = trimWide(text);
    }

    dialog.done = true;
    DestroyWindow(hwnd);
}

LRESULT CALLBACK deviceNameDialogProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    auto* dialog = reinterpret_cast<DeviceNameDialogState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (message) {
    case WM_NCCREATE: {
        const auto* createStruct = reinterpret_cast<LPCREATESTRUCTW>(lparam);
        dialog = reinterpret_cast<DeviceNameDialogState*>(createStruct->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(dialog));
        return TRUE;
    }
    case WM_CREATE: {
        if (!dialog) {
            return -1;
        }

        HFONT font = reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
        HWND deviceLabel = CreateWindowExW(
            0,
            L"STATIC",
            L"Device",
            WS_CHILD | WS_VISIBLE,
            16,
            14,
            388,
            18,
            hwnd,
            nullptr,
            nullptr,
            nullptr
        );
        HWND deviceValue = CreateWindowExW(
            0,
            L"STATIC",
            dialog->deviceLabel.c_str(),
            WS_CHILD | WS_VISIBLE | SS_LEFTNOWORDWRAP,
            16,
            36,
            398,
            20,
            hwnd,
            nullptr,
            nullptr,
            nullptr
        );
        HWND nameLabel = CreateWindowExW(
            0,
            L"STATIC",
            L"Name",
            WS_CHILD | WS_VISIBLE,
            16,
            68,
            388,
            18,
            hwnd,
            nullptr,
            nullptr,
            nullptr
        );
        dialog->editWindow = CreateWindowExW(
            WS_EX_CLIENTEDGE,
            L"EDIT",
            dialog->initialName.c_str(),
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
            16,
            90,
            398,
            24,
            hwnd,
            reinterpret_cast<HMENU>(kDeviceNameEditControlId),
            nullptr,
            nullptr
        );
        HWND okButton = CreateWindowExW(
            0,
            L"BUTTON",
            L"OK",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
            248,
            126,
            78,
            26,
            hwnd,
            reinterpret_cast<HMENU>(IDOK),
            nullptr,
            nullptr
        );
        HWND cancelButton = CreateWindowExW(
            0,
            L"BUTTON",
            L"Cancel",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
            336,
            126,
            78,
            26,
            hwnd,
            reinterpret_cast<HMENU>(IDCANCEL),
            nullptr,
            nullptr
        );

        for (HWND child : {deviceLabel, deviceValue, nameLabel, dialog->editWindow, okButton, cancelButton}) {
            if (child) {
                SendMessageW(child, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
            }
        }
        if (dialog->editWindow) {
            SetFocus(dialog->editWindow);
            SendMessageW(dialog->editWindow, EM_SETSEL, 0, -1);
        }
        return 0;
    }
    case WM_COMMAND:
        if (LOWORD(wparam) == IDOK && dialog) {
            finishDeviceNameDialog(hwnd, *dialog, true);
            return 0;
        }
        if (LOWORD(wparam) == IDCANCEL && dialog) {
            finishDeviceNameDialog(hwnd, *dialog, false);
            return 0;
        }
        break;
    case WM_CLOSE:
        if (dialog) {
            finishDeviceNameDialog(hwnd, *dialog, false);
            return 0;
        }
        break;
    default:
        break;
    }

    return DefWindowProcW(hwnd, message, wparam, lparam);
}

bool promptForDeviceName(
    HWND parent,
    const std::wstring& deviceLabel,
    const std::wstring& initialName,
    std::wstring& outName
)
{
    DeviceNameDialogState dialog;
    dialog.parent = parent;
    dialog.deviceLabel = deviceLabel;
    dialog.initialName = initialName;
    dialog.resultName = dialog.initialName;

    constexpr DWORD dialogStyle = WS_POPUP | WS_CAPTION | WS_SYSMENU;
    constexpr DWORD dialogExStyle = WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE;
    RECT dialogRect{0, 0, kDeviceNameDialogWidth, kDeviceNameDialogHeight};
    AdjustWindowRectEx(&dialogRect, dialogStyle, FALSE, dialogExStyle);
    const int dialogWindowWidth = dialogRect.right - dialogRect.left;
    const int dialogWindowHeight = dialogRect.bottom - dialogRect.top;

    RECT parentRect;
    GetWindowRect(parent, &parentRect);
    const int x = parentRect.left +
        ((parentRect.right - parentRect.left) - dialogWindowWidth) / 2;
    const int y = parentRect.top +
        ((parentRect.bottom - parentRect.top) - dialogWindowHeight) / 2;

    HINSTANCE instance = reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(parent, GWLP_HINSTANCE));
    EnableWindow(parent, FALSE);
    HWND dialogWindow = CreateWindowExW(
        dialogExStyle,
        kDeviceNameDialogClassName,
        L"Set Name",
        dialogStyle,
        x,
        y,
        dialogWindowWidth,
        dialogWindowHeight,
        parent,
        nullptr,
        instance,
        &dialog
    );
    if (!dialogWindow) {
        EnableWindow(parent, TRUE);
        SetActiveWindow(parent);
        return false;
    }

    ShowWindow(dialogWindow, SW_SHOW);
    UpdateWindow(dialogWindow);

    MSG msg{};
    BOOL messageResult = TRUE;
    while (!dialog.done && (messageResult = GetMessageW(&msg, nullptr, 0, 0)) > 0) {
        if (!IsWindow(dialogWindow) || !IsDialogMessageW(dialogWindow, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    EnableWindow(parent, TRUE);
    SetActiveWindow(parent);
    if (messageResult == 0) {
        PostQuitMessage(static_cast<int>(msg.wParam));
    }

    if (!dialog.accepted) {
        return false;
    }

    outName = dialog.resultName;
    return true;
}

bool setDeviceCustomName(HWND hwnd, AppWindowState& state, const ovtr::DeviceDescriptor& device)
{
    const std::string key = deviceNameKeyForDevice(device);
    const std::wstring deviceLabel = widen(deviceDisplayName(device));
    const std::wstring initialName = widen(customNameForDevice(state, device));

    std::wstring nameText;
    if (!promptForDeviceName(hwnd, deviceLabel, initialName, nameText)) {
        appendDebugLog(state, L"Set name canceled");
        return false;
    }

    const std::string customName = trimAscii(narrow(trimWide(nameText)));
    if (customName.empty()) {
        state.deviceCustomNames.erase(key);
        appendDebugLog(state, L"Device name cleared: " + deviceLabel);
    } else {
        state.deviceCustomNames[key] = customName;
        appendDebugLog(state, L"Device name set: " + deviceLabel + L" -> " + widen(customName));
    }

    saveDeviceNameConfig(state);
    if (state.glWindow) {
        renderViewport(state.glWindow);
    }
    InvalidateRect(hwnd, nullptr, FALSE);
    return true;
}

RgbColor& viewportDialogColorByIndex(ViewportSettings& settings, const int index)
{
    if (index == 0) {
        return settings.labelTextColor;
    }
    if (index == 1) {
        return settings.gridColor;
    }
    if (index == 2) {
        return settings.backgroundColor;
    }
    return settings.importedGlbColor;
}

std::wstring formatIntegerText(const int value)
{
    return std::to_wstring(value);
}

std::wstring formatFloatText(const float value)
{
    std::wostringstream stream;
    stream << std::fixed << std::setprecision(3) << value;
    return stream.str();
}

void setEditText(HWND editWindow, const std::wstring& text)
{
    if (editWindow) {
        SetWindowTextW(editWindow, text.c_str());
    }
}

bool readIntegerEdit(HWND editWindow, int& value)
{
    if (!editWindow) {
        return false;
    }

    wchar_t buffer[64]{};
    GetWindowTextW(editWindow, buffer, static_cast<int>(std::size(buffer)));
    wchar_t* end = nullptr;
    const long parsed = std::wcstol(buffer, &end, 10);
    if (end == buffer) {
        return false;
    }
    while (*end != L'\0') {
        if (std::iswspace(*end) == 0) {
            return false;
        }
        ++end;
    }
    value = clampColorComponent(static_cast<int>(parsed));
    return true;
}

bool readFloatEdit(HWND editWindow, float& value)
{
    if (!editWindow) {
        return false;
    }

    wchar_t buffer[64]{};
    GetWindowTextW(editWindow, buffer, static_cast<int>(std::size(buffer)));
    wchar_t* end = nullptr;
    const float parsed = std::wcstof(buffer, &end);
    if (end == buffer || !std::isfinite(parsed)) {
        return false;
    }
    while (*end != L'\0') {
        if (std::iswspace(*end) == 0) {
            return false;
        }
        ++end;
    }
    value = std::clamp(parsed, 0.0f, 10.0f);
    return true;
}

bool readFiniteFloatEdit(HWND editWindow, float& value)
{
    if (!editWindow) {
        return false;
    }

    wchar_t buffer[64]{};
    GetWindowTextW(editWindow, buffer, static_cast<int>(std::size(buffer)));
    wchar_t* end = nullptr;
    const float parsed = std::wcstof(buffer, &end);
    if (end == buffer || !std::isfinite(parsed)) {
        return false;
    }
    while (*end != L'\0') {
        if (std::iswspace(*end) == 0) {
            return false;
        }
        ++end;
    }
    value = parsed;
    return true;
}

void setOriginDialogValuesOnState(
    AppWindowState& state,
    const bool enabled,
    const std::array<float, 3>& offset,
    const std::array<float, 3>& rotation
)
{
    const bool active = enabled && !originValuesAreZero(offset, rotation);
    state.originEnabled = active;
    state.originOffset = active ? offset : std::array<float, 3>{0.0f, 0.0f, 0.0f};
    state.originRotationDegrees = active ? rotation : std::array<float, 3>{0.0f, 0.0f, 0.0f};
    state.selectedOriginRuntimeIndex = kNoSelectedRuntimeIndex;
}

void renderOriginDialogPreview(HWND hwnd, OriginDialogState& dialog)
{
    if (!dialog.appState) {
        return;
    }

    setOriginDialogValuesOnState(
        *dialog.appState,
        dialog.workingEnabled,
        dialog.workingOffset,
        dialog.workingRotationDegrees
    );
    dialog.appState->originStatusMessage = dialog.appState->originEnabled
        ? "origin preview updated"
        : "origin preview disabled";

    if (dialog.appState->glWindow) {
        renderViewport(dialog.appState->glWindow);
    }
    if (dialog.parent) {
        invalidateStatusPanel(dialog.parent);
        InvalidateRect(dialog.parent, nullptr, FALSE);
    } else {
        InvalidateRect(hwnd, nullptr, FALSE);
    }
}

void updateOriginDialogControls(OriginDialogState& dialog)
{
    dialog.updatingControls = true;
    if (dialog.enabledCheck) {
        SendMessageW(
            dialog.enabledCheck,
            BM_SETCHECK,
            dialog.workingEnabled ? BST_CHECKED : BST_UNCHECKED,
            0
        );
    }

    for (int axis = 0; axis < 3; ++axis) {
        setEditText(
            dialog.positionEdits[static_cast<std::size_t>(axis)],
            formatFloatText(dialog.workingOffset[static_cast<std::size_t>(axis)])
        );
        setEditText(
            dialog.rotationEdits[static_cast<std::size_t>(axis)],
            formatFloatText(dialog.workingRotationDegrees[static_cast<std::size_t>(axis)])
        );
    }
    dialog.updatingControls = false;
}

bool readOriginDialogControls(HWND hwnd, OriginDialogState& dialog, const bool showWarning)
{
    std::array<float, 3> offset{};
    std::array<float, 3> rotation{};
    for (int axis = 0; axis < 3; ++axis) {
        if (!readFiniteFloatEdit(dialog.positionEdits[static_cast<std::size_t>(axis)], offset[static_cast<std::size_t>(axis)]) ||
            !readFiniteFloatEdit(dialog.rotationEdits[static_cast<std::size_t>(axis)], rotation[static_cast<std::size_t>(axis)])) {
            if (showWarning) {
                MessageBoxW(hwnd, L"Origin values must be valid numbers.", L"Origin Settings", MB_OK | MB_ICONWARNING);
            }
            return false;
        }
    }

    dialog.workingEnabled = dialog.enabledCheck
        ? SendMessageW(dialog.enabledCheck, BM_GETCHECK, 0, 0) == BST_CHECKED
        : true;
    dialog.workingOffset = offset;
    dialog.workingRotationDegrees = rotation;
    return true;
}

void previewOriginDialogFromControls(HWND hwnd, OriginDialogState& dialog)
{
    if (dialog.updatingControls) {
        return;
    }
    if (!readOriginDialogControls(hwnd, dialog, false)) {
        return;
    }
    renderOriginDialogPreview(hwnd, dialog);
}

void restoreOriginDialogSnapshot(HWND hwnd, OriginDialogState& dialog)
{
    if (!dialog.appState) {
        return;
    }

    setOriginDialogValuesOnState(
        *dialog.appState,
        dialog.originalEnabled,
        dialog.originalOffset,
        dialog.originalRotationDegrees
    );
    dialog.appState->originStatusMessage = "origin settings canceled";
    appendDebugLog(*dialog.appState, dialog.appState->originStatusMessage);

    if (dialog.appState->glWindow) {
        renderViewport(dialog.appState->glWindow);
    }
    if (dialog.parent) {
        invalidateStatusPanel(dialog.parent);
        InvalidateRect(dialog.parent, nullptr, FALSE);
    } else {
        InvalidateRect(hwnd, nullptr, FALSE);
    }
}

void finishOriginDialog(HWND hwnd, OriginDialogState& dialog, const bool accepted)
{
    if (accepted) {
        if (!readOriginDialogControls(hwnd, dialog, true)) {
            return;
        }
        if (dialog.appState) {
            renderOriginDialogPreview(hwnd, dialog);
            dialog.appState->originStatusMessage = dialog.appState->originEnabled
                ? "origin settings saved"
                : "origin settings disabled";
            appendDebugLog(*dialog.appState, dialog.appState->originStatusMessage);
            saveOriginConfig(*dialog.appState);
        }
    } else {
        restoreOriginDialogSnapshot(hwnd, dialog);
    }

    dialog.accepted = accepted;
    dialog.done = true;
    DestroyWindow(hwnd);
}

LRESULT CALLBACK originDialogProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    auto* dialog = reinterpret_cast<OriginDialogState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (message) {
    case WM_NCCREATE: {
        const auto* createStruct = reinterpret_cast<LPCREATESTRUCTW>(lparam);
        dialog = reinterpret_cast<OriginDialogState*>(createStruct->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(dialog));
        return TRUE;
    }
    case WM_CREATE: {
        if (!dialog) {
            return -1;
        }

        HFONT font = reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
        dialog->enabledCheck = CreateWindowExW(0, L"BUTTON", L"Enable origin",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
            22, 18, 180, 24, hwnd,
            reinterpret_cast<HMENU>(kOriginDialogEnabledControlId),
            nullptr, nullptr);

        static constexpr const wchar_t* kAxisLabels[] = {L"X", L"Y", L"Z"};
        for (int axis = 0; axis < 3; ++axis) {
            const int x = 140 + axis * 92;
            HWND axisHeader = CreateWindowExW(0, L"STATIC", kAxisLabels[axis],
                WS_CHILD | WS_VISIBLE | SS_CENTER,
                x, 54, 72, 18, hwnd, nullptr, nullptr, nullptr);
            if (axisHeader) {
                SendMessageW(axisHeader, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
            }
        }

        HWND positionLabel = CreateWindowExW(0, L"STATIC", L"Position", WS_CHILD | WS_VISIBLE,
            22, 82, 90, 20, hwnd, nullptr, nullptr, nullptr);
        HWND rotationLabel = CreateWindowExW(0, L"STATIC", L"Rotation", WS_CHILD | WS_VISIBLE,
            22, 126, 90, 20, hwnd, nullptr, nullptr, nullptr);

        for (int axis = 0; axis < 3; ++axis) {
            const int x = 140 + axis * 92;
            dialog->positionEdits[static_cast<std::size_t>(axis)] = CreateWindowExW(
                WS_EX_CLIENTEDGE,
                L"EDIT",
                L"",
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                x,
                78,
                72,
                24,
                hwnd,
                reinterpret_cast<HMENU>(kOriginDialogEditBaseControlId + axis),
                nullptr,
                nullptr
            );
            dialog->rotationEdits[static_cast<std::size_t>(axis)] = CreateWindowExW(
                WS_EX_CLIENTEDGE,
                L"EDIT",
                L"",
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                x,
                122,
                72,
                24,
                hwnd,
                reinterpret_cast<HMENU>(kOriginDialogEditBaseControlId + 3 + axis),
                nullptr,
                nullptr
            );
        }

        HWND okButton = CreateWindowExW(0, L"BUTTON", L"OK",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
            278, 174, 78, 26, hwnd, reinterpret_cast<HMENU>(IDOK), nullptr, nullptr);
        HWND cancelButton = CreateWindowExW(0, L"BUTTON", L"Cancel",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
            370, 174, 78, 26, hwnd, reinterpret_cast<HMENU>(IDCANCEL), nullptr, nullptr);

        for (HWND child : {dialog->enabledCheck, positionLabel, rotationLabel, okButton, cancelButton}) {
            if (child) {
                SendMessageW(child, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
            }
        }
        for (HWND child : dialog->positionEdits) {
            if (child) {
                SendMessageW(child, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
            }
        }
        for (HWND child : dialog->rotationEdits) {
            if (child) {
                SendMessageW(child, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
            }
        }

        updateOriginDialogControls(*dialog);
        if (dialog->positionEdits[0]) {
            SetFocus(dialog->positionEdits[0]);
            SendMessageW(dialog->positionEdits[0], EM_SETSEL, 0, -1);
        }
        return 0;
    }
    case WM_COMMAND: {
        if (!dialog) {
            break;
        }

        const UINT command = LOWORD(wparam);
        const UINT notification = HIWORD(wparam);
        if (command == kOriginDialogEnabledControlId && notification == BN_CLICKED) {
            previewOriginDialogFromControls(hwnd, *dialog);
            return 0;
        }
        if (command >= kOriginDialogEditBaseControlId &&
            command < kOriginDialogEditBaseControlId + 6 &&
            notification == EN_CHANGE) {
            previewOriginDialogFromControls(hwnd, *dialog);
            return 0;
        }
        if (command == IDOK) {
            finishOriginDialog(hwnd, *dialog, true);
            return 0;
        }
        if (command == IDCANCEL) {
            finishOriginDialog(hwnd, *dialog, false);
            return 0;
        }
        break;
    }
    case WM_CLOSE:
        if (dialog) {
            finishOriginDialog(hwnd, *dialog, false);
            return 0;
        }
        break;
    default:
        break;
    }

    return DefWindowProcW(hwnd, message, wparam, lparam);
}

void updateViewportColorDialogControls(ViewportColorDialogState& dialog)
{
    for (int i = 0; i < kViewportColorCount; ++i) {
        const RgbColor color = viewportDialogColorByIndex(dialog.workingSettings, i);
        const ColorEditControls& controls = dialog.colorControls[static_cast<std::size_t>(i)];
        setEditText(controls.red, formatIntegerText(color.r));
        setEditText(controls.green, formatIntegerText(color.g));
        setEditText(controls.blue, formatIntegerText(color.b));
    }
    setEditText(dialog.outlineEdit, formatFloatText(dialog.workingSettings.outlineMultiplier));
}

bool readViewportColorDialogControls(HWND hwnd, ViewportColorDialogState& dialog)
{
    for (int i = 0; i < kViewportColorCount; ++i) {
        RgbColor color{};
        const ColorEditControls& controls = dialog.colorControls[static_cast<std::size_t>(i)];
        if (!readIntegerEdit(controls.red, color.r) ||
            !readIntegerEdit(controls.green, color.g) ||
            !readIntegerEdit(controls.blue, color.b)) {
            MessageBoxW(hwnd, L"RGB values must be numbers from 0 to 255.", L"Color Settings", MB_OK | MB_ICONWARNING);
            return false;
        }
        viewportDialogColorByIndex(dialog.workingSettings, i) = clampRgbColor(color);
    }

    float outlineMultiplier = 1.0f;
    if (!readFloatEdit(dialog.outlineEdit, outlineMultiplier)) {
        MessageBoxW(hwnd, L"Outline thickness must be a number from 0.0 to 10.0.", L"Color Settings", MB_OK | MB_ICONWARNING);
        return false;
    }
    dialog.workingSettings.outlineMultiplier = outlineMultiplier;
    return true;
}

void chooseViewportDialogColor(HWND hwnd, ViewportColorDialogState& dialog, const int colorIndex)
{
    if (!readViewportColorDialogControls(hwnd, dialog)) {
        return;
    }

    CHOOSECOLORW chooser{};
    chooser.lStructSize = sizeof(chooser);
    chooser.hwndOwner = hwnd;
    chooser.rgbResult = colorRefFromRgb(viewportDialogColorByIndex(dialog.workingSettings, colorIndex));
    chooser.lpCustColors = dialog.customColors.data();
    chooser.Flags = CC_FULLOPEN | CC_RGBINIT;
    if (ChooseColorW(&chooser)) {
        viewportDialogColorByIndex(dialog.workingSettings, colorIndex) = rgbFromColorRef(chooser.rgbResult);
        updateViewportColorDialogControls(dialog);
    }
}

void resetViewportDialogColors(ViewportColorDialogState& dialog)
{
    float currentOutlineMultiplier = dialog.workingSettings.outlineMultiplier;
    if (readFloatEdit(dialog.outlineEdit, currentOutlineMultiplier)) {
        dialog.workingSettings.outlineMultiplier = currentOutlineMultiplier;
    }

    const ViewportSettings defaults{};
    dialog.workingSettings.labelTextColor = defaults.labelTextColor;
    dialog.workingSettings.gridColor = defaults.gridColor;
    dialog.workingSettings.backgroundColor = defaults.backgroundColor;
    dialog.workingSettings.importedGlbColor = defaults.importedGlbColor;
    updateViewportColorDialogControls(dialog);
}

void finishViewportColorDialog(HWND hwnd, ViewportColorDialogState& dialog, const bool accepted)
{
    if (accepted) {
        if (!readViewportColorDialogControls(hwnd, dialog)) {
            return;
        }
        if (dialog.appState) {
            dialog.appState->viewportSettings = dialog.workingSettings;
            saveViewportSettingsConfig(*dialog.appState);
            appendDebugLog(*dialog.appState, L"Viewport color settings applied");
            if (dialog.appState->glWindow) {
                renderViewport(dialog.appState->glWindow);
            }
        }
    }

    dialog.accepted = accepted;
    dialog.done = true;
    DestroyWindow(hwnd);
}

LRESULT CALLBACK viewportColorDialogProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    auto* dialog = reinterpret_cast<ViewportColorDialogState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (message) {
    case WM_NCCREATE: {
        const auto* createStruct = reinterpret_cast<LPCREATESTRUCTW>(lparam);
        dialog = reinterpret_cast<ViewportColorDialogState*>(createStruct->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(dialog));
        return TRUE;
    }
    case WM_CREATE: {
        if (!dialog) {
            return -1;
        }

        HFONT font = reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
        static constexpr const wchar_t* kColorLabels[] = {
            L"Device name font",
            L"Grid",
            L"Background",
            L"Imported GLB",
        };

        for (const auto& header : {
                 std::pair<const wchar_t*, RECT>{L"R", RECT{214, 14, 262, 32}},
                 std::pair<const wchar_t*, RECT>{L"G", RECT{274, 14, 322, 32}},
                 std::pair<const wchar_t*, RECT>{L"B", RECT{334, 14, 382, 32}},
             }) {
            HWND headerWindow = CreateWindowExW(0, L"STATIC", header.first, WS_CHILD | WS_VISIBLE | SS_CENTER,
                header.second.left, header.second.top, header.second.right - header.second.left,
                header.second.bottom - header.second.top, hwnd, nullptr, nullptr, nullptr);
            if (headerWindow) {
                SendMessageW(headerWindow, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
            }
        }

        for (int i = 0; i < kViewportColorCount; ++i) {
            const int y = 38 + i * 38;
            HWND label = CreateWindowExW(0, L"STATIC", kColorLabels[i], WS_CHILD | WS_VISIBLE,
                18, y + 4, 170, 20, hwnd, nullptr, nullptr, nullptr);
            if (label) {
                SendMessageW(label, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
            }

            ColorEditControls controls{};
            controls.red = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL | ES_NUMBER,
                210, y, 52, 24, hwnd,
                reinterpret_cast<HMENU>(kViewportColorEditBaseControlId + i * 10),
                nullptr, nullptr);
            controls.green = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL | ES_NUMBER,
                270, y, 52, 24, hwnd,
                reinterpret_cast<HMENU>(kViewportColorEditBaseControlId + i * 10 + 1),
                nullptr, nullptr);
            controls.blue = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL | ES_NUMBER,
                330, y, 52, 24, hwnd,
                reinterpret_cast<HMENU>(kViewportColorEditBaseControlId + i * 10 + 2),
                nullptr, nullptr);
            controls.pick = CreateWindowExW(0, L"BUTTON", L"Pick",
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                404, y - 1, 76, 26, hwnd,
                reinterpret_cast<HMENU>(kViewportColorPickBaseControlId + i),
                nullptr, nullptr);
            dialog->colorControls[static_cast<std::size_t>(i)] = controls;
            for (HWND child : {controls.red, controls.green, controls.blue, controls.pick}) {
                if (child) {
                    SendMessageW(child, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
                }
            }
        }

        HWND outlineLabel = CreateWindowExW(0, L"STATIC", L"Device outline thickness", WS_CHILD | WS_VISIBLE,
            18, 194, 180, 20, hwnd, nullptr, nullptr, nullptr);
        dialog->outlineEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
            210, 190, 88, 24, hwnd,
            reinterpret_cast<HMENU>(kViewportOutlineEditControlId),
            nullptr, nullptr);
        HWND outlineHint = CreateWindowExW(0, L"STATIC", L"Multiplier, 0.0 - 10.0", WS_CHILD | WS_VISIBLE,
            312, 194, 180, 20, hwnd, nullptr, nullptr, nullptr);
        HWND resetButton = CreateWindowExW(0, L"BUTTON", L"Reset",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
            270, 242, 78, 26, hwnd, reinterpret_cast<HMENU>(kViewportColorResetControlId), nullptr, nullptr);
        HWND okButton = CreateWindowExW(0, L"BUTTON", L"OK",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
            360, 242, 78, 26, hwnd, reinterpret_cast<HMENU>(IDOK), nullptr, nullptr);
        HWND cancelButton = CreateWindowExW(0, L"BUTTON", L"Cancel",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
            450, 242, 78, 26, hwnd, reinterpret_cast<HMENU>(IDCANCEL), nullptr, nullptr);
        for (HWND child : {outlineLabel, dialog->outlineEdit, outlineHint, resetButton, okButton, cancelButton}) {
            if (child) {
                SendMessageW(child, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
            }
        }

        updateViewportColorDialogControls(*dialog);
        return 0;
    }
    case WM_COMMAND: {
        if (!dialog) {
            break;
        }
        const UINT command = LOWORD(wparam);
        if (command >= kViewportColorPickBaseControlId &&
            command < kViewportColorPickBaseControlId + kViewportColorCount) {
            chooseViewportDialogColor(hwnd, *dialog, static_cast<int>(command - kViewportColorPickBaseControlId));
            return 0;
        }
        if (command == kViewportColorResetControlId) {
            resetViewportDialogColors(*dialog);
            return 0;
        }
        if (command == IDOK) {
            finishViewportColorDialog(hwnd, *dialog, true);
            return 0;
        }
        if (command == IDCANCEL) {
            finishViewportColorDialog(hwnd, *dialog, false);
            return 0;
        }
        break;
    }
    case WM_CLOSE:
        if (dialog) {
            finishViewportColorDialog(hwnd, *dialog, false);
            return 0;
        }
        break;
    default:
        break;
    }

    return DefWindowProcW(hwnd, message, wparam, lparam);
}

bool showViewportColorSettings(HWND parent, AppWindowState& state)
{
    ViewportColorDialogState dialog;
    dialog.parent = parent;
    dialog.appState = &state;
    dialog.workingSettings = state.viewportSettings;
    dialog.customColors.fill(RGB(255, 255, 255));

    constexpr DWORD dialogStyle = WS_POPUP | WS_CAPTION | WS_SYSMENU;
    constexpr DWORD dialogExStyle = WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE;
    RECT dialogRect{0, 0, kViewportColorDialogWidth, kViewportColorDialogHeight};
    AdjustWindowRectEx(&dialogRect, dialogStyle, FALSE, dialogExStyle);
    const int dialogWindowWidth = dialogRect.right - dialogRect.left;
    const int dialogWindowHeight = dialogRect.bottom - dialogRect.top;

    RECT parentRect;
    GetWindowRect(parent, &parentRect);
    const int x = parentRect.left + ((parentRect.right - parentRect.left) - dialogWindowWidth) / 2;
    const int y = parentRect.top + ((parentRect.bottom - parentRect.top) - dialogWindowHeight) / 2;

    HINSTANCE instance = reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(parent, GWLP_HINSTANCE));
    EnableWindow(parent, FALSE);
    HWND dialogWindow = CreateWindowExW(
        dialogExStyle,
        kViewportColorDialogClassName,
        L"Color Settings",
        dialogStyle,
        x,
        y,
        dialogWindowWidth,
        dialogWindowHeight,
        parent,
        nullptr,
        instance,
        &dialog
    );
    if (!dialogWindow) {
        EnableWindow(parent, TRUE);
        SetActiveWindow(parent);
        return false;
    }

    ShowWindow(dialogWindow, SW_SHOW);
    UpdateWindow(dialogWindow);

    MSG msg{};
    BOOL messageResult = TRUE;
    while (!dialog.done && (messageResult = GetMessageW(&msg, nullptr, 0, 0)) > 0) {
        if (!IsWindow(dialogWindow) || !IsDialogMessageW(dialogWindow, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    EnableWindow(parent, TRUE);
    SetActiveWindow(parent);
    if (messageResult == 0) {
        PostQuitMessage(static_cast<int>(msg.wParam));
    }
    return dialog.accepted;
}

bool showOriginSettings(HWND parent, AppWindowState& state)
{
    OriginDialogState dialog;
    dialog.parent = parent;
    dialog.appState = &state;
    dialog.originalEnabled = state.originEnabled;
    dialog.originalOffset = state.originOffset;
    dialog.originalRotationDegrees = state.originRotationDegrees;
    dialog.workingEnabled = state.originEnabled;
    dialog.workingOffset = state.originOffset;
    dialog.workingRotationDegrees = state.originRotationDegrees;

    constexpr DWORD dialogStyle = WS_POPUP | WS_CAPTION | WS_SYSMENU;
    constexpr DWORD dialogExStyle = WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE;
    RECT dialogRect{0, 0, kOriginDialogWidth, kOriginDialogHeight};
    AdjustWindowRectEx(&dialogRect, dialogStyle, FALSE, dialogExStyle);
    const int dialogWindowWidth = dialogRect.right - dialogRect.left;
    const int dialogWindowHeight = dialogRect.bottom - dialogRect.top;

    RECT parentRect;
    GetWindowRect(parent, &parentRect);
    const int x = parentRect.left + ((parentRect.right - parentRect.left) - dialogWindowWidth) / 2;
    const int y = parentRect.top + ((parentRect.bottom - parentRect.top) - dialogWindowHeight) / 2;

    HINSTANCE instance = reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(parent, GWLP_HINSTANCE));
    EnableWindow(parent, FALSE);
    HWND dialogWindow = CreateWindowExW(
        dialogExStyle,
        kOriginDialogClassName,
        L"Origin Settings",
        dialogStyle,
        x,
        y,
        dialogWindowWidth,
        dialogWindowHeight,
        parent,
        nullptr,
        instance,
        &dialog
    );
    if (!dialogWindow) {
        EnableWindow(parent, TRUE);
        SetActiveWindow(parent);
        return false;
    }

    ShowWindow(dialogWindow, SW_SHOW);
    UpdateWindow(dialogWindow);

    MSG msg{};
    BOOL messageResult = TRUE;
    while (!dialog.done && (messageResult = GetMessageW(&msg, nullptr, 0, 0)) > 0) {
        if (!IsWindow(dialogWindow) || !IsDialogMessageW(dialogWindow, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    if (!dialog.done) {
        restoreOriginDialogSnapshot(dialogWindow, dialog);
    }

    EnableWindow(parent, TRUE);
    SetActiveWindow(parent);
    if (messageResult == 0) {
        PostQuitMessage(static_cast<int>(msg.wParam));
    }
    return dialog.accepted;
}

void appendPopupMenuItem(HMENU menu, PopupMenuItem& item)
{
    AppendMenuW(
        menu,
        MF_OWNERDRAW,
        item.commandId,
        reinterpret_cast<LPCWSTR>(&item)
    );
}

bool measurePopupMenuItem(HWND hwnd, MEASUREITEMSTRUCT& measure)
{
    if (measure.CtlType != ODT_MENU || measure.itemData == 0) {
        return false;
    }

    const auto* item = reinterpret_cast<const PopupMenuItem*>(measure.itemData);
    HDC dc = GetDC(hwnd);
    HFONT font = reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
    HGDIOBJ previousFont = nullptr;
    if (dc && font) {
        previousFont = SelectObject(dc, font);
    }

    SIZE textSize{96, 18};
    if (dc) {
        GetTextExtentPoint32W(
            dc,
            item->label.c_str(),
            static_cast<int>(item->label.size()),
            &textSize
        );
    }

    if (dc && previousFont) {
        SelectObject(dc, previousFont);
    }
    if (dc) {
        ReleaseDC(hwnd, dc);
    }

    measure.itemWidth = static_cast<UINT>(std::max<LONG>(textSize.cx + 72, 168));
    measure.itemHeight = 34;
    return true;
}

bool drawPopupMenuItem(const DRAWITEMSTRUCT& draw)
{
    if (draw.CtlType != ODT_MENU || draw.itemData == 0 || !draw.hDC) {
        return false;
    }

    const auto* item = reinterpret_cast<const PopupMenuItem*>(draw.itemData);
    const bool selected = (draw.itemState & ODS_SELECTED) != 0;
    const bool disabled = (draw.itemState & ODS_DISABLED) != 0;

    HBRUSH backgroundBrush = CreateSolidBrush(selected ? RGB(210, 220, 234) : RGB(248, 248, 248));
    FillRect(draw.hDC, &draw.rcItem, backgroundBrush);
    DeleteObject(backgroundBrush);

    if (selected) {
        HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(166, 184, 207));
        HGDIOBJ previousPen = SelectObject(draw.hDC, borderPen);
        HGDIOBJ previousBrush = SelectObject(draw.hDC, GetStockObject(NULL_BRUSH));
        RoundRect(
            draw.hDC,
            draw.rcItem.left + 4,
            draw.rcItem.top + 3,
            draw.rcItem.right - 4,
            draw.rcItem.bottom - 3,
            5,
            5
        );
        SelectObject(draw.hDC, previousBrush);
        SelectObject(draw.hDC, previousPen);
        DeleteObject(borderPen);
    }

    RECT textRect = draw.rcItem;
    textRect.left += 28;
    textRect.right -= 18;
    SetBkMode(draw.hDC, TRANSPARENT);
    SetTextColor(draw.hDC, disabled ? RGB(150, 150, 150) : RGB(18, 22, 28));

    HFONT font = reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
    HGDIOBJ previousFont = nullptr;
    if (font) {
        previousFont = SelectObject(draw.hDC, font);
    }
    DrawTextW(
        draw.hDC,
        item->label.c_str(),
        static_cast<int>(item->label.size()),
        &textRect,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS
    );
    if (previousFont) {
        SelectObject(draw.hDC, previousFont);
    }
    return true;
}

int CALLBACK browseFolderCallback(HWND hwnd, UINT message, LPARAM, LPARAM data)
{
    if (message == BFFM_INITIALIZED && data != 0) {
        SendMessageW(hwnd, BFFM_SETSELECTIONW, TRUE, data);
    }
    return 0;
}

bool chooseExportDirectory(HWND owner, const std::filesystem::path& initialDirectory, std::filesystem::path& outDirectory)
{
    const std::wstring initialText = normalizedExportDirectoryPath(initialDirectory).wstring();
    wchar_t displayName[MAX_PATH]{};
    BROWSEINFOW browse{};
    browse.hwndOwner = owner;
    browse.pszDisplayName = displayName;
    browse.lpszTitle = L"Select export folder";
    browse.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE | BIF_EDITBOX;
    browse.lpfn = browseFolderCallback;
    browse.lParam = reinterpret_cast<LPARAM>(initialText.c_str());

    const HRESULT oleResult = OleInitialize(nullptr);
    PIDLIST_ABSOLUTE itemList = SHBrowseForFolderW(&browse);
    bool selected = false;
    if (itemList) {
        wchar_t pathBuffer[MAX_PATH]{};
        if (SHGetPathFromIDListW(itemList, pathBuffer)) {
            outDirectory = std::filesystem::path(pathBuffer);
            selected = true;
        }
        CoTaskMemFree(itemList);
    }
    if (SUCCEEDED(oleResult)) {
        OleUninitialize();
    }
    return selected;
}

bool chooseImportGlbFile(HWND owner, const std::filesystem::path& initialDirectory, std::filesystem::path& outPath)
{
    std::array<wchar_t, MAX_PATH> fileName{};
    const std::wstring initialDirectoryText = normalizedExportDirectoryPath(initialDirectory).wstring();

    OPENFILENAMEW openFile{};
    openFile.lStructSize = sizeof(openFile);
    openFile.hwndOwner = owner;
    openFile.lpstrFilter = L"glTF Binary (*.glb)\0*.glb\0All Files (*.*)\0*.*\0";
    openFile.lpstrFile = fileName.data();
    openFile.nMaxFile = static_cast<DWORD>(fileName.size());
    openFile.lpstrInitialDir = initialDirectoryText.empty() ? nullptr : initialDirectoryText.c_str();
    openFile.lpstrTitle = L"Import GLB";
    openFile.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    openFile.lpstrDefExt = L"glb";

    if (!GetOpenFileNameW(&openFile)) {
        return false;
    }

    outPath = std::filesystem::path(fileName.data());
    return !outPath.empty();
}

void importGlbFromFile(HWND hwnd, AppWindowState& state)
{
    std::filesystem::path selectedPath;
    if (!chooseImportGlbFile(hwnd, activeExportDirectoryPath(state), selectedPath)) {
        return;
    }

    state.exportStatusMessage.clear();
    appendDebugLog(state, L"Starting GLB import: " + selectedPath.wstring());
    ovtr::GltfImportResult result = ovtr::importGlbScene(selectedPath);
    if (!result.success) {
        state.importStatusMessage = "GLB import failed: " + result.error;
        appendDebugLog(state, state.importStatusMessage);
        MessageBoxW(hwnd, widen(state.importStatusMessage).c_str(), L"Import GLB", MB_OK | MB_ICONWARNING);
        invalidateStatusPanel(hwnd);
        InvalidateRect(hwnd, nullptr, FALSE);
        return;
    }

    state.importedScene = std::move(result.scene);
    state.importedSceneLoaded = true;
    state.importedScenePlaying = false;
    state.importedSceneTimelineDragging = false;
    state.importedScenePlaybackSeconds = 0.0;
    state.importedSceneLastUpdate = std::chrono::steady_clock::now();
    state.importStatusMessage = "GLB imported: " + selectedPath.filename().string();
    appendDebugLog(state, state.importStatusMessage);

    layoutChildWindows(hwnd);
    if (state.glWindow) {
        renderViewport(state.glWindow);
    }
    invalidateStatusPanel(hwnd);
    InvalidateRect(hwnd, nullptr, FALSE);
}

void closeImportedGlb(HWND hwnd, AppWindowState& state)
{
    if (!state.importedSceneLoaded) {
        return;
    }

    const std::string fileName = state.importedScene.sourcePath.filename().string();
    state.importedScene = {};
    state.importedSceneLoaded = false;
    state.importedScenePlaying = false;
    state.importedSceneTimelineDragging = false;
    state.importedScenePlaybackSeconds = 0.0;
    state.importedSceneLastUpdate = {};
    state.importStatusMessage = fileName.empty() ? "GLB import closed" : "GLB import closed: " + fileName;
    appendDebugLog(state, state.importStatusMessage);

    layoutChildWindows(hwnd);
    invalidateWindowLayout(hwnd);
}

void invalidateImportedAnimationBar(HWND hwnd, const AppWindowState& state)
{
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    const ViewportControlLayout layout = viewportControlLayoutForClient(
        &state,
        clientRect.right - clientRect.left,
        clientRect.bottom - clientRect.top
    );
    if (layout.animationValid) {
        InvalidateRect(hwnd, &layout.animationBarRect, FALSE);
    }
}

void seekImportedGlbFromTimeline(HWND hwnd, AppWindowState& state, const RECT& timelineRect, const POINT point)
{
    if (!state.importedSceneLoaded || timelineRect.right <= timelineRect.left) {
        return;
    }

    const int clampedX = std::clamp(point.x, timelineRect.left, timelineRect.right);
    const double factor = static_cast<double>(clampedX - timelineRect.left) /
        static_cast<double>(timelineRect.right - timelineRect.left);
    setImportedScenePlaybackSeconds(state, factor * importedSceneDurationSeconds(state));

    invalidateImportedAnimationBar(hwnd, state);
}

void toggleImportedGlbPlayback(HWND hwnd, AppWindowState& state)
{
    if (!state.importedSceneLoaded) {
        return;
    }

    const double durationSeconds = importedSceneDurationSeconds(state);
    if (!state.importedScenePlaying && state.importedScenePlaybackSeconds >= durationSeconds) {
        setImportedScenePlaybackSeconds(state, 0.0);
    }
    state.importedScenePlaying = !state.importedScenePlaying;
    state.importedSceneLastUpdate = std::chrono::steady_clock::now();
    appendDebugLog(state, state.importedScenePlaying ? L"Imported GLB playback started" : L"Imported GLB playback paused");
    invalidateStatusPanel(hwnd);
    InvalidateRect(hwnd, nullptr, FALSE);
}

void startImportedGlbPlaybackForRecording(HWND hwnd, AppWindowState& state)
{
    if (!state.importedSceneLoaded) {
        return;
    }

    const double durationSeconds = importedSceneDurationSeconds(state);
    if (durationSeconds <= 0.0) {
        state.importedScenePlaying = false;
        setImportedScenePlaybackSeconds(state, 0.0);
        invalidateImportedAnimationBar(hwnd, state);
        return;
    }

    if (state.importedScenePlaybackSeconds >= durationSeconds) {
        setImportedScenePlaybackSeconds(state, 0.0);
    } else {
        state.importedSceneLastUpdate = std::chrono::steady_clock::now();
    }
    state.importedScenePlaying = true;
    appendDebugLog(state, L"Imported GLB playback auto-started for recording");
    invalidateImportedAnimationBar(hwnd, state);
}

bool handleImportedAnimationControlClick(HWND hwnd, AppWindowState& state, const ViewportControlLayout& layout, const POINT point)
{
    if (!state.importedSceneLoaded || !layout.animationValid) {
        return false;
    }

    if (PtInRect(&layout.firstFrameButtonRect, point)) {
        state.importedScenePlaying = false;
        setImportedScenePlaybackSeconds(state, 0.0);
    } else if (PtInRect(&layout.playPauseButtonRect, point)) {
        toggleImportedGlbPlayback(hwnd, state);
        return true;
    } else if (PtInRect(&layout.lastFrameButtonRect, point)) {
        state.importedScenePlaying = false;
        setImportedScenePlaybackSeconds(state, importedSceneDurationSeconds(state));
    } else if (PtInRect(&layout.closeButtonRect, point)) {
        closeImportedGlb(hwnd, state);
        return true;
    } else if (PtInRect(&layout.timelineRect, point)) {
        state.importedSceneTimelineDragging = true;
        SetCapture(hwnd);
        seekImportedGlbFromTimeline(hwnd, state, layout.timelineRect, point);
        return true;
    } else {
        return false;
    }

    if (state.glWindow) {
        renderViewport(state.glWindow);
    }
    invalidateStatusPanel(hwnd);
    InvalidateRect(hwnd, nullptr, FALSE);
    return true;
}

void finishExportLocationDialog(HWND hwnd, ExportLocationDialogState& dialog, const bool accepted)
{
    if (accepted) {
        if (!dialog.editWindow ||
            !dialog.delayEditWindow ||
            !dialog.resampleFpsEditWindow ||
            !dialog.saveFormatComboWindow) {
            return;
        }

        const int textLength = GetWindowTextLengthW(dialog.editWindow);
        std::wstring text(static_cast<std::size_t>(textLength) + 1, L'\0');
        GetWindowTextW(dialog.editWindow, text.data(), textLength + 1);
        text.resize(static_cast<std::size_t>(textLength));

        const std::filesystem::path directory = normalizedExportDirectoryPath(
            std::filesystem::path(trimWide(text))
        );
        std::error_code createError;
        std::filesystem::create_directories(directory, createError);
        if (createError) {
            const std::wstring message = L"Could not create export folder:\n" +
                directory.wstring() + L"\n\n" + widen(createError.message());
            MessageBoxW(hwnd, message.c_str(), L"Record Settings", MB_OK | MB_ICONWARNING);
            return;
        }

        float recordDelaySeconds = 0.0f;
        if (!readFiniteFloatEdit(dialog.delayEditWindow, recordDelaySeconds) ||
            recordDelaySeconds < 0.0f) {
            MessageBoxW(
                hwnd,
                L"Record delay must be 0 or greater.",
                L"Record Settings",
                MB_OK | MB_ICONWARNING
            );
            return;
        }

        float exportSampleRate = static_cast<float>(kDefaultExportSampleRate);
        if (!readFiniteFloatEdit(dialog.resampleFpsEditWindow, exportSampleRate) ||
            exportSampleRate <= 0.0f) {
            MessageBoxW(
                hwnd,
                L"Resample FPS must be greater than 0.",
                L"Record Settings",
                MB_OK | MB_ICONWARNING
            );
            return;
        }

        dialog.resultDirectory = directory;
        dialog.resultRecordDelaySeconds = sanitizedRecordDelaySeconds(recordDelaySeconds);
        dialog.resultExportSampleRate = sanitizedExportSampleRate(exportSampleRate);
        const LRESULT saveFormatIndex = SendMessageW(dialog.saveFormatComboWindow, CB_GETCURSEL, 0, 0);
        dialog.resultSaveFormat = saveFormatIndex == 1 ? ExportFormat::Fbx : ExportFormat::Glb;
        if (dialog.appState) {
            dialog.appState->exportDirectory = directory;
            dialog.appState->recordDelaySeconds = dialog.resultRecordDelaySeconds;
            dialog.appState->recordExportSampleRate = dialog.resultExportSampleRate;
            dialog.appState->recordSaveFormat = dialog.resultSaveFormat;
            dialog.appState->exportStatusMessage = "Record settings updated";
            appendDebugLog(*dialog.appState, dialog.appState->exportStatusMessage);
            saveRecordSettingsConfig(*dialog.appState);
            if (dialog.parent) {
                invalidateStatusPanel(dialog.parent);
                InvalidateRect(dialog.parent, nullptr, FALSE);
            }
        }
    }

    dialog.accepted = accepted;
    dialog.done = true;
    DestroyWindow(hwnd);
}

LRESULT CALLBACK exportLocationDialogProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    auto* dialog = reinterpret_cast<ExportLocationDialogState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (message) {
    case WM_NCCREATE: {
        const auto* createStruct = reinterpret_cast<LPCREATESTRUCTW>(lparam);
        dialog = reinterpret_cast<ExportLocationDialogState*>(createStruct->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(dialog));
        return TRUE;
    }
    case WM_CREATE: {
        if (!dialog) {
            return -1;
        }

        HFONT font = reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
        HWND label = CreateWindowExW(0, L"STATIC", L"Export folder", WS_CHILD | WS_VISIBLE,
            18, 18, 140, 20, hwnd, nullptr, nullptr, nullptr);
        dialog->editWindow = CreateWindowExW(
            WS_EX_CLIENTEDGE,
            L"EDIT",
            normalizedExportDirectoryPath(dialog->initialDirectory).wstring().c_str(),
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
            18,
            44,
            410,
            24,
            hwnd,
            reinterpret_cast<HMENU>(kExportLocationEditControlId),
            nullptr,
            nullptr
        );
        HWND browseButton = CreateWindowExW(0, L"BUTTON", L"Browse...",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
            440, 43, 100, 26, hwnd,
            reinterpret_cast<HMENU>(kExportLocationBrowseControlId),
            nullptr, nullptr);
        HWND delayLabel = CreateWindowExW(0, L"STATIC", L"Record delay (seconds)", WS_CHILD | WS_VISIBLE,
            18, 84, 160, 20, hwnd, nullptr, nullptr, nullptr);
        dialog->delayEditWindow = CreateWindowExW(
            WS_EX_CLIENTEDGE,
            L"EDIT",
            formatFloatText(dialog->initialRecordDelaySeconds).c_str(),
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
            18,
            110,
            120,
            24,
            hwnd,
            reinterpret_cast<HMENU>(kRecordDelayEditControlId),
            nullptr,
            nullptr
        );
        HWND delayHint = CreateWindowExW(0, L"STATIC", L"0 starts immediately", WS_CHILD | WS_VISIBLE,
            150, 113, 180, 20, hwnd, nullptr, nullptr, nullptr);
        HWND saveFormatLabel = CreateWindowExW(0, L"STATIC", L"Save format", WS_CHILD | WS_VISIBLE,
            18, 146, 140, 20, hwnd, nullptr, nullptr, nullptr);
        dialog->saveFormatComboWindow = CreateWindowExW(
            0,
            L"COMBOBOX",
            L"",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST,
            18,
            172,
            120,
            120,
            hwnd,
            reinterpret_cast<HMENU>(kRecordSaveFormatControlId),
            nullptr,
            nullptr
        );
        if (dialog->saveFormatComboWindow) {
            SendMessageW(dialog->saveFormatComboWindow, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"glb"));
            SendMessageW(dialog->saveFormatComboWindow, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"fbx"));
            SendMessageW(
                dialog->saveFormatComboWindow,
                CB_SETCURSEL,
                dialog->initialSaveFormat == ExportFormat::Fbx ? 1 : 0,
                0
            );
        }
        HWND resampleFpsLabel = CreateWindowExW(0, L"STATIC", L"Resample FPS", WS_CHILD | WS_VISIBLE,
            178, 146, 140, 20, hwnd, nullptr, nullptr, nullptr);
        dialog->resampleFpsEditWindow = CreateWindowExW(
            WS_EX_CLIENTEDGE,
            L"EDIT",
            formatFloatText(dialog->initialExportSampleRate).c_str(),
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
            178,
            172,
            96,
            24,
            hwnd,
            reinterpret_cast<HMENU>(kRecordResampleFpsEditControlId),
            nullptr,
            nullptr
        );
        HWND resampleFpsHint = CreateWindowExW(0, L"STATIC", L"fps", WS_CHILD | WS_VISIBLE,
            286, 175, 42, 20, hwnd, nullptr, nullptr, nullptr);
        HWND okButton = CreateWindowExW(0, L"BUTTON", L"OK",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
            370, 198, 78, 26, hwnd, reinterpret_cast<HMENU>(IDOK), nullptr, nullptr);
        HWND cancelButton = CreateWindowExW(0, L"BUTTON", L"Cancel",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
            462, 198, 78, 26, hwnd, reinterpret_cast<HMENU>(IDCANCEL), nullptr, nullptr);

        for (HWND child : {
            label,
            dialog->editWindow,
            browseButton,
            delayLabel,
            dialog->delayEditWindow,
            delayHint,
            saveFormatLabel,
            dialog->saveFormatComboWindow,
            resampleFpsLabel,
            dialog->resampleFpsEditWindow,
            resampleFpsHint,
            okButton,
            cancelButton,
        }) {
            if (child) {
                SendMessageW(child, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
            }
        }
        if (dialog->editWindow) {
            SetFocus(dialog->editWindow);
            SendMessageW(dialog->editWindow, EM_SETSEL, 0, -1);
        }
        return 0;
    }
    case WM_COMMAND: {
        if (!dialog) {
            break;
        }

        const UINT command = LOWORD(wparam);
        if (command == kExportLocationBrowseControlId) {
            std::filesystem::path selectedDirectory;
            if (chooseExportDirectory(hwnd, dialog->resultDirectory, selectedDirectory)) {
                dialog->resultDirectory = normalizedExportDirectoryPath(selectedDirectory);
                setEditText(dialog->editWindow, dialog->resultDirectory.wstring());
            }
            return 0;
        }
        if (command == IDOK) {
            finishExportLocationDialog(hwnd, *dialog, true);
            return 0;
        }
        if (command == IDCANCEL) {
            finishExportLocationDialog(hwnd, *dialog, false);
            return 0;
        }
        break;
    }
    case WM_CLOSE:
        if (dialog) {
            finishExportLocationDialog(hwnd, *dialog, false);
            return 0;
        }
        break;
    default:
        break;
    }

    return DefWindowProcW(hwnd, message, wparam, lparam);
}

bool showExportLocationSettings(HWND parent, AppWindowState& state)
{
    ExportLocationDialogState dialog;
    dialog.parent = parent;
    dialog.appState = &state;
    dialog.initialDirectory = activeExportDirectoryPath(state);
    dialog.resultDirectory = dialog.initialDirectory;
    dialog.initialRecordDelaySeconds = sanitizedRecordDelaySeconds(state.recordDelaySeconds);
    dialog.resultRecordDelaySeconds = dialog.initialRecordDelaySeconds;
    dialog.initialExportSampleRate = sanitizedExportSampleRate(state.recordExportSampleRate);
    dialog.resultExportSampleRate = dialog.initialExportSampleRate;
    dialog.initialSaveFormat = state.recordSaveFormat;
    dialog.resultSaveFormat = dialog.initialSaveFormat;

    constexpr DWORD dialogStyle = WS_POPUP | WS_CAPTION | WS_SYSMENU;
    constexpr DWORD dialogExStyle = WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE;
    RECT dialogRect{0, 0, kExportLocationDialogWidth, kExportLocationDialogHeight};
    AdjustWindowRectEx(&dialogRect, dialogStyle, FALSE, dialogExStyle);
    const int dialogWindowWidth = dialogRect.right - dialogRect.left;
    const int dialogWindowHeight = dialogRect.bottom - dialogRect.top;

    RECT parentRect;
    GetWindowRect(parent, &parentRect);
    const int x = parentRect.left + ((parentRect.right - parentRect.left) - dialogWindowWidth) / 2;
    const int y = parentRect.top + ((parentRect.bottom - parentRect.top) - dialogWindowHeight) / 2;

    HINSTANCE instance = reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(parent, GWLP_HINSTANCE));
    EnableWindow(parent, FALSE);
    HWND dialogWindow = CreateWindowExW(
        dialogExStyle,
        kExportLocationDialogClassName,
        L"Record Settings",
        dialogStyle,
        x,
        y,
        dialogWindowWidth,
        dialogWindowHeight,
        parent,
        nullptr,
        instance,
        &dialog
    );
    if (!dialogWindow) {
        EnableWindow(parent, TRUE);
        SetActiveWindow(parent);
        return false;
    }

    ShowWindow(dialogWindow, SW_SHOW);
    UpdateWindow(dialogWindow);

    MSG msg{};
    BOOL messageResult = TRUE;
    while (!dialog.done && (messageResult = GetMessageW(&msg, nullptr, 0, 0)) > 0) {
        if (!IsWindow(dialogWindow) || !IsDialogMessageW(dialogWindow, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    EnableWindow(parent, TRUE);
    SetActiveWindow(parent);
    if (messageResult == 0) {
        PostQuitMessage(static_cast<int>(msg.wParam));
    }
    return dialog.accepted;
}

void showTopSettingsMenu(HWND hwnd, AppWindowState& state, const RECT& settingRect)
{
    HMENU menu = CreatePopupMenu();
    if (!menu) {
        return;
    }

    std::array<PopupMenuItem, 3> menuItems{{
        PopupMenuItem{kSettingsMenuColorId, L"Color..."},
        PopupMenuItem{kSettingsMenuOriginId, L"Origin..."},
        PopupMenuItem{kSettingsMenuLocationId, L"Record..."},
    }};
    for (PopupMenuItem& item : menuItems) {
        appendPopupMenuItem(menu, item);
    }

    POINT menuPoint{settingRect.left, settingRect.bottom};
    ClientToScreen(hwnd, &menuPoint);
    SetForegroundWindow(hwnd);
    const UINT command = TrackPopupMenu(
        menu,
        TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD,
        menuPoint.x,
        menuPoint.y,
        0,
        hwnd,
        nullptr
    );
    DestroyMenu(menu);

    if (command == kSettingsMenuColorId) {
        appendDebugLog(state, L"Color settings opened");
        showViewportColorSettings(hwnd, state);
        InvalidateRect(hwnd, nullptr, FALSE);
    } else if (command == kSettingsMenuOriginId) {
        appendDebugLog(state, L"Origin settings opened");
        showOriginSettings(hwnd, state);
        InvalidateRect(hwnd, nullptr, FALSE);
    } else if (command == kSettingsMenuLocationId) {
        appendDebugLog(state, L"Record settings opened");
        showExportLocationSettings(hwnd, state);
        InvalidateRect(hwnd, nullptr, FALSE);
    }
}

void showTopFileMenu(HWND hwnd, AppWindowState& state, const RECT& fileRect)
{
    HMENU menu = CreatePopupMenu();
    if (!menu) {
        return;
    }

    PopupMenuItem importItem{kFileMenuImportGlbId, L"Import GLB..."};
    appendPopupMenuItem(menu, importItem);

    POINT menuPoint{fileRect.left, fileRect.bottom};
    ClientToScreen(hwnd, &menuPoint);
    SetForegroundWindow(hwnd);
    const UINT command = TrackPopupMenu(
        menu,
        TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD,
        menuPoint.x,
        menuPoint.y,
        0,
        hwnd,
        nullptr
    );
    DestroyMenu(menu);

    if (command == kFileMenuImportGlbId) {
        importGlbFromFile(hwnd, state);
    }
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

    state.glOverlayFontBase = glGenLists(128);
    if (state.glOverlayFontBase != 0) {
        HFONT overlayFont = CreateFontW(
            -kDelayCountdownFontHeight,
            0,
            0,
            0,
            FW_BOLD,
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
        if (overlayFont) {
            previousFont = SelectObject(state.glDeviceContext, overlayFont);
        }
        wglUseFontBitmapsW(state.glDeviceContext, 0, 128, state.glOverlayFontBase);
        if (previousFont) {
            SelectObject(state.glDeviceContext, previousFont);
        }
        if (overlayFont) {
            DeleteObject(overlayFont);
        }
    }

    return true;
}

void updateOriginEditorLayout(HWND hwnd, AppWindowState& state)
{
    if (!state.originEditWindow || !IsWindow(state.originEditWindow)) {
        state.originEditWindow = nullptr;
        state.originEditOriginalProc = nullptr;
        return;
    }

    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    const RECT editRect = originEditorRectForClient(
        &state,
        clientRect.right - clientRect.left,
        clientRect.bottom - clientRect.top
    );
    if (editRect.right <= editRect.left || editRect.bottom <= editRect.top) {
        ShowWindow(state.originEditWindow, SW_HIDE);
        return;
    }

    MoveWindow(
        state.originEditWindow,
        editRect.left,
        editRect.top,
        editRect.right - editRect.left,
        editRect.bottom - editRect.top,
        TRUE
    );
    ShowWindow(state.originEditWindow, SW_SHOW);
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
    state->debugMonitorHeight = clampDebugMonitorHeightForClient(state->debugMonitorHeight, height);
    const RECT glRect = viewportRenderRectForClient(state, width, height);
    const int glWidth = glRect.right - glRect.left;
    const int glHeight = glRect.bottom - glRect.top;

    if (glWidth > 160 && glHeight > 160) {
        MoveWindow(state->glWindow, glRect.left, glRect.top, glWidth, glHeight, TRUE);
        ShowWindow(state->glWindow, SW_SHOW);
    } else {
        ShowWindow(state->glWindow, SW_HIDE);
    }

    updateOriginEditorLayout(hwnd, *state);
}

void invalidateWindowLayout(HWND hwnd)
{
    RedrawWindow(
        hwnd,
        nullptr,
        nullptr,
        RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_UPDATENOW
    );
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
    auto* state = reinterpret_cast<AppWindowState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    const int leftPanelWidth = leftPanelWidthForClient(state, clientRect.right - clientRect.left);
    const int statusBarTop = clientRect.bottom > kStatusBarHeight
        ? clientRect.bottom - kStatusBarHeight
        : 0;
    const int debugMonitorHeight = activeDebugMonitorHeight(
        state,
        clientRect.bottom - clientRect.top
    );
    const int debugMonitorTop = statusBarTop > debugMonitorHeight
        ? statusBarTop - debugMonitorHeight
        : statusBarTop;
    RECT statusPanelRect{0, 0, leftPanelWidth + kSplitterWidth, debugMonitorTop};
    RECT statusBarRect{0, debugMonitorTop, clientRect.right, clientRect.bottom};
    InvalidateRect(hwnd, &statusPanelRect, FALSE);
    InvalidateRect(hwnd, &statusBarRect, FALSE);
    const ViewportControlLayout controls = viewportControlLayoutForClient(
        state,
        clientRect.right - clientRect.left,
        clientRect.bottom - clientRect.top
    );
    if (controls.valid) {
        InvalidateRect(hwnd, &controls.barRect, FALSE);
    }
}

void refreshStatus(HWND hwnd, const bool forceDeviceEnumeration)
{
    auto* state = reinterpret_cast<AppWindowState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (!state) {
        return;
    }

    if (forceDeviceEnumeration) {
        appendDebugLog(*state, L"Manual runtime/device refresh requested");
    }

    const bool providerWasInitialized = state->provider.isInitialized();
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
        clearMissingDeviceSelection(*state);
    } else {
        state->devices.clear();
        state->lastDeviceEnumeration = {};
        state->poses = {};
        ensureOriginSelection(*state);
        clearMissingDeviceSelection(*state);
        state->providerError = state->provider.lastError();
    }

    if (!providerWasInitialized && state->provider.isInitialized()) {
        appendDebugLog(*state, L"OpenVR provider initialized");
    }
    if (state->providerError != state->lastLoggedProviderError) {
        if (state->providerError.empty()) {
            appendDebugLog(*state, L"OpenVR provider error cleared");
        } else {
            appendDebugLog(*state, "OpenVR provider error: " + state->providerError);
        }
        state->lastLoggedProviderError = state->providerError;
    }
    if (state->devices.size() != state->lastLoggedDeviceCount) {
        appendDebugLog(*state, L"Tracked device count: " + std::to_wstring(state->devices.size()));
        state->lastLoggedDeviceCount = state->devices.size();
    }

    updateFpsCounters(*state);
    invalidateStatusPanel(hwnd);
}

void updateDelayedRecordingStart(HWND hwnd, AppWindowState& state);

void refreshPoseAndViewport(HWND hwnd)
{
    auto* state = reinterpret_cast<AppWindowState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (!state) {
        return;
    }

    updateDelayedRecordingStart(hwnd, *state);
    updateImportedScenePlayback(*state);

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
                        state->originOffset,
                        state->originRotationDegrees
                    ).poses;

                    if (!state->recorder.appendFrame(frame)) {
                        const std::string appendError = state->recorder.lastError();
                        if (state->recordingError != appendError) {
                            appendDebugLog(*state, "Frame append failed: " + appendError);
                        }
                        state->recordingError = appendError;
                        invalidateStatusPanel(hwnd);
                    }
                }
            }
        }
    }

    if (state->glWindow) {
        renderViewport(state->glWindow);
    }
    if (state->importedSceneLoaded) {
        RECT clientRect;
        GetClientRect(hwnd, &clientRect);
        const ViewportControlLayout controls = viewportControlLayoutForClient(
            state,
            clientRect.right - clientRect.left,
            clientRect.bottom - clientRect.top
        );
        if (controls.animationValid) {
            InvalidateRect(hwnd, &controls.animationBarRect, FALSE);
        }
    }
}

void startRecordingNow(HWND hwnd, AppWindowState& state)
{
    state.recordingDelayActive = false;
    state.exportStatusMessage.clear();

    ovtr::RecordingSession session;
    session.sessionId = "session_" + localTimestampForPath();
    session.sessionName = session.sessionId;
    session.createdAtUtc = utcTimestampIso();
    session.appVersion = "0.1.0";
    session.targetSampleRate = kTargetViewportFps;
    session.devices = state.devices;
    applyCustomNamesToExportDevices(state, session.devices);

    state.currentSessionFolder = std::filesystem::current_path() / "recordings" / session.sessionId;

    ovtr::RecordingStartOptions options;
    options.sessionFolder = state.currentSessionFolder;
    options.session = session;

    state.recordingStart = std::chrono::steady_clock::now();
    state.recordingDroppedFrames = 0;
    state.recordingScheduler = ovtr::SamplingScheduler(kTargetViewportFps);
    state.recordingScheduler.reset(state.recordingStart);

    appendDebugLog(state, "Starting recording: " + state.currentSessionFolder.string());
    if (!state.recorder.start(options)) {
        state.recordingError = state.recorder.lastError();
        appendDebugLog(state, "Recording start failed: " + state.recordingError);
    } else {
        appendDebugLog(state, L"Recording started");
        startImportedGlbPlaybackForRecording(hwnd, state);
    }

    invalidateStatusPanel(hwnd);
}

void updateDelayedRecordingStart(HWND hwnd, AppWindowState& state)
{
    if (!state.recordingDelayActive) {
        return;
    }

    const auto now = std::chrono::steady_clock::now();
    if (now < state.recordingDelayDeadline) {
        return;
    }

    appendDebugLog(state, L"Record delay elapsed");
    startRecordingNow(hwnd, state);
}

void toggleRecording(HWND hwnd)
{
    auto* state = reinterpret_cast<AppWindowState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (!state) {
        return;
    }

    state->recordingError.clear();

    if (state->recordingDelayActive) {
        state->recordingDelayActive = false;
        appendDebugLog(*state, L"Recording start canceled");
        invalidateStatusPanel(hwnd);
        return;
    }

    if (state->recorder.state() == ovtr::RecorderState::Recording ||
        state->recorder.state() == ovtr::RecorderState::Paused) {
        appendDebugLog(*state, L"Stopping recording");
        const auto now = std::chrono::steady_clock::now();
        const double durationSeconds = std::chrono::duration<double>(now - state->recordingStart).count();
        if (!state->recorder.stop(durationSeconds, state->recordingDroppedFrames)) {
            state->recordingError = state->recorder.lastError();
            appendDebugLog(*state, "Recording stop failed: " + state->recordingError);
        } else {
            std::wostringstream stream;
            stream << L"Recording stopped: frames " << state->recorder.frameCount()
                   << L", dropped " << state->recordingDroppedFrames;
            appendDebugLog(*state, stream.str());
            exportCurrentSession(hwnd, state->recordSaveFormat);
        }
        invalidateStatusPanel(hwnd);
        return;
    }

    if (state->recorder.state() != ovtr::RecorderState::Idle &&
        state->recorder.state() != ovtr::RecorderState::Error) {
        appendDebugLog(*state, L"Recording toggle ignored: recorder is busy");
        return;
    }

    state->exportStatusMessage.clear();
    const float recordDelaySeconds = sanitizedRecordDelaySeconds(state->recordDelaySeconds);
    if (recordDelaySeconds > 0.0f) {
        const auto delayDuration = std::chrono::duration_cast<std::chrono::steady_clock::duration>(
            std::chrono::duration<double>(recordDelaySeconds)
        );
        state->recordingDelayActive = true;
        state->recordingDelayDeadline = std::chrono::steady_clock::now() + delayDuration;
        appendDebugLog(*state, L"Recording scheduled after " + formatFloatText(recordDelaySeconds) + L"s");
        invalidateStatusPanel(hwnd);
        return;
    }

    startRecordingNow(hwnd, *state);
}

bool isPathWithinDirectory(
    const std::filesystem::path& child,
    const std::filesystem::path& parent
)
{
    std::error_code error;
    const std::filesystem::path childPath = std::filesystem::weakly_canonical(child, error);
    if (error) {
        return false;
    }

    error.clear();
    const std::filesystem::path parentPath = std::filesystem::weakly_canonical(parent, error);
    if (error) {
        return false;
    }

    const std::filesystem::path relative = childPath.lexically_relative(parentPath);
    if (relative.empty()) {
        return false;
    }

    const auto first = relative.begin();
    if (first == relative.end()) {
        return false;
    }

    return *first != "..";
}

bool deleteTemporarySessionFolder(AppWindowState& state, std::string& message)
{
    message.clear();
    if (state.currentSessionFolder.empty()) {
        return true;
    }

    const std::filesystem::path sessionFolder = state.currentSessionFolder;
    const std::filesystem::path recordingsRoot = std::filesystem::current_path() / "recordings";
    const std::string folderName = sessionFolder.filename().string();
    if (folderName.rfind("session_", 0) != 0 ||
        !isPathWithinDirectory(sessionFolder, recordingsRoot)) {
        message = "session cleanup skipped: folder is outside recordings";
        return false;
    }

    std::error_code existsError;
    if (!std::filesystem::exists(sessionFolder, existsError)) {
        state.currentSessionFolder.clear();
        return true;
    }

    std::error_code removeError;
    const std::uintmax_t removedCount = std::filesystem::remove_all(sessionFolder, removeError);
    if (removeError) {
        message = "session cleanup failed: " + removeError.message();
        return false;
    }

    state.currentSessionFolder.clear();
    message = "Temporary session folder deleted: " + sessionFolder.string() +
        " (" + std::to_string(removedCount) + " entries)";
    return true;
}

void exportCurrentSession(HWND hwnd, const ExportFormat format)
{
    auto* state = reinterpret_cast<AppWindowState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (!state) {
        return;
    }

    state->exportStatusMessage.clear();

    if (state->recordingDelayActive ||
        state->recorder.state() == ovtr::RecorderState::Recording ||
        state->recorder.state() == ovtr::RecorderState::Paused ||
        state->recorder.state() == ovtr::RecorderState::Starting ||
        state->recorder.state() == ovtr::RecorderState::Stopping ||
        state->recorder.state() == ovtr::RecorderState::Finalizing) {
        state->exportStatusMessage = "stop recording before exporting";
        appendDebugLog(*state, "Export blocked: " + state->exportStatusMessage);
        invalidateStatusPanel(hwnd);
        return;
    }

    if (state->currentSessionFolder.empty()) {
        state->exportStatusMessage = "no recorded session available";
        appendDebugLog(*state, "Export blocked: " + state->exportStatusMessage);
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
    applyCustomNamesToExportDevices(*state, session.devices);
    if (session.framesPath.empty()) {
        session.framesPath = state->currentSessionFolder / "frames.bin";
    }
    if (session.frameIndexPath.empty()) {
        session.frameIndexPath = state->currentSessionFolder / "frame_index.bin";
    }

    ovtr::ExportResult result;
    const std::filesystem::path exportDirectory = activeExportDirectoryPath(*state);
    const double exportSampleRate = static_cast<double>(
        sanitizedExportSampleRate(state->recordExportSampleRate)
    );
    appendDebugLog(*state, format == ExportFormat::Fbx ? L"Starting FBX export" : L"Starting GLB export");
    if (format == ExportFormat::Fbx) {
        ovtr::FbxExportOptions options;
        options.outputPath = exportDirectory / (session.sessionId + ".fbx");
        options.includeGeometry = true;
        options.includeTrackingReference = true;
        options.exportSampleRate = exportSampleRate;
        result = ovtr::exportSessionToFbxAscii(session, options);
    } else {
        ovtr::GltfExportOptions options;
        options.outputPath = exportDirectory / (session.sessionId + ".glb");
        options.includeTrackingReference = true;
        options.exportSampleRate = exportSampleRate;
        options.format = ovtr::GltfExportFormat::Glb;
        result = ovtr::exportSessionToGltf(session, options);
    }

    if (result.success) {
        state->exportStatusMessage = (format == ExportFormat::Fbx ? "FBX saved to " : "GLB saved to ") +
            result.outputPath.string();
        appendDebugLog(*state, state->exportStatusMessage);
        std::string cleanupMessage;
        if (deleteTemporarySessionFolder(*state, cleanupMessage)) {
            if (!cleanupMessage.empty()) {
                appendDebugLog(*state, cleanupMessage);
            }
        } else {
            if (!cleanupMessage.empty()) {
                appendDebugLog(*state, cleanupMessage);
                state->exportStatusMessage += " (" + cleanupMessage + ")";
            }
        }
    } else {
        state->exportStatusMessage = (format == ExportFormat::Fbx ? "FBX export failed: " : "GLB export failed: ") +
            result.error;
        appendDebugLog(*state, state->exportStatusMessage);
    }

    invalidateStatusPanel(hwnd);
}

void drawOriginStepperButton(HDC drawDc, const RECT& rect, const wchar_t* label)
{
    HBRUSH buttonBrush = CreateSolidBrush(RGB(30, 34, 42));
    HPEN buttonPen = CreatePen(PS_SOLID, 1, RGB(67, 74, 88));
    HGDIOBJ previousBrush = SelectObject(drawDc, buttonBrush);
    HGDIOBJ previousPen = SelectObject(drawDc, buttonPen);
    RoundRect(drawDc, rect.left, rect.top, rect.right, rect.bottom, 4, 4);
    SelectObject(drawDc, previousBrush);
    SelectObject(drawDc, previousPen);
    DeleteObject(buttonBrush);
    DeleteObject(buttonPen);

    RECT textRect = rect;
    SetTextColor(drawDc, RGB(225, 231, 240));
    DrawTextW(drawDc, label, -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

void drawDeviceToggleButton(HDC drawDc, HFONT font, const RECT& rect, const bool expanded)
{
    HBRUSH buttonBrush = CreateSolidBrush(expanded ? RGB(48, 63, 82) : RGB(30, 34, 42));
    HPEN buttonPen = CreatePen(PS_SOLID, 1, expanded ? RGB(92, 126, 168) : RGB(67, 74, 88));
    HGDIOBJ previousBrush = SelectObject(drawDc, buttonBrush);
    HGDIOBJ previousPen = SelectObject(drawDc, buttonPen);
    RoundRect(drawDc, rect.left, rect.top, rect.right, rect.bottom, 6, 6);
    SelectObject(drawDc, previousBrush);
    SelectObject(drawDc, previousPen);
    DeleteObject(buttonBrush);
    DeleteObject(buttonPen);

    HGDIOBJ previousFont = nullptr;
    if (font) {
        previousFont = SelectObject(drawDc, font);
    }

    SetTextColor(drawDc, RGB(225, 231, 240));
    const wchar_t* label = L"Device";
    const int labelLength = static_cast<int>(std::wcslen(label));
    const int totalHeight = rect.bottom - rect.top;
    const int lineHeight = totalHeight >= 84 ? 14 : (totalHeight / (labelLength + 1));
    const int labelHeight = lineHeight * labelLength;
    int y = rect.top + (totalHeight - labelHeight) / 2;
    for (int i = 0; i < labelLength; ++i) {
        RECT charRect{rect.left, y, rect.right, y + lineHeight};
        DrawTextW(drawDc, label + i, 1, &charRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        y += lineHeight;
    }

    if (previousFont) {
        SelectObject(drawDc, previousFont);
    }
}

bool isRecordingControlActive(const AppWindowState& state)
{
    return state.recordingDelayActive ||
        state.recorder.state() == ovtr::RecorderState::Recording ||
        state.recorder.state() == ovtr::RecorderState::Paused;
}

void drawViewportIconButton(
    HDC drawDc,
    const RECT& rect,
    const bool active
)
{
    HBRUSH buttonBrush = CreateSolidBrush(active ? RGB(58, 42, 42) : RGB(30, 34, 42));
    HPEN buttonPen = CreatePen(PS_SOLID, 1, active ? RGB(180, 72, 72) : RGB(67, 74, 88));
    HGDIOBJ previousBrush = SelectObject(drawDc, buttonBrush);
    HGDIOBJ previousPen = SelectObject(drawDc, buttonPen);
    RoundRect(drawDc, rect.left, rect.top, rect.right, rect.bottom, 6, 6);
    SelectObject(drawDc, previousBrush);
    SelectObject(drawDc, previousPen);
    DeleteObject(buttonBrush);
    DeleteObject(buttonPen);
}

void drawViewportRecordButton(HDC drawDc, const RECT& rect, const bool active)
{
    drawViewportIconButton(drawDc, rect, active);

    const int inset = active ? 7 : 8;
    RECT dotRect{
        rect.left + inset,
        rect.top + inset,
        rect.right - inset,
        rect.bottom - inset
    };
    HBRUSH dotBrush = CreateSolidBrush(active ? RGB(255, 88, 82) : RGB(228, 68, 64));
    HGDIOBJ previousBrush = SelectObject(drawDc, dotBrush);
    HGDIOBJ previousPen = SelectObject(drawDc, GetStockObject(NULL_PEN));
    Ellipse(drawDc, dotRect.left, dotRect.top, dotRect.right, dotRect.bottom);
    SelectObject(drawDc, previousBrush);
    SelectObject(drawDc, previousPen);
    DeleteObject(dotBrush);
}

void drawImportedAnimationButton(HDC drawDc, HFONT font, const RECT& rect, const wchar_t* label)
{
    HBRUSH buttonBrush = CreateSolidBrush(RGB(30, 34, 42));
    HPEN buttonPen = CreatePen(PS_SOLID, 1, RGB(67, 74, 88));
    HGDIOBJ previousBrush = SelectObject(drawDc, buttonBrush);
    HGDIOBJ previousPen = SelectObject(drawDc, buttonPen);
    RoundRect(drawDc, rect.left, rect.top, rect.right, rect.bottom, 6, 6);
    SelectObject(drawDc, previousBrush);
    SelectObject(drawDc, previousPen);
    DeleteObject(buttonBrush);
    DeleteObject(buttonPen);

    if (font) {
        SelectObject(drawDc, font);
    }
    SetTextColor(drawDc, RGB(225, 231, 240));
    RECT textRect = rect;
    DrawTextW(drawDc, label, -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
}

void drawImportedAnimationControls(
    HDC drawDc,
    HFONT font,
    const ViewportControlLayout& layout,
    const AppWindowState& state
)
{
    if (!layout.animationValid || !state.importedSceneLoaded) {
        return;
    }

    HBRUSH barBrush = CreateSolidBrush(RGB(20, 23, 28));
    FillRect(drawDc, &layout.animationBarRect, barBrush);
    DeleteObject(barBrush);

    HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(54, 58, 66));
    HGDIOBJ previousPen = SelectObject(drawDc, borderPen);
    MoveToEx(drawDc, layout.animationBarRect.left, layout.animationBarRect.top, nullptr);
    LineTo(drawDc, layout.animationBarRect.right, layout.animationBarRect.top);
    SelectObject(drawDc, previousPen);
    DeleteObject(borderPen);

    drawImportedAnimationButton(drawDc, font, layout.firstFrameButtonRect, L"|<");
    drawImportedAnimationButton(drawDc, font, layout.playPauseButtonRect, state.importedScenePlaying ? L"||" : L">");
    drawImportedAnimationButton(drawDc, font, layout.lastFrameButtonRect, L">|");
    drawImportedAnimationButton(drawDc, font, layout.closeButtonRect, L"Close");

    RECT trackRect{
        layout.timelineRect.left,
        layout.timelineRect.top + (layout.timelineRect.bottom - layout.timelineRect.top - 6) / 2,
        layout.timelineRect.right,
        layout.timelineRect.top + (layout.timelineRect.bottom - layout.timelineRect.top - 6) / 2 + 6
    };
    HBRUSH trackBrush = CreateSolidBrush(RGB(48, 54, 66));
    FillRect(drawDc, &trackRect, trackBrush);
    DeleteObject(trackBrush);

    const double durationSeconds = importedSceneDurationSeconds(state);
    const double factor = durationSeconds > 0.0
        ? std::clamp(state.importedScenePlaybackSeconds / durationSeconds, 0.0, 1.0)
        : 0.0;
    RECT fillRect = trackRect;
    fillRect.right = fillRect.left + static_cast<int>(
        static_cast<double>(trackRect.right - trackRect.left) * factor
    );
    HBRUSH fillBrush = CreateSolidBrush(RGB(212, 222, 236));
    FillRect(drawDc, &fillRect, fillBrush);
    DeleteObject(fillBrush);

    const int handleCenterX = fillRect.right;
    RECT handleRect{handleCenterX - 4, trackRect.top - 5, handleCenterX + 4, trackRect.bottom + 5};
    HBRUSH handleBrush = CreateSolidBrush(RGB(255, 255, 255));
    HGDIOBJ previousBrush = SelectObject(drawDc, handleBrush);
    HGDIOBJ previousHandlePen = SelectObject(drawDc, GetStockObject(NULL_PEN));
    Ellipse(drawDc, handleRect.left, handleRect.top, handleRect.right, handleRect.bottom);
    SelectObject(drawDc, previousBrush);
    SelectObject(drawDc, previousHandlePen);
    DeleteObject(handleBrush);

    std::wostringstream frameText;
    frameText << L"Frame " << importedSceneCurrentFrame(state)
              << L" / " << importedSceneTotalFrames(state);
    if (font) {
        SelectObject(drawDc, font);
    }
    SetTextColor(drawDc, RGB(202, 211, 224));
    RECT frameTextRect = layout.frameTextRect;
    DrawTextW(
        drawDc,
        frameText.str().c_str(),
        -1,
        &frameTextRect,
        DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS
    );
}

void drawViewportControlBar(
    HDC drawDc,
    HFONT font,
    const ViewportControlLayout& layout,
    const AppWindowState& state
)
{
    if (!layout.valid) {
        return;
    }

    drawImportedAnimationControls(drawDc, font, layout, state);

    HBRUSH barBrush = CreateSolidBrush(RGB(18, 20, 24));
    FillRect(drawDc, &layout.barRect, barBrush);
    DeleteObject(barBrush);

    HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(54, 58, 66));
    HGDIOBJ previousPen = SelectObject(drawDc, borderPen);
    MoveToEx(drawDc, layout.barRect.left, layout.barRect.top, nullptr);
    LineTo(drawDc, layout.barRect.right, layout.barRect.top);
    SelectObject(drawDc, previousPen);
    DeleteObject(borderPen);

    drawViewportRecordButton(drawDc, layout.recordButtonRect, isRecordingControlActive(state));

    if (font) {
        SelectObject(drawDc, font);
    }
}

void drawTopBarMenuButton(HDC drawDc, HFONT font, const RECT& rect, const wchar_t* label)
{
    if (rect.right <= rect.left || rect.bottom <= rect.top) {
        return;
    }

    HBRUSH buttonBrush = CreateSolidBrush(RGB(30, 34, 42));
    HPEN buttonPen = CreatePen(PS_SOLID, 1, RGB(67, 74, 88));
    HGDIOBJ previousBrush = SelectObject(drawDc, buttonBrush);
    HGDIOBJ previousPen = SelectObject(drawDc, buttonPen);
    RoundRect(drawDc, rect.left, rect.top, rect.right, rect.bottom, 6, 6);
    SelectObject(drawDc, previousBrush);
    SelectObject(drawDc, previousPen);
    DeleteObject(buttonBrush);
    DeleteObject(buttonPen);

    if (font) {
        SelectObject(drawDc, font);
    }
    SetTextColor(drawDc, RGB(225, 231, 240));
    RECT textRect = rect;
    DrawTextW(drawDc, label, -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
}

void drawOriginStepperRow(
    HDC drawDc,
    HFONT labelFont,
    HFONT valueFont,
    const OriginPanelLayout& layout,
    const AppWindowState& state,
    const bool rotation
)
{
    const RECT rowRect = originStepperRowRect(layout, rotation);
    const int rowHeight = rowRect.bottom - rowRect.top;
    const int rowWidth = rowRect.right - rowRect.left;
    if (rowHeight < kOriginStepperButtonSize || rowWidth <= kOriginStepperLabelWidth + 48) {
        return;
    }

    SelectObject(drawDc, labelFont);
    SetTextColor(drawDc, RGB(168, 180, 196));
    RECT rowLabelRect{
        rowRect.left,
        rowRect.top,
        rowRect.left + kOriginStepperLabelWidth,
        rowRect.bottom
    };
    DrawTextW(
        drawDc,
        rotation ? L"Rot" : L"Pos",
        -1,
        &rowLabelRect,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS
    );

    const int columnsLeft = rowRect.left + kOriginStepperLabelWidth;
    const int columnWidth = (rowRect.right - columnsLeft) / 3;
    if (columnWidth < 56) {
        return;
    }

    static constexpr const wchar_t* kAxisLabels[] = {L"X", L"Y", L"Z"};
    const std::array<float, 3>& values = rotation ? state.originRotationDegrees : state.originOffset;
    const int buttonTop = rowRect.top + (rowHeight - kOriginStepperButtonSize) / 2;

    for (int axis = 0; axis < 3; ++axis) {
        const int columnLeft = columnsLeft + axis * columnWidth;
        const int columnRight = axis == 2 ? rowRect.right : columnLeft + columnWidth;
        const int minusLeft = columnLeft + 14;
        const int plusRight = columnRight - 4;
        RECT minusRect{
            minusLeft,
            buttonTop,
            minusLeft + kOriginStepperButtonSize,
            buttonTop + kOriginStepperButtonSize
        };
        RECT plusRect{
            plusRight - kOriginStepperButtonSize,
            buttonTop,
            plusRight,
            buttonTop + kOriginStepperButtonSize
        };
        RECT axisRect{
            columnLeft,
            rowRect.top,
            minusRect.left - 2,
            rowRect.bottom
        };
        RECT valueRect{
            minusRect.right + 2,
            rowRect.top,
            plusRect.left - 2,
            rowRect.bottom
        };

        SelectObject(drawDc, labelFont);
        SetTextColor(drawDc, RGB(168, 180, 196));
        DrawTextW(drawDc, kAxisLabels[axis], -1, &axisRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

        SelectObject(drawDc, valueFont);
        drawOriginStepperButton(drawDc, minusRect, L"-");
        const std::wstring valueText = formatOriginStepperValue(values[static_cast<std::size_t>(axis)]);
        SetTextColor(drawDc, RGB(202, 211, 224));
        DrawTextW(
            drawDc,
            valueText.c_str(),
            static_cast<int>(valueText.size()),
            &valueRect,
            DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS
        );
        drawOriginStepperButton(drawDc, plusRect, L"+");
    }
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
    HFONT statusFont = CreateFontW(
        15,
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
    HFONT debugFont = CreateFontW(
        14,
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
        L"Consolas"
    );

    auto* state = reinterpret_cast<AppWindowState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    const bool devicePanelVisible = state && state->devicePanelVisible;
    const auto deviceRows = devicePanelVisible ? makeDeviceListRows(*state) : std::vector<DeviceListRow>{};
    const int leftPanelWidth = leftPanelWidthForClient(state, clientRect.right - clientRect.left);
    const int statusBarTop = clientHeight > kStatusBarHeight ? clientHeight - kStatusBarHeight : 0;
    const int debugMonitorHeight = activeDebugMonitorHeight(state, clientHeight);
    const int debugMonitorTop = statusBarTop > debugMonitorHeight
        ? statusBarTop - debugMonitorHeight
        : statusBarTop;
    const int contentBottom = leftPanelContentBottomForClient(state, clientHeight);
    const OriginPanelLayout originPanelLayout = state
        ? originPanelLayoutForClient(state, clientWidth, clientHeight)
        : OriginPanelLayout{};
    const DeviceListLayout deviceListLayout = state
        ? deviceListLayoutForClient(state, clientWidth, clientHeight)
        : DeviceListLayout{};
    const ViewportControlLayout viewportControlLayout = state
        ? viewportControlLayoutForClient(state, clientWidth, clientHeight)
        : ViewportControlLayout{};

    const RECT topBarRect{0, 0, clientWidth, clientHeight < kTopBarHeight ? clientHeight : kTopBarHeight};
    if (topBarRect.bottom > topBarRect.top) {
        HBRUSH topBarBrush = CreateSolidBrush(RGB(18, 20, 24));
        FillRect(drawDc, &topBarRect, topBarBrush);
        DeleteObject(topBarBrush);

        HPEN topBarPen = CreatePen(PS_SOLID, 1, RGB(54, 58, 66));
        HGDIOBJ previousTopBarPen = SelectObject(drawDc, topBarPen);
        MoveToEx(drawDc, 0, topBarRect.bottom - 1, nullptr);
        LineTo(drawDc, clientWidth, topBarRect.bottom - 1);
        SelectObject(drawDc, previousTopBarPen);
        DeleteObject(topBarPen);

        drawTopBarMenuButton(
            drawDc,
            statusFont ? statusFont : bodyFont,
            topBarFileRectForClient(clientWidth, clientHeight),
            L"File"
        );
        drawTopBarMenuButton(
            drawDc,
            statusFont ? statusFont : bodyFont,
            topBarSettingRectForClient(clientWidth, clientHeight),
            L"Setting"
        );
    }

    if (state && deviceListLayout.valid) {
        clampDeviceListScroll(*state, deviceListLayout.visibleItemCount);

        RECT deviceBoxRect = deviceListLayout.boxRect;
        HBRUSH boxBrush = CreateSolidBrush(RGB(20, 23, 28));
        FillRect(drawDc, &deviceBoxRect, boxBrush);
        DeleteObject(boxBrush);

        HPEN boxPen = CreatePen(PS_SOLID, 1, RGB(58, 64, 76));
        HGDIOBJ previousPen = SelectObject(drawDc, boxPen);
        HGDIOBJ previousBrush = SelectObject(drawDc, GetStockObject(NULL_BRUSH));
        Rectangle(drawDc, deviceBoxRect.left, deviceBoxRect.top, deviceBoxRect.right, deviceBoxRect.bottom);
        SelectObject(drawDc, previousBrush);
        SelectObject(drawDc, previousPen);
        DeleteObject(boxPen);

        const int maxScrollOffset = maxDeviceListScrollOffset(*state, deviceListLayout.visibleItemCount);
        const bool showScrollbar = maxScrollOffset > 0;
        const int itemTextRight = showScrollbar
            ? deviceListLayout.contentRect.right - 14
            : deviceListLayout.contentRect.right;
        const int listTextWidth = itemTextRight - deviceListLayout.headerRect.left;
        const int nameDividerX = deviceListLayout.headerRect.left + (listTextWidth * 24) / 100;
        const int modelDividerX = deviceListLayout.headerRect.left + (listTextWidth * 64) / 100;
        const int nameColumnLeft = deviceListLayout.headerRect.left;
        const int nameColumnRight = nameDividerX - 10;
        const int modelColumnLeft = nameDividerX + 12;
        const int modelColumnRight = modelDividerX - 10;
        const int serialColumnLeft = modelDividerX + 12;

        HBRUSH headerBrush = CreateSolidBrush(RGB(24, 28, 35));
        FillRect(drawDc, &deviceListLayout.headerRect, headerBrush);
        DeleteObject(headerBrush);

        HPEN gridPen = CreatePen(PS_SOLID, 1, RGB(43, 48, 59));
        HGDIOBJ previousGridPen = SelectObject(drawDc, gridPen);
        MoveToEx(drawDc, nameDividerX, deviceListLayout.headerRect.top, nullptr);
        LineTo(drawDc, nameDividerX, deviceListLayout.contentRect.bottom);
        MoveToEx(drawDc, modelDividerX, deviceListLayout.headerRect.top, nullptr);
        LineTo(drawDc, modelDividerX, deviceListLayout.contentRect.bottom);
        MoveToEx(drawDc, deviceListLayout.headerRect.left, deviceListLayout.headerRect.bottom, nullptr);
        LineTo(drawDc, itemTextRight, deviceListLayout.headerRect.bottom);
        SelectObject(drawDc, previousGridPen);

        SelectObject(drawDc, statusFont ? statusFont : bodyFont);
        SetTextColor(drawDc, RGB(168, 180, 196));
        RECT nameHeaderRect{
            nameColumnLeft,
            deviceListLayout.headerRect.top,
            nameColumnRight,
            deviceListLayout.headerRect.bottom
        };
        RECT modelHeaderRect{
            modelColumnLeft,
            deviceListLayout.headerRect.top,
            modelColumnRight,
            deviceListLayout.headerRect.bottom
        };
        RECT serialHeaderRect{
            serialColumnLeft,
            deviceListLayout.headerRect.top,
            itemTextRight,
            deviceListLayout.headerRect.bottom
        };
        DrawTextW(drawDc, L"Name", -1, &nameHeaderRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        DrawTextW(drawDc, L"Model", -1, &modelHeaderRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        DrawTextW(drawDc, L"Serial", -1, &serialHeaderRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

        SelectObject(drawDc, bodyFont);
        SetTextColor(drawDc, RGB(202, 211, 224));
        int itemY = deviceListLayout.contentRect.top;
        if (deviceRows.empty()) {
            const std::wstring emptyText = L"No tracked devices";
            RECT itemRect{
                deviceListLayout.contentRect.left,
                itemY,
                itemTextRight,
                itemY + kDeviceListItemHeight
            };
            DrawTextW(drawDc, emptyText.c_str(), static_cast<int>(emptyText.size()), &itemRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        } else {
            const int firstItemIndex = state->deviceListScrollOffset;
            const int lastItemIndex = firstItemIndex + deviceListLayout.visibleItemCount <
                    static_cast<int>(deviceRows.size())
                ? firstItemIndex + deviceListLayout.visibleItemCount
                : static_cast<int>(deviceRows.size());

            HBRUSH selectedRowBrush = CreateSolidBrush(RGB(38, 55, 78));
            HBRUSH selectedAccentBrush = CreateSolidBrush(RGB(98, 139, 190));

            for (int i = firstItemIndex; i < lastItemIndex; ++i) {
                const DeviceListRow& row = deviceRows[static_cast<std::size_t>(i)];
                const int rowBottom = itemY + kDeviceListItemHeight;
                const bool selected = row.runtimeIndex == state->selectedDeviceRuntimeIndex;
                if (selected) {
                    RECT highlightRect{
                        deviceListLayout.contentRect.left,
                        itemY + 1,
                        itemTextRight,
                        rowBottom
                    };
                    FillRect(drawDc, &highlightRect, selectedRowBrush);

                    RECT accentRect{
                        deviceListLayout.contentRect.left,
                        itemY + 1,
                        deviceListLayout.contentRect.left + 3,
                        rowBottom
                    };
                    FillRect(drawDc, &accentRect, selectedAccentBrush);
                }

                RECT customNameRect{
                    deviceListLayout.contentRect.left + (selected ? 8 : 0),
                    itemY,
                    nameColumnRight,
                    rowBottom
                };
                RECT modelRect{
                    modelColumnLeft,
                    itemY,
                    modelColumnRight,
                    rowBottom
                };
                RECT serialRect{
                    serialColumnLeft,
                    itemY,
                    itemTextRight,
                    rowBottom
                };
                SetTextColor(drawDc, selected ? RGB(236, 242, 250) : RGB(202, 211, 224));
                DrawTextW(drawDc, row.customName.c_str(), static_cast<int>(row.customName.size()), &customNameRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
                DrawTextW(drawDc, row.model.c_str(), static_cast<int>(row.model.size()), &modelRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
                DrawTextW(drawDc, row.serial.c_str(), static_cast<int>(row.serial.size()), &serialRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

                if (i + 1 < lastItemIndex && rowBottom < deviceListLayout.contentRect.bottom) {
                    HGDIOBJ previousGridPen = SelectObject(drawDc, gridPen);
                    MoveToEx(drawDc, deviceListLayout.contentRect.left, rowBottom, nullptr);
                    LineTo(drawDc, itemTextRight, rowBottom);
                    SelectObject(drawDc, previousGridPen);
                }
                itemY += kDeviceListItemHeight;
            }

            DeleteObject(selectedAccentBrush);
            DeleteObject(selectedRowBrush);

            if (showScrollbar) {
                RECT scrollbarTrack{
                    deviceListLayout.contentRect.right - 8,
                    deviceListLayout.contentRect.top,
                    deviceListLayout.contentRect.right,
                    deviceListLayout.contentRect.bottom
                };
                HBRUSH trackBrush = CreateSolidBrush(RGB(34, 38, 47));
                FillRect(drawDc, &scrollbarTrack, trackBrush);
                DeleteObject(trackBrush);

                const int totalItems = static_cast<int>(deviceRows.size());
                const int trackHeight = scrollbarTrack.bottom - scrollbarTrack.top;
                int thumbHeight = (trackHeight * deviceListLayout.visibleItemCount) / totalItems;
                if (thumbHeight < 18) {
                    thumbHeight = 18;
                }
                if (thumbHeight > trackHeight) {
                    thumbHeight = trackHeight;
                }

                const int travel = trackHeight - thumbHeight;
                const int thumbTop = scrollbarTrack.top +
                    (maxScrollOffset > 0 ? (state->deviceListScrollOffset * travel) / maxScrollOffset : 0);
                RECT thumbRect{scrollbarTrack.left, thumbTop, scrollbarTrack.right, thumbTop + thumbHeight};
                HBRUSH thumbBrush = CreateSolidBrush(RGB(88, 101, 123));
                FillRect(drawDc, &thumbRect, thumbBrush);
                DeleteObject(thumbBrush);
            }
        }
        DeleteObject(gridPen);
    }

    if (state && originPanelLayout.valid) {
        HBRUSH panelBrush = CreateSolidBrush(RGB(20, 23, 28));
        FillRect(drawDc, &originPanelLayout.boxRect, panelBrush);
        DeleteObject(panelBrush);

        HPEN panelPen = CreatePen(PS_SOLID, 1, RGB(58, 64, 76));
        HGDIOBJ previousPen = SelectObject(drawDc, panelPen);
        HGDIOBJ previousBrush = SelectObject(drawDc, GetStockObject(NULL_BRUSH));
        Rectangle(
            drawDc,
            originPanelLayout.boxRect.left,
            originPanelLayout.boxRect.top,
            originPanelLayout.boxRect.right,
            originPanelLayout.boxRect.bottom
        );
        SelectObject(drawDc, previousBrush);
        SelectObject(drawDc, previousPen);
        DeleteObject(panelPen);

        SelectObject(drawDc, statusFont ? statusFont : bodyFont);
        SetTextColor(drawDc, RGB(168, 180, 196));
        RECT originTitleRect{
            originPanelLayout.boxRect.left + kOriginPanelPadding,
            originPanelLayout.boxRect.top + 8,
            originPanelLayout.boxRect.right - kOriginPanelPadding,
            originPanelLayout.boxRect.top + 32
        };
        DrawTextW(drawDc, L"Origin", -1, &originTitleRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

        RECT originStateRect = originTitleRect;
        const std::wstring originStateText = state->originEnabled ? L"Enabled" : L"Disabled";
        SetTextColor(drawDc, state->originEnabled ? RGB(180, 216, 174) : RGB(156, 166, 178));
        DrawTextW(
            drawDc,
            originStateText.c_str(),
            static_cast<int>(originStateText.size()),
            &originStateRect,
            DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS
        );

        const bool editingOrigin = state->originEditWindow != nullptr && IsWindow(state->originEditWindow);
        if (!editingOrigin) {
            drawOriginStepperRow(
                drawDc,
                statusFont ? statusFont : bodyFont,
                debugFont ? debugFont : bodyFont,
                originPanelLayout,
                *state,
                false
            );
            drawOriginStepperRow(
                drawDc,
                statusFont ? statusFont : bodyFont,
                debugFont ? debugFont : bodyFont,
                originPanelLayout,
                *state,
                true
            );
        }
    }

    if (state) {
        RECT railRect{0, kTopBarHeight, kDeviceToggleRailWidth, contentBottom};
        HBRUSH railBrush = CreateSolidBrush(RGB(20, 23, 28));
        FillRect(drawDc, &railRect, railBrush);
        DeleteObject(railBrush);

        HPEN railPen = CreatePen(PS_SOLID, 1, RGB(43, 48, 59));
        HGDIOBJ previousRailPen = SelectObject(drawDc, railPen);
        MoveToEx(drawDc, kDeviceToggleRailWidth - 1, railRect.top, nullptr);
        LineTo(drawDc, kDeviceToggleRailWidth - 1, railRect.bottom);
        SelectObject(drawDc, previousRailPen);
        DeleteObject(railPen);

        const RECT deviceButtonRect = deviceToggleButtonRectForClient(state, clientWidth, clientHeight);
        if (deviceButtonRect.right > deviceButtonRect.left && deviceButtonRect.bottom > deviceButtonRect.top) {
            drawDeviceToggleButton(
                drawDc,
                statusFont ? statusFont : bodyFont,
                deviceButtonRect,
                state->devicePanelVisible
            );
        }

        const RECT splitterRect = splitterRectForClient(state, clientWidth, clientHeight);
        if (splitterRect.bottom > splitterRect.top && splitterRect.right > splitterRect.left) {
            HBRUSH splitterBrush = CreateSolidBrush(
                state->splitterDragging ? RGB(50, 60, 76) : RGB(28, 31, 38)
            );
            FillRect(drawDc, &splitterRect, splitterBrush);
            DeleteObject(splitterBrush);

            HPEN splitterPen = CreatePen(PS_SOLID, 1, RGB(58, 64, 76));
            HGDIOBJ previousPen = SelectObject(drawDc, splitterPen);
            MoveToEx(drawDc, splitterRect.left, splitterRect.top, nullptr);
            LineTo(drawDc, splitterRect.left, splitterRect.bottom);
            MoveToEx(drawDc, splitterRect.right - 1, splitterRect.top, nullptr);
            LineTo(drawDc, splitterRect.right - 1, splitterRect.bottom);
            SelectObject(drawDc, previousPen);
            DeleteObject(splitterPen);

            if (state->devicePanelVisible) {
                HPEN handlePen = CreatePen(
                    PS_SOLID,
                    1,
                    state->splitterDragging ? RGB(128, 154, 190) : RGB(84, 92, 108)
                );
                previousPen = SelectObject(drawDc, handlePen);
                const int handleX = splitterRect.left + (kSplitterWidth / 2);
                const int handleTop = splitterRect.top + 20;
                const int handleBottom = splitterRect.bottom > 20 ? splitterRect.bottom - 20 : splitterRect.bottom;
                MoveToEx(drawDc, handleX, handleTop, nullptr);
                LineTo(drawDc, handleX, handleBottom);
                SelectObject(drawDc, previousPen);
                DeleteObject(handlePen);
            }
        }
    }

    if (state) {
        drawViewportControlBar(
            drawDc,
            statusFont ? statusFont : bodyFont,
            viewportControlLayout,
            *state
        );
    }

    if (state && debugMonitorHeight > 0) {
        RECT debugPanelRect{0, debugMonitorTop, clientWidth, statusBarTop};
        HBRUSH debugPanelBrush = CreateSolidBrush(RGB(21, 23, 28));
        FillRect(drawDc, &debugPanelRect, debugPanelBrush);
        DeleteObject(debugPanelBrush);

        const RECT debugResizeRect = debugResizeRectForClient(state, clientWidth, clientHeight);
        if (debugResizeRect.bottom > debugResizeRect.top) {
            HBRUSH resizeBrush = CreateSolidBrush(
                state->debugResizeDragging ? RGB(50, 60, 76) : RGB(25, 28, 35)
            );
            FillRect(drawDc, &debugResizeRect, resizeBrush);
            DeleteObject(resizeBrush);
        }

        HPEN debugBorderPen = CreatePen(PS_SOLID, 1, RGB(62, 67, 78));
        HGDIOBJ previousPen = SelectObject(drawDc, debugBorderPen);
        MoveToEx(drawDc, 0, debugMonitorTop, nullptr);
        LineTo(drawDc, clientWidth, debugMonitorTop);
        if (debugResizeRect.bottom > debugResizeRect.top) {
            MoveToEx(drawDc, 0, debugResizeRect.bottom, nullptr);
            LineTo(drawDc, clientWidth, debugResizeRect.bottom);
        }
        MoveToEx(drawDc, 0, statusBarTop - 1, nullptr);
        LineTo(drawDc, clientWidth, statusBarTop - 1);
        SelectObject(drawDc, previousPen);
        DeleteObject(debugBorderPen);

        const auto debugLines = makeDebugMonitorLines(*state);
        const int splitX = clientWidth > 900 ? (clientWidth * 48) / 100 : clientWidth / 2;
        const int leftColumnRight = splitX - 18;
        const RECT debugInfoBodyRect = debugInfoRectForClient(state, clientWidth, clientHeight);
        const int visibleInfoLineCount = visibleDebugLogLineCount(debugInfoBodyRect);
        clampDebugInfoScroll(*state, visibleInfoLineCount);
        const int totalInfoLines = static_cast<int>(debugLines.size()) > 1
            ? static_cast<int>(debugLines.size()) - 1
            : 0;
        const int maxInfoScrollOffset = maxDebugInfoScrollOffset(*state, visibleInfoLineCount);
        const int firstInfoIndex = 1 + state->debugInfoScrollOffset;
        const int lastInfoIndex = firstInfoIndex + visibleInfoLineCount < static_cast<int>(debugLines.size())
            ? firstInfoIndex + visibleInfoLineCount
            : static_cast<int>(debugLines.size());
        const RECT messagesBodyRect = debugMessagesRectForClient(state, clientWidth, clientHeight);
        const int visibleMessageLineCount = visibleDebugLogLineCount(messagesBodyRect);
        clampDebugLogScroll(*state, visibleMessageLineCount);
        const int totalMessageLines = static_cast<int>(state->debugLogLines.size());
        const int maxScrollOffset = maxDebugLogScrollOffset(*state, visibleMessageLineCount);
        const int lastMessageIndex = totalMessageLines - state->debugLogScrollOffset;
        const int firstMessageIndex = lastMessageIndex > visibleMessageLineCount
            ? lastMessageIndex - visibleMessageLineCount
            : 0;

        SelectObject(drawDc, statusFont ? statusFont : bodyFont);
        SetTextColor(drawDc, RGB(226, 230, 236));
        RECT debugTitleRect{kContentMargin, debugMonitorTop + kDebugPanelPaddingTop, leftColumnRight, debugMonitorTop + kDebugPanelPaddingTop + 22};
        DrawTextW(drawDc, L"Debug Monitor", -1, &debugTitleRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

        std::wstring messagesTitle = L"Messages";
        if (totalMessageLines > 0) {
            messagesTitle += L"  ";
            messagesTitle += std::to_wstring(firstMessageIndex + 1);
            messagesTitle += L"-";
            messagesTitle += std::to_wstring(lastMessageIndex);
            messagesTitle += L" / ";
            messagesTitle += std::to_wstring(totalMessageLines);
        }
        RECT messagesTitleRect{
            messagesBodyRect.left,
            debugMonitorTop + kDebugPanelPaddingTop,
            clientWidth - kContentMargin,
            debugMonitorTop + kDebugPanelPaddingTop + 22
        };
        DrawTextW(drawDc, messagesTitle.c_str(), static_cast<int>(messagesTitle.size()), &messagesTitleRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

        SelectObject(drawDc, debugFont ? debugFont : (statusFont ? statusFont : bodyFont));
        SetTextColor(drawDc, RGB(176, 185, 198));
        int leftY = debugInfoBodyRect.top;
        const int infoTextRight = maxInfoScrollOffset > 0
            ? debugInfoBodyRect.right - 12
            : debugInfoBodyRect.right;
        for (int i = firstInfoIndex; i < lastInfoIndex; ++i) {
            if (i < 1 || i >= static_cast<int>(debugLines.size())) {
                continue;
            }
            if (leftY + kDebugPanelLineHeight > debugInfoBodyRect.bottom) {
                break;
            }
            RECT lineRect{debugInfoBodyRect.left, leftY, infoTextRight, leftY + kDebugPanelLineHeight};
            DrawTextW(drawDc, debugLines[i].c_str(), static_cast<int>(debugLines[i].size()), &lineRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
            leftY += kDebugPanelLineHeight;
        }

        if (maxInfoScrollOffset > 0 && totalInfoLines > 0) {
            RECT scrollbarTrack{
                debugInfoBodyRect.right - 8,
                debugInfoBodyRect.top,
                debugInfoBodyRect.right,
                debugInfoBodyRect.bottom
            };
            HBRUSH trackBrush = CreateSolidBrush(RGB(34, 38, 47));
            FillRect(drawDc, &scrollbarTrack, trackBrush);
            DeleteObject(trackBrush);

            const int trackHeight = scrollbarTrack.bottom - scrollbarTrack.top;
            int thumbHeight = (trackHeight * visibleInfoLineCount) / totalInfoLines;
            if (thumbHeight < 18) {
                thumbHeight = 18;
            }
            if (thumbHeight > trackHeight) {
                thumbHeight = trackHeight;
            }

            const int travel = trackHeight - thumbHeight;
            const int thumbTop = scrollbarTrack.top +
                (maxInfoScrollOffset > 0 ? (state->debugInfoScrollOffset * travel) / maxInfoScrollOffset : 0);
            RECT thumbRect{scrollbarTrack.left, thumbTop, scrollbarTrack.right, thumbTop + thumbHeight};
            HBRUSH thumbBrush = CreateSolidBrush(RGB(88, 101, 123));
            FillRect(drawDc, &thumbRect, thumbBrush);
            DeleteObject(thumbBrush);
        }

        int rightY = messagesBodyRect.top;
        const int messageTextRight = maxScrollOffset > 0
            ? messagesBodyRect.right - 12
            : messagesBodyRect.right;
        if (totalMessageLines == 0) {
            const std::wstring emptyMessage = L"No debug messages yet.";
            RECT lineRect{messagesBodyRect.left, rightY, messageTextRight, rightY + kDebugPanelLineHeight};
            DrawTextW(drawDc, emptyMessage.c_str(), static_cast<int>(emptyMessage.size()), &lineRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        } else {
            for (int i = firstMessageIndex; i < lastMessageIndex; ++i) {
                if (rightY + kDebugPanelLineHeight > messagesBodyRect.bottom) {
                    break;
                }
                RECT lineRect{messagesBodyRect.left, rightY, messageTextRight, rightY + kDebugPanelLineHeight};
                DrawTextW(drawDc, state->debugLogLines[static_cast<std::size_t>(i)].c_str(), static_cast<int>(state->debugLogLines[static_cast<std::size_t>(i)].size()), &lineRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
                rightY += kDebugPanelLineHeight;
            }
        }

        if (maxScrollOffset > 0) {
            RECT scrollbarTrack{messagesBodyRect.right - 8, messagesBodyRect.top, messagesBodyRect.right, messagesBodyRect.bottom};
            HBRUSH trackBrush = CreateSolidBrush(RGB(34, 38, 47));
            FillRect(drawDc, &scrollbarTrack, trackBrush);
            DeleteObject(trackBrush);

            const int trackHeight = scrollbarTrack.bottom - scrollbarTrack.top;
            int thumbHeight = (trackHeight * visibleMessageLineCount) / totalMessageLines;
            if (thumbHeight < 18) {
                thumbHeight = 18;
            }
            if (thumbHeight > trackHeight) {
                thumbHeight = trackHeight;
            }

            const int travel = trackHeight - thumbHeight;
            const int thumbTop = scrollbarTrack.top +
                (maxScrollOffset > 0 ? ((maxScrollOffset - state->debugLogScrollOffset) * travel) / maxScrollOffset : travel);
            RECT thumbRect{scrollbarTrack.left, thumbTop, scrollbarTrack.right, thumbTop + thumbHeight};
            HBRUSH thumbBrush = CreateSolidBrush(RGB(88, 101, 123));
            FillRect(drawDc, &thumbRect, thumbBrush);
            DeleteObject(thumbBrush);
        }
    }

    RECT statusBarRect{0, statusBarTop, clientWidth, clientHeight};
    HBRUSH statusBarBrush = CreateSolidBrush(RGB(18, 20, 24));
    FillRect(drawDc, &statusBarRect, statusBarBrush);
    DeleteObject(statusBarBrush);

    HPEN statusBorderPen = CreatePen(PS_SOLID, 1, RGB(54, 58, 66));
    HGDIOBJ previousPen = SelectObject(drawDc, statusBorderPen);
    MoveToEx(drawDc, 0, statusBarTop, nullptr);
    LineTo(drawDc, clientWidth, statusBarTop);
    SelectObject(drawDc, previousPen);
    DeleteObject(statusBorderPen);

    SelectObject(drawDc, statusFont ? statusFont : bodyFont);
    const std::wstring statusMessage = state
        ? L"Status: " + makeStatusBarMessage(*state)
        : L"Status: Loading...";
    const std::wstring statusMetrics = state ? makeStatusBarMetrics(*state) : std::wstring{};
    const int statusSplitX = clientWidth > 760 ? clientWidth / 2 : (clientWidth * 45) / 100;
    const RECT debugButtonRect = debugButtonRectForClient(clientWidth, clientHeight);
    RECT messageRect{kContentMargin, statusBarTop, statusSplitX - 12, clientHeight};
    RECT metricsRect{statusSplitX, statusBarTop, debugButtonRect.left - 12, clientHeight};

    SetTextColor(drawDc, RGB(226, 230, 236));
    DrawTextW(drawDc, statusMessage.c_str(), static_cast<int>(statusMessage.size()), &messageRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    SetTextColor(drawDc, RGB(166, 176, 190));
    if (metricsRect.right > metricsRect.left) {
        DrawTextW(drawDc, statusMetrics.c_str(), static_cast<int>(statusMetrics.size()), &metricsRect, DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    }

    HBRUSH buttonBrush = CreateSolidBrush(state && state->debugMonitorVisible ? RGB(48, 63, 82) : RGB(30, 34, 42));
    HPEN buttonPen = CreatePen(PS_SOLID, 1, state && state->debugMonitorVisible ? RGB(92, 126, 168) : RGB(67, 74, 88));
    HGDIOBJ previousButtonBrush = SelectObject(drawDc, buttonBrush);
    HGDIOBJ previousButtonPen = SelectObject(drawDc, buttonPen);
    RoundRect(drawDc, debugButtonRect.left, debugButtonRect.top, debugButtonRect.right, debugButtonRect.bottom, 6, 6);
    SelectObject(drawDc, previousButtonBrush);
    SelectObject(drawDc, previousButtonPen);
    DeleteObject(buttonBrush);
    DeleteObject(buttonPen);

    SelectObject(drawDc, statusFont ? statusFont : bodyFont);
    SetTextColor(drawDc, RGB(225, 231, 240));
    RECT debugButtonTextRect = debugButtonRect;
    DrawTextW(drawDc, L"Debug", -1, &debugButtonTextRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

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
    DeleteObject(bodyFont);
    DeleteObject(statusFont);
    DeleteObject(debugFont);
    EndPaint(hwnd, &paint);
}

LRESULT CALLBACK windowProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    switch (message) {
    case WM_MEASUREITEM: {
        auto* measure = reinterpret_cast<MEASUREITEMSTRUCT*>(lparam);
        if (measure && measurePopupMenuItem(hwnd, *measure)) {
            return TRUE;
        }
        break;
    }
    case WM_DRAWITEM: {
        const auto* draw = reinterpret_cast<DRAWITEMSTRUCT*>(lparam);
        if (draw && drawPopupMenuItem(*draw)) {
            return TRUE;
        }
        break;
    }
    case WM_CREATE: {
        auto* state = new AppWindowState();
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));
        state->glWindow = CreateWindowExW(
            0,
            kViewportWindowClassName,
            nullptr,
            WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
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
            if (setupOpenGLForChild(state->glWindow, *state)) {
                appendDebugLog(*state, L"OpenGL viewport initialized");
            } else {
                appendDebugLog(*state, L"OpenGL viewport initialization failed");
            }
        }
        appendDebugLog(*state, L"Application window created");
        loadOriginConfig(*state);
        loadDeviceNameConfig(*state);
        loadViewportSettingsConfig(*state);
        loadRecordSettingsConfig(*state);
        layoutChildWindows(hwnd);
        invalidateWindowLayout(hwnd);
        SetTimer(hwnd, kStatusTimerId, kStatusIntervalMs, nullptr);
        refreshStatus(hwnd);
        refreshPoseAndViewport(hwnd);
        return 0;
    }
    case WM_DESTROY: {
        KillTimer(hwnd, kStatusTimerId);
        auto* state = reinterpret_cast<AppWindowState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (state) {
            if (state->originEditWindow) {
                DestroyWindow(state->originEditWindow);
                state->originEditWindow = nullptr;
                state->originEditOriginalProc = nullptr;
            }
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
                if (state->glOverlayFontBase != 0) {
                    glDeleteLists(state->glOverlayFontBase, 128);
                    state->glOverlayFontBase = 0;
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
        invalidateWindowLayout(hwnd);
        return 0;
    case WM_SETCURSOR: {
        if (LOWORD(lparam) == HTCLIENT) {
            auto* state = reinterpret_cast<AppWindowState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
            if (state) {
                RECT clientRect;
                GetClientRect(hwnd, &clientRect);
                POINT point{};
                GetCursorPos(&point);
                ScreenToClient(hwnd, &point);
                const RECT settingRect = topBarSettingRectForClient(
                    clientRect.right - clientRect.left,
                    clientRect.bottom - clientRect.top
                );
                if (PtInRect(&settingRect, point)) {
                    SetCursor(LoadCursor(nullptr, IDC_HAND));
                    return TRUE;
                }
                const RECT fileRect = topBarFileRectForClient(
                    clientRect.right - clientRect.left,
                    clientRect.bottom - clientRect.top
                );
                if (PtInRect(&fileRect, point)) {
                    SetCursor(LoadCursor(nullptr, IDC_HAND));
                    return TRUE;
                }
                const ViewportControlLayout viewportControls = viewportControlLayoutForClient(
                    state,
                    clientRect.right - clientRect.left,
                    clientRect.bottom - clientRect.top
                );
                if (viewportControls.animationValid &&
                    (PtInRect(&viewportControls.firstFrameButtonRect, point) ||
                     PtInRect(&viewportControls.playPauseButtonRect, point) ||
                     PtInRect(&viewportControls.lastFrameButtonRect, point) ||
                     PtInRect(&viewportControls.timelineRect, point) ||
                     PtInRect(&viewportControls.closeButtonRect, point))) {
                    SetCursor(LoadCursor(nullptr, IDC_HAND));
                    return TRUE;
                }
                const OriginStepperButton originButton = originStepperButtonFromPoint(
                    state,
                    clientRect.right - clientRect.left,
                    clientRect.bottom - clientRect.top,
                    point
                );
                if (originButton.valid) {
                    SetCursor(LoadCursor(nullptr, IDC_HAND));
                    return TRUE;
                }
                const RECT deviceButtonRect = deviceToggleButtonRectForClient(
                    state,
                    clientRect.right - clientRect.left,
                    clientRect.bottom - clientRect.top
                );
                if (PtInRect(&deviceButtonRect, point)) {
                    SetCursor(LoadCursor(nullptr, IDC_HAND));
                    return TRUE;
                }
                const RECT debugResizeRect = debugResizeRectForClient(
                    state,
                    clientRect.right - clientRect.left,
                    clientRect.bottom - clientRect.top
                );
                if (state->debugResizeDragging || PtInRect(&debugResizeRect, point)) {
                    SetCursor(LoadCursor(nullptr, IDC_SIZENS));
                    return TRUE;
                }
                const RECT splitterRect = splitterRectForClient(
                    state,
                    clientRect.right - clientRect.left,
                    clientRect.bottom - clientRect.top
                );
                if (state->devicePanelVisible && (state->splitterDragging || PtInRect(&splitterRect, point))) {
                    SetCursor(LoadCursor(nullptr, IDC_SIZEWE));
                    return TRUE;
                }
            }
        }
        break;
    }
    case WM_MOUSEWHEEL: {
        auto* state = reinterpret_cast<AppWindowState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (!state) {
            break;
        }

        RECT clientRect;
        GetClientRect(hwnd, &clientRect);
        const int clientWidth = clientRect.right - clientRect.left;
        const int clientHeight = clientRect.bottom - clientRect.top;
        POINT point{GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
        ScreenToClient(hwnd, &point);
        const int wheelDelta = GET_WHEEL_DELTA_WPARAM(wparam);
        int wheelSteps = wheelDelta / WHEEL_DELTA;
        if (wheelSteps == 0) {
            wheelSteps = wheelDelta > 0 ? 1 : -1;
        }

        const DeviceListLayout deviceListLayout = deviceListLayoutForClient(
            state,
            clientWidth,
            clientHeight
        );
        if (deviceListLayout.valid && PtInRect(&deviceListLayout.boxRect, point)) {
            state->deviceListScrollOffset -= wheelSteps * 3;
            clampDeviceListScroll(*state, deviceListLayout.visibleItemCount);
            InvalidateRect(hwnd, &deviceListLayout.boxRect, FALSE);
            return 0;
        }

        const RECT debugInfoRect = debugInfoRectForClient(state, clientWidth, clientHeight);
        if (state->debugMonitorVisible && PtInRect(&debugInfoRect, point)) {
            const int visibleLineCount = visibleDebugLogLineCount(debugInfoRect);
            state->debugInfoScrollOffset -= wheelSteps * 3;
            clampDebugInfoScroll(*state, visibleLineCount);
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }

        const RECT messagesRect = debugMessagesRectForClient(state, clientWidth, clientHeight);
        if (state->debugMonitorVisible && PtInRect(&messagesRect, point)) {
            const int visibleLineCount = visibleDebugLogLineCount(messagesRect);
            state->debugLogScrollOffset += wheelSteps * 3;
            clampDebugLogScroll(*state, visibleLineCount);
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        break;
    }
    case WM_MOUSEMOVE: {
        auto* state = reinterpret_cast<AppWindowState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (!state) {
            break;
        }

        RECT clientRect;
        GetClientRect(hwnd, &clientRect);
        const int clientWidth = clientRect.right - clientRect.left;
        const int clientHeight = clientRect.bottom - clientRect.top;
        POINT point{GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};

        if (state->importedSceneTimelineDragging) {
            const ViewportControlLayout viewportControls = viewportControlLayoutForClient(
                state,
                clientWidth,
                clientHeight
            );
            seekImportedGlbFromTimeline(hwnd, *state, viewportControls.timelineRect, point);
            SetCursor(LoadCursor(nullptr, IDC_HAND));
            return 0;
        }

        if (state->debugResizeDragging) {
            const int statusBarTop = clientHeight > kStatusBarHeight
                ? clientHeight - kStatusBarHeight
                : 0;
            const int newDebugMonitorHeight = clampDebugMonitorHeightForClient(
                statusBarTop - point.y,
                clientHeight
            );
            if (state->debugMonitorHeight != newDebugMonitorHeight) {
                state->debugMonitorHeight = newDebugMonitorHeight;
                layoutChildWindows(hwnd);
                invalidateWindowLayout(hwnd);
            }
            SetCursor(LoadCursor(nullptr, IDC_SIZENS));
            return 0;
        }

        if (state->splitterDragging) {
            const int newLeftPanelWidth = clampLeftPanelWidthForClient(point.x, clientWidth);
            if (state->leftPanelWidth != newLeftPanelWidth) {
                state->leftPanelWidth = newLeftPanelWidth;
                layoutChildWindows(hwnd);
                invalidateWindowLayout(hwnd);
            }
            SetCursor(LoadCursor(nullptr, IDC_SIZEWE));
            return 0;
        }

        const RECT splitterRect = splitterRectForClient(state, clientWidth, clientHeight);
        if (state->devicePanelVisible && PtInRect(&splitterRect, point)) {
            SetCursor(LoadCursor(nullptr, IDC_SIZEWE));
            return 0;
        }
        const RECT debugResizeRect = debugResizeRectForClient(state, clientWidth, clientHeight);
        if (PtInRect(&debugResizeRect, point)) {
            SetCursor(LoadCursor(nullptr, IDC_SIZENS));
            return 0;
        }
        break;
    }
    case WM_LBUTTONDBLCLK: {
        auto* state = reinterpret_cast<AppWindowState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (!state) {
            break;
        }

        RECT clientRect;
        GetClientRect(hwnd, &clientRect);
        POINT point{GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
        const OriginPanelLayout originLayout = originPanelLayoutForClient(
            state,
            clientRect.right - clientRect.left,
            clientRect.bottom - clientRect.top
        );
        const OriginStepperButton originButton = originStepperButtonFromPoint(
            state,
            clientRect.right - clientRect.left,
            clientRect.bottom - clientRect.top,
            point
        );
        if (originButton.valid) {
            applyOriginStepperButton(hwnd, *state, originButton);
            return 0;
        }
        if (originLayout.valid && PtInRect(&originLayout.boxRect, point)) {
            showOriginEditor(hwnd, *state);
            return 0;
        }
        break;
    }
    case WM_LBUTTONDOWN: {
        auto* state = reinterpret_cast<AppWindowState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (!state) {
            break;
        }

        RECT clientRect;
        GetClientRect(hwnd, &clientRect);
        const RECT debugButtonRect = debugButtonRectForClient(
            clientRect.right - clientRect.left,
            clientRect.bottom - clientRect.top
        );
        POINT point{GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
        const RECT settingRect = topBarSettingRectForClient(
            clientRect.right - clientRect.left,
            clientRect.bottom - clientRect.top
        );
        if (PtInRect(&settingRect, point)) {
            showTopSettingsMenu(hwnd, *state, settingRect);
            return 0;
        }
        const RECT fileRect = topBarFileRectForClient(
            clientRect.right - clientRect.left,
            clientRect.bottom - clientRect.top
        );
        if (PtInRect(&fileRect, point)) {
            showTopFileMenu(hwnd, *state, fileRect);
            return 0;
        }
        const ViewportControlLayout viewportControls = viewportControlLayoutForClient(
            state,
            clientRect.right - clientRect.left,
            clientRect.bottom - clientRect.top
        );
        if (handleImportedAnimationControlClick(hwnd, *state, viewportControls, point)) {
            return 0;
        }
        if (viewportControls.valid && PtInRect(&viewportControls.recordButtonRect, point)) {
            toggleRecording(hwnd);
            InvalidateRect(hwnd, &viewportControls.barRect, FALSE);
            return 0;
        }
        const RECT deviceButtonRect = deviceToggleButtonRectForClient(
            state,
            clientRect.right - clientRect.left,
            clientRect.bottom - clientRect.top
        );
        if (PtInRect(&deviceButtonRect, point)) {
            state->devicePanelVisible = !state->devicePanelVisible;
            state->splitterDragging = false;
            if (!state->devicePanelVisible && state->originEditWindow) {
                closeOriginEditor(hwnd, *state);
            }
            appendDebugLog(
                *state,
                state->devicePanelVisible ? L"Device panel opened" : L"Device panel closed"
            );
            layoutChildWindows(hwnd);
            invalidateWindowLayout(hwnd);
            return 0;
        }
        const OriginStepperButton originButton = originStepperButtonFromPoint(
            state,
            clientRect.right - clientRect.left,
            clientRect.bottom - clientRect.top,
            point
        );
        if (originButton.valid) {
            applyOriginStepperButton(hwnd, *state, originButton);
            return 0;
        }
        const RECT debugResizeRect = debugResizeRectForClient(
            state,
            clientRect.right - clientRect.left,
            clientRect.bottom - clientRect.top
        );
        if (PtInRect(&debugResizeRect, point)) {
            state->debugResizeDragging = true;
            state->debugMonitorHeight = activeDebugMonitorHeight(
                state,
                clientRect.bottom - clientRect.top
            );
            SetCapture(hwnd);
            SetCursor(LoadCursor(nullptr, IDC_SIZENS));
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }

        const RECT splitterRect = splitterRectForClient(
            state,
            clientRect.right - clientRect.left,
            clientRect.bottom - clientRect.top
        );
        if (state->devicePanelVisible && PtInRect(&splitterRect, point)) {
            state->splitterDragging = true;
            state->leftPanelWidth = leftPanelWidthForClient(state, clientRect.right - clientRect.left);
            SetCapture(hwnd);
            SetCursor(LoadCursor(nullptr, IDC_SIZEWE));
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }

        const DeviceListLayout deviceListLayout = deviceListLayoutForClient(
            state,
            clientRect.right - clientRect.left,
            clientRect.bottom - clientRect.top
        );
        const std::uint32_t clickedRuntimeIndex =
            deviceRuntimeIndexFromListPoint(*state, deviceListLayout, point);
        if (clickedRuntimeIndex != kNoSelectedRuntimeIndex) {
            const ovtr::DeviceDescriptor* clickedDevice = deviceForRuntimeIndex(
                state->devices,
                clickedRuntimeIndex
            );
            if (clickedDevice == nullptr) {
                break;
            }
            toggleListDeviceSelection(*state, *clickedDevice);
            if (state->glWindow) {
                renderViewport(state->glWindow);
            }
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }

        if (PtInRect(&debugButtonRect, point)) {
            state->debugMonitorVisible = !state->debugMonitorVisible;
            appendDebugLog(
                *state,
                state->debugMonitorVisible ? L"Debug monitor opened" : L"Debug monitor closed"
            );
            layoutChildWindows(hwnd);
            invalidateWindowLayout(hwnd);
            return 0;
        }
        break;
    }
    case WM_RBUTTONDOWN: {
        auto* state = reinterpret_cast<AppWindowState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (!state) {
            break;
        }

        RECT clientRect;
        GetClientRect(hwnd, &clientRect);
        POINT point{GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
        const DeviceListLayout deviceListLayout = deviceListLayoutForClient(
            state,
            clientRect.right - clientRect.left,
            clientRect.bottom - clientRect.top
        );
        if (!deviceListLayout.valid || !PtInRect(&deviceListLayout.boxRect, point)) {
            break;
        }

        const std::uint32_t clickedRuntimeIndex =
            deviceRuntimeIndexFromListPoint(*state, deviceListLayout, point);
        if (clickedRuntimeIndex != kNoSelectedRuntimeIndex) {
            const ovtr::DeviceDescriptor* clickedDevice = deviceForRuntimeIndex(
                state->devices,
                clickedRuntimeIndex
            );
            if (clickedDevice && state->selectedDeviceRuntimeIndex != clickedDevice->runtimeIndex) {
                state->selectedDeviceRuntimeIndex = clickedDevice->runtimeIndex;
                appendDebugLog(*state, L"Device selected: " + widen(deviceDisplayName(*clickedDevice)));
                if (state->glWindow) {
                    renderViewport(state->glWindow);
                }
                InvalidateRect(hwnd, nullptr, FALSE);
            }
        }

        if (selectedListDevice(*state) == nullptr) {
            break;
        }

        HMENU menu = CreatePopupMenu();
        if (!menu) {
            break;
        }

        std::array<PopupMenuItem, 2> menuItems{{
            PopupMenuItem{kDeviceContextMenuSetNameId, L"Set Name"},
            PopupMenuItem{kDeviceContextMenuSetOriginId, L"Set to Origin"},
        }};
        appendPopupMenuItem(menu, menuItems[0]);
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        appendPopupMenuItem(menu, menuItems[1]);

        POINT screenPoint = point;
        ClientToScreen(hwnd, &screenPoint);
        SetForegroundWindow(hwnd);
        const UINT command = TrackPopupMenu(
            menu,
            TPM_RIGHTBUTTON | TPM_RETURNCMD,
            screenPoint.x,
            screenPoint.y,
            0,
            hwnd,
            nullptr
        );
        DestroyMenu(menu);

        if (command == kDeviceContextMenuSetNameId) {
            const ovtr::DeviceDescriptor* selected = selectedListDevice(*state);
            if (selected) {
                setDeviceCustomName(hwnd, *state, *selected);
            }
            invalidateStatusPanel(hwnd);
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }

        if (command == kDeviceContextMenuSetOriginId) {
            const ovtr::DeviceDescriptor* selected = selectedListDevice(*state);
            if (selected == nullptr) {
                invalidateStatusPanel(hwnd);
                InvalidateRect(hwnd, nullptr, FALSE);
                return 0;
            }

            const ovtr::DeviceDescriptor selectedSnapshot = *selected;
            if (!confirmSetOrigin(hwnd, *state, selectedSnapshot)) {
                invalidateStatusPanel(hwnd);
                InvalidateRect(hwnd, nullptr, FALSE);
                return 0;
            }
            if (state->provider.isInitialized()) {
                state->provider.pollPoses(state->poses);
            }
            if (setOriginFromDevice(*state, selectedSnapshot)) {
                refreshPoseAndViewport(hwnd);
            }
            invalidateStatusPanel(hwnd);
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        return 0;
    }
    case WM_LBUTTONUP: {
        auto* state = reinterpret_cast<AppWindowState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (state && state->importedSceneTimelineDragging) {
            state->importedSceneTimelineDragging = false;
            state->importedSceneLastUpdate = std::chrono::steady_clock::now();
            if (GetCapture() == hwnd) {
                ReleaseCapture();
            }
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        if (state && state->debugResizeDragging) {
            state->debugResizeDragging = false;
            if (GetCapture() == hwnd) {
                ReleaseCapture();
            }
            appendDebugLog(*state, L"Debug panel height: " + std::to_wstring(state->debugMonitorHeight) + L" px");
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        if (state && state->splitterDragging) {
            state->splitterDragging = false;
            if (GetCapture() == hwnd) {
                ReleaseCapture();
            }
            appendDebugLog(*state, L"Left panel width: " + std::to_wstring(state->leftPanelWidth) + L" px");
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        break;
    }
    case WM_CAPTURECHANGED: {
        auto* state = reinterpret_cast<AppWindowState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (state && state->importedSceneTimelineDragging && reinterpret_cast<HWND>(lparam) != hwnd) {
            state->importedSceneTimelineDragging = false;
            state->importedSceneLastUpdate = std::chrono::steady_clock::now();
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        if (state && state->debugResizeDragging && reinterpret_cast<HWND>(lparam) != hwnd) {
            state->debugResizeDragging = false;
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        if (state && state->splitterDragging && reinterpret_cast<HWND>(lparam) != hwnd) {
            state->splitterDragging = false;
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        break;
    }
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
                ensureOriginSelection(*state);
                const ovtr::DeviceDescriptor* selected = selectedOriginDevice(*state);
                if (selected == nullptr) {
                    state->originStatusMessage = "no device selected for origin";
                    appendDebugLog(*state, state->originStatusMessage);
                } else {
                    const ovtr::DeviceDescriptor selectedSnapshot = *selected;
                    if (confirmSetOrigin(hwnd, *state, selectedSnapshot)) {
                        setOriginFromDevice(*state, selectedSnapshot);
                        refreshPoseAndViewport(hwnd);
                    }
                }
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
        if (wparam == VK_SPACE) {
            toggleRecording(hwnd);
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
    windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    windowClass.lpfnWndProc = windowProc;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    windowClass.lpszClassName = kMainWindowClassName;

    RegisterClassW(&windowClass);

    WNDCLASSW viewportClass{};
    viewportClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    viewportClass.lpfnWndProc = viewportProc;
    viewportClass.hInstance = instance;
    viewportClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    viewportClass.lpszClassName = kViewportWindowClassName;

    RegisterClassW(&viewportClass);

    WNDCLASSW nameDialogClass{};
    nameDialogClass.style = CS_HREDRAW | CS_VREDRAW;
    nameDialogClass.lpfnWndProc = deviceNameDialogProc;
    nameDialogClass.hInstance = instance;
    nameDialogClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    nameDialogClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    nameDialogClass.lpszClassName = kDeviceNameDialogClassName;

    RegisterClassW(&nameDialogClass);

    WNDCLASSW originDialogClass{};
    originDialogClass.style = CS_HREDRAW | CS_VREDRAW;
    originDialogClass.lpfnWndProc = originDialogProc;
    originDialogClass.hInstance = instance;
    originDialogClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    originDialogClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    originDialogClass.lpszClassName = kOriginDialogClassName;

    RegisterClassW(&originDialogClass);

    WNDCLASSW exportLocationDialogClass{};
    exportLocationDialogClass.style = CS_HREDRAW | CS_VREDRAW;
    exportLocationDialogClass.lpfnWndProc = exportLocationDialogProc;
    exportLocationDialogClass.hInstance = instance;
    exportLocationDialogClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    exportLocationDialogClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    exportLocationDialogClass.lpszClassName = kExportLocationDialogClassName;

    RegisterClassW(&exportLocationDialogClass);

    WNDCLASSW colorDialogClass{};
    colorDialogClass.style = CS_HREDRAW | CS_VREDRAW;
    colorDialogClass.lpfnWndProc = viewportColorDialogProc;
    colorDialogClass.hInstance = instance;
    colorDialogClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    colorDialogClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    colorDialogClass.lpszClassName = kViewportColorDialogClassName;

    RegisterClassW(&colorDialogClass);

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
