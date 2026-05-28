#include "TestCases.h"
#include "TestSupport.h"
#include "Win32LayoutTestSupport.h"

#include "platform/win32/Layout.h"

namespace ovtr::test {

void testWin32DebugLayout()
{
    require(ovtr::win32::maxDebugMonitorHeightForClient(600) == 352, "debug monitor max height");
    require(ovtr::win32::maxDebugMonitorHeightForClient(120) == 0, "debug monitor max clamps to zero");
    require(ovtr::win32::clampDebugMonitorHeightForClient(50, 600) == 120, "debug monitor min clamp");
    require(ovtr::win32::clampDebugMonitorHeightForClient(500, 600) == 352, "debug monitor max clamp");
    require(ovtr::win32::clampDebugMonitorHeightForClient(220, 600) == 220, "debug monitor preserves valid height");

    require(
        sameRect(ovtr::win32::debugButtonRectForClient(1000, 600), 892, 570, 968, 594),
        "debug button rect"
    );
    require(
        sameRect(ovtr::win32::debugMessagesRectForClient(220, 1000, 600), 498, 382, 968, 556),
        "debug messages rect"
    );
    require(
        sameRect(ovtr::win32::debugInfoRectForClient(220, 1000, 600), 32, 382, 462, 556),
        "debug info rect"
    );
    require(
        sameRect(ovtr::win32::debugResizeRectForClient(220, 1000, 600), 0, 344, 1000, 352),
        "debug resize rect"
    );
    require(
        sameRect(ovtr::win32::debugMessagesRectForClient(0, 1000, 600), 0, 0, 0, 0),
        "debug messages rect hidden"
    );
    require(
        ovtr::win32::visibleDebugLineCountForRect(RECT{0, 10, 100, 64}) == 3,
        "debug visible line count"
    );
    require(
        ovtr::win32::visibleDebugLineCountForRect(RECT{0, 64, 100, 10}) == 0,
        "debug visible line count rejects invalid rect"
    );
    require(
        ovtr::win32::maxDebugScrollOffset(20, 5) == 15,
        "debug max scroll offset"
    );
    require(
        ovtr::win32::clampDebugScrollOffset(-3, 20, 5) == 0,
        "debug clamps negative scroll"
    );
    require(
        ovtr::win32::clampDebugScrollOffset(30, 20, 5) == 15,
        "debug clamps large scroll"
    );

    const ovtr::win32::DebugScrollbarLayout debugScrollbar =
        ovtr::win32::debugScrollbarLayoutForRect(RECT{10, 20, 110, 200}, 20, 5, 3, false);
    require(debugScrollbar.valid, "debug scrollbar layout is valid");
    require(sameRect(debugScrollbar.trackRect, 102, 20, 110, 200), "debug scrollbar track rect");
    require(sameRect(debugScrollbar.thumbRect, 102, 47, 110, 92), "debug scrollbar thumb rect");

    const ovtr::win32::DebugScrollbarLayout reversedDebugScrollbar =
        ovtr::win32::debugScrollbarLayoutForRect(RECT{10, 20, 110, 200}, 20, 5, 3, true);
    require(
        sameRect(reversedDebugScrollbar.thumbRect, 102, 128, 110, 173),
        "debug reversed scrollbar thumb rect"
    );
    require(
        !ovtr::win32::debugScrollbarLayoutForRect(RECT{10, 20, 110, 200}, 3, 5, 0, false).valid,
        "debug scrollbar hidden when no scroll range"
    );
}

} // namespace ovtr::test
