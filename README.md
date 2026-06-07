# toyxyz_openvr_toolkit

Native C++20 desktop tool for recording SteamVR/OpenVR tracker poses, solving mapping actors, and exporting sessions/skeleton motion.

## Dependencies

- Windows 10/11
- Visual Studio 2022 Build Tools or Visual Studio 2022 with C++ workload
- CMake 3.25+
- Ninja, for the `default` and `desktop` presets
- OpenVR SDK under `third_party/openvr`
- vcpkg packages from `vcpkg.json`:
  - `glfw3`
  - `glm`
  - `nlohmann-json`
  - `glad`
  - `spdlog`
  - `catch2`

## Build

Visual Studio generator:

```powershell
cmake --preset vs2022
cmake --build --preset vs2022
```

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
