#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "platform/win32/Win32AppWindow.h"

#include "platform/win32/Dialogs.h"
#include "platform/win32/ExportProgressDialog.h"
#include "platform/win32/OriginDialog.h"
#include "platform/win32/ViewportWindow.h"
#include "platform/win32/Win32AppWindowInternal.h"

namespace ovtr::win32 {
namespace {

bool registerMainWindowClass(HINSTANCE instance)
{
    WNDCLASSW windowClass{};
    windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    windowClass.lpfnWndProc = mainWindowProc;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    windowClass.lpszClassName = kMainWindowClassName;
    return RegisterClassW(&windowClass) != 0;
}

bool registerViewportWindowClass(HINSTANCE instance)
{
    WNDCLASSW viewportClass{};
    viewportClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    viewportClass.lpfnWndProc = viewportProc;
    viewportClass.hInstance = instance;
    viewportClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    viewportClass.lpszClassName = kViewportWindowClassName;
    return RegisterClassW(&viewportClass) != 0;
}

} // namespace

bool registerBootstrapWindowClasses(HINSTANCE instance)
{
    const bool mainRegistered = registerMainWindowClass(instance);
    const bool viewportRegistered = registerViewportWindowClass(instance);

    registerDeviceNameDialogClass(instance);
    registerOriginDialogClass(instance);
    registerRecordSettingsDialogClass(instance);
    registerStreamingSettingsDialogClass(instance);
    registerViewportColorDialogClass(instance);
    registerExportProgressDialogClass(instance);

    return mainRegistered && viewportRegistered;
}

} // namespace ovtr::win32
