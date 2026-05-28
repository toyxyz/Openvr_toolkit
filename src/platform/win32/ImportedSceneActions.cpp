#include "platform/win32/ImportedSceneActions.h"

#include "import/GltfImporter.h"
#include "platform/win32/AppConfig.h"
#include "platform/win32/AppState.h"
#include "platform/win32/AppLog.h"
#include "platform/win32/Dialogs.h"
#include "platform/win32/ImportedScenePlayback.h"
#include "platform/win32/ViewportRenderer.h"
#include "platform/win32/WindowLayout.h"
#include "platform/win32/Win32String.h"

#include <chrono>
#include <filesystem>
#include <string>

namespace ovtr::win32 {

void importGlbFromFile(HWND hwnd, AppWindowState& state)
{
    std::filesystem::path selectedPath;
    if (!chooseImportGlbFile(hwnd, activeExportDirectoryPath(state), selectedPath)) {
        return;
    }

    state.exportStatusMessage.clear();
    appendDebugLog(state, L"Starting GLB import: " + selectedPath.wstring());
    ovtr::GltfImportResult result = ovtr::importGlbScene(selectedPath);
    if (!result.success) {
        state.importStatusMessage = "GLB import failed: " + result.error;
        appendDebugLog(state, state.importStatusMessage);
        MessageBoxW(hwnd, widen(state.importStatusMessage).c_str(), L"Import GLB", MB_OK | MB_ICONWARNING);
        invalidateStatusPanel(hwnd);
        InvalidateRect(hwnd, nullptr, FALSE);
        return;
    }

    state.importedScene = std::move(result.scene);
    state.importedSceneLoaded = true;
    state.importedScenePlaying = false;
    state.importedSceneTimelineDragging = false;
    state.importedScenePlaybackSeconds = 0.0;
    state.importedSceneLastUpdate = std::chrono::steady_clock::now();
    state.importStatusMessage = "GLB imported: " + selectedPath.filename().string();
    appendDebugLog(state, state.importStatusMessage);

    layoutChildWindows(hwnd);
    if (state.glWindow) {
        renderViewport(state.glWindow);
    }
    invalidateStatusPanel(hwnd);
    InvalidateRect(hwnd, nullptr, FALSE);
}

void closeImportedGlb(HWND hwnd, AppWindowState& state)
{
    if (!closeImportedScene(state)) {
        return;
    }

    appendDebugLog(state, state.importStatusMessage);
    layoutChildWindows(hwnd);
    invalidateWindowLayout(hwnd);
}

} // namespace ovtr::win32
