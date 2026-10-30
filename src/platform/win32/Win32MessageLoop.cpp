#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>

#include "platform/win32/Win32AppWindow.h"
#include "app/FrameLoopSchedule.h"

#include "platform/win32/FrameUpdate.h"

#include <chrono>
#include <stdexcept>

namespace ovtr::win32 {
int runMessageAndFrameLoop(HWND hwnd)
{
    timeBeginPeriod(1);

    MSG message{};
    bool running = true;
    auto nextFrame = std::chrono::steady_clock::now();

    while (running && IsWindow(hwnd)) {
        const auto batchStarted = std::chrono::steady_clock::now();
        while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
            if (message.message == WM_QUIT) {
                running = false;
                break;
            }

            TranslateMessage(&message);
            DispatchMessageW(&message);
            if (ovtr::shouldYieldMessages(std::chrono::steady_clock::now(), batchStarted, nextFrame)) {
                break;
            }
        }

        if (!running || !IsWindow(hwnd)) {
            break;
        }

        const auto now = std::chrono::steady_clock::now();
        if (now >= nextFrame) {
            refreshPoseAndViewport(hwnd);

            nextFrame = ovtr::nextViewportFrame(now, std::chrono::steady_clock::now());
        }

        const auto afterWork = std::chrono::steady_clock::now();
        const auto timeUntilNextFrame = nextFrame - afterWork;
        if (timeUntilNextFrame > ovtr::FrameClock::duration::zero()) {
            const auto timeout = std::chrono::ceil<std::chrono::milliseconds>(timeUntilNextFrame);
            if (MsgWaitForMultipleObjectsEx(0, nullptr, static_cast<DWORD>(timeout.count()),
                QS_ALLINPUT, MWMO_INPUTAVAILABLE) == WAIT_FAILED) {
                throw std::runtime_error("Waiting for UI input failed: " + std::to_string(GetLastError()));
            }
        }
    }

    timeEndPeriod(1);
    return static_cast<int>(message.wParam);
}

} // namespace ovtr::win32
