#include "platform/win32/DebugMonitorStatusLines.h"

#include "platform/win32/AppImportedSceneState.h"
#include "platform/win32/AppRecordingState.h"
#include "platform/win32/Win32String.h"

#include <iomanip>
#include <sstream>

namespace ovtr::win32 {

void appendDebugMonitorImportLines(
    std::vector<std::wstring>& lines,
    const AppRecordingState& recordingState,
    const AppImportedSceneState& importedSceneState
)
{
    if (!recordingState.currentSessionFolder.empty()) {
        lines.emplace_back(L"Session folder: " + widen(recordingState.currentSessionFolder.string()));
    }
    if (importedSceneState.importedSceneLoaded) {
        std::wostringstream stream;
        stream << L"Imported GLB: " << widen(importedSceneState.importedScene.sourcePath.filename().string())
               << L"   Nodes: " << importedSceneState.importedScene.nodes.size()
               << L"   Meshes: " << importedSceneState.importedScene.meshes.size()
               << L"   Duration: " << std::fixed << std::setprecision(3)
               << importedSceneState.importedScene.durationSeconds << L"s";
        lines.emplace_back(stream.str());
    }
    if (!importedSceneState.importStatusMessage.empty()) {
        lines.emplace_back(L"Import status: " + widen(importedSceneState.importStatusMessage));
    }
}

} // namespace ovtr::win32
