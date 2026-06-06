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
    require(!recordingControls.animationValid, "recording controls have no animation row");
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
            716
        ),
        "viewport render rect with recording controls"
    );

    const ovtr::win32::ViewportControlLayout animationControls =
        ovtr::win32::viewportControlLayoutForClient(420, 764, true, 1200, 800);
    require(animationControls.valid, "viewport animation layout base controls are valid");
    require(animationControls.animationValid, "viewport animation layout is valid");
    require(sameRect(animationControls.animationBarRect, 428, 668, 1200, 716), "animation bar rect");
    require(sameRect(animationControls.firstFrameButtonRect, 442, 677, 472, 707), "first frame rect");
    require(sameRect(animationControls.playPauseButtonRect, 480, 677, 510, 707), "play pause rect");
    require(sameRect(animationControls.lastFrameButtonRect, 518, 677, 548, 707), "last frame rect");
    require(sameRect(animationControls.timelineRect, 566, 682, 944, 702), "timeline rect");
    require(sameRect(animationControls.frameTextRect, 962, 668, 1104, 716), "frame text rect");
    require(sameRect(animationControls.closeButtonRect, 1116, 677, 1186, 707), "close button rect");
    require(
        sameRect(
            ovtr::win32::viewportRenderRectForClient(420, 764, animationControls, 1200),
            428,
            32,
            1200,
            668
        ),
        "viewport render rect with animation controls"
    );
}

} // namespace ovtr::test
