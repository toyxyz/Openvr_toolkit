#include "TestCases.h"
#include "TestSupport.h"
#include "Win32LayoutTestSupport.h"

#include "platform/win32/Layout.h"

namespace ovtr::test {

void testWin32ViewportControlLayout()
{
    const ovtr::win32::ViewportControlLayout recordingControls =
        ovtr::win32::viewportControlLayoutForClient(420, 764, false, 1200, 800);
    require(recordingControls.valid, "viewport control layout is valid");
    require(recordingControls.animationValid, "timeline stays visible before loading an animation");
    require(sameRect(recordingControls.barRect, 428, 716, 1200, 764), "viewport control bar rect");
    require(
        sameRect(recordingControls.quadViewButtonRect, 442, 717, 487, 762),
        "viewport quad view button rect"
    );
    require(
        sameRect(recordingControls.showTextButtonRect, 495, 717, 540, 762),
        "viewport show text button rect"
    );
    require(
        sameRect(recordingControls.showModelButtonRect, 548, 717, 593, 762),
        "viewport show model button rect"
    );
    require(
        sameRect(recordingControls.smoothButtonRect, 601, 717, 667, 762),
        "viewport smooth button rect"
    );
    require(
        sameRect(recordingControls.lockButtonRect, 675, 717, 727, 762),
        "viewport lock button rect"
    );
    require(
        sameRect(recordingControls.recordButtonRect, 791, 717, 836, 762),
        "viewport record button rect"
    );
    require(
        sameRect(recordingControls.sessionBoxRect, 916, 724, 1186, 756),
        "viewport session box rect"
    );
    require(
        sameRect(recordingControls.sessionValueRect, 994, 728, 1176, 752),
        "viewport session value rect"
    );
    require(
        sameRect(
            ovtr::win32::viewportRenderRectForClient(420, 764, recordingControls, 1200),
            428,
            32,
            1200,
            622
        ),
        "viewport render rect with recording controls"
    );

    const auto glbAndSession = ovtr::win32::viewportControlLayoutForClient(420, 764, true, 1200, 800, 0, true);
    require(glbAndSession.importedSceneBarValid, "loaded GLB has its own information bar");
    require(glbAndSession.importedSceneBarRect.bottom <= glbAndSession.animationBarRect.top,
        "GLB information bar is above transport and timeline");
    RECT overlap{};
    require(!IntersectRect(&overlap, &glbAndSession.importedSceneCloseButtonRect, &glbAndSession.closeButtonRect),
        "GLB and session close hit regions must not overlap");
    require(ovtr::win32::viewportRenderRectForClient(420, 764, glbAndSession, 1200).bottom ==
        glbAndSession.importedSceneBarRect.top, "GL viewport leaves room for the GLB bar");
    const auto narrowGlb = ovtr::win32::viewportControlLayoutForClient(100, 764, true, 720, 800, 48, true);
    require(narrowGlb.importedSceneBarValid && narrowGlb.importedSceneLabelRect.right <=
        narrowGlb.importedSceneCloseButtonRect.left, "GLB name leaves room for Close at narrower widths");
    require(!recordingControls.importedSceneBarValid, "no GLB information bar before importing a file");

    const ovtr::win32::ViewportControlLayout animationControls =
        ovtr::win32::viewportControlLayoutForClient(420, 764, true, 1200, 800);
    require(animationControls.valid, "viewport animation layout base controls are valid");
    require(animationControls.animationValid, "viewport animation layout is valid");
    require(sameRect(animationControls.animationBarRect, 428, 622, 1200, 716), "animation bar rect");
    require(sameRect(animationControls.firstFrameButtonRect, 776, 626, 800, 650), "first frame rect");
    require(sameRect(animationControls.playPauseButtonRect, 802, 626, 826, 650), "play pause rect");
    require(sameRect(animationControls.lastFrameButtonRect, 828, 626, 852, 650), "last frame rect");
    require(sameRect(animationControls.timelineRect, 442, 655, 1186, 711), "timeline rect");
    require(sameRect(animationControls.frameTextRect, 440, 626, 768, 650), "frame text rect");
    require(sameRect(animationControls.closeButtonRect, 1120, 626, 1190, 650), "close button rect");
    require(
        sameRect(
            ovtr::win32::viewportRenderRectForClient(420, 764, animationControls, 1200),
            428,
            32,
            1200,
            622
        ),
        "viewport render rect with animation controls"
    );
}

} // namespace ovtr::test
