#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "platform/win32/ConfigTypes.h"
#include "platform/win32/RecordSettingsModel.h"

#include <filesystem>
#include <string>

namespace ovtr::win32 {

std::wstring formatIntegerText(int value);
std::wstring formatFloatText(float value);
void setEditText(HWND editWindow, const std::wstring& text);
std::wstring readWindowText(HWND window);
std::wstring readTrimmedWindowText(HWND window);
bool readIntegerEdit(HWND editWindow, int& value);
bool readFloatEdit(HWND editWindow, float& value);
bool readFiniteFloatEdit(HWND editWindow, float& value);

bool registerDeviceNameDialogClass(HINSTANCE instance);
bool promptForDeviceName(
    HWND parent,
    const std::wstring& deviceLabel,
    const std::wstring& initialName,
    std::wstring& outName
);
bool promptForMarkerName(
    HWND parent,
    const std::wstring& markerLabel,
    const std::wstring& initialName,
    std::wstring& outName
);

bool registerRecordSettingsDialogClass(HINSTANCE instance);
bool promptForRecordSettings(
    HWND parent,
    const RecordSettingsDialogInput& input,
    RecordSettingsDialogResult& outResult
);

bool registerStreamingSettingsDialogClass(HINSTANCE instance);
bool promptForStreamingSettings(
    HWND parent,
    const StreamingSettingsConfig& input,
    StreamingSettingsConfig& outResult
);

bool registerViewportColorDialogClass(HINSTANCE instance);
bool promptForViewportColorSettings(
    HWND parent,
    ViewportSettings initialSettings,
    ViewportSettings& outSettings
);

bool chooseExportDirectory(
    HWND owner,
    const std::filesystem::path& initialDirectory,
    std::filesystem::path& outDirectory
);
bool chooseImportGlbFile(
    HWND owner,
    const std::filesystem::path& initialDirectory,
    std::filesystem::path& outPath
);
bool chooseProfileFile(
    HWND owner,
    const std::filesystem::path& initialDirectory,
    std::filesystem::path& outPath
);

} // namespace ovtr::win32
