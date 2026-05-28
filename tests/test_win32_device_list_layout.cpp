#include "TestCases.h"
#include "TestSupport.h"
#include "Win32LayoutTestSupport.h"

#include "platform/win32/Layout.h"

namespace ovtr::test {

void testWin32DeviceListLayout()
{
    const ovtr::win32::DeviceListLayout emptyDeviceList =
        ovtr::win32::deviceListLayoutForClient(true, 420, 764, false, 0, 0);
    require(emptyDeviceList.valid, "empty device list layout is valid");
    require(emptyDeviceList.visibleItemCount == 23, "empty device list visible item count");
    require(sameRect(emptyDeviceList.sessionBoxRect, 56, 44, 396, 84), "empty session box rect");
    require(sameRect(emptyDeviceList.boxRect, 56, 92, 396, 170), "empty device list box rect");
    require(sameRect(emptyDeviceList.headerRect, 68, 104, 384, 132), "empty device list header rect");
    require(sameRect(emptyDeviceList.contentRect, 68, 132, 384, 158), "empty device list content rect");

    const ovtr::win32::DeviceListLayout deviceList =
        ovtr::win32::deviceListLayoutForClient(true, 420, 764, false, 0, 3);
    require(deviceList.valid, "device list layout is valid");
    require(deviceList.visibleItemCount == 23, "device list visible item count");
    require(sameRect(deviceList.sessionBoxRect, 56, 44, 396, 84), "session box rect");
    require(sameRect(deviceList.boxRect, 56, 92, 396, 222), "device list box rect");
    require(sameRect(deviceList.headerRect, 68, 104, 384, 132), "device list header rect");
    require(sameRect(deviceList.contentRect, 68, 132, 384, 210), "device list content rect");

    const ovtr::win32::DeviceListLayout clippedDeviceList =
        ovtr::win32::deviceListLayoutForClient(true, 420, 764, true, 300, 20);
    require(clippedDeviceList.valid, "device list clips above origin panel");
    require(clippedDeviceList.visibleItemCount == 5, "clipped device list visible item count");
    require(sameRect(clippedDeviceList.boxRect, 56, 92, 396, 274), "clipped device list box rect");
    require(sameRect(clippedDeviceList.contentRect, 68, 132, 384, 262), "clipped device list content rect");
    require(
        ovtr::win32::maxDeviceListScrollOffset(20, clippedDeviceList.visibleItemCount) == 15,
        "device list max scroll offset"
    );
    require(
        ovtr::win32::clampDeviceListScrollOffset(-2, 20, clippedDeviceList.visibleItemCount) == 0,
        "device list clamps negative scroll"
    );
    require(
        ovtr::win32::clampDeviceListScrollOffset(30, 20, clippedDeviceList.visibleItemCount) == 15,
        "device list clamps large scroll"
    );
    require(
        ovtr::win32::deviceListItemTextRight(deviceList, 3) == 384,
        "device list text right without scrollbar"
    );
    require(
        ovtr::win32::deviceListItemTextRight(clippedDeviceList, 20) == 370,
        "device list text right reserves scrollbar"
    );
    require(
        ovtr::win32::deviceListRowIndexFromPoint(deviceList, POINT{70, 138}, 3, 0) == 0,
        "device list hit tests first row"
    );
    require(
        ovtr::win32::deviceListRowIndexFromPoint(deviceList, POINT{70, 190}, 3, 0) == 2,
        "device list hit tests third row"
    );
    require(
        ovtr::win32::deviceListRowIndexFromPoint(clippedDeviceList, POINT{70, 138}, 20, 5) == 5,
        "device list hit test includes scroll offset"
    );
    require(
        ovtr::win32::deviceListRowIndexFromPoint(clippedDeviceList, POINT{371, 138}, 20, 5) == -1,
        "device list hit test rejects scrollbar gutter"
    );
    require(
        ovtr::win32::deviceListRowIndexFromPoint(deviceList, POINT{70, 230}, 3, 0) == -1,
        "device list hit test rejects outside content"
    );

    require(
        !ovtr::win32::deviceListLayoutForClient(false, 420, 764, false, 0, 3).valid,
        "device list hidden when panel is hidden"
    );
    require(
        !ovtr::win32::deviceListLayoutForClient(true, 110, 764, false, 0, 3).valid,
        "device list rejects narrow panel"
    );
}

} // namespace ovtr::test
