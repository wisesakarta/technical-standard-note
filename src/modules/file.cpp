/*
  Technical Standard

  File I/O operations with multi-encoding support (UTF-8, UTF-16, ANSI).
  Handles BOM detection, line ending conversion, and recent files tracking.
*/

#include "file.h"
#include "core/globals.h"
#include "lang/lang.h"
#include "editor.h"
#include "ui.h"
#include "settings.h"
#include "resource.h"
#include <shlwapi.h>
#include <algorithm>
#include <cwctype>

static bool ShouldUseLargeFileMode(size_t bytes)
{
    return bytes >= LARGE_FILE_MODE_THRESHOLD_BYTES;
}

static std::wstring NormalizeRecentPath(const std::wstring &path)
{
    if (path.empty())
        return {};

    wchar_t fullPath[MAX_PATH] = {};
    DWORD len = GetFullPathNameW(path.c_str(), MAX_PATH, fullPath, nullptr);
    std::wstring normalized = (len > 0 && len < MAX_PATH) ? std::wstring(fullPath) : path;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), towlower);
    return normalized;
}

static std::wstring ToWin32IoPath(const std::wstring &path)
{
    if (path.empty())
        return path;
    if (path.rfind(L"\\\\?\\", 0) == 0)
        return path;

    if (path.rfind(L"\\\\", 0) == 0)
        return L"\\\\?\\UNC\\" + path.substr(2);

    if (path.size() >= MAX_PATH && path.size() > 2 && path[1] == L':' &&
        (path[2] == L'\\' || path[2] == L'/'))
        return L"\\\\?\\" + path;

    return path;
}

const wchar_t *GetEncodingName(Encoding e)
{
    const auto &lang = GetLangStrings();
    switch (e)
    {
    case Encoding::UTF8:
        return lang.encodingUTF8.c_str();
    case Encoding::UTF8BOM:
        return lang.encodingUTF8BOM.c_str();
    case Encoding::UTF16LE:
        return lang.encodingUTF16LE.c_str();
    case Encoding::UTF16BE:
        return lang.encodingUTF16BE.c_str();
    case Encoding::ANSI:
        return lang.encodingANSI.c_str();
    }
    return L"";
}

const wchar_t *GetLineEndingName(LineEnding le)
{
    const auto &lang = GetLangStrings();
    switch (le)
    {
    case LineEnding::CRLF:
        return lang.lineEndingCRLF.c_str();
    case LineEnding::LF:
        return lang.lineEndingLF.c_str();
    case LineEnding::CR:
        return lang.lineEndingCR.c_str();
    }
    return L"";
}

bool LoadFile(const std::wstring &path)
{
    const auto &lang = GetLangStrings();
    const std::wstring ioPath = ToWin32IoPath(path);
    HANDLE hFile = CreateFileW(ioPath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        MessageBoxW(g_hwndMain, lang.msgCannotOpenFile.c_str(), lang.msgError.c_str(), MB_ICONERROR);
        return false;
    }
    LARGE_INTEGER fileSize = {};
    if (!GetFileSizeEx(hFile, &fileSize) || fileSize.QuadPart < 0)
    {
        CloseHandle(hFile);
        MessageBoxW(g_hwndMain, lang.msgCannotOpenFile.c_str(), lang.msgError.c_str(), MB_ICONERROR);
        return false;
    }
    if (fileSize.QuadPart > static_cast<LONGLONG>(MAXDWORD))
    {
        CloseHandle(hFile);
        MessageBoxW(g_hwndMain, lang.msgFileTooLargeOpen.c_str(), lang.msgError.c_str(), MB_ICONERROR);
        return false;
    }

    const DWORD size = static_cast<DWORD>(fileSize.QuadPart);
    std::vector<BYTE> data(size);
    DWORD totalRead = 0;
    while (totalRead < size)
    {
        DWORD chunkRead = 0;
        if (!ReadFile(hFile, data.data() + totalRead, size - totalRead, &chunkRead, nullptr))
        {
            CloseHandle(hFile);
            MessageBoxW(g_hwndMain, lang.msgCannotOpenFile.c_str(), lang.msgError.c_str(), MB_ICONERROR);
            return false;
        }
        if (chunkRead == 0)
        {
            CloseHandle(hFile);
            MessageBoxW(g_hwndMain, lang.msgCannotOpenFile.c_str(), lang.msgError.c_str(), MB_ICONERROR);
            return false;
        }
        totalRead += chunkRead;
    }
    CloseHandle(hFile);
    auto [enc, le] = DetectEncoding(data);
    std::wstring text = DecodeText(data, enc);

    const bool largeFileMode = ShouldUseLargeFileMode(static_cast<size_t>(size));
    const bool wrapModeChanged = (g_state.largeFileMode != largeFileMode);
    g_state.largeFileMode = largeFileMode;
    g_state.largeFileBytes = static_cast<size_t>(size);
    if (wrapModeChanged)
        ApplyWordWrap();

    SetEditorText(text);
    g_state.filePath = path;
    g_state.encoding = enc;
    g_state.lineEnding = le;
    g_state.modified = false;
    UpdateTitle();
    UpdateStatus();
    AddRecentFile(path);
    return true;
}

void SaveToPath(const std::wstring &path)
{
    const auto &lang = GetLangStrings();
    std::wstring text = GetEditorText();
    std::vector<BYTE> data = EncodeText(text, g_state.encoding, g_state.lineEnding);
    if (data.size() > static_cast<size_t>(MAXDWORD))
    {
        MessageBoxW(g_hwndMain, lang.msgFileTooLargeSave.c_str(), lang.msgError.c_str(), MB_ICONERROR);
        return;
    }

    const std::wstring ioPath = ToWin32IoPath(path);
    HANDLE hFile = CreateFileW(ioPath.c_str(), GENERIC_WRITE, 0, nullptr,
                               CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        MessageBoxW(g_hwndMain, lang.msgCannotSaveFile.c_str(), lang.msgError.c_str(), MB_ICONERROR);
        return;
    }
    const DWORD bytesToWrite = static_cast<DWORD>(data.size());
    DWORD totalWritten = 0;
    while (totalWritten < bytesToWrite)
    {
        DWORD chunkWritten = 0;
        if (!WriteFile(hFile, data.data() + totalWritten, bytesToWrite - totalWritten, &chunkWritten, nullptr))
        {
            CloseHandle(hFile);
            MessageBoxW(g_hwndMain, lang.msgCannotSaveFile.c_str(), lang.msgError.c_str(), MB_ICONERROR);
            return;
        }
        if (chunkWritten == 0)
        {
            CloseHandle(hFile);
            MessageBoxW(g_hwndMain, lang.msgCannotSaveFile.c_str(), lang.msgError.c_str(), MB_ICONERROR);
            return;
        }
        totalWritten += chunkWritten;
    }
    CloseHandle(hFile);

    const bool largeFileMode = ShouldUseLargeFileMode(static_cast<size_t>(bytesToWrite));
    const bool wrapModeChanged = (g_state.largeFileMode != largeFileMode);
    g_state.largeFileMode = largeFileMode;
    g_state.largeFileBytes = static_cast<size_t>(bytesToWrite);
    if (wrapModeChanged)
        ApplyWordWrap();

    g_state.filePath = path;
    g_state.modified = false;
    UpdateTitle();
    AddRecentFile(path);
}

void AddRecentFile(const std::wstring &path)
{
    const std::wstring normalizedPath = NormalizeRecentPath(path);
    if (normalizedPath.empty())
        return;

    auto it = std::find_if(g_state.recentFiles.begin(), g_state.recentFiles.end(),
                           [&](const std::wstring &existing)
                           { return NormalizeRecentPath(existing) == normalizedPath; });
    if (it != g_state.recentFiles.end())
        g_state.recentFiles.erase(it);
    g_state.recentFiles.push_front(path);
    while (g_state.recentFiles.size() > MAX_RECENT_FILES)
        g_state.recentFiles.pop_back();
    UpdateRecentFilesMenu();
    SaveFontSettings();
}

void UpdateRecentFilesMenu()
{
    HMENU hMenu = GetMenu(g_hwndMain);
    if (!hMenu)
        return;
    HMENU hFileMenu = GetSubMenu(hMenu, 0);
    if (!hFileMenu)
        return;
    static HMENU hRecentMenu = nullptr;
    if (hRecentMenu)
    {
        int count = GetMenuItemCount(hFileMenu);
        for (int i = 0; i < count; ++i)
        {
            if (GetSubMenu(hFileMenu, i) == hRecentMenu)
            {
                RemoveMenu(hFileMenu, static_cast<UINT>(i), MF_BYPOSITION);
                break;
            }
        }
        DestroyMenu(hRecentMenu);
        hRecentMenu = nullptr;
    }
    if (g_state.recentFiles.empty())
        return;
    const auto &lang = GetLangStrings();
    hRecentMenu = CreatePopupMenu();
    int id = IDM_FILE_RECENT_BASE;
    for (const auto &file : g_state.recentFiles)
    {
        std::wstring display = PathFindFileNameW(file.c_str());
        AppendMenuW(hRecentMenu, MF_STRING, id++, display.c_str());
    }
    int insertPos = GetMenuItemCount(hFileMenu);
    MENUITEMINFOW mii = {};
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_ID;
    for (int i = 0; i < insertPos; ++i)
    {
        if (GetMenuItemInfoW(hFileMenu, static_cast<UINT>(i), TRUE, &mii) && mii.wID == IDM_FILE_PRINT)
        {
            insertPos = i;
            break;
        }
    }
    InsertMenuW(hFileMenu, static_cast<UINT>(insertPos), MF_BYPOSITION | MF_POPUP, reinterpret_cast<UINT_PTR>(hRecentMenu), lang.menuRecentFiles.c_str());
}
