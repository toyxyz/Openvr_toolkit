#pragma once

namespace ovtr::win32 {

struct AppImportedSceneState;
struct AppViewportState;
struct AppWindowState;

void drawImportedGltfScene3D(
    const AppImportedSceneState& importedSceneState,
    const AppViewportState& viewportState
);
void drawImportedGltfScene3D(AppWindowState& state);

} // namespace ovtr::win32
