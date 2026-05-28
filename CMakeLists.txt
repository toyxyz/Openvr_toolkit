cmake_minimum_required(VERSION 3.24)

project(OpenVRTrackerRecorder
    VERSION 0.1.0
    DESCRIPTION "Native SteamVR tracker recorder"
    LANGUAGES CXX
)

option(OVTR_BUILD_TESTS "Build unit tests" ON)
option(OVTR_BUILD_DESKTOP_APP "Build GLFW/OpenGL/ImGui desktop app" OFF)
option(OVTR_BUILD_WIN32_BOOTSTRAP_APP "Build dependency-free Win32 desktop bootstrap app" ON)

set(OPENVR_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third_party/openvr" CACHE PATH "Path to OpenVR SDK")

if(WIN32 AND EXISTS "${OPENVR_ROOT}/headers/openvr.h")
    add_library(openvr_api SHARED IMPORTED)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(_ovtr_openvr_arch "win64")
    else()
        set(_ovtr_openvr_arch "win32")
    endif()
    set_target_properties(openvr_api PROPERTIES
        IMPORTED_IMPLIB "${OPENVR_ROOT}/lib/${_ovtr_openvr_arch}/openvr_api.lib"
        IMPORTED_LOCATION "${OPENVR_ROOT}/bin/${_ovtr_openvr_arch}/openvr_api.dll"
        INTERFACE_INCLUDE_DIRECTORIES "${OPENVR_ROOT}/headers"
    )
    set(OVTR_HAS_OPENVR_SDK ON)
else()
    set(OVTR_HAS_OPENVR_SDK OFF)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(openvr_tracker_recorder_core STATIC
    src/app/Application.cpp
    src/data/SessionTypes.cpp
    src/export/ExportPoseTrack.cpp
    src/export/FbxAsciiFormatting.cpp
    src/export/FbxAsciiAnimationWriter.cpp
    src/export/FbxAsciiConnectionsWriter.cpp
    src/export/FbxAsciiGeometryWriter.cpp
    src/export/FbxAsciiMath.cpp
    src/export/FbxAsciiExporter.cpp
    src/export/FbxAsciiWriter.cpp
    src/export/FbxEulerMath.cpp
    src/export/FbxCoordinateConversion.cpp
    src/export/FbxExportFileWriter.cpp
    src/export/FbxExportPoseKeys.cpp
    src/export/FbxExportSceneBuilder.cpp
    src/export/FbxExportTimelineRanges.cpp
    src/export/FbxTimeline.cpp
    src/export/GltfExportBuffers.cpp
    src/export/GltfExportAnimationData.cpp
    src/export/GltfExportKeyResampler.cpp
    src/export/GltfExportMeshData.cpp
    src/export/GltfFileWriter.cpp
    src/export/GltfExportSceneBuilder.cpp
    src/export/GltfExporter.cpp
    src/export/GltfJsonAnimationSection.cpp
    src/export/GltfJsonBufferSections.cpp
    src/export/GltfJsonBuilder.cpp
    src/export/GltfJsonFormatting.cpp
    src/export/GltfJsonSceneSections.cpp
    src/export/RenderModelGeometry.cpp
    src/import/GltfAccessor.cpp
    src/import/GltfAccessorIndexReader.cpp
    src/import/GltfAccessorLayout.cpp
    src/import/GltfAccessorReadLayout.cpp
    src/import/GltfAccessorReader.cpp
    src/import/GltfGlbReader.cpp
    src/import/GltfImportedAnimationChannels.cpp
    src/import/GltfImportedAnimationKeys.cpp
    src/import/GltfImportedAnimations.cpp
    src/import/GltfImportedMeshes.cpp
    src/import/GltfImportedNodes.cpp
    src/import/GltfImportedSceneBuilder.cpp
    src/import/GltfImporter.cpp
    src/import/GltfJson.cpp
    src/import/GltfJsonNumberParser.cpp
    src/import/GltfJsonParser.cpp
    src/import/GltfJsonParserState.cpp
    src/import/GltfJsonStringParser.cpp
    src/import/GltfJsonStructureParser.cpp
    src/import/GltfJsonUnicode.cpp
    src/import/GltfJsonUtils.cpp
    src/import/GltfJsonValueParser.cpp
    src/math/PoseInterpolation.cpp
    src/math/PoseOriginTransform.cpp
    src/math/PoseTransform.cpp
    src/math/QuaternionUtils.cpp
    src/recording/BinarySessionFrameReader.cpp
    src/recording/BinarySessionFrameWriter.cpp
    src/recording/BinarySessionReaderIndex.cpp
    src/recording/BinarySessionReader.cpp
    src/recording/BinarySessionWriter.cpp
    src/recording/BinarySessionWriterHeaders.cpp
    src/recording/RecordingController.cpp
    src/recording/RecordingControllerManifest.cpp
    src/recording/RecordingControllerState.cpp
    src/recording/SamplingScheduler.cpp
    src/recording/SessionManifest.cpp
    src/util/BinaryBuffer.cpp
    src/util/Identifier.cpp
    src/util/JsonWriter.cpp
    src/util/SteamVRRuntime.cpp
    src/util/SteamVRRuntimeProbeWin32Paths.cpp
    src/util/SteamVRRuntimeProbeWin32.cpp
    src/vr/MockVRProvider.cpp
    src/vr/OpenVRProvider.cpp
    src/vr/OpenVRProviderDevices.cpp
    src/vr/OpenVRProviderDetails.cpp
    src/vr/OpenVRProviderPolling.cpp
    src/vr/OpenVRProviderPoseConversion.cpp
)

target_include_directories(openvr_tracker_recorder_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(openvr_tracker_recorder_core PUBLIC cxx_std_20)

if(OVTR_HAS_OPENVR_SDK)
    target_compile_definitions(openvr_tracker_recorder_core PUBLIC OVTR_HAS_OPENVR_SDK=1)
    target_link_libraries(openvr_tracker_recorder_core PUBLIC openvr_api)
endif()

if(MSVC)
    target_compile_options(openvr_tracker_recorder_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(openvr_tracker_recorder_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(OpenVRTrackerRecorder
    src/main.cpp
)

target_link_libraries(OpenVRTrackerRecorder
    PRIVATE
        openvr_tracker_recorder_core
)

if(OVTR_HAS_OPENVR_SDK)
    add_custom_command(TARGET OpenVRTrackerRecorder POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:openvr_api>
            $<TARGET_FILE_DIR:OpenVRTrackerRecorder>
    )
endif()

if(WIN32 AND OVTR_BUILD_WIN32_BOOTSTRAP_APP)
    add_executable(OpenVRTrackerRecorderDesktop WIN32
        src/platform/win32/AppConfig.cpp
        src/platform/win32/AppDeviceNameConfig.cpp
        src/platform/win32/AppLog.cpp
        src/platform/win32/AppOriginConfig.cpp
        src/platform/win32/AppRecordSettingsConfig.cpp
        src/platform/win32/AppViewportSettingsConfig.cpp
        src/platform/win32/ConfigStore.cpp
        src/platform/win32/ConfigDeviceNamesText.cpp
        src/platform/win32/ConfigExportPaths.cpp
        src/platform/win32/ConfigNamedPaths.cpp
        src/platform/win32/ConfigOriginText.cpp
        src/platform/win32/ConfigRecordSettingsText.cpp
        src/platform/win32/ConfigText.cpp
        src/platform/win32/ConfigViewportSettingsClamp.cpp
        src/platform/win32/ConfigViewportSettingsSerialize.cpp
        src/platform/win32/ConfigViewportSettingsText.cpp
        src/platform/win32/DeviceActions.cpp
        src/platform/win32/DeviceListLayout.cpp
        src/platform/win32/DeviceListScrollLayout.cpp
        src/platform/win32/DeviceNameDialog.cpp
        src/platform/win32/DeviceNameDialogControls.cpp
        src/platform/win32/DeviceNameDialogPrompt.cpp
        src/platform/win32/DeviceNameDialogSession.cpp
        src/platform/win32/DeviceNameKeys.cpp
        src/platform/win32/DevicePanelPainter.cpp
        src/platform/win32/DevicePanelPainterAppStateAdapters.cpp
        src/platform/win32/DevicePanelRowPainter.cpp
        src/platform/win32/DevicePanelRowsPainter.cpp
        src/platform/win32/DevicePanelScrollbarPainter.cpp
        src/platform/win32/DebugPanel.cpp
        src/platform/win32/DebugLayout.cpp
        src/platform/win32/DebugMonitorBodyLayout.cpp
        src/platform/win32/DebugMonitorImportLines.cpp
        src/platform/win32/DebugMonitorLayout.cpp
        src/platform/win32/DebugMonitorOriginLines.cpp
        src/platform/win32/DebugMonitorRecordingLines.cpp
        src/platform/win32/DebugMonitorStatus.cpp
        src/platform/win32/DebugPanelLinePainter.cpp
        src/platform/win32/DebugPanelContentSections.cpp
        src/platform/win32/DebugPanelContentPainter.cpp
        src/platform/win32/DebugPanelPainter.cpp
        src/platform/win32/DebugScrollLayout.cpp
        src/platform/win32/DebugScrollbarPainter.cpp
        src/platform/win32/DeviceList.cpp
        src/platform/win32/DeviceListAppStateAdapters.cpp
        src/platform/win32/DeviceListRows.cpp
        src/platform/win32/DeviceLookup.cpp
        src/platform/win32/Dialogs.cpp
        src/platform/win32/FileDialogs.cpp
        src/platform/win32/DevicePanelTablePainter.cpp
        src/platform/win32/FrameUpdate.cpp
        src/platform/win32/ImportedSceneActions.cpp
        src/platform/win32/ImportedSceneControlActions.cpp
        src/platform/win32/ImportedSceneLifecycle.cpp
        src/platform/win32/ImportedScenePlayback.cpp
        src/platform/win32/ImportedScenePlaybackAppStateAdapters.cpp
        src/platform/win32/ImportedScenePlaybackControl.cpp
        src/platform/win32/ImportedScenePlaybackRecording.cpp
        src/platform/win32/ImportedScenePlaybackSeek.cpp
        src/platform/win32/ImportedSceneTimeline.cpp
        src/platform/win32/Layout.cpp
        src/platform/win32/LeftPanelLayout.cpp
        src/platform/win32/Menus.cpp
        src/platform/win32/ModalDialog.cpp
        src/platform/win32/OriginActions.cpp
        src/platform/win32/OriginAxisAdjustment.cpp
        src/platform/win32/OriginDialogActionControls.cpp
        src/platform/win32/OriginDialogControlState.cpp
        src/platform/win32/OriginDialog.cpp
        src/platform/win32/OriginDialogControlSections.cpp
        src/platform/win32/OriginDialogControls.cpp
        src/platform/win32/OriginDialogModel.cpp
        src/platform/win32/OriginDialogModelAppStateAdapters.cpp
        src/platform/win32/OriginDialogPreview.cpp
        src/platform/win32/OriginDialogPrompt.cpp
        src/platform/win32/OriginDialogSession.cpp
        src/platform/win32/OriginDialogValueControls.cpp
        src/platform/win32/OriginEditorActions.cpp
        src/platform/win32/OriginEditor.cpp
        src/platform/win32/OriginEditorLayout.cpp
        src/platform/win32/OriginEditorSubclass.cpp
        src/platform/win32/OriginFormatting.cpp
        src/platform/win32/OriginLayout.cpp
        src/platform/win32/OriginPanelPainter.cpp
        src/platform/win32/OriginPanelPainterAppStateAdapters.cpp
        src/platform/win32/OriginPoseState.cpp
        src/platform/win32/OriginSelectionNavigation.cpp
        src/platform/win32/OriginSelectionState.cpp
        src/platform/win32/OriginState.cpp
        src/platform/win32/OriginStateAppStateAdapters.cpp
        src/platform/win32/OriginStepperPainter.cpp
        src/platform/win32/OriginStepperLayout.cpp
        src/platform/win32/PanelLayout.cpp
        src/platform/win32/PaintWidgets.cpp
        src/platform/win32/PoseSamplingWorker.cpp
        src/platform/win32/RecordSettingsActionButtons.cpp
        src/platform/win32/RecordSettingsCaptureControls.cpp
        src/platform/win32/RecordSettingsDelayControls.cpp
        src/platform/win32/RecordSettingsDialog.cpp
        src/platform/win32/RecordSettingsDialogControlSections.cpp
        src/platform/win32/RecordSettingsDialogControls.cpp
        src/platform/win32/RecordSettingsDialogPrompt.cpp
        src/platform/win32/RecordSettingsDialogSession.cpp
        src/platform/win32/RecordSettingsExportFolderControls.cpp
        src/platform/win32/RecordSettingsFormatControls.cpp
        src/platform/win32/RecordSettingsModel.cpp
        src/platform/win32/RecordSettingsResampleControls.cpp
        src/platform/win32/RecordingCleanupActions.cpp
        src/platform/win32/RecordingExportDispatch.cpp
        src/platform/win32/RecordingExportMessages.cpp
        src/platform/win32/RecordingExportActions.cpp
        src/platform/win32/RecordingSessionActions.cpp
        src/platform/win32/RecordingExportPlan.cpp
        src/platform/win32/RecordingStartActions.cpp
        src/platform/win32/RecordingStartPlan.cpp
        src/platform/win32/RecordingStateQueries.cpp
        src/platform/win32/RecordingUiActions.cpp
        src/platform/win32/RuntimeDeviceRefresh.cpp
        src/platform/win32/RuntimeFpsCounters.cpp
        src/platform/win32/RuntimeStatus.cpp
        src/platform/win32/RuntimeStatusAppStateAdapters.cpp
        src/platform/win32/RuntimeStatusLogging.cpp
        src/platform/win32/StatusBarPainter.cpp
        src/platform/win32/StatusBarPainterAppStateAdapters.cpp
        src/platform/win32/StatusBarText.cpp
        src/platform/win32/StatusPanel.cpp
        src/platform/win32/StatusPanelAppStateAdapters.cpp
        src/platform/win32/TopMenuSettingsActions.cpp
        src/platform/win32/TopMenus.cpp
        src/platform/win32/TopBarLayout.cpp
        src/platform/win32/ViewportAnimationControlLayout.cpp
        src/platform/win32/ViewportAnimationFrameTextPainter.cpp
        src/platform/win32/ViewportAnimationControlPainter.cpp
        src/platform/win32/ViewportAnimationControlPainterAppStateAdapters.cpp
        src/platform/win32/ViewportAnimationControlSections.cpp
        src/platform/win32/ViewportAnimationTimelinePainter.cpp
        src/platform/win32/ViewportColorDialog.cpp
        src/platform/win32/ViewportColorDialogColorConversion.cpp
        src/platform/win32/ViewportColorDialogControlSections.cpp
        src/platform/win32/ViewportColorDialogControlState.cpp
        src/platform/win32/ViewportColorDialogControls.cpp
        src/platform/win32/ViewportColorDialogFooterControls.cpp
        src/platform/win32/ViewportColorDialogHeaderControls.cpp
        src/platform/win32/ViewportColorDialogPrompt.cpp
        src/platform/win32/ViewportColorDialogRows.cpp
        src/platform/win32/ViewportCamera.cpp
        src/platform/win32/ViewportCameraAppStateAdapters.cpp
        src/platform/win32/ViewportControlPainter.cpp
        src/platform/win32/ViewportControlPainterAppStateAdapters.cpp
        src/platform/win32/ViewportControlLayout.cpp
        src/platform/win32/ViewportDeviceMarker.cpp
        src/platform/win32/ViewportDrawPrimitives.cpp
        src/platform/win32/ViewportColorDialogSession.cpp
        src/platform/win32/ViewportGlColor.cpp
        src/platform/win32/ViewportGlFonts.cpp
        src/platform/win32/ViewportGlResources.cpp
        src/platform/win32/ViewportGridAxes.cpp
        src/platform/win32/ViewportImportedScenePrimitives.cpp
        src/platform/win32/ViewportImportedSceneRenderer.cpp
        src/platform/win32/ViewportImportedSceneRendererAppStateAdapters.cpp
        src/platform/win32/ViewportLabelText.cpp
        src/platform/win32/ViewportOverlayRenderer.cpp
        src/platform/win32/ViewportOverlayRendererAppStateAdapters.cpp
        src/platform/win32/ViewportQuaternionMatrix.cpp
        src/platform/win32/ViewportRenderModelCleanup.cpp
        src/platform/win32/ViewportRenderModelDraw.cpp
        src/platform/win32/ViewportRenderModelMatcap.cpp
        src/platform/win32/ViewportRenderModelRenderer.cpp
        src/platform/win32/ViewportRenderModelResources.cpp
        src/platform/win32/ViewportRenderModelTexture.cpp
        src/platform/win32/ViewportRenderer.cpp
        src/platform/win32/ViewportSceneRenderer.cpp
        src/platform/win32/ViewportSceneRendererAppStateAdapters.cpp
        src/platform/win32/ViewportCameraMath.cpp
        src/platform/win32/ViewportMath.cpp
        src/platform/win32/ViewportProjectionMath.cpp
        src/platform/win32/ViewportSettingsModel.cpp
        src/platform/win32/ViewportVectorMath.cpp
        src/platform/win32/ViewportWindow.cpp
        src/platform/win32/ViewportWindowInput.cpp
        src/platform/win32/WindowCameraKeyboard.cpp
        src/platform/win32/WindowClickActions.cpp
        src/platform/win32/WindowCaptureChanged.cpp
        src/platform/win32/WindowCommandKeyboard.cpp
        src/platform/win32/WindowCursor.cpp
        src/platform/win32/WindowDebugPanelClickActions.cpp
        src/platform/win32/WindowDeviceContextMenuActions.cpp
        src/platform/win32/WindowDeviceContextMenu.cpp
        src/platform/win32/WindowDevicePanelClickActions.cpp
        src/platform/win32/WindowDeviceRailPainter.cpp
        src/platform/win32/WindowDeviceRailPainterAppStateAdapters.cpp
        src/platform/win32/WindowDeviceStateLayout.cpp
        src/platform/win32/WindowDeviceStateLayoutAppStateAdapters.cpp
        src/platform/win32/WindowDrag.cpp
        src/platform/win32/WindowDragRelease.cpp
        src/platform/win32/WindowDebugStateLayout.cpp
        src/platform/win32/WindowDebugStateLayoutAppStateAdapters.cpp
        src/platform/win32/WindowInput.cpp
        src/platform/win32/WindowInvalidation.cpp
        src/platform/win32/WindowKeyboard.cpp
        src/platform/win32/WindowOriginKeyboard.cpp
        src/platform/win32/WindowChromePainter.cpp
        src/platform/win32/WindowLayout.cpp
        src/platform/win32/WindowOriginStateLayout.cpp
        src/platform/win32/WindowPainter.cpp
        src/platform/win32/WindowPaintFonts.cpp
        src/platform/win32/WindowPaintScene.cpp
        src/platform/win32/WindowPanelClickActions.cpp
        src/platform/win32/WindowScroll.cpp
        src/platform/win32/WindowStateLayout.cpp
        src/platform/win32/WindowTopBarClickActions.cpp
        src/platform/win32/WindowViewportStateLayout.cpp
        src/platform/win32/WindowViewportClickActions.cpp
        src/platform/win32/Win32AppLifecycle.cpp
        src/platform/win32/Win32AppWindow.cpp
        src/platform/win32/Win32AppWindowMessages.cpp
        src/platform/win32/Win32MessageLoop.cpp
        src/platform/win32/Win32String.cpp
        src/platform/win32/Win32WicImageLoader.cpp
        src/platform/win32/Win32WindowClasses.cpp
        src/platform/win32/Win32WindowCreation.cpp
        src/platform/win32/Win32BootstrapApp.cpp
    )

    target_link_libraries(OpenVRTrackerRecorderDesktop
        PRIVATE
            openvr_tracker_recorder_core
            user32
            gdi32
            comdlg32
            shell32
            ole32
            opengl32
            windowscodecs
            winmm
    )

    if(OVTR_HAS_OPENVR_SDK)
        add_custom_command(TARGET OpenVRTrackerRecorderDesktop POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:openvr_api>
                $<TARGET_FILE_DIR:OpenVRTrackerRecorderDesktop>
        )
    endif()
endif()

if(OVTR_BUILD_DESKTOP_APP)
    find_package(OpenGL REQUIRED)
    find_package(glfw3 CONFIG REQUIRED)
    find_package(glad CONFIG REQUIRED)

    add_library(imgui_vendor STATIC
        third_party/imgui/imgui.cpp
        third_party/imgui/imgui_draw.cpp
        third_party/imgui/imgui_tables.cpp
        third_party/imgui/imgui_widgets.cpp
        third_party/imgui/backends/imgui_impl_glfw.cpp
        third_party/imgui/backends/imgui_impl_opengl3.cpp
    )

    target_include_directories(imgui_vendor PUBLIC
        third_party/imgui
        third_party/imgui/backends
    )

    target_link_libraries(imgui_vendor PUBLIC glfw OpenGL::GL)

    target_compile_definitions(OpenVRTrackerRecorder PRIVATE OVTR_DESKTOP_APP=1)
    target_link_libraries(OpenVRTrackerRecorder PRIVATE imgui_vendor glfw glad::glad OpenGL::GL)
endif()

if(OVTR_BUILD_TESTS)
    enable_testing()

    add_executable(openvr_tracker_recorder_tests
        tests/FbxTestSupport.cpp
        tests/GltfImportTestSupport.cpp
        tests/TestFileSupport.cpp
        tests/TestFrameSupport.cpp
        tests/TestGeometrySupport.cpp
        tests/TestSupport.cpp
        tests/core_tests.cpp
        tests/test_binary_session.cpp
        tests/test_binary_session_guards.cpp
        tests/test_export_paths.cpp
        tests/test_fbx_animation_exporter.cpp
        tests/test_fbx_euler_exporter.cpp
        tests/test_fbx_exporter.cpp
        tests/test_fbx_geometry_exporter.cpp
        tests/test_fbx_math.cpp
        tests/test_glb_exporter.cpp
        tests/test_glb_import_accessors.cpp
        tests/test_gltf_exporter.cpp
        tests/test_identifier.cpp
        tests/test_importer.cpp
        tests/test_math.cpp
        tests/test_provider.cpp
        tests/test_recording.cpp
        tests/test_sampling_scheduler.cpp
        tests/test_session_manifest.cpp
    )

    if(WIN32)
        target_sources(openvr_tracker_recorder_tests
            PRIVATE
                src/platform/win32/ConfigStore.cpp
                src/platform/win32/ConfigDeviceNamesText.cpp
                src/platform/win32/ConfigExportPaths.cpp
                src/platform/win32/ConfigNamedPaths.cpp
                src/platform/win32/ConfigOriginText.cpp
                src/platform/win32/ConfigRecordSettingsText.cpp
                src/platform/win32/ConfigText.cpp
                src/platform/win32/ConfigViewportSettingsClamp.cpp
                src/platform/win32/ConfigViewportSettingsSerialize.cpp
                src/platform/win32/ConfigViewportSettingsText.cpp
                src/platform/win32/DebugPanel.cpp
                src/platform/win32/DebugLayout.cpp
                src/platform/win32/DebugMonitorBodyLayout.cpp
                src/platform/win32/DebugMonitorImportLines.cpp
                src/platform/win32/DebugMonitorLayout.cpp
                src/platform/win32/DebugMonitorOriginLines.cpp
                src/platform/win32/DebugMonitorRecordingLines.cpp
                src/platform/win32/DebugMonitorStatus.cpp
                src/platform/win32/DebugScrollLayout.cpp
                src/platform/win32/DeviceListLayout.cpp
                src/platform/win32/DeviceListScrollLayout.cpp
                src/platform/win32/DeviceList.cpp
                src/platform/win32/DeviceListAppStateAdapters.cpp
                src/platform/win32/DeviceListRows.cpp
                src/platform/win32/DeviceLookup.cpp
                src/platform/win32/DeviceNameKeys.cpp
                src/platform/win32/Dialogs.cpp
                src/platform/win32/ImportedSceneLifecycle.cpp
                src/platform/win32/ImportedScenePlayback.cpp
                src/platform/win32/ImportedScenePlaybackAppStateAdapters.cpp
                src/platform/win32/ImportedScenePlaybackControl.cpp
                src/platform/win32/ImportedScenePlaybackRecording.cpp
                src/platform/win32/ImportedScenePlaybackSeek.cpp
                src/platform/win32/ImportedSceneTimeline.cpp
                src/platform/win32/Layout.cpp
                src/platform/win32/LeftPanelLayout.cpp
                src/platform/win32/OriginDialogModel.cpp
                src/platform/win32/OriginDialogModelAppStateAdapters.cpp
                src/platform/win32/OriginEditorLayout.cpp
                src/platform/win32/OriginAxisAdjustment.cpp
                src/platform/win32/OriginFormatting.cpp
                src/platform/win32/OriginLayout.cpp
                src/platform/win32/OriginPoseState.cpp
                src/platform/win32/OriginSelectionNavigation.cpp
                src/platform/win32/OriginSelectionState.cpp
                src/platform/win32/OriginState.cpp
                src/platform/win32/OriginStateAppStateAdapters.cpp
                src/platform/win32/OriginStepperLayout.cpp
                src/platform/win32/PanelLayout.cpp
                src/platform/win32/PoseSamplingWorker.cpp
                src/platform/win32/RecordSettingsModel.cpp
                src/platform/win32/RecordingCleanupActions.cpp
                src/platform/win32/RecordingExportPlan.cpp
                src/platform/win32/RecordingExportMessages.cpp
                src/platform/win32/RecordingSessionActions.cpp
                src/platform/win32/RecordingStartPlan.cpp
                src/platform/win32/RecordingStateQueries.cpp
                src/platform/win32/RuntimeFpsCounters.cpp
                src/platform/win32/StatusPanel.cpp
                src/platform/win32/StatusPanelAppStateAdapters.cpp
                src/platform/win32/StatusBarText.cpp
                src/platform/win32/TopBarLayout.cpp
                src/platform/win32/ViewportAnimationControlLayout.cpp
                src/platform/win32/ViewportCamera.cpp
                src/platform/win32/ViewportControlLayout.cpp
                src/platform/win32/ViewportCameraMath.cpp
                src/platform/win32/ViewportMath.cpp
                src/platform/win32/ViewportProjectionMath.cpp
                src/platform/win32/ViewportSettingsModel.cpp
                src/platform/win32/ViewportVectorMath.cpp
                src/platform/win32/WindowDebugStateLayout.cpp
                src/platform/win32/WindowDebugStateLayoutAppStateAdapters.cpp
                src/platform/win32/WindowDeviceStateLayout.cpp
                src/platform/win32/WindowDeviceStateLayoutAppStateAdapters.cpp
                src/platform/win32/WindowLayout.cpp
                src/platform/win32/WindowOriginStateLayout.cpp
                src/platform/win32/WindowStateLayout.cpp
                src/platform/win32/WindowViewportStateLayout.cpp
                src/platform/win32/Win32String.cpp
                tests/test_win32_config_store_basics.cpp
                tests/test_win32_config_store.cpp
                tests/test_win32_debug_panel.cpp
                tests/test_win32_debug_layout.cpp
                tests/test_win32_device_list.cpp
                tests/test_win32_device_list_layout.cpp
                tests/test_win32_imported_scene_lifecycle.cpp
                tests/test_win32_imported_scene_playback.cpp
        tests/test_win32_layout.cpp
        tests/test_win32_origin_formatting.cpp
                tests/test_win32_origin_dialog_state.cpp
                tests/test_win32_origin_layout.cpp
                tests/test_win32_origin_state.cpp
                tests/test_win32_pose_sampling.cpp
                tests/test_win32_record_settings_config.cpp
                tests/test_win32_recording_actions.cpp
                tests/test_win32_recording_cleanup.cpp
                tests/test_win32_runtime_status.cpp
                tests/test_win32_status_panel.cpp
                tests/test_win32_string.cpp
                tests/test_win32_viewport_control_layout.cpp
                tests/test_win32_viewport_settings_config.cpp
                tests/test_win32_viewport_math.cpp
        )
        target_link_libraries(openvr_tracker_recorder_tests
            PRIVATE
                user32
                gdi32
                comdlg32
                shell32
                ole32
                opengl32
        )
    endif()

    target_link_libraries(openvr_tracker_recorder_tests
        PRIVATE
            openvr_tracker_recorder_core
    )

    if(OVTR_HAS_OPENVR_SDK)
        add_custom_command(TARGET openvr_tracker_recorder_tests POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:openvr_api>
                $<TARGET_FILE_DIR:openvr_tracker_recorder_tests>
        )
    endif()

    add_test(NAME core_tests COMMAND openvr_tracker_recorder_tests)
endif()
