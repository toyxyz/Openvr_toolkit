#include "platform/win32/ImportedScenePlayback.h"

#include "platform/win32/AppImportedSceneState.h"

#include <string>

namespace ovtr::win32 {

bool closeImportedScene(AppImportedSceneState& state)
{
    if (!state.importedSceneLoaded) {
        return false;
    }

    const std::string fileName = state.importedScene.sourcePath.filename().string();
    state.importedScene = {};
    state.importedSceneLoaded = false;
    state.importedScenePlaying = false;
    state.importedSceneTimelineDragging = false;
    state.importedScenePlaybackSeconds = 0.0;
    state.importedSceneLastUpdate = {};
    state.importStatusMessage = fileName.empty()
        ? "GLB import closed"
        : "GLB import closed: " + fileName;
    return true;
}

} // namespace ovtr::win32
