#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>

#include "platform/win32/Win32AppWindow.h"

#include "platform/win32/FrameUpdate.h"

#include <chrono>

namespace ovtr::win32 {
namespace {

constexpr double kTargetViewportFps = 90.0;

} // namespace

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

} // namespace ovtr::win32
