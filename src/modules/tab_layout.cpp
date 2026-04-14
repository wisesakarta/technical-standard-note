/*
  Solum

  Tab layout and DPI/font metrics controller.
*/

#include "tab_layout.h"

#include "core/globals.h"
#include "design_system.h"

#include <commctrl.h>
#include <algorithm>

namespace
{
int g_tabsDpi = 96;
HFONT g_hTabFontRegular = nullptr;
HFONT g_hTabFontActive = nullptr;
}

int TabScalePx(int px)
{
    return MulDiv(px, g_tabsDpi, 96);
}

void TabDestroyFonts()
{
    if (g_hTabFontRegular)
    {
        DeleteObject(g_hTabFontRegular);
        g_hTabFontRegular = nullptr;
    }
    if (g_hTabFontActive)
    {
        DeleteObject(g_hTabFontActive);
        g_hTabFontActive = nullptr;
    }
}

void TabRefreshDpi()
{
    if (!g_hwndMain)
        return;

    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    if (user32)
    {
        using fnGetDpiForWindow = UINT(WINAPI *)(HWND);
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
#endif
        auto getDpiForWindow = reinterpret_cast<fnGetDpiForWindow>(GetProcAddress(user32, "GetDpiForWindow"));
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
        if (getDpiForWindow)
        {
            HWND refDpi = g_hwndTabs ? g_hwndTabs : g_hwndMain;
            g_tabsDpi = static_cast<int>(getDpiForWindow(refDpi));
            if (g_tabsDpi > 0)
                return;
        }
    }

    HWND ref = g_hwndTabs ? g_hwndTabs : g_hwndMain;
    HDC hdc = GetDC(ref);
    if (!hdc)
    {
        g_tabsDpi = 96;
        return;
    }
    const int dpi = GetDeviceCaps(hdc, LOGPIXELSX);
    ReleaseDC(ref, hdc);
    g_tabsDpi = (dpi > 0) ? dpi : 96;
}

void TabRefreshVisualMetrics()
{
    if (!g_hwndTabs)
        return;

    TabDestroyFonts();

    LOGFONTW baseLf{};
    if (!SystemParametersInfoW(SPI_GETICONTITLELOGFONT, sizeof(baseLf), &baseLf, 0))
    {
        baseLf.lfHeight = -MulDiv(DesignSystem::kChromeFontPointSize, g_tabsDpi, 72);
        wcscpy_s(baseLf.lfFaceName, DesignSystem::kUiFontPrimary);
        baseLf.lfWeight = FW_NORMAL;
    }
    baseLf.lfHeight = -MulDiv(DesignSystem::kChromeFontPointSize, g_tabsDpi, 72);
    baseLf.lfWeight = FW_NORMAL;
    baseLf.lfQuality = CLEARTYPE_QUALITY;
    wcscpy_s(baseLf.lfFaceName, DesignSystem::kUiFontPrimary);

    g_hTabFontRegular = CreateFontIndirectW(&baseLf);
    if (!g_hTabFontRegular)
    {
        wcscpy_s(baseLf.lfFaceName, DesignSystem::kUiFontFallback);
        g_hTabFontRegular = CreateFontIndirectW(&baseLf);
    }

    LOGFONTW activeLf = baseLf;
    activeLf.lfWeight = FW_NORMAL;
    g_hTabFontActive = CreateFontIndirectW(&activeLf);
    if (!g_hTabFontActive)
    {
        wcscpy_s(activeLf.lfFaceName, DesignSystem::kUiFontFallback);
        g_hTabFontActive = CreateFontIndirectW(&activeLf);
    }

    HFONT effectiveFont = g_hTabFontRegular ? g_hTabFontRegular : reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
    SendMessageW(g_hwndTabs, WM_SETFONT, reinterpret_cast<WPARAM>(effectiveFont), TRUE);
    SendMessageW(g_hwndTabs,
                 TCM_SETPADDING,
                 0,
                 MAKELPARAM(TabScalePx(DesignSystem::kTabInnerPaddingHPx), TabScalePx(DesignSystem::kTabInnerPaddingVPx)));
    SendMessageW(g_hwndTabs,
                 TCM_SETITEMSIZE,
                 0,
                 MAKELPARAM(TabScalePx(DesignSystem::kTabFixedWidthPx), TabScalePx(DesignSystem::kChromeBandHeightPx)));
}

HFONT TabGetRegularFont()
{
    return g_hTabFontRegular;
}

HFONT TabGetActiveFont()
{
    return g_hTabFontActive;
}
