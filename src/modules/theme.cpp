/*
  Technical Standard

  Theme management implementation with Windows dark mode API integration support.
  Controls visual appearance of title bar, menu bar, editor, and status controls.
*/

#include <windows.h>
#include <uxtheme.h>
#include <richedit.h>
#include <algorithm>
#include "theme.h"
#include "core/types.h"
#include "core/globals.h"
#include "editor.h"
#include "resource.h"
#include "settings.h"

#ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#endif

#ifndef DWMWA_BORDER_COLOR
#define DWMWA_BORDER_COLOR 34
#endif

#ifndef DWMWA_CAPTION_COLOR
#define DWMWA_CAPTION_COLOR 35
#endif

#ifndef DWMWA_TEXT_COLOR
#define DWMWA_TEXT_COLOR 36
#endif

#ifndef DWMWA_SYSTEMBACKDROP_TYPE
#define DWMWA_SYSTEMBACKDROP_TYPE 38
#endif

#ifndef DWMWCP_ROUND
#define DWMWCP_ROUND 2
#endif

#ifndef DWMSBT_AUTO
#define DWMSBT_AUTO 0
#define DWMSBT_NONE 1
#define DWMSBT_MAINWINDOW 2
#define DWMSBT_TRANSIENTWINDOW 3
#define DWMSBT_TABBEDWINDOW 4
#endif

COLORREF ThemeColorEditorBackground(bool dark)
{
    return dark ? RGB(28, 30, 34) : RGB(247, 247, 248);
}

COLORREF ThemeColorEditorText(bool dark)
{
    return dark ? RGB(232, 235, 239) : RGB(28, 30, 34);
}

COLORREF ThemeColorStatusBackground(bool dark)
{
    return dark ? RGB(34, 37, 42) : RGB(242, 242, 244);
}

COLORREF ThemeColorStatusText(bool dark)
{
    return dark ? RGB(214, 218, 225) : RGB(64, 69, 77);
}

COLORREF ThemeColorMenuBackground(bool dark)
{
    return dark ? RGB(34, 37, 42) : RGB(242, 242, 244);
}

COLORREF ThemeColorMenuHoverBackground(bool dark)
{
    return dark ? RGB(52, 56, 64) : RGB(229, 231, 236);
}

COLORREF ThemeColorMenuText(bool dark)
{
    return dark ? RGB(233, 236, 242) : RGB(44, 47, 53);
}

COLORREF ThemeColorChromeBorder(bool dark)
{
    return dark ? RGB(66, 71, 80) : RGB(212, 215, 222);
}

bool SetTitleBarDark(HWND hwnd, BOOL dark)
{
    HMODULE hDwmapi = GetModuleHandleW(L"dwmapi.dll");
    const bool loadedNow = (hDwmapi == nullptr);
    if (!hDwmapi)
        hDwmapi = LoadLibraryW(L"dwmapi.dll");
    if (!hDwmapi)
        return false;

    typedef HRESULT(WINAPI * fnDwmSetWindowAttribute)(HWND, DWORD, LPCVOID, DWORD);
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
#endif
    auto dwmSetWindowAttribute = reinterpret_cast<fnDwmSetWindowAttribute>(GetProcAddress(hDwmapi, "DwmSetWindowAttribute"));
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

    bool applied = false;
    if (dwmSetWindowAttribute)
    {
        const DWORD attrs[] = {DWMWA_USE_IMMERSIVE_DARK_MODE, 19};
        for (DWORD attr : attrs)
        {
            if (SUCCEEDED(dwmSetWindowAttribute(hwnd, attr, &dark, sizeof(dark))))
                applied = true;
        }

        const DWORD cornerPref = DWMWCP_ROUND;
        dwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &cornerPref, sizeof(cornerPref));

        const DWORD backdropType = g_state.useTabs ? DWMSBT_TABBEDWINDOW : DWMSBT_MAINWINDOW;
        dwmSetWindowAttribute(hwnd, DWMWA_SYSTEMBACKDROP_TYPE, &backdropType, sizeof(backdropType));

        const COLORREF captionColor = dark ? RGB(32, 35, 40) : RGB(242, 242, 244);
        const COLORREF titleTextColor = dark ? RGB(244, 246, 250) : RGB(35, 38, 43);
        const COLORREF borderColor = ThemeColorChromeBorder(dark != FALSE);
        dwmSetWindowAttribute(hwnd, DWMWA_CAPTION_COLOR, &captionColor, sizeof(captionColor));
        dwmSetWindowAttribute(hwnd, DWMWA_TEXT_COLOR, &titleTextColor, sizeof(titleTextColor));
        dwmSetWindowAttribute(hwnd, DWMWA_BORDER_COLOR, &borderColor, sizeof(borderColor));
    }
    if (loadedNow)
        FreeLibrary(hDwmapi);
    return applied;
}

bool IsDarkMode()
{
    if (g_state.theme == Theme::Dark)
        return true;
    if (g_state.theme == Theme::Light)
        return false;
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        DWORD value = 1, size = sizeof(value);
        if (RegQueryValueExW(hKey, L"AppsUseLightTheme", nullptr, nullptr, reinterpret_cast<LPBYTE>(&value), &size) == ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            return value == 0;
        }
        RegCloseKey(hKey);
    }
    return false;
}

static void ApplyUxthemeDarkPreference(HWND hwnd, BOOL dark)
{
    if (!hwnd)
        return;

    HMODULE hUxtheme = GetModuleHandleW(L"uxtheme.dll");
    if (!hUxtheme)
        return;

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
#endif
    auto allowDarkModeForWindow = reinterpret_cast<fnAllowDarkModeForWindow>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(133)));
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
    if (allowDarkModeForWindow)
        allowDarkModeForWindow(hwnd, dark);
}

void ApplyThemeToWindowTree(HWND hwnd)
{
    if (!hwnd)
        return;

    const BOOL dark = IsDarkMode() ? TRUE : FALSE;
    ApplyUxthemeDarkPreference(hwnd, dark);
    SetWindowTheme(hwnd, dark ? L"DarkMode_Explorer" : nullptr, nullptr);
    SetTitleBarDark(hwnd, dark);

    for (HWND child = GetWindow(hwnd, GW_CHILD); child; child = GetWindow(child, GW_HWNDNEXT))
    {
        ApplyUxthemeDarkPreference(child, dark);
        SetWindowTheme(child, dark ? L"DarkMode_Explorer" : nullptr, nullptr);
    }
}

LRESULT CALLBACK StatusSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (IsDarkMode())
    {
        const COLORREF bgColor = ThemeColorStatusBackground(true);
        const COLORREF textColor = ThemeColorStatusText(true);
        const COLORREF borderColor = ThemeColorChromeBorder(true);
        if (msg == WM_ERASEBKGND)
        {
            RECT rc;
            GetClientRect(hwnd, &rc);
            HBRUSH hbr = g_hbrStatusDark ? g_hbrStatusDark : CreateSolidBrush(bgColor);
            FillRect(reinterpret_cast<HDC>(wParam), &rc, hbr);
            if (!g_hbrStatusDark)
                DeleteObject(hbr);
            return 1;
        }
        if (msg == WM_PAINT)
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rc;
            GetClientRect(hwnd, &rc);
            HBRUSH hbr = g_hbrStatusDark ? g_hbrStatusDark : CreateSolidBrush(bgColor);
            FillRect(hdc, &rc, hbr);
            if (!g_hbrStatusDark)
                DeleteObject(hbr);

            RECT topBorder = rc;
            topBorder.bottom = std::min(topBorder.bottom, topBorder.top + 1);
            HBRUSH hbrBorder = CreateSolidBrush(borderColor);
            if (hbrBorder)
            {
                FillRect(hdc, &topBorder, hbrBorder);
                DeleteObject(hbrBorder);
            }

            HFONT hFont = reinterpret_cast<HFONT>(SendMessageW(hwnd, WM_GETFONT, 0, 0));
            if (!hFont)
                hFont = reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
            HFONT hOldFont = reinterpret_cast<HFONT>(SelectObject(hdc, hFont));
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, textColor);
            int parts[4];
            int partCount = static_cast<int>(SendMessageW(hwnd, SB_GETPARTS, 4, reinterpret_cast<LPARAM>(parts)));
            const int horizontalPad = 8;
            int left = horizontalPad;
            for (int i = 0; i < partCount && i < 4; i++)
            {
                const int right = (parts[i] == -1) ? rc.right : parts[i];
                RECT rcPart = {left, rc.top + 2, right, rc.bottom - 2};
                wchar_t szText[256] = {};
                SendMessageW(hwnd, SB_GETTEXTW, i, reinterpret_cast<LPARAM>(szText));
                if (szText[0] != L'\0')
                    DrawTextW(hdc, szText, -1, &rcPart, DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_END_ELLIPSIS);
                if (right < rc.right - 1)
                {
                    RECT separator = {right, rc.top + 2, right + 1, rc.bottom - 2};
                    HBRUSH hbrSep = CreateSolidBrush(borderColor);
                    if (hbrSep)
                    {
                        FillRect(hdc, &separator, hbrSep);
                        DeleteObject(hbrSep);
                    }
                }
                left = right + horizontalPad;
            }
            SelectObject(hdc, hOldFont);
            EndPaint(hwnd, &ps);
            return 0;
        }
    }
    return CallWindowProcW(g_origStatusProc, hwnd, msg, wParam, lParam);
}

void ApplyTheme()
{
    BOOL dark = IsDarkMode();
    HMODULE hUxtheme = GetModuleHandleW(L"uxtheme.dll");
    if (hUxtheme)
    {
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
#endif
        auto allowDarkModeForApp = reinterpret_cast<fnAllowDarkModeForApp>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(132)));
        if (allowDarkModeForApp)
            allowDarkModeForApp(dark);
        auto setPreferredAppMode = reinterpret_cast<fnSetPreferredAppMode>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(135)));
        if (setPreferredAppMode)
            setPreferredAppMode(dark ? ForceDark : ForceLight);
        auto refreshPolicy = reinterpret_cast<fnRefreshImmersiveColorPolicyState>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(104)));
        if (refreshPolicy)
            refreshPolicy();
        auto flushMenuThemes = reinterpret_cast<fnFlushMenuThemes>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(136)));
        if (flushMenuThemes)
            flushMenuThemes();
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
    }
    if (g_hbrStatusDark)
    {
        DeleteObject(g_hbrStatusDark);
        g_hbrStatusDark = nullptr;
    }
    if (g_hbrMenuDark)
    {
        DeleteObject(g_hbrMenuDark);
        g_hbrMenuDark = nullptr;
    }
    if (g_hbrDialogDark)
    {
        DeleteObject(g_hbrDialogDark);
        g_hbrDialogDark = nullptr;
    }
    if (g_hbrDialogEditDark)
    {
        DeleteObject(g_hbrDialogEditDark);
        g_hbrDialogEditDark = nullptr;
    }
    if (dark)
    {
        g_hbrStatusDark = CreateSolidBrush(ThemeColorStatusBackground(true));
        g_hbrMenuDark = CreateSolidBrush(ThemeColorMenuBackground(true));
        g_hbrDialogDark = CreateSolidBrush(ThemeColorMenuBackground(true));
        g_hbrDialogEditDark = CreateSolidBrush(ThemeColorEditorBackground(true));
    }
    ApplyThemeToWindowTree(g_hwndMain);
    if (g_hwndTabs)
        SetWindowTheme(g_hwndTabs, dark ? L"" : L"Explorer", nullptr);
    COLORREF bgColor = ThemeColorEditorBackground(dark != FALSE);
    COLORREF textColor = ThemeColorEditorText(dark != FALSE);
    SendMessageW(g_hwndEditor, EM_SETBKGNDCOLOR, 0, bgColor);
    CHARFORMAT2W cf = {};
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_COLOR;
    cf.crTextColor = textColor;
    SendMessageW(g_hwndEditor, EM_SETCHARFORMAT, SCF_ALL, reinterpret_cast<LPARAM>(&cf));
    SendMessageW(g_hwndEditor, EM_SETCHARFORMAT, SCF_DEFAULT, reinterpret_cast<LPARAM>(&cf));
    ApplyEditorScrollbarChrome();
    SendMessageW(g_hwndStatus, SB_SETBKCOLOR, 0, dark ? ThemeColorStatusBackground(true) : CLR_DEFAULT);
    if (g_hwndFindDlg)
        ApplyThemeToWindowTree(g_hwndFindDlg);
    CheckMenuItem(GetMenu(g_hwndMain), IDM_VIEW_DARKMODE, MF_BYCOMMAND | (dark ? MF_CHECKED : MF_UNCHECKED));
    InvalidateRect(g_hwndEditor, nullptr, FALSE);
    InvalidateRect(g_hwndCommandBar, nullptr, FALSE);
    InvalidateRect(g_hwndTabs, nullptr, FALSE);
    InvalidateRect(g_hwndStatus, nullptr, FALSE);
    InvalidateRect(g_hwndMain, nullptr, FALSE);
    DrawMenuBar(g_hwndMain);
}

void ToggleDarkMode()
{
    g_state.theme = IsDarkMode() ? Theme::Light : Theme::Dark;
    ApplyTheme();
    SaveFontSettings();
}
