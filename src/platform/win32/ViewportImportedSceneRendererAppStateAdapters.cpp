#include "platform/win32/ViewportImportedSceneRenderer.h"

#include "platform/win32/AppState.h"

namespace ovtr::win32 {

void drawImportedGltfScene3D(AppWindowState& state)
{
    drawImportedGltfScene3D(
        static_cast<const AppImportedSceneState&>(state),
        static_cast<AppViewportState&>(state)
    );
}

} // namespace ovtr::win32
