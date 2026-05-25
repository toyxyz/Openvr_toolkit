# SteamVR Tracker Recorder 전문 구현 계획서 v2

문서 버전: 2.0  
작성일: 2026-05-25  
대상 플랫폼: Windows 10/11 우선, Linux 선택 지원  
주요 스택: C++20, OpenVR, Dear ImGui, GLFW, OpenGL, CMake

---

## 1. 문서 목적

본 문서는 Unity, Unreal Engine, Qt 없이 **C++20 + OpenVR + Dear ImGui + GLFW + OpenGL** 기반으로 SteamVR tracked device의 움직임을 실시간으로 표시, 녹화, 재생, 익스포트하는 네이티브 데스크톱 애플리케이션을 구현하기 위한 전문 개발 계획서이다.

기존 계획서의 핵심 방향은 유지하되, 실제 개발 착수에 필요한 다음 항목을 보강한다.

- 빌드 가능한 CMake/의존성 전략
- OpenVR 초기화 정책과 실패 상태 처리
- UI/rendering/tracking/file writing의 실행 모델
- 장시간 녹화를 고려한 session storage 구조
- 샘플링 타이밍과 frame drop 정책
- 좌표계 변환과 exporter 검증 전략
- MVP와 후속 기능의 경계 재정의

본 계획서의 목표는 "아이디어 문서"가 아니라, 개발자가 Phase 0부터 구현을 시작할 수 있는 **실행 가능한 설계 기준서**를 제공하는 것이다.

---

## 2. 제품 정의

### 2.1 제품 개념

SteamVR runtime에 연결된 HMD, Controller, Vive Tracker, Generic Tracker의 pose 데이터를 OpenVR API로 수집하고, 이를 데스크톱 3D viewport에 실시간 표시하며, 사용자가 원하는 장치를 선택해 녹화하고, CSV/JSON/Blender/BVH 등 외부 도구에서 사용할 수 있는 형태로 내보내는 독립형 모션 트래킹 레코더를 개발한다.

### 2.2 주요 사용자

- VR 모션캡처 실험자
- Vive Tracker 기반 full-body tracking 사용자
- DCC 툴로 tracker 데이터를 가져가려는 애니메이션/테크니컬 아티스트
- 연구/분석용 pose logging이 필요한 개발자
- SteamVR 장치 상태를 시각적으로 진단하려는 도구 개발자

### 2.3 제품 목표

1. SteamVR 장치의 pose를 안정적으로 수집한다.
2. 장치 index가 바뀌어도 serial number 기반으로 장치를 식별한다.
3. raw OpenVR pose를 손상 없이 저장한다.
4. UI 부하가 녹화 타이밍을 흔들지 않도록 sampling과 rendering을 분리한다.
5. 장시간 녹화가 가능한 session storage를 MVP 단계부터 도입한다.
6. CSV/JSON은 분석과 앱 재로드용으로, Blender/BVH는 DCC 연동용으로 단계적으로 확장한다.

### 2.4 비목표

초기 버전에서는 다음 기능을 제외한다.

- 완전한 full-body IK solver
- 고급 retargeting 및 자동 skeleton solving
- SteamVR driver 개발
- SteamVR runtime 없이 tracker 직접 제어
- FBX exporter 완전 구현
- 멀티유저 네트워크 세션
- 클라우드 동기화
- 상용 모션캡처 패키지 수준의 자동 보정

---

## 3. 핵심 설계 원칙

### 3.1 Raw First

내부 저장 데이터는 OpenVR이 제공하는 raw pose를 기준으로 한다. 화면 표시, calibration, Blender/BVH 변환은 모두 별도 변환 계층에서 처리한다.

### 3.2 Serial First

OpenVR device index는 runtime 세션 안에서만 유효한 임시 식별자이다. 영구 식별자는 `Prop_SerialNumber_String`으로 얻은 serial number를 사용한다.

### 3.3 Decoupled Real-Time Path

tracking sampling, UI rendering, file writing은 서로 직접 의존하지 않는다. 각 계층은 snapshot, queue, cache를 통해 느슨하게 연결한다.

### 3.4 Storage Before Fancy UI

장시간 녹화 안정성이 핵심 가치이므로, binary session writer와 session manifest는 MVP 단계부터 설계한다. UI 기능은 이 storage 모델 위에 얹는다.

### 3.5 Testable Conversion

좌표계 변환, quaternion 추출, exporter 변환은 반드시 단위 테스트와 known-pose fixture로 검증한다. "눈으로 보기에는 맞는 것 같다"를 완료 기준으로 삼지 않는다.

---

## 4. 기술 스택 결정

### 4.1 기본 스택

```text
Language: C++20
VR API: OpenVR SDK
UI: Dear ImGui Docking branch
Window/Input: GLFW
Rendering: OpenGL 3.3 Core Profile 이상
OpenGL Loader: glad 또는 gl3w
Math: glm
Serialization: nlohmann/json
Build: CMake 3.24+
Package Manager: vcpkg 우선, 일부 source/vendor 허용
Test: Catch2 또는 GoogleTest
Logging: spdlog 권장
```

### 4.2 추가 의존성 권장

기존 계획서에는 OpenGL function loader가 빠져 있다. Windows에서 OpenGL 3.3+ 함수를 안정적으로 사용하려면 GLFW만으로는 부족하므로 `glad`, `gl3w`, `glew` 중 하나가 필요하다. 본 계획서는 `glad`를 기본 권장한다.

```text
vcpkg 권장 패키지:
- glfw3
- glm
- nlohmann-json
- glad
- spdlog
- catch2 또는 gtest

source/vendor 권장:
- openvr
- dear-imgui docking branch
```

### 4.3 OpenVR application type 정책

MVP에서는 `VRApplication_Background`를 기본으로 사용한다.

중요한 전제:

- background app은 SteamVR을 자동으로 시작하지 않는다.
- SteamVR이 실행 중이 아니면 `VR_Init`이 실패할 수 있다.
- 앱은 이 실패를 정상 상태로 처리하고, UI에서 "SteamVR 실행 필요" 상태를 보여줘야 한다.
- pose 수집이 목적이므로 `VRApplication_Utility`를 pose capture 용도로 사용하지 않는다.

초기 UX 정책:

```text
1. 앱 실행
2. VR_IsRuntimeInstalled()로 OpenVR runtime 설치 여부 확인
3. VR_Init(..., VRApplication_Background) 시도
4. 실패 시 error symbol과 recovery action 표시
5. 사용자가 SteamVR을 실행한 뒤 Reconnect 버튼으로 재시도
```

---

## 5. 시스템 아키텍처

### 5.1 계층 구조

```text
Desktop Application
 ├─ App Layer
 │   ├─ Application
 │   ├─ AppState
 │   ├─ AppConfig
 │   └─ CommandBus
 │
 ├─ VR Layer
 │   ├─ IVRProvider
 │   ├─ OpenVRProvider
 │   ├─ DeviceRegistry
 │   ├─ PoseSampler
 │   └─ VREventPump
 │
 ├─ Recording Layer
 │   ├─ RecordingController
 │   ├─ SamplingScheduler
 │   ├─ SampleQueue
 │   ├─ BinarySessionWriter
 │   ├─ BinarySessionReader
 │   └─ PreviewCache
 │
 ├─ Playback Layer
 │   ├─ PlaybackController
 │   ├─ TimelineIndex
 │   └─ PoseInterpolator
 │
 ├─ Rendering Layer
 │   ├─ GLContext
 │   ├─ GLRenderer
 │   ├─ Framebuffer
 │   ├─ Camera
 │   ├─ GridRenderer
 │   ├─ DeviceRenderer
 │   ├─ TrailRenderer
 │   └─ DebugOverlayRenderer
 │
 ├─ UI Layer
 │   ├─ ImGuiLayer
 │   ├─ DockspaceUI
 │   ├─ DevicePanel
 │   ├─ RecorderPanel
 │   ├─ TimelinePanel
 │   ├─ ViewportPanel
 │   ├─ ExportPanel
 │   ├─ CalibrationPanel
 │   └─ LogPanel
 │
 ├─ Export Layer
 │   ├─ ExportManager
 │   ├─ CsvExporter
 │   ├─ JsonExporter
 │   ├─ BlenderExporter
 │   └─ BvhExporter
 │
 ├─ Math Layer
 │   ├─ Transform
 │   ├─ CoordinateConverter
 │   ├─ QuaternionUtils
 │   └─ KnownPoseFixtures
 │
 └─ Platform/Util
     ├─ FileSystem
     ├─ Logger
     ├─ Timer
     ├─ Threading
     └─ LicenseNotice
```

### 5.2 실행 모델

MVP는 복잡도를 낮추기 위해 2-thread 모델로 시작한다.

```text
Main/UI/Render Thread
 ├─ GLFW event polling
 ├─ ImGui frame
 ├─ OpenGL viewport rendering
 ├─ UI command dispatch
 └─ latest pose snapshot 표시

Tracking/Recording Thread
 ├─ OpenVR pose polling
 ├─ fixed-rate sampling
 ├─ device registry refresh
 ├─ sample queue append
 └─ binary writer flush
```

장기적으로 file writing을 별도 writer thread로 분리할 수 있다.

```text
Phase 0-2: single thread 허용
Phase 3 MVP: tracking/recording thread 도입
Phase 6 이후: writer thread 분리 검토
```

### 5.3 데이터 흐름

```text
OpenVR Runtime
   ↓
OpenVRProvider::poll()
   ↓
PoseSampler
   ├─ LatestPoseSnapshot ──────→ UI/Renderer
   └─ FrameSample ─────────────→ RecordingController
                                  ↓
                             BinarySessionWriter
                                  ↓
                              session folder
                                  ↓
                       Playback / Export / Preview Cache
```

---

## 6. 프로젝트 디렉터리 구조

```text
OpenVRTrackerRecorder/
 ├─ CMakeLists.txt
 ├─ CMakePresets.json
 ├─ vcpkg.json
 ├─ README.md
 ├─ LICENSE
 ├─ docs/
 │   ├─ architecture.md
 │   ├─ file_format.md
 │   ├─ export_spec.md
 │   ├─ coordinate_system.md
 │   └─ license_notices.md
 │
 ├─ third_party/
 │   ├─ openvr/
 │   └─ imgui/
 │
 ├─ assets/
 │   ├─ fonts/
 │   ├─ shaders/
 │   │   ├─ grid.vert
 │   │   ├─ grid.frag
 │   │   ├─ device.vert
 │   │   ├─ device.frag
 │   │   ├─ line.vert
 │   │   └─ line.frag
 │   └─ icons/
 │
 ├─ src/
 │   ├─ main.cpp
 │   ├─ app/
 │   ├─ vr/
 │   ├─ data/
 │   ├─ recording/
 │   ├─ playback/
 │   ├─ render/
 │   ├─ ui/
 │   ├─ export/
 │   ├─ math/
 │   └─ util/
 │
 ├─ tests/
 │   ├─ unit/
 │   ├─ integration/
 │   └─ fixtures/
 │
 ├─ tools/
 │   ├─ inspect_session/
 │   └─ generate_fixture/
 │
 └─ packaging/
     ├─ windows/
     └─ licenses/
```

---

## 7. 핵심 데이터 모델

### 7.1 DeviceInfo

```cpp
struct DeviceInfo
{
    uint32_t runtimeIndex = 0;
    std::string serial;
    std::string modelName;
    std::string renderModelName;
    std::string manufacturerName;
    std::string displayName;

    vr::ETrackedDeviceClass deviceClass = vr::TrackedDeviceClass_Invalid;
    std::string assignedRole;

    bool connected = false;
    bool poseValid = false;
    bool recordEnabled = true;
    bool visibleInViewport = true;
};
```

### 7.2 DeviceId

파일 포맷에서는 문자열 serial을 매 pose마다 반복하지 않는다. session 안에서만 유효한 compact device id를 부여한다.

```cpp
using DeviceId = uint32_t;

struct DeviceDescriptor
{
    DeviceId id = 0;
    std::string serial;
    std::string role;
    std::string deviceClass;
    std::string modelName;
};
```

### 7.3 PoseSample

```cpp
struct PoseSample
{
    DeviceId deviceId = 0;
    uint32_t runtimeIndex = 0;

    float position[3] = {0.0f, 0.0f, 0.0f};
    float rotation[4] = {0.0f, 0.0f, 0.0f, 1.0f}; // x, y, z, w
    float velocity[3] = {0.0f, 0.0f, 0.0f};
    float angularVelocity[3] = {0.0f, 0.0f, 0.0f};

    uint8_t flags = 0;
};
```

### 7.4 FrameSample

```cpp
struct FrameSample
{
    uint64_t frameIndex = 0;
    uint64_t timestampNs = 0;
    double timeSeconds = 0.0;
    std::vector<PoseSample> poses;
};
```

### 7.5 RecordingSession

`RecordingSession`은 전체 frame을 보관하지 않는다. metadata와 file reference 중심으로 둔다.

```cpp
struct RecordingSession
{
    std::string sessionId;
    std::string sessionName;
    std::string createdAtUtc;
    std::string appVersion;

    double targetSampleRate = 90.0;
    std::string trackingUniverse = "Standing";
    std::string coordinateSystem = "OpenVR";
    std::string unit = "meter";

    std::vector<DeviceDescriptor> devices;
    std::filesystem::path framesPath;
    std::filesystem::path frameIndexPath;
};
```

---

## 8. OpenVR 연동 설계

### 8.1 IVRProvider 인터페이스

테스트 가능성을 위해 OpenVR 직접 의존을 추상화한다.

```cpp
class IVRProvider
{
public:
    virtual ~IVRProvider() = default;

    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    virtual bool isInitialized() const = 0;

    virtual bool pollEvents(std::vector<VREvent>& outEvents) = 0;
    virtual bool pollPoses(PosePollResult& outResult) = 0;
    virtual std::vector<DeviceInfo> enumerateDevices() = 0;
};
```

테스트에서는 `MockVRProvider`를 사용해 hardware 없이 pose conversion, recording, playback, exporter를 검증한다.

### 8.2 OpenVRProvider 책임

- `VR_IsRuntimeInstalled()` 확인
- `VR_Init()` 및 `VR_Shutdown()` 관리
- `IVRSystem` pointer lifetime 관리
- `GetDeviceToAbsoluteTrackingPose()` 호출
- `PollNextEvent()` 기반 runtime event 수집
- device property string 조회
- OpenVR error symbol/name logging

### 8.3 초기화 실패 처리

초기화 실패는 fatal crash가 아니다. 앱은 다음 상태를 UI에 표시한다.

```text
RuntimeNotInstalled
SteamVRNotRunning
HmdNotPresent
InitFailed
Connected
ConnectionLost
```

로그에는 최소한 다음 정보를 남긴다.

```text
- EVRInitError enum value
- VR_GetVRInitErrorAsSymbol()
- VR_GetVRInitErrorAsEnglishDescription()
- runtime installed 여부
- hmd present 여부
```

### 8.4 Pose polling

녹화용 pose는 현재 pose 기준으로 수집한다.

```cpp
system->GetDeviceToAbsoluteTrackingPose(
    vr::TrackingUniverseStanding,
    0.0f,
    poses.data(),
    static_cast<uint32_t>(poses.size())
);
```

주의:

- render prediction과 recording sample은 분리한다.
- `TrackingUniverseStanding`과 `TrackingUniverseSeated` 선택을 session metadata에 저장한다.
- recording 중 tracking universe 변경은 새 session으로 취급하는 것이 안전하다.

### 8.5 Device registry 갱신

다음 경우 registry를 갱신한다.

- 앱 초기화 직후
- `VREvent_TrackedDeviceActivated`
- `VREvent_TrackedDeviceDeactivated`
- `VREvent_TrackedDeviceUpdated`
- 주기적 fallback refresh, 예: 1초마다

registry 갱신 결과는 serial 기준으로 merge한다.

---

## 9. Pose 변환과 좌표계

### 9.1 OpenVR 좌표계

내부 raw 저장 기준:

```text
Unit: meter
Up: +Y
Forward: 일반적으로 -Z 기준으로 해석
Handedness: OpenVR/SteamVR convention을 문서화하고 테스트로 고정
```

정확한 방향은 known-pose fixture와 실제 tracker 회전 테스트로 검증한다.

### 9.2 Matrix to transform

OpenVR의 `HmdMatrix34_t`에서 translation은 다음과 같이 추출한다.

```cpp
x = m.m[0][3];
y = m.m[1][3];
z = m.m[2][3];
```

rotation 추출은 row/column 해석 오류가 잦으므로 `CoordinateConverter`에 단일 함수로 캡슐화한다.

```cpp
Transform openVrMatrixToTransform(const vr::HmdMatrix34_t& matrix);
```

필수 테스트:

```text
- identity matrix
- +X translation
- +Y translation
- +Z translation
- X axis 90도 회전
- Y axis 90도 회전
- Z axis 90도 회전
- quaternion normalize 검증
```

### 9.3 변환 계층

```text
Raw Pose
 ├─ Render Pose
 │   └─ OpenGL viewport 표시용 변환
 ├─ Export Pose
 │   ├─ CSV/JSON: raw 유지 또는 옵션 변환
 │   ├─ Blender: Z-up 옵션
 │   └─ BVH: skeleton local transform
 └─ Calibrated Pose
     └─ world/device offset 적용
```

원칙:

- raw pose는 절대 덮어쓰지 않는다.
- calibration은 재생/표시/export 시점에 적용한다.
- export 파일에는 적용한 transform policy를 metadata로 기록한다.

---

## 10. 녹화 시스템 설계

### 10.1 녹화 모드

MVP 기본값은 Fixed FPS Mode이다.

```text
Supported rates: 30, 60, 90, 120 FPS
Default: 90 FPS
Clock: std::chrono::steady_clock
Timestamp: recording start 기준 nanoseconds
```

Raw Polling Mode는 추후 advanced mode로 제공한다.

### 10.2 SamplingScheduler

```cpp
class SamplingScheduler
{
public:
    explicit SamplingScheduler(double targetFps);

    bool shouldSample(std::chrono::steady_clock::time_point now);
    void markSampled(std::chrono::steady_clock::time_point now);
    uint64_t frameIndex() const;
    double targetFps() const;
};
```

### 10.3 Overrun 정책

tracking thread가 목표 frame interval을 놓친 경우 정책을 명확히 한다.

MVP 권장:

```text
Small delay: 다음 tick에서 하나만 sample
Large delay: frameIndex는 연속 증가시키되 droppedFrameCount 기록
No synthetic pose: 누락된 frame을 임의 보간해 raw recording에 쓰지 않음
Playback interpolation: 재생 단계에서만 선택적으로 수행
```

### 10.4 Recorder 상태

```cpp
enum class RecorderState
{
    Idle,
    Starting,
    Recording,
    Paused,
    Stopping,
    Finalizing,
    Error
};
```

### 10.5 RecordingController 책임

- session folder 생성
- device snapshot 고정
- binary writer open/close
- pause/resume time accounting
- dropped frame count 계산
- preview cache 업데이트
- stop 시 manifest finalize
- 비정상 종료 recovery marker 관리

---

## 11. Session Storage 설계

### 11.1 Session folder 구조

```text
session_2026-05-25_13-30-00/
 ├─ manifest.json
 ├─ devices.json
 ├─ roles.json
 ├─ calibration.json
 ├─ frames.bin
 ├─ frame_index.bin
 ├─ preview_cache.bin
 └─ logs.txt
```

### 11.2 manifest.json

```json
{
  "format": "OpenVRTrackerRecorderSession",
  "formatVersion": 1,
  "appVersion": "0.1.0",
  "createdAtUtc": "2026-05-25T04:30:00Z",
  "sampleRate": 90.0,
  "trackingUniverse": "Standing",
  "coordinateSystem": {
    "name": "OpenVR",
    "unit": "meter",
    "upAxis": "Y",
    "forwardAxis": "-Z"
  },
  "files": {
    "devices": "devices.json",
    "frames": "frames.bin",
    "frameIndex": "frame_index.bin",
    "roles": "roles.json",
    "calibration": "calibration.json"
  },
  "stats": {
    "frameCount": 0,
    "durationSeconds": 0.0,
    "droppedFrames": 0
  },
  "finalized": false
}
```

### 11.3 devices.json

```json
{
  "devices": [
    {
      "id": 1,
      "serial": "LHR-ABC123",
      "class": "GenericTracker",
      "modelName": "Vive Tracker",
      "manufacturer": "HTC",
      "role": "waist",
      "recordEnabled": true
    }
  ]
}
```

### 11.4 frames.bin header

Binary format은 초기에 versioning을 포함한다.

```text
FileHeader
- char magic[8] = "OVTRBIN\0"
- uint32 formatVersion = 1
- uint32 endian = 0x01020304
- uint32 headerSize
- uint32 reserved

FrameRecord
- uint64 frameIndex
- uint64 timestampNs
- double timeSeconds
- uint32 poseCount
- uint32 flags
- PoseRecord[poseCount]

PoseRecord
- uint32 deviceId
- uint32 runtimeIndex
- float position[3]
- float rotation[4]
- float velocity[3]
- float angularVelocity[3]
- uint32 flags
```

Pose flags:

```text
bit 0: device connected
bit 1: pose valid
bit 2: record enabled
bit 3: calibrated preview available
```

### 11.5 frame_index.bin

seek 성능을 위해 frame offset index를 별도로 유지한다.

```text
FrameIndexHeader
- char magic[8] = "OVTRIDX\0"
- uint32 formatVersion
- uint32 entrySize

FrameIndexEntry
- uint64 frameIndex
- uint64 timestampNs
- uint64 byteOffset
```

### 11.6 Recovery 전략

녹화 시작 시 `manifest.json`의 `finalized`를 `false`로 저장한다. 정상 종료 시 frame count, duration, checksum/statistics를 갱신하고 `finalized`를 `true`로 바꾼다.

앱 시작 시 `finalized=false`인 session을 발견하면 recovery dialog를 보여준다.

---

## 12. Playback 설계

### 12.1 PlaybackController 책임

- session manifest load
- frame index load
- frame seek
- playback clock 관리
- speed multiplier 적용
- loop/in-out range 처리
- interpolation 옵션 처리
- renderer에 playback snapshot 제공

### 12.2 Playback 상태

```cpp
enum class PlaybackState
{
    Empty,
    Stopped,
    Playing,
    Paused,
    Seeking,
    Error
};
```

### 12.3 Interpolation 정책

MVP에서는 frame nearest display를 기본으로 한다. 추후 옵션으로 interpolation을 제공한다.

```text
Position: linear interpolation
Rotation: quaternion slerp
Invalid pose: 이전 valid pose hold 또는 skip 옵션
```

### 12.4 Timeline 기능

- play / pause
- previous frame / next frame
- current frame slider
- timecode 표시
- playback speed 0.25x, 0.5x, 1x, 2x, 4x
- loop toggle
- in/out range
- selected device focus

---

## 13. Rendering 설계

### 13.1 OpenGL context

```text
OpenGL profile: Core
Minimum version: 3.3
Depth test: enabled for scene
MSAA: optional
Framebuffer: viewport panel size에 맞춰 resize
```

### 13.2 ImGui viewport 통합

3D scene은 offscreen framebuffer에 렌더링하고 ImGui panel에서 texture로 표시한다.

```text
Framebuffer
   ↓
GLRenderer::renderScene()
   ↓
color texture
   ↓
ImGui::Image()
   ↓
ViewportPanel
```

### 13.3 렌더링 요소

MVP:

- world grid
- origin marker
- XYZ axis
- device proxy cube/sphere
- orientation axis
- selected device highlight
- simple trail

후속:

- device render model
- label/text overlay
- calibrated skeleton preview
- chaperone boundary

### 13.4 카메라 조작

```text
Left drag: orbit
Middle drag: pan
Mouse wheel: zoom
F: focus selected device
Home: reset camera
Shift + drag: slower movement
```

### 13.5 Render performance budget

기본 목표:

```text
UI/render: 60 FPS 이상
recording: 90 FPS 목표 유지
tracker count: 8 devices 기준 안정 동작
trail points: device당 제한된 ring buffer 사용
```

---

## 14. Dear ImGui UI 설계

### 14.1 전체 레이아웃

```text
┌─────────────────────────────────────────────────────────────┐
│ Menu Bar                                                    │
├─────────────────┬───────────────────────────┬───────────────┤
│ Device Panel    │                           │ Inspector     │
│ Role Mapping    │       3D Viewport         │ Export        │
│ SteamVR Status  │                           │ Calibration   │
├─────────────────┴───────────────────────────┴───────────────┤
│ Recorder Controls / Timeline                                │
├─────────────────────────────────────────────────────────────┤
│ Log Console                                                 │
└─────────────────────────────────────────────────────────────┘
```

### 14.2 Menu

```text
File
 - New Session
 - Open Session
 - Save Session Metadata
 - Export
 - Reveal Session Folder
 - Exit

View
 - Reset Layout
 - Device Panel
 - Timeline
 - Export Panel
 - Log Console

Recording
 - Start
 - Pause
 - Resume
 - Stop

Tools
 - Reconnect SteamVR
 - Role Profile Editor
 - Calibration
 - Coordinate Test

Help
 - About
 - License Notices
 - Diagnostics
```

### 14.3 Device Panel

필수 열:

```text
Record
Visible
Index
Serial
Class
Role
Model
Connected
Pose Valid
Position
Rotation
```

기능:

- record enabled toggle
- viewport visible toggle
- role combo box
- selected device highlight
- serial copy
- disconnected device 유지 표시

### 14.4 Recorder Panel

표시 항목:

- state
- target sample rate
- actual sample rate estimate
- duration
- frame count
- dropped frame count
- active device count
- current session folder
- disk write status

### 14.5 Export Panel

옵션:

- format: CSV, JSON, Blender Python, BVH
- frame range
- selected devices
- include invalid poses
- coordinate system
- apply calibration
- resample FPS
- output path

### 14.6 Diagnostics Panel

MVP 이후라도 빠르게 추가할 가치가 높은 패널이다.

- OpenVR init status
- runtime installed
- HMD present
- tracking universe
- connected device count
- last VR event
- last write error
- current memory usage estimate
- queue length

---

## 15. Export 설계

### 15.1 ExportManager

```cpp
class ExportManager
{
public:
    ExportResult exportSession(
        const RecordingSession& session,
        const ExportOptions& options
    );
};
```

### 15.2 CSV Export

CSV는 분석 친화적이고 streaming export가 쉬우므로 MVP 필수 포맷이다.

권장 schema:

```csv
time,frame,timestamp_ns,device_id,serial,role,class,px,py,pz,qx,qy,qz,qw,vx,vy,vz,avx,avy,avz,connected,valid
```

정책:

- 기본 좌표계는 OpenVR raw
- calibration 적용 여부를 export header comment 또는 sidecar JSON에 기록
- invalid pose 포함 여부 선택 가능

### 15.3 JSON Export

JSON은 앱 외부 연동과 소규모 session 확인에 유리하다. 장시간 녹화 전체를 단일 JSON으로 쓰면 파일이 비대해지므로 두 모드를 제공한다.

```text
Compact JSON: 짧은 session / interchange
JSON Lines: 긴 session / streaming-friendly
```

### 15.4 Blender Python Export

목표:

- 각 tracker를 Empty object로 생성
- frame별 location/rotation quaternion keyframe 삽입
- Blender Z-up 변환 옵션 제공
- object name은 role + serial suffix로 안정적으로 생성

결과물:

```text
export_folder/
 ├─ recording_for_blender.json
 └─ import_openvr_tracker_recording.py
```

### 15.5 BVH Export

BVH는 tracker pose dump가 아니라 skeleton animation이다. 따라서 MVP 필수 기능이 아니다.

필수 선행 조건:

- role mapping
- rest pose calibration
- bone hierarchy
- world pose to local bone transform 변환
- rotation order 정의
- DCC import 검증

권장 단계:

```text
1. tracker point diagnostic BVH
2. simple humanoid hierarchy
3. rest pose capture
4. local rotation conversion
5. Blender/MotionBuilder import test
```

---

## 16. Calibration 설계

### 16.1 Calibration 원칙

- raw data는 변경하지 않는다.
- calibration은 profile로 저장한다.
- profile은 serial number 기준으로 device offset을 적용한다.
- world origin reset은 별도 transform으로 저장한다.

### 16.2 Role profile

```json
{
  "version": 1,
  "roles": {
    "LHR-ABC123": "waist",
    "LHR-DEF456": "left_foot"
  }
}
```

### 16.3 Calibration profile

```json
{
  "version": 1,
  "world": {
    "positionOffset": [0.0, 0.0, 0.0],
    "rotationOffset": [0.0, 0.0, 0.0, 1.0]
  },
  "devices": {
    "LHR-ABC123": {
      "role": "waist",
      "positionOffset": [0.0, 0.05, 0.0],
      "rotationOffset": [0.0, 0.0, 0.0, 1.0]
    }
  }
}
```

### 16.4 MVP calibration 범위

MVP에서는 다음만 지원한다.

- manual role assignment
- role profile 저장/불러오기
- world origin reset
- per-device position/rotation offset 수동 입력

자동 body calibration은 후속 기능으로 분리한다.

---

## 17. 빌드 및 패키징 계획

### 17.1 CMake 구조

권장 타깃:

```text
openvr_tracker_recorder_app
openvr_tracker_recorder_core
openvr_tracker_recorder_tests
imgui_vendor
openvr_vendor
```

### 17.2 ImGui vendor target

ImGui는 include directory만 추가해서는 빌드되지 않는다. 다음 소스가 target에 포함되어야 한다.

```text
imgui.cpp
imgui_demo.cpp
imgui_draw.cpp
imgui_tables.cpp
imgui_widgets.cpp
backends/imgui_impl_glfw.cpp
backends/imgui_impl_opengl3.cpp
```

### 17.3 OpenVR library 연결

OpenVR는 platform별 import library와 runtime DLL 복사가 필요하다.

Windows 예:

```text
Link: openvr_api.lib
Runtime copy: openvr_api.dll
Include: openvr/headers
```

CMake에서는 imported target을 명시한다.

```cmake
add_library(openvr_api SHARED IMPORTED)
set_target_properties(openvr_api PROPERTIES
    IMPORTED_IMPLIB "${OPENVR_ROOT}/lib/win64/openvr_api.lib"
    IMPORTED_LOCATION "${OPENVR_ROOT}/bin/win64/openvr_api.dll"
    INTERFACE_INCLUDE_DIRECTORIES "${OPENVR_ROOT}/headers"
)
```

빌드 후 DLL copy:

```cmake
add_custom_command(TARGET openvr_tracker_recorder_app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:openvr_api>
        $<TARGET_FILE_DIR:openvr_tracker_recorder_app>
)
```

### 17.4 vcpkg manifest

```json
{
  "name": "openvr-tracker-recorder",
  "version-string": "0.1.0",
  "dependencies": [
    "glfw3",
    "glm",
    "nlohmann-json",
    "glad",
    "spdlog",
    "catch2"
  ]
}
```

### 17.5 Windows 배포 구조

```text
OpenVRTrackerRecorder/
 ├─ OpenVRTrackerRecorder.exe
 ├─ openvr_api.dll
 ├─ assets/
 │   ├─ fonts/
 │   └─ shaders/
 ├─ licenses/
 │   ├─ imgui_LICENSE.txt
 │   ├─ glfw_LICENSE.txt
 │   ├─ openvr_LICENSE.txt
 │   ├─ glm_LICENSE.txt
 │   ├─ glad_LICENSE.txt
 │   └─ nlohmann_json_LICENSE.txt
 └─ README.txt
```

---

## 18. 테스트 계획

### 18.1 Unit tests

```text
- matrix to transform conversion
- quaternion normalization
- coordinate conversion
- binary frame encode/decode
- frame index seek
- role profile load/save
- calibration profile apply
- CSV schema output
- JSON schema output
```

### 18.2 Hardware integration tests

```text
- SteamVR 미실행 상태에서 앱 실행
- SteamVR 실행 후 reconnect
- HMD만 연결
- HMD + controller 연결
- HMD + tracker 1개 연결
- HMD + tracker 3개 이상 연결
- 녹화 중 tracker 연결 해제
- 녹화 중 tracker 재연결
- device index 변경 후 role 유지
```

### 18.3 Accuracy tests

```text
- 정지 tracker jitter 측정
- 1m 이동 기록 거리 검증
- 90도 회전 quaternion 방향 검증
- viewport axis 방향 검증
- Blender import 후 축 방향 확인
```

### 18.4 Performance and soak tests

```text
- 60 FPS 10분 녹화
- 90 FPS 10분 녹화
- 90 FPS 1시간 녹화
- tracker 1/3/8개 비교
- UI FPS와 recording FPS 분리 확인
- writer queue backlog 확인
- file writing 중 frame drop 확인
```

### 18.5 Export tests

```text
- CSV column order 검증
- CSV Python/pandas load 검증
- JSON schema 검증
- JSON Lines streaming load 검증
- Blender script 실행 검증
- BVH import 검증
```

---

## 19. 개발 로드맵

### Phase 0: Build and UI Foundation

목표:

- CMake project 생성
- vcpkg manifest 구성
- GLFW window 생성
- OpenGL 3.3 context 생성
- glad 초기화
- ImGui docking UI 표시
- spdlog logging 연결

완료 기준:

- 앱이 실행되고 empty dockspace가 표시된다.
- Debug/Release 빌드가 모두 성공한다.
- ImGui backend가 정상 렌더링된다.

### Phase 1: OpenVR Connection and Device Registry

목표:

- OpenVR 초기화
- SteamVR 상태 표시
- init failure UI 처리
- device enumerate
- serial/model/class 조회
- VR event pump 구현

완료 기준:

- SteamVR 미실행/실행 상태를 구분해 표시한다.
- connected device가 table에 표시된다.
- device reconnect 이벤트가 로그에 남는다.

### Phase 2: Pose Sampling and Conversion

목표:

- `GetDeviceToAbsoluteTrackingPose` polling
- PoseSample 생성
- matrix/quaternion 변환 함수 구현
- latest pose snapshot 생성
- conversion unit tests 작성

완료 기준:

- HMD/controller/tracker position과 rotation이 UI에 표시된다.
- identity/axis rotation 테스트가 통과한다.

### Phase 3: Storage-Backed Recording MVP

목표:

- RecordingController 구현
- SamplingScheduler 구현
- session folder 생성
- manifest/devices/frames/frame_index 저장
- record/stop UI
- preview cache 최소 구현

완료 기준:

- 3개 이상의 tracker를 60초 이상 60/90 FPS로 녹화한다.
- 모든 frame을 메모리에 쌓지 않는다.
- session manifest가 정상 finalize된다.

### Phase 4: 3D Viewport MVP

목표:

- framebuffer 생성
- ImGui viewport panel 통합
- grid/axis 렌더링
- tracker proxy 렌더링
- orbit camera 구현
- trail ring buffer 구현

완료 기준:

- tracker 움직임이 viewport에 실시간 표시된다.
- UI FPS 부하가 recording FPS에 직접 영향을 주지 않는다.

### Phase 5: Playback MVP

목표:

- BinarySessionReader 구현
- frame index seek
- timeline slider
- play/pause/step
- playback speed
- viewport playback 표시

완료 기준:

- 저장된 session을 다시 열어 동일한 움직임을 재생할 수 있다.
- 1분 session seek가 즉시 반응한다.

### Phase 6: CSV/JSON Export

목표:

- CSV exporter
- compact JSON exporter
- JSON Lines exporter
- export options UI
- export tests

완료 기준:

- CSV를 Python/pandas로 읽을 수 있다.
- JSON/JSONL을 앱 또는 별도 tool에서 reload할 수 있다.

### Phase 7: Role Mapping and Calibration

목표:

- role profile editor
- serial 기반 role persistence
- world origin reset
- per-device offset
- calibration profile save/load

완료 기준:

- tracker 재연결 후 role이 유지된다.
- calibration profile 적용 전/후 pose를 비교할 수 있다.

### Phase 8: Blender Export

목표:

- Blender import JSON 생성
- Blender Python script 생성
- Empty object keyframe 삽입
- Blender Z-up 변환 옵션

완료 기준:

- Blender에서 tracker motion을 재생할 수 있다.
- 축 방향 검증 fixture가 통과한다.

### Phase 9: Hardening and Long Recording

목표:

- 90 FPS 1시간 soak test
- recovery flow
- writer backlog diagnostics
- memory usage 개선
- packaging script
- license notice 정리

완료 기준:

- 장시간 녹화 후 session이 손상 없이 열리고 export된다.
- 비정상 종료 session recovery가 가능하다.

### Phase 10: BVH Prototype

목표:

- simple skeleton 정의
- role to bone mapping
- rest pose capture
- BVH motion section 생성
- DCC import 검증

완료 기준:

- 최소 waist/head/hands/feet 기반 BVH prototype을 생성할 수 있다.

---

## 20. MVP 범위 재정의

### 20.1 MVP 필수 기능

```text
- CMake 기반 Windows 빌드
- GLFW + OpenGL + Dear ImGui docking UI
- OpenVR background init 및 상태 표시
- SteamVR reconnect
- HMD/Controller/Generic Tracker 목록 표시
- serial/model/class 표시
- position/rotation 실시간 표시
- 3D viewport 실시간 표시
- serial 기반 role 수동 지정
- 60/90 FPS fixed-rate recording
- binary session 저장
- session reload
- timeline playback
- CSV export
- JSON 또는 JSON Lines export
```

### 20.2 MVP 제외 기능

```text
- BVH 완성형 export
- FBX export
- full-body IK
- advanced retargeting
- SteamVR render model 표시
- OSC/UDP streaming
- cloud sync
- automatic body calibration
```

### 20.3 MVP 성공 기준

```text
SteamVR에 연결된 Vive Tracker 3개 이상을 3D viewport에 실시간 표시하고,
60 FPS 또는 90 FPS로 1분 이상 녹화한 뒤,
binary session으로 저장하고,
앱에서 다시 열어 timeline playback을 수행하며,
CSV/JSON 계열 포맷으로 export할 수 있다.
```

---

## 21. 주요 리스크와 대응

| 리스크 | 영향 | 대응 |
|---|---:|---|
| SteamVR 미실행 시 background init 실패 | 높음 | 정상 상태로 처리, reconnect UI 제공 |
| OpenVR device index 변동 | 높음 | serial 기반 registry와 role profile |
| UI 부하로 recording FPS 저하 | 높음 | tracking/recording thread 분리 |
| 장시간 녹화 메모리 증가 | 높음 | streaming binary writer와 preview cache |
| 좌표계/회전 변환 오류 | 높음 | conversion unit tests와 known-pose fixture |
| OpenGL loader 누락 | 중간 | glad/gl3w 의존성 명시 |
| ImGui 소스 빌드 누락 | 중간 | imgui_vendor CMake target 구성 |
| OpenVR DLL 배포 누락 | 중간 | post-build copy와 packaging 검증 |
| BVH 품질 기대치 과대 | 중간 | MVP 제외, prototype으로 명시 |
| 라이선스 고지 누락 | 중간 | packaging/licenses 자동 수집 또는 체크리스트 |

---

## 22. 문서화 산출물

개발과 함께 다음 문서를 유지한다.

```text
docs/architecture.md
- 계층 구조, thread model, data flow

docs/file_format.md
- manifest/devices/frames/frame_index schema

docs/coordinate_system.md
- OpenVR/OpenGL/Blender/BVH coordinate conversion

docs/export_spec.md
- CSV/JSON/JSONL/Blender/BVH export schema

docs/license_notices.md
- third-party licenses and versions

README.md
- 설치, 빌드, 실행, SteamVR requirement
```

---

## 23. 릴리스 체크리스트

```text
- Release build 성공
- clean machine에서 실행 확인
- openvr_api.dll 포함
- assets/shaders 포함
- licenses 포함
- SteamVR 미실행 상태 UX 확인
- SteamVR 실행 상태 reconnect 확인
- 3 tracker 1분 녹화 확인
- session reload 확인
- CSV export 확인
- JSON/JSONL export 확인
- crash/recovery 최소 확인
```

---

## 24. 결론

C++20 + OpenVR + Dear ImGui + GLFW + OpenGL 조합은 SteamVR tracker recorder를 엔진 없이 가볍고 직접적으로 구현하기에 적합하다. 다만 성공 여부는 UI 자체보다 **pose sampling의 안정성, 저장 포맷의 견고함, 좌표계 변환의 검증 가능성**에 달려 있다.

본 v2 계획서는 binary session storage, OpenVR background 실패 처리, OpenGL/ImGui 빌드 구성, fixed-rate sampling, recovery, testing을 초기 설계에 포함시켜 실제 구현 위험을 낮춘다.

개발 우선순위는 다음과 같다.

1. 빌드 가능한 최소 앱을 만든다.
2. OpenVR 연결과 device registry를 안정화한다.
3. pose conversion을 테스트로 고정한다.
4. 장시간 녹화를 고려한 binary session writer를 MVP에 포함한다.
5. 3D viewport와 playback을 storage 모델 위에 구현한다.
6. CSV/JSON export를 먼저 완성한다.
7. Blender/BVH는 검증 가능한 단계로 확장한다.

이 순서대로 구현하면 Brekel OpenVR Recorder와 유사한 독립형 SteamVR tracking recorder를 현실적인 범위에서 안정적으로 개발할 수 있다.
