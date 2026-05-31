#include "platform/win32/Dialogs.h"

#include "platform/win32/Win32ComResources.h"

#include <commdlg.h>
#include <shlobj.h>

#include <array>

namespace ovtr::win32 {
namespace {

int CALLBACK browseFolderCallback(HWND hwnd, UINT message, LPARAM, LPARAM data)
{
    if (message == BFFM_INITIALIZED && data != 0) {
        SendMessageW(hwnd, BFFM_SETSELECTIONW, TRUE, data);
    }
    return 0;
}

} // namespace

bool chooseExportDirectory(
    HWND owner,
    const std::filesystem::path& initialDirectory,
    std::filesystem::path& outDirectory
)
{
    const std::wstring initialText = initialDirectory.wstring();
    wchar_t displayName[MAX_PATH]{};
    BROWSEINFOW browse{};
    browse.hwndOwner = owner;
    browse.pszDisplayName = displayName;
    browse.lpszTitle = L"Select export folder";
    browse.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE | BIF_EDITBOX;
    browse.lpfn = browseFolderCallback;
    browse.lParam = reinterpret_cast<LPARAM>(initialText.c_str());

    const OleInitializer ole;
    CoTaskMemItemList itemList(SHBrowseForFolderW(&browse));
    if (!itemList.get()) {
        return false;
    }

    wchar_t pathBuffer[MAX_PATH]{};
    if (!SHGetPathFromIDListW(itemList.get(), pathBuffer)) {
        return false;
    }

    outDirectory = std::filesystem::path(pathBuffer);
    return true;
}

bool chooseImportGlbFile(
    HWND owner,
    const std::filesystem::path& initialDirectory,
    std::filesystem::path& outPath
)
{
    std::array<wchar_t, MAX_PATH> fileName{};
    const std::wstring initialDirectoryText = initialDirectory.wstring();

    OPENFILENAMEW openFile{};
    openFile.lStructSize = sizeof(openFile);
    openFile.hwndOwner = owner;
    openFile.lpstrFilter = L"glTF Binary (*.glb)\0*.glb\0All Files (*.*)\0*.*\0";
    openFile.lpstrFile = fileName.data();
    openFile.nMaxFile = static_cast<DWORD>(fileName.size());
    openFile.lpstrInitialDir = initialDirectoryText.empty() ? nullptr : initialDirectoryText.c_str();
    openFile.lpstrTitle = L"Import GLB";
    openFile.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    openFile.lpstrDefExt = L"glb";

    if (!GetOpenFileNameW(&openFile)) {
        return false;
    }

    outPath = std::filesystem::path(fileName.data());
    return !outPath.empty();
}

bool chooseProfileFile(
    HWND owner,
    const std::filesystem::path& initialDirectory,
    std::filesystem::path& outPath
)
{
    std::array<wchar_t, MAX_PATH> fileName{};
    const std::wstring initialDirectoryText = initialDirectory.wstring();

    OPENFILENAMEW openFile{};
    openFile.lStructSize = sizeof(openFile);
    openFile.hwndOwner = owner;
    openFile.lpstrFilter = L"Profile (*.profile)\0*.profile\0All Files (*.*)\0*.*\0";
    openFile.lpstrFile = fileName.data();
    openFile.nMaxFile = static_cast<DWORD>(fileName.size());
    openFile.lpstrInitialDir = initialDirectoryText.empty() ? nullptr : initialDirectoryText.c_str();
    openFile.lpstrTitle = L"Load Profile";
    openFile.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    openFile.lpstrDefExt = L"profile";

    if (!GetOpenFileNameW(&openFile)) {
        return false;
    }

    outPath = std::filesystem::path(fileName.data());
    return !outPath.empty();
}

} // namespace ovtr::win32
