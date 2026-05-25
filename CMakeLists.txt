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
    src/export/FbxAsciiExporter.cpp
    src/math/QuaternionUtils.cpp
    src/recording/BinarySessionReader.cpp
    src/recording/BinarySessionWriter.cpp
    src/recording/RecordingController.cpp
    src/recording/SamplingScheduler.cpp
    src/recording/SessionManifest.cpp
    src/util/SteamVRRuntime.cpp
    src/vr/MockVRProvider.cpp
    src/vr/OpenVRProvider.cpp
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
        src/platform/win32/Win32BootstrapApp.cpp
    )

    target_link_libraries(OpenVRTrackerRecorderDesktop
        PRIVATE
            openvr_tracker_recorder_core
            user32
            gdi32
            opengl32
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
        tests/core_tests.cpp
    )

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
