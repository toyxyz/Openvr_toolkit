#include "platform/win32/DebugMonitorStatusLines.h"

#include "platform/win32/AppOriginState.h"
#include "platform/win32/AppRuntimeState.h"
#include "platform/win32/DeviceList.h"
#include "platform/win32/Win32String.h"

#include <iomanip>
#include <sstream>

namespace ovtr::win32 {

void appendDebugMonitorOriginLines(
    std::vector<std::wstring>& lines,
    const AppRuntimeState& runtimeState,
    const AppOriginState& originState
)
{
    std::wostringstream stream;
    stream << L"Origin: " << (originState.originEnabled ? L"Enabled" : L"Disabled");
    const ovtr::DeviceDescriptor* selected = selectedOriginDevice(runtimeState, originState);
    if (selected != nullptr) {
        stream << L"   Selected: " << widen(deviceDisplayName(*selected));
    }
    if (originState.originEnabled) {
        stream << std::fixed << std::setprecision(3)
               << L"   Offset: (" << originState.originOffset[0]
               << L", " << originState.originOffset[1]
               << L", " << originState.originOffset[2] << L")"
               << L"   Rotation: (" << originState.originRotationDegrees[0]
               << L", " << originState.originRotationDegrees[1]
               << L", " << originState.originRotationDegrees[2] << L")";
    }
    lines.emplace_back(stream.str());
    if (!originState.originStatusMessage.empty()) {
        lines.emplace_back(L"Origin status: " + widen(originState.originStatusMessage));
    }
}

} // namespace ovtr::win32
