# toyxyz_openvr_toolkit

Native SteamVR tracker recorder prototype based on C++20, OpenVR, Dear ImGui, GLFW, and OpenGL.

The current implementation contains the first buildable core from the implementation plan:

- core data model for devices, poses, frames, and sessions
- fixed-rate sampling scheduler
- binary frame writer/reader with frame index
- manifest JSON writer
- mock VR provider for hardware-free tests
- dependency-free Win32 bootstrap desktop shell
- CMake project skeleton with optional desktop app target

## Build Core

```powershell
cmake --preset default
cmake --build --preset default
ctest --preset default
```

When using the `Ninja` preset with MSVC on Windows, run those commands from a Visual Studio Developer PowerShell/Command Prompt. A plain PowerShell session may find `cl.exe` but still miss Windows SDK tools and libraries such as `rc.exe`, `mt.exe`, and `kernel32.lib`.

From a plain PowerShell session, the Visual Studio generator preset is usually easier:

```powershell
cmake --preset vs2022
cmake --build --preset vs2022
ctest --preset vs2022
```

The default preset intentionally builds only the dependency-light core. The GLFW/OpenGL/ImGui desktop target is gated behind `OVTR_BUILD_DESKTOP_APP=ON` until the third-party SDKs are installed.

## Run

After `cmake --build --preset vs2022`, the current executables are:

```text
build/vs2022/Debug/OpenVRTrackerRecorder.exe
build/vs2022/Debug/toyxyz_openvr_toolkit.exe
```

`OpenVRTrackerRecorder.exe` is the console core diagnostic app.

`toyxyz_openvr_toolkit.exe` is the current desktop bootstrap shell. It does not use ImGui yet; it uses a dependency-free Win32 window to show whether `openvr_api.dll`, the SteamVR runtime, and an HMD are detected. When the OpenVR SDK is present under `third_party/openvr`, it also initializes OpenVR, lists tracked devices with their latest positions, and draws a simple OpenGL top-down X/Z viewport.

Keyboard controls:

```text
R: start/stop binary session recording
F5: refresh runtime/device status
Esc: exit
Left drag: orbit the 3D viewport
Middle drag: pan the 3D viewport
Mouse wheel: zoom in/out
Arrow keys: rotate the camera
+ / -: zoom in/out
F3: reset camera
F2: toggle device labels
```

Recordings are written under:

```text
build/vs2022/Debug/recordings/session_YYYYMMDD_HHMMSS/
```

Each session currently contains `manifest.json`, `frames.bin`, and `frame_index.bin`.

## Desktop App Dependencies

The desktop app path expects:

- GLFW
- OpenGL
- glad
- Dear ImGui docking branch under `third_party/imgui`
- OpenVR SDK under `third_party/openvr`

See [docs/openvr_tracker_recorder_implementation_plan_v2.md](docs/openvr_tracker_recorder_implementation_plan_v2.md) for the full implementation plan.
