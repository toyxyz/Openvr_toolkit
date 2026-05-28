# CONTINUITY.md

## Snapshot
- 2026-05-28 [USER] Goal: add a Quad view toggle on the viewport control bar with Perspective, Front, Top, and Left panes.
- 2026-05-28 [USER] Success criteria: Perspective keeps existing mouse controls; ortho panes allow middle-button pan only; recording delay/elapsed overlays stay at the original whole-viewport position.
- 2026-05-28 [ASSUMPTION] Current phase: implemented and automated build/test verified; manual visual acceptance of corrected Top view pan remains.
- 2026-05-28 [CODE] Current architecture: C++20 native Win32/OpenVR tracker recorder with modular `src` areas for app, data, export, import, math, platform, recording, render, ui, util, and vr.
- 2026-05-28 [TOOL] Last verified state: default and VS2022 tests passed; VS2022 Debug app rebuilt after correcting Top view vertical pan.

## Invariants / Constraints
- 2026-05-28 [USER] Code files must stay under 300 physical lines unless explicitly exempted in this ledger.
- 2026-05-28 [USER] Keep entrypoints thin and delegate business logic, UI layout, rendering, polling, serialization, and platform details.
- 2026-05-28 [USER] No silent fallbacks during development unless intentional, explicit, documented, and tested.
- 2026-05-28 [USER] No empty catch blocks; caught failures need useful handling.
- 2026-05-28 [USER] Prefer mature open-source, self-hostable libraries when they reduce risk, but ask before major dependency decisions.
- 2026-05-28 [USER] UI must reflect end-user workflows and keep UI separate from core logic.
- 2026-05-28 [USER] `CONTINUITY.md` is the canonical project briefing and must stay short, factual, dated, and tagged.

## Decisions
- D001 ACTIVE 2026-05-28 [USER] Maintain `CONTINUITY.md` as the canonical continuity ledger.
  - Rationale: preserve project context across compaction and long-running AI-assisted work.
  - Supersedes: none.
- D002 ACTIVE 2026-05-28 [CODE] Use fixed-function OpenGL sphere-map matcap for Win32 viewport device render models.
  - Rationale: matches the existing immediate-mode OpenGL renderer without adding shader or asset dependencies.
  - Supersedes: none.
- D003 ACTIVE 2026-05-28 [USER] Load viewport render model matcap from `config/render_model_matcap.png`.
  - Rationale: user supplied a runtime matcap PNG and wants that texture used for device model material.
  - Supersedes: procedural-only matcap texture generation.
- D004 ACTIVE 2026-05-28 [USER] Keep VSync/rendering behavior unchanged and move pose polling/recording append to a fixed 90Hz worker.
  - Rationale: preserve viewport presentation behavior while making pose capture independent of monitor refresh.
  - Supersedes: main-loop-coupled pose polling and recording append.

## State

### Done (recent)
- 2026-05-28 [TOOL] Created initial `CONTINUITY.md` after reading `AGENTS.md`.
- 2026-05-28 [CODE] Added shared matcap texture resource, GL sphere-map state scope, and WIC PNG/JPG RGBA loader.
- 2026-05-28 [CODE] Switched Win32 device render model surface pass from diffuse UV texture rendering to matcap sampling.
- 2026-05-28 [CODE] Stopped uploading OpenVR diffuse textures for viewport device render models.
- 2026-05-28 [CODE] Made `config/render_model_matcap.png` the primary matcap source.
- 2026-05-28 [CODE] Added configurable render-model outline/material colors to viewport settings config, color dialog slots, and renderer.
- 2026-05-28 [CODE] Added color preview swatches next to each Color settings row.
- 2026-05-28 [CODE] Added top menu active state so File/Setting buttons reuse the Device tab active color while their popup menu is open.
- 2026-05-28 [CODE] Enlarged the viewport record button from 30px to 45px.
- 2026-05-28 [CODE] Removed the viewport record button text label and re-centered the button.
- 2026-05-28 [CODE] Promoted current viewport colors to defaults: label white, grid 34/36/43, background 37/50/65, imported GLB 66/104/255, render outline 255/133/32, material white.
- 2026-05-28 [CODE] Added a white top-right viewport overlay showing elapsed seconds while `RecorderState::Recording`.
- 2026-05-28 [CODE] Increased the recording elapsed overlay font from the label font to a dedicated 2x GL font.
- 2026-05-28 [CODE] Restored selected device render-model red outline by using the `selected` flag in `ViewportRenderModelRenderer.cpp`.
- 2026-05-28 [CODE] Added `AppPoseSamplingState` and a 90Hz pose sampling worker with provider, pose snapshot, recording, and origin locking.
- 2026-05-28 [CODE] Moved recording frame append to the pose worker using `SamplingScheduler`, preserving origin transform snapshots.
- 2026-05-28 [CODE] Changed viewport refresh to render from the latest thread-safe pose snapshot instead of calling `pollPoses()`.
- 2026-05-28 [CODE] Serialized provider polling/events/enumeration/shutdown through `providerMutex`.
- 2026-05-28 [CODE] Added Win32 pose sampling tests for pose snapshot copy and 90Hz recording scheduler append behavior.
- 2026-05-28 [CODE] Removed legacy Origin keyboard shortcuts `Tab`, `O`, and `C`; Origin actions are now UI-driven only.
- 2026-05-28 [CODE] Forwarded viewport child `WM_KEYDOWN` to the main window key handler so camera shortcuts work when the viewport has focus.
- 2026-05-28 [CODE] Changed camera reset shortcut from `Home` to `F3`.
- 2026-05-28 [CODE] Changed the default and `F3` reset camera to a front view with a slight downward pitch: yaw 0, pitch 18, distance 5.5, pan 0/0/0.
- 2026-05-28 [CODE] Added `F2` as a viewport device name/serial label visibility toggle; labels default to visible.
- 2026-05-28 [TOOL] Built a VS2022 Release app and assembled `toyxyz_vr_toolkit_v1` with executable, OpenVR DLL, MSVC x64 runtime DLLs, config/matcap, output folders, OpenVR license, and README.
- 2026-05-28 [CODE] Renamed the Win32 desktop output executable and title bar to `toyxyz_openvr_toolkit`.
- 2026-05-28 [TOOL] Assembled `toyxyz_openvr_toolkit/` as a Release distribution package.
- 2026-05-28 [USER] Release packages must not include user-specific `config/*.cfg` files.
- 2026-05-28 [CODE] Renamed Setting > Color to `Appearance` and added grid size/density controls to viewport appearance settings.
- 2026-05-28 [TOOL] Updated `toyxyz_openvr_toolkit/toyxyz_openvr_toolkit.exe` from the latest Release build; package still contains no `config/*.cfg`.
- 2026-05-28 [CODE] Added distribution package folders to `.gitignore`.
- 2026-05-28 [CODE] Added Quad view runtime state, lower-left control bar toggle button, 4-pane perspective/ortho rendering, pane hit-testing, and ortho middle-drag pan.
- 2026-05-28 [CODE] Kept recording delay and elapsed-second overlays as one whole-viewport 2D pass after Quad pane rendering.
- 2026-05-28 [CODE] Fixed Top view vertical middle-drag pan direction by flipping the Top pane Z pan mapping.
- 2026-05-28 [CODE] Decoupled Front/Top/Left ortho cameras from Perspective camera pan and dolly; ortho panes now use only their own middle-drag pan.
- 2026-05-28 [CODE] Updated `F3` reset to restore Perspective camera defaults plus Front/Top/Left ortho pan and clear active drag state.
- 2026-05-28 [CODE] Added per-pane Front/Top/Left ortho scroll zoom and included ortho zoom in `F3` reset.
- 2026-05-28 [CODE] Corrected Quad Top view camera pitch from bottom-view orientation to top-view orientation.
- 2026-05-28 [CODE] Corrected Quad Top view vertical middle-drag pan direction after the Top camera orientation fix.

### Now
- 2026-05-28 [ASSUMPTION] Corrected Top view vertical pan is built into the latest VS2022 Debug exe and automated tests pass.

### Next
- 2026-05-28 [ASSUMPTION] Manually verify Top view up/down pan now follows mouse drag direction.

## Open Questions
- 2026-05-28 [ASSUMPTION] Whether to remove the explicit procedural fallback after runtime PNG loading is visually confirmed is not yet decided.
- 2026-05-28 [CODE] `CMakeLists.txt` is an existing 300-line rule exception at 591 lines; this task made a small target property edit there.

## Working Set
- 2026-05-28 [TOOL] AGENTS.md
- 2026-05-28 [TOOL] CONTINUITY.md
- 2026-05-28 [CODE] CMakeLists.txt
- 2026-05-28 [CODE] src/platform/win32/ConfigTypes.h
- 2026-05-28 [CODE] src/platform/win32/ConfigViewportSettings*.cpp
- 2026-05-28 [CODE] src/platform/win32/ViewportSettingsModel.{h,cpp}
- 2026-05-28 [CODE] src/platform/win32/ViewportColorDialog*.{h,cpp}
- 2026-05-28 [CODE] src/platform/win32/ViewportRenderModel*.{h,cpp}
- 2026-05-28 [CODE] src/platform/win32/Win32WicImageLoader.{h,cpp}
- 2026-05-28 [CODE] src/platform/win32/AppViewportState.h
- 2026-05-28 [CODE] src/platform/win32/AppTopBarState.h
- 2026-05-28 [CODE] src/platform/win32/PaintWidgets.{h,cpp}
- 2026-05-28 [CODE] src/platform/win32/WindowChromePainter.{h,cpp}
- 2026-05-28 [CODE] src/platform/win32/TopMenus.cpp
- 2026-05-28 [CODE] src/platform/win32/ViewportControlLayoutMetrics.h
- 2026-05-28 [CODE] src/platform/win32/ViewportControlLayout.cpp
- 2026-05-28 [CODE] src/platform/win32/ViewportControlPainter.cpp
- 2026-05-28 [CODE] src/platform/win32/ViewportPaneTypes.h
- 2026-05-28 [CODE] src/platform/win32/ViewportQuadView.{h,cpp}
- 2026-05-28 [CODE] src/platform/win32/ViewportRenderer.cpp
- 2026-05-28 [CODE] src/platform/win32/ViewportWindowInput.{h,cpp}
- 2026-05-28 [CODE] src/platform/win32/WindowCameraKeyboard.cpp
- 2026-05-28 [CODE] src/platform/win32/ViewportOverlayRenderer.cpp
- 2026-05-28 [CODE] src/platform/win32/ViewportGlFonts.cpp
- 2026-05-28 [CODE] src/platform/win32/ViewportGlResources.cpp
- 2026-05-28 [CODE] src/platform/win32/AppPoseSamplingState.h
- 2026-05-28 [CODE] src/platform/win32/PoseSamplingWorker.{h,cpp}
- 2026-05-28 [CODE] src/platform/win32/FrameUpdate.cpp
- 2026-05-28 [CODE] src/platform/win32/RuntimeDeviceRefresh.cpp
- 2026-05-28 [CODE] src/platform/win32/RuntimeFpsCounters.cpp
- 2026-05-28 [CODE] src/platform/win32/RecordingStartActions.cpp
- 2026-05-28 [CODE] src/platform/win32/RecordingUiActions.cpp
- 2026-05-28 [CODE] src/platform/win32/RecordingStateQueries.{h,cpp}
- 2026-05-28 [CODE] tests/test_win32_pose_sampling.cpp
- 2026-05-28 [CODE] tests/test_win32_viewport_settings_config.cpp
- 2026-05-28 [CODE] tests/test_win32_viewport_control_layout.cpp

## Packages
- 2026-05-28 [TOOL] `toyxyz_vr_toolkit_v1/` contains `OpenVRTrackerRecorderDesktop.exe` built from `build/vs2022/Release`, `openvr_api.dll`, VC143 CRT DLLs, portable `config/`, empty `recordings/` and `exports/`, and `licenses/OpenVR_LICENSE.txt`.
- 2026-05-28 [TOOL] `toyxyz_openvr_toolkit/` contains `toyxyz_openvr_toolkit.exe` built from `build/vs2022/Release`, `openvr_api.dll`, VC143 CRT DLLs, `config/render_model_matcap.png` only, empty `recordings/` and `exports/`, `licenses/OpenVR_LICENSE.txt`, and README.

## Incidents
- None.

## Receipts
- 2026-05-28 [TOOL] Ran `rg --files -g AGENTS.md -g CONTINUITY.md`; only `AGENTS.md` existed.
- 2026-05-28 [TOOL] Read `AGENTS.md`.
- 2026-05-28 [TOOL] README lists canonical commands: `cmake --preset default`, `cmake --build --preset default`, `ctest --preset default`; VS alternative uses `vs2022` preset.
- 2026-05-28 [TOOL] Plain `cmake --build --preset default` failed because the shell lacked MSVC/Windows SDK include paths.
- 2026-05-28 [TOOL] Ran VS Developer Command Prompt + `cmake --build --preset default`; succeeded after final matcap/resource changes.
- 2026-05-28 [TOOL] Ran VS Developer Command Prompt + `ctest --preset default`; `core_tests` passed after final matcap/resource changes.
- 2026-05-28 [TOOL] Confirmed `build/vs2022/Debug/config/render_model_matcap.png`; PNG is 512x512, 108328 bytes, SHA256 DBADD8B67FC3FD3F9E9FF7A802A65B84234F501C4894A043E831F502D8876C2E.
- 2026-05-28 [TOOL] Ran VS Developer Command Prompt + `cmake --build --preset default`; succeeded after WIC matcap loader changes.
- 2026-05-28 [TOOL] Ran VS Developer Command Prompt + `ctest --preset default`; `core_tests` passed after WIC matcap loader changes.
- 2026-05-28 [TOOL] Ran VS Developer Command Prompt + `cmake --build --preset vs2022`; timed out at 120s, but log showed `OpenVRTrackerRecorderDesktop.exe` was produced.
- 2026-05-28 [TOOL] Ran VS Developer Command Prompt + `ctest --preset vs2022`; `core_tests` passed after WIC matcap loader changes.
- 2026-05-28 [TOOL] Ran VS Developer Command Prompt + `cmake --build --preset default`; succeeded after render-model color settings, swatches, and fallback tint.
- 2026-05-28 [TOOL] Ran VS Developer Command Prompt + `ctest --preset default`; `core_tests` passed after render-model color settings, swatches, and fallback tint.
- 2026-05-28 [TOOL] Checked touched code/test file lengths; all were under the 300 physical line limit.
- 2026-05-28 [TOOL] Reconfigured `build/vs2022` with preset defaults and rebuilt target `OpenVRTrackerRecorderDesktop`; output exe timestamp became 2026-05-28 15:44:30 KST.
- 2026-05-28 [TOOL] Ran VS Developer Command Prompt + `ctest --preset vs2022`; `core_tests` passed after rebuilding the VS2022 Debug app.
- 2026-05-28 [TOOL] Ran VS Developer Command Prompt + `cmake --build --preset default`; succeeded after top menu active-color change.
- 2026-05-28 [TOOL] Rebuilt VS2022 Debug target `OpenVRTrackerRecorderDesktop`; output exe timestamp became 2026-05-28 15:54:12 KST.
- 2026-05-28 [TOOL] Ran VS Developer Command Prompt + `ctest --preset default && ctest --preset vs2022`; both `core_tests` runs passed after top menu active-color change.
- 2026-05-28 [TOOL] Ran VS Developer Command Prompt + `cmake --build --preset default --clean-first && ctest --preset default`; succeeded after record button size/label change.
- 2026-05-28 [TOOL] Rebuilt VS2022 Debug target `OpenVRTrackerRecorderDesktop`; output exe timestamp became 2026-05-28 16:04:48 KST.
- 2026-05-28 [TOOL] Ran VS Developer Command Prompt + `ctest --preset vs2022`; `core_tests` passed after record button size/label change.
- 2026-05-28 [TOOL] Ran VS Developer Command Prompt + `cmake --build --preset default --clean-first && ctest --preset default`; succeeded after removing the Record text label.
- 2026-05-28 [TOOL] Rebuilt VS2022 Debug target `OpenVRTrackerRecorderDesktop`; output exe timestamp became 2026-05-28 16:09:03 KST.
- 2026-05-28 [TOOL] Ran VS Developer Command Prompt + `ctest --preset vs2022`; `core_tests` passed after removing the Record text label.
- 2026-05-28 [TOOL] Read `build/vs2022/Debug/config/openvr_tracker_recorder_viewport.cfg` and used its current color values as code defaults.
- 2026-05-28 [TOOL] Ran VS Developer Command Prompt + `cmake --build --preset default --clean-first && ctest --preset default`; succeeded after viewport color defaults change.
- 2026-05-28 [TOOL] Rebuilt VS2022 Debug target `OpenVRTrackerRecorderDesktop`; output exe timestamp became 2026-05-28 16:15:44 KST.
- 2026-05-28 [TOOL] Ran VS Developer Command Prompt + `ctest --preset vs2022`; `core_tests` passed after viewport color defaults change.
- 2026-05-28 [TOOL] Ran VS Developer Command Prompt + `cmake --build --preset default`; first attempt found a Windows `max` macro collision, then succeeded after replacing `std::max` usage.
- 2026-05-28 [TOOL] Ran VS Developer Command Prompt + `ctest --preset default`; `core_tests` passed after recording elapsed overlay change.
- 2026-05-28 [TOOL] Rebuilt VS2022 Debug target `OpenVRTrackerRecorderDesktop`; output exe timestamp became 2026-05-28 16:36:40 KST.
- 2026-05-28 [TOOL] Ran VS Developer Command Prompt + `ctest --preset vs2022`; `core_tests` passed after recording elapsed overlay change.
- 2026-05-28 [TOOL] Ran VS Developer Command Prompt + `cmake --build --preset default`; succeeded after adding the 2x recording timer GL font.
- 2026-05-28 [TOOL] Ran VS Developer Command Prompt + `ctest --preset default`; `core_tests` passed after 2x recording timer font change.
- 2026-05-28 [TOOL] Rebuilt VS2022 Debug target `OpenVRTrackerRecorderDesktop`; output exe timestamp became 2026-05-28 16:43:55 KST.
- 2026-05-28 [TOOL] Ran VS Developer Command Prompt + `ctest --preset vs2022`; `core_tests` passed after 2x recording timer font change.
- 2026-05-28 [TOOL] Ran VS Developer Command Prompt + `cmake --build --preset default`; succeeded after selected render-model red outline fix.
- 2026-05-28 [TOOL] Ran VS Developer Command Prompt + `ctest --preset default`; `core_tests` passed after selected render-model red outline fix.
- 2026-05-28 [TOOL] Rebuilt VS2022 Debug target `OpenVRTrackerRecorderDesktop`; output exe timestamp became 2026-05-28 16:47:54 KST.
- 2026-05-28 [TOOL] Ran VS Developer Command Prompt + `ctest --preset vs2022`; `core_tests` passed after selected render-model red outline fix.
- 2026-05-28 [TOOL] Ran VS Developer Command Prompt + `cmake --build --preset default`; succeeded after pose worker decoupling.
- 2026-05-28 [TOOL] Ran VS Developer Command Prompt + `cmake --build --preset default --target openvr_tracker_recorder_tests --clean-first && ctest --preset default`; `core_tests` passed after clean rebuild.
- 2026-05-28 [TOOL] Ran VS Developer Command Prompt + `cmake --build --preset default && ctest --preset default`; build and `core_tests` passed after pose worker decoupling.
- 2026-05-28 [TOOL] Ran VS Developer Command Prompt + `cmake --build --preset vs2022 --target OpenVRTrackerRecorderDesktop`; succeeded after pose worker decoupling.
- 2026-05-28 [TOOL] Ran VS Developer Command Prompt + `ctest --preset vs2022`; `core_tests` passed after pose worker decoupling.
- 2026-05-28 [USER] Manual recording report: 10-second recording UI showed `Pose FPS 89.9`, `View FPS 89.9`, `Frames 960`, `Dropped 0`.
- 2026-05-28 [USER] Manual 60Hz monitor report: about 10-second recording UI showed `Pose FPS 90.0`, `View FPS 60.0`, `Frames 927`, `Dropped 0`.
- 2026-05-28 [TOOL] Ran VS Developer Command Prompt + `cmake --build --preset default && ctest --preset default`; passed after removing legacy Origin keyboard shortcuts.
- 2026-05-28 [TOOL] Ran VS Developer Command Prompt + `cmake --build --preset vs2022 --target OpenVRTrackerRecorderDesktop && ctest --preset vs2022`; passed after removing legacy Origin keyboard shortcuts.
- 2026-05-28 [TOOL] Ran VS Developer Command Prompt + `cmake --build --preset default && ctest --preset default`; passed after viewport key forwarding fix.
- 2026-05-28 [TOOL] Ran VS Developer Command Prompt + `cmake --build --preset vs2022 --target OpenVRTrackerRecorderDesktop && ctest --preset vs2022`; passed after viewport key forwarding fix.
- 2026-05-28 [TOOL] Ran VS Developer Command Prompt + `cmake --build --preset default && ctest --preset default`; passed after changing camera reset shortcut to `F3`.
- 2026-05-28 [TOOL] Ran VS Developer Command Prompt + `cmake --build --preset vs2022 --target OpenVRTrackerRecorderDesktop && ctest --preset vs2022`; passed after changing camera reset shortcut to `F3`.
- 2026-05-28 [TOOL] Ran VS Developer Command Prompt + `cmake --build --preset default && ctest --preset default`; passed after changing the default camera view.
- 2026-05-28 [TOOL] Ran VS Developer Command Prompt + `cmake --build --preset vs2022 --target OpenVRTrackerRecorderDesktop && ctest --preset vs2022`; passed after changing the default camera view.
- 2026-05-28 [TOOL] Ran VS Developer Command Prompt + `cmake --build --preset default && ctest --preset default`; passed after adding the `F2` device label toggle.
- 2026-05-28 [TOOL] Ran VS Developer Command Prompt + `cmake --build --preset vs2022 --target OpenVRTrackerRecorderDesktop && ctest --preset vs2022`; passed after adding the `F2` device label toggle.
- 2026-05-28 [TOOL] Ran VS Developer Command Prompt + `cmake --build --preset vs2022 --config Release --target OpenVRTrackerRecorderDesktop`; initial 120s attempt timed out, rerun with longer timeout succeeded.
- 2026-05-28 [TOOL] Ran VS Developer Command Prompt + `cmake --build --preset vs2022 --config Release --target openvr_tracker_recorder_tests && ctest --test-dir build/vs2022 -C Release --output-on-failure`; `core_tests` passed.
- 2026-05-28 [TOOL] Ran `dumpbin /dependents toyxyz_vr_toolkit_v1/OpenVRTrackerRecorderDesktop.exe`; package includes `openvr_api.dll` and VC143 runtime DLLs alongside system dependencies.
- 2026-05-28 [TOOL] Ran VS Developer Command Prompt + `cmake --build --preset default && ctest --preset default`; linked `toyxyz_openvr_toolkit.exe` and `core_tests` passed after app rename.
- 2026-05-28 [TOOL] Ran VS Developer Command Prompt + `cmake --build --preset vs2022 --config Release --target OpenVRTrackerRecorderDesktop && cmake --build --preset vs2022 --config Release --target openvr_tracker_recorder_tests && ctest --test-dir build/vs2022 -C Release --output-on-failure`; `core_tests` passed after app rename.
- 2026-05-28 [TOOL] Ran `dumpbin /dependents toyxyz_openvr_toolkit/toyxyz_openvr_toolkit.exe`; package includes `openvr_api.dll` and VC143 runtime DLLs alongside system dependencies.
- 2026-05-28 [TOOL] Confirmed `toyxyz_openvr_toolkit` title string is embedded in the packaged executable.
- 2026-05-28 [TOOL] Removed `config/*.cfg` from `toyxyz_openvr_toolkit/`; verified no `*.cfg` files remain in the release package.
- 2026-05-28 [TOOL] Ran VS Developer Command Prompt + `cmake --build --preset default && ctest --preset default`; `core_tests` passed after Appearance/grid settings changes.
- 2026-05-28 [TOOL] Ran VS Developer Command Prompt + `cmake --build --preset vs2022 --target OpenVRTrackerRecorderDesktop && ctest --preset vs2022`; `core_tests` passed after Appearance/grid settings changes.
- 2026-05-28 [TOOL] Ran VS Developer Command Prompt + `cmake --build --preset vs2022 --config Release --target OpenVRTrackerRecorderDesktop && cmake --build --preset vs2022 --config Release --target openvr_tracker_recorder_tests && ctest --test-dir build/vs2022 -C Release --output-on-failure`; `core_tests` passed after Appearance/grid settings changes.
- 2026-05-28 [TOOL] Checked manually written `src/platform/win32` and `tests` code file lengths; none exceeded 300 lines after Appearance/grid settings changes.
- 2026-05-28 [TOOL] Re-copied Release `toyxyz_openvr_toolkit.exe` into `toyxyz_openvr_toolkit/` and verified `NO_CFG_FILES`.
- 2026-05-28 [TOOL] Ran `git check-ignore -v` for files under `toyxyz_openvr_toolkit/` and `toyxyz_vr_toolkit_v1/`; both package folders are ignored.
- 2026-05-28 [TOOL] Initial incremental `cmake --build --preset default && ctest --preset default` after adding `ViewportControlLayout::quadViewButtonRect` built but failed one layout test due stale object layout; clean rebuild mitigated it.
- 2026-05-28 [TOOL] Ran VS Developer Command Prompt + `cmake --build --preset default --clean-first && ctest --preset default`; `core_tests` passed after Quad view changes.
- 2026-05-28 [TOOL] Ran VS Developer Command Prompt + `cmake --build --preset default && ctest --preset default`; no work to do and `core_tests` passed after Quad view changes.
- 2026-05-28 [TOOL] Ran VS Developer Command Prompt + `cmake --build --preset vs2022 --target OpenVRTrackerRecorderDesktop && ctest --preset vs2022`; Debug app rebuilt and `core_tests` passed after Quad view changes.
- 2026-05-28 [TOOL] Checked touched manually written code/test file lengths; all were under 300 lines after Quad view changes.
- 2026-05-28 [TOOL] Ran VS Developer Command Prompt + `cmake --build --preset default && ctest --preset default && cmake --build --preset vs2022 --target OpenVRTrackerRecorderDesktop && ctest --preset vs2022`; both `core_tests` runs passed after Top view pan direction fix.
- 2026-05-28 [TOOL] Confirmed latest Debug exe `build/vs2022/Debug/toyxyz_openvr_toolkit.exe` timestamp is 2026-05-28 19:40:24 KST.
- 2026-05-28 [TOOL] Ran VS Developer Command Prompt + `cmake --build --preset default && ctest --preset default && cmake --build --preset vs2022 --target OpenVRTrackerRecorderDesktop && ctest --preset vs2022`; both `core_tests` runs passed after Quad ortho camera separation.
- 2026-05-28 [TOOL] Confirmed latest Debug exe `build/vs2022/Debug/toyxyz_openvr_toolkit.exe` timestamp is 2026-05-28 19:43:23 KST.
- 2026-05-28 [TOOL] Ran VS Developer Command Prompt + `cmake --build --preset default && ctest --preset default && cmake --build --preset vs2022 --target OpenVRTrackerRecorderDesktop && ctest --preset vs2022`; both `core_tests` runs passed after F3 all-view reset.
- 2026-05-28 [TOOL] Confirmed latest Debug exe `build/vs2022/Debug/toyxyz_openvr_toolkit.exe` timestamp is 2026-05-28 19:50:18 KST.
- 2026-05-28 [TOOL] Checked `src/platform/win32/WindowCameraKeyboard.cpp`; 78 lines after F3 all-view reset.
- 2026-05-28 [TOOL] Ran VS Developer Command Prompt + `cmake --build --preset default && ctest --preset default && cmake --build --preset vs2022 --target OpenVRTrackerRecorderDesktop && ctest --preset vs2022`; both `core_tests` runs passed after Quad ortho scroll zoom.
- 2026-05-28 [TOOL] Confirmed latest Debug exe `build/vs2022/Debug/toyxyz_openvr_toolkit.exe` timestamp is 2026-05-28 19:58:01 KST.
- 2026-05-28 [TOOL] Checked touched file lengths after Quad ortho scroll zoom: `ViewportRenderer.cpp` 271, `ViewportWindowInput.cpp` 235, `ViewportQuadView.cpp` 122, `WindowCameraKeyboard.cpp` 81, `test_win32_viewport_math.cpp` 176.
- 2026-05-28 [TOOL] Ran VS Developer Command Prompt + `cmake --build --preset default && ctest --preset default && cmake --build --preset vs2022 --target OpenVRTrackerRecorderDesktop && ctest --preset vs2022`; both `core_tests` runs passed after correcting Top view orientation.
- 2026-05-28 [TOOL] Confirmed latest Debug exe `build/vs2022/Debug/toyxyz_openvr_toolkit.exe` timestamp is 2026-05-28 20:04:33 KST.
- 2026-05-28 [TOOL] Checked `src/platform/win32/ViewportRenderer.cpp`; 271 lines after correcting Top view orientation.
- 2026-05-28 [TOOL] Ran VS Developer Command Prompt + `cmake --build --preset default && ctest --preset default && cmake --build --preset vs2022 --target OpenVRTrackerRecorderDesktop && ctest --preset vs2022`; both `core_tests` runs passed after correcting Top view vertical pan.
- 2026-05-28 [TOOL] Confirmed latest Debug exe `build/vs2022/Debug/toyxyz_openvr_toolkit.exe` timestamp is 2026-05-28 20:11:12 KST.
- 2026-05-28 [TOOL] Checked touched file lengths after Top view vertical pan fix: `ViewportQuadView.cpp` 122, `test_win32_viewport_math.cpp` 176.
