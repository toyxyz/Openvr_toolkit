# OpenVR Tracker Recorder 리팩토링 계획

## 1. 범위와 기준선

리팩토링 범위는 `src/`, `tests/`, 루트 CMake 설정으로 한정한다. `third_party/openvr`는 벤더 코드로 간주하여 수정 대상에서 제외한다.

현재 목표 기준선은 다음과 같다.

- `cmake --build --preset default` 성공
- `ctest --preset default` 성공
- Windows UI/Win32 코드 변경 후 `cmake --build --preset vs2022` 성공

핵심 방향은 동작 변경 없이 구조 안정성을 높이는 것이다. 새 기능, 새 파일 포맷, 대규모 UI 프레임워크 전환은 이번 리팩토링 범위가 아니다.

## 2. 설계 원칙

- **SOLID**: exporter, importer, recording I/O, Win32 UI, viewport rendering의 변경 이유를 분리한다.
- **DRY**: pose interpolation, safe-name, little-endian/binary buffer, JSON escaping/writing 중복을 공통 유틸로 모은다.
- **KISS**: 기존 동작을 유지하는 작은 추출과 명확한 파일 경계를 우선한다.
- **YAGNI**: ImGui/GLFW 전환, 새 exporter 포맷, 외부 JSON 라이브러리 도입, binary session format 변경은 제외한다.
- **RAII**: Win32, OpenGL, OpenVR 자원은 생성과 해제를 같은 타입 안에 묶는다.
- **캡슐화**: Win32 전용 타입은 `platform/win32` 밖으로 새지 않게 하고, core API는 platform-neutral 타입을 유지한다.
- **의존성 관리**: core library는 dependency-light 상태를 유지하고 CMake에는 필요한 새 소스만 추가한다.

## 3. 실행 항목

### 3.1 공통 유틸 추출

- `src/math/PoseInterpolation.h/.cpp`
  - `quaternionDot`
  - `slerpQuaternion`
  - `lerpVec3`
  - pose/keyframe sampling helper

- `src/util/Identifier.h/.cpp`
  - `sanitizeIdentifier`
  - `deviceClassPrefix`
  - device safe-name 생성
  - 기존 `makeFbxSafeName`은 compatibility wrapper로 유지

- `src/util/BinaryBuffer.h/.cpp`
  - little-endian read/write
  - file byte read/write
  - 4-byte alignment/padding

- `src/util/JsonWriter.h/.cpp`
  - string escaping
  - 작은 JSON field/array/object writer
  - 외부 JSON 의존성은 추가하지 않음

### 3.2 Export/import 정리

- `src/export/ExportPoseTrack.h/.cpp`를 통해 FBX/glTF exporter가 공통 pose track 수집 경로를 사용한다.
- FBX exporter는 scene build, timeline, coordinate conversion, ASCII writer를 분리한다.
- `FbxExportOptions`에 `geometryProvider`를 추가해 테스트에서 OpenVR render-model loading을 주입 가능하게 한다.
- glTF exporter는 buffer 조립, JSON section writer, file writer, scene builder를 분리한다.
- glTF importer는 GLB reader, JSON parser, accessor reader, imported scene builder를 분리한다.
- 파일명만 있는 output path도 성공해야 한다.

### 3.3 Recording I/O 안정화

- `BinarySessionReader`는 truncated/partial index entry를 조용히 무시하지 않고 실패해야 한다.
- 비정상적으로 큰 `poseCount`는 과도한 reserve 전에 실패해야 한다.
- header size, entry size, endian marker 검증 실패 메시지를 명확히 유지한다.
- writer/controller는 output parent path가 비어 있을 때 불필요한 directory creation을 시도하지 않는다.

### 3.4 Win32 bootstrap 분해

`Win32BootstrapApp.cpp`는 message loop/window proc dispatch 중심으로 축소한다. 주요 책임은 다음 단위로 분리한다.

- `AppState`
- `ConfigStore`
- `Layout`
- `ViewportRenderer`
- `Dialogs`
- `RecordingActions`
- `DebugPanel`
- `Win32Resources`

Win32 UI 동작, 단축키, 파일 포맷, 설정 파일 형식은 유지한다.

### 3.5 RAII와 OpenVR 수명

- `OpenVRProvider`는 destructor에서 shutdown 누락에 안전해야 한다.
- OpenVR render model/texture handle은 scope guard 또는 RAII wrapper로 해제한다.
- OpenGL texture/list, Win32 GDI/DC/menu/window 자원은 RAII wrapper로 소유한다.

## 4. 테스트 계획

기존 `core_tests` 실행기는 유지하되, 실제 테스트 구현 파일은 기능별로 분리한다.

- recording/session tests
- FBX exporter/math tests
- glTF/GLB exporter tests
- glTF importer tests
- math/identifier tests
- provider tests
- Win32 pure logic tests

추가 또는 유지해야 할 회귀 테스트는 다음과 같다.

- corrupt/truncated frame index 거부
- 비정상 pose count 방어
- FBX/glTF/GLB output path가 파일명만 있을 때 성공
- FBX/glTF safe-name 동일성 및 UTF-8 유지
- shared slerp/resample 결과 보존
- GLB import missing JSON/BIN chunk, invalid accessor, 65535 초과 index 실패
- Win32 config parse, origin transform, layout rect 계산

## 5. 완료 기준

- `Win32BootstrapApp.cpp`가 message loop/window proc 중심 파일로 축소되어 있다.
- FBX/glTF exporter가 공통 pose track 수집 경로를 사용한다.
- 반복되던 interpolation, safe-name, little-endian, binary file, JSON escaping/writing 로직이 공통 유틸로 이동되어 있다.
- Win32/OpenGL/OpenVR 자원이 RAII wrapper를 통해 해제된다.
- 기존 public API 호환성이 유지되며, 계획된 `FbxExportOptions::geometryProvider`만 명시적으로 추가된다.
- `cmake --build --preset default`, `ctest --preset default`, `cmake --build --preset vs2022`, `ctest --preset vs2022`가 통과한다.
