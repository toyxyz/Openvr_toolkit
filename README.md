# toyxyz_openvr_toolkit

Native C++20 desktop tool for recording SteamVR/OpenVR tracker poses, solving mapping actors, and exporting sessions/skeleton motion.

Download release : https://github.com/toyxyz/Openvr_toolkit/releases

Tutorial : https://www.youtube.com/watch?v=btdB7rOIkqo


If you would like to support the development of this project, a paid version is also available here. 

https://toyxyz.gumroad.com/l/akrhgz

It offers the same features as the free version. Your support would be greatly appreciated.

https://github.com/user-attachments/assets/5aafaa3e-3346-44ea-9441-c2d7087ac1f5




## Dependencies

- Windows 10/11
- Visual Studio 2022 Build Tools or Visual Studio 2022 with C++ workload
- CMake 3.25+
- Ninja, for the `default` and `desktop` presets
- OpenVR SDK under `third_party/openvr`
- Dear ImGui docking branch under `third_party/imgui`
- vcpkg packages from `vcpkg.json`:
  - `glfw3`
  - `glm`
  - `nlohmann-json`
  - `glad`
  - `spdlog`
  - `catch2`

## Build

For an optimized application build, use `cmake --preset release` followed by
`cmake --build --preset release --target OpenVRTrackerRecorderDesktop` from an x64
Visual Studio developer environment with Ninja available. The executable is
`build/release/toyxyz_openvr_toolkit.exe`. The `default` preset is a Debug build.

Development testing uses the Visual Studio 2022 Debug executable at
`build/vs2022/Debug/toyxyz_openvr_toolkit.exe`. Build this configuration after code
changes before testing. `build/default` is a separate Ninja Debug build.

Visual Studio generator (the build preset selects Debug):

```powershell
cmake --preset vs2022
cmake --build --preset vs2022
```

If an automated Visual Studio build stalls before compilation, use
`cmake --build --preset vs2022 --parallel 1 -- /nr:false` to avoid worker-node reuse.

Ninja core build:

```powershell
cmake --preset default
cmake --build --preset default
```

Ninja desktop build:

```powershell
cmake --preset desktop
cmake --build --preset desktop
```

Run tests:

```powershell
ctest --preset vs2022 --output-on-failure
```
