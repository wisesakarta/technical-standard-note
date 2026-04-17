/*
  Otso

  Icon-related view actions (custom icon file, system icon picker, reset).
*/

#include "commands.h"

#include "core/file_dialog_filters.h"
#include "core/globals.h"
#include "settings.h"
#include "resource.h"
#include "lang/lang.h"

#include <commdlg.h>
#include <shlwapi.h>

namespace
{
using fnPickIconDlg = BOOL(WINAPI *)(HWND, LPWSTR, UINT, int *);

bool IsIconResourceContainer(const std::wstring &path)
{
    const wchar_t *ext = PathFindExtensionW(path.c_str());
    if (!ext || ext[0] == L'\0')
        return false;
    return lstrcmpiW(ext, L".dll") == 0 ||
           lstrcmpiW(ext, L".exe") == 0 ||
           lstrcmpiW(ext, L".icl") == 0 ||
           lstrcmpiW(ext, L".mun") == 0;
}

std::wstring DefaultSystemIconLibrary()
{
    wchar_t path[MAX_PATH] = {};
    DWORD len = ExpandEnvironmentStringsW(L"%SystemRoot%\\SystemResources\\imageres.dll.mun", path, MAX_PATH);
    if (len == 0 || len >= MAX_PATH || GetFileAttributesW(path) == INVALID_FILE_ATTRIBUTES)
    {
        len = ExpandEnvironmentStringsW(L"%SystemRoot%\\System32\\shell32.dll", path, MAX_PATH);
        if (len == 0 || len >= MAX_PATH)
            return L"shell32.dll";
    }
    return path;
}

HICON LoadIconFromPath(const std::wstring &path, int iconIndex)
{
    if (path.empty())
        return nullptr;

    const bool fileIconOnly = !IsIconResourceContainer(path);
    if (fileIconOnly)
    {
        return static_cast<HICON>(LoadImageW(nullptr, path.c_str(), IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE));
    }

    HICON hLarge = nullptr;
    HICON hSmall = nullptr;
    const UINT extracted = ExtractIconExW(path.c_str(), iconIndex, &hLarge, &hSmall, 1);
    if (extracted == 0)
        return nullptr;

    HICON chosen = hLarge ? hLarge : hSmall;
    if (!chosen)
        return nullptr;

    if (hLarge && hLarge != chosen)
        DestroyIcon(hLarge);
    if (hSmall && hSmall != chosen)
        DestroyIcon(hSmall);
    return chosen;
}

bool OpenSystemIconPicker(HWND owner, LPWSTR iconPath, UINT cchIconPath, int *iconIndex)
{
    HMODULE hShell32 = LoadLibraryW(L"shell32.dll");
    if (!hShell32)
        return false;

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
#endif
    auto pickIconDlg = reinterpret_cast<fnPickIconDlg>(GetProcAddress(hShell32, "PickIconDlg"));
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
    if (!pickIconDlg)
    {
        FreeLibrary(hShell32);
        return false;
    }

    BOOL ok = pickIconDlg(owner, iconPath, cchIconPath, iconIndex);
    FreeLibrary(hShell32);
    return ok != FALSE;
}
}

void ViewChangeIcon()
{
    const auto &lang = GetLangStrings();
    const std::wstring filter = BuildIconFilesFilter(lang);
    wchar_t path[MAX_PATH] = {0};
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g_hwndMain;
    ofn.lpstrFilter = filter.c_str();
    ofn.lpstrFile = path;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    if (GetOpenFileNameW(&ofn))
        ApplyCustomIcon(path, 0);
}

void ViewChooseSystemIcon()
{
    std::wstring initialPath = DefaultSystemIconLibrary();
    int iconIndex = 0;

    if (!g_state.customIconPath.empty() && IsIconResourceContainer(g_state.customIconPath))
    {
        initialPath = g_state.customIconPath;
        iconIndex = g_state.customIconIndex;
    }

    wchar_t iconPath[MAX_PATH] = {};
    wcsncpy_s(iconPath, initialPath.c_str(), _TRUNCATE);

    if (!OpenSystemIconPicker(g_hwndMain, iconPath, MAX_PATH, &iconIndex))
        return;

    ApplyCustomIcon(iconPath, iconIndex);
}

void ViewResetIcon()
{
    if (g_hCustomIcon)
    {
        DestroyIcon(g_hCustomIcon);
        g_hCustomIcon = nullptr;
    }
    g_state.customIconPath.clear();
    g_state.customIconIndex = 0;
    HICON hDefaultIcon = LoadIconW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDI_NOTEPAD));
    SendMessageW(g_hwndMain, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(hDefaultIcon));
    SendMessageW(g_hwndMain, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(hDefaultIcon));
    SaveFontSettings();
}

bool ApplyCustomIcon(const std::wstring &iconPath, int iconIndex, bool showError)
{
    const auto &lang = GetLangStrings();
    HICON hNewIcon = LoadIconFromPath(iconPath, iconIndex);
    if (!hNewIcon)
    {
        if (showError)
            MessageBoxW(g_hwndMain, lang.msgIconLoadFailed.c_str(), lang.appName.c_str(), MB_ICONERROR);
        return false;
    }

    if (g_hCustomIcon && g_hCustomIcon != hNewIcon)
        DestroyIcon(g_hCustomIcon);

    g_hCustomIcon = hNewIcon;
    g_state.customIconPath = iconPath;
    g_state.customIconIndex = iconIndex;
    SendMessageW(g_hwndMain, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(hNewIcon));
    SendMessageW(g_hwndMain, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(hNewIcon));
    SaveFontSettings();
    return true;
}
