#pragma once

#include "import/GltfImporter.h"

#include <chrono>
#include <string>

namespace ovtr::win32 {

struct AppImportedSceneState {
    std::string importStatusMessage;
    ImportedGltfScene importedScene;
    bool importedSceneLoaded = false;
    bool importedScenePlaying = false;
    bool importedSceneTimelineDragging = false;
    double importedScenePlaybackSeconds = 0.0;
    std::chrono::steady_clock::time_point importedSceneLastUpdate{};
};

} // namespace ovtr::win32
