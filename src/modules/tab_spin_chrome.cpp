/*
  Solum

  Tab overflow spin-button custom chrome implementation.
*/

#include "tab_spin_chrome.h"

#include <windowsx.h>
#include "theme.h"

#include <algorithm>
#include <gdiplus.h>
#include "background.h"

namespace
{
WNDPROC g_origTabSpinProc = nullptr;
HWND g_hwndTabSpin = nullptr;
bool g_tabSpinHoverLeft = false;
bool g_tabSpinHoverRight = false;
bool g_tabSpinPressLeft = false;
bool g_tabSpinPressRight = false;
bool g_trackingTabSpinMouse = false;

void FillSolidRectDc(HDC hdc, const RECT &rc, COLORREF color)
{
    HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(DC_BRUSH));
    const COLORREF oldBrushColor = SetDCBrushColor(hdc, color);
    FillRect(hdc, &rc, reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
    SetDCBrushColor(hdc, oldBrushColor);
    SelectObject(hdc, oldBrush);
}

LRESULT CALLBACK TabSpinSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_PAINT:
    {
        PAINTSTRUCT ps{};
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT rc{};
        GetClientRect(hwnd, &rc);

        const bool dark = IsDarkMode();
        const TabPaintPalette palette = GetTabPaintPalette(dark);

        FillSolidRectDc(hdc, rc, palette.stripBg);

        const int halfW = (rc.right - rc.left) / 2;
        RECT rcLeft = rc;
        rcLeft.right = rc.left + halfW;
        RECT rcRight = rc;
        rcRight.left = rcLeft.right;

        auto drawSpinButton = [&](const RECT &r, bool hover, bool press, bool isLeft)
        {
            COLORREF bg = palette.stripBg;
            if (press)
                bg = palette.inactiveBg;
            else if (hover)
                bg = palette.hoverBg;

            FillSolidRectDc(hdc, r, bg);

            if (!EnsureBackgroundGraphicsReady())
                return;

            Gdiplus::Graphics graphics(hdc);
            graphics.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);

            const float cx = static_cast<float>(r.left) + static_cast<float>(r.right - r.left) / 2.0f;
            const float cy = static_cast<float>(r.top) + static_cast<float>(r.bottom - r.top) / 2.0f;

            Gdiplus::PointF pts[3];
            const float spread = 4.5f;
            const float depth = 2.0f;

            if (isLeft)
            {
                pts[0] = { cx + depth, cy - spread };
                pts[1] = { cx - depth, cy };
                pts[2] = { cx + depth, cy + spread };
            }
            else
            {
                pts[0] = { cx - depth, cy - spread };
                pts[1] = { cx + depth, cy };
                pts[2] = { cx - depth, cy + spread };
            }

            Gdiplus::Color gdiColor;
            gdiColor.SetFromCOLORREF(palette.textColor);
            Gdiplus::Pen pen(gdiColor, 1.6f);
            pen.SetStartCap(Gdiplus::LineCapRound);
            pen.SetEndCap(Gdiplus::LineCapRound);
            pen.SetLineJoin(Gdiplus::LineJoinRound);

            graphics.DrawLines(&pen, pts, 3);
        };

        drawSpinButton(rcLeft, g_tabSpinHoverLeft, g_tabSpinPressLeft, true);
        drawSpinButton(rcRight, g_tabSpinHoverRight, g_tabSpinPressRight, false);

        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_MOUSEMOVE:
    {
        POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        RECT rc{};
        GetClientRect(hwnd, &rc);
        const int halfW = (rc.right - rc.left) / 2;

        const bool hLeft = (pt.x < rc.left + halfW);
        const bool hRight = (pt.x >= rc.left + halfW);

        if (hLeft != g_tabSpinHoverLeft || hRight != g_tabSpinHoverRight)
        {
            g_tabSpinHoverLeft = hLeft;
            g_tabSpinHoverRight = hRight;
            InvalidateRect(hwnd, nullptr, FALSE);
        }

        if (!g_trackingTabSpinMouse)
        {
            TRACKMOUSEEVENT tme{};
            tme.cbSize = sizeof(tme);
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = hwnd;
            TrackMouseEvent(&tme);
            g_trackingTabSpinMouse = true;
        }
        break;
    }
    case WM_MOUSELEAVE:
    {
        g_trackingTabSpinMouse = false;
        g_tabSpinHoverLeft = false;
        g_tabSpinHoverRight = false;
        InvalidateRect(hwnd, nullptr, FALSE);
        break;
    }
    case WM_LBUTTONDOWN:
    {
        POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        RECT rc{};
        GetClientRect(hwnd, &rc);
        const int halfW = (rc.right - rc.left) / 2;

        if (pt.x < rc.left + halfW)
            g_tabSpinPressLeft = true;
        else
            g_tabSpinPressRight = true;

        InvalidateRect(hwnd, nullptr, FALSE);
        break;
    }
    case WM_LBUTTONUP:
    {
        g_tabSpinPressLeft = false;
        g_tabSpinPressRight = false;
        InvalidateRect(hwnd, nullptr, FALSE);
        break;
    }
    case WM_ERASEBKGND:
        return 1;
    }

    if (g_origTabSpinProc)
        return CallWindowProcW(g_origTabSpinProc, hwnd, msg, wParam, lParam);
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}
} // namespace

void TabSpinAttachIfNeeded(HWND hwndTabs)
{
    if (!hwndTabs)
        return;

    HWND hSpin = FindWindowExW(hwndTabs, nullptr, L"msctls_updown32", nullptr);
    if (hSpin && hSpin != g_hwndTabSpin)
    {
        if (g_hwndTabSpin && g_origTabSpinProc)
            SetWindowLongPtrW(g_hwndTabSpin, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(g_origTabSpinProc));

        g_hwndTabSpin = hSpin;
        g_origTabSpinProc = reinterpret_cast<WNDPROC>(
            SetWindowLongPtrW(hSpin, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(TabSpinSubclassProc)));

        // Force full-height to fit the tab bar
        RECT rcTabs;
        GetClientRect(hwndTabs, &rcTabs);
        RECT rcSpin;
        GetWindowRect(hSpin, &rcSpin);
        MapWindowPoints(HWND_DESKTOP, hwndTabs, reinterpret_cast<LPPOINT>(&rcSpin), 2);
        
        int newHeight = rcTabs.bottom - rcTabs.top;
        MoveWindow(hSpin, rcSpin.left, 0, rcSpin.right - rcSpin.left, newHeight, TRUE);
    }
    else if (hSpin && hSpin == g_hwndTabSpin)
    {
        // Keep it fitted on refreshes
        RECT rcTabs;
        GetClientRect(hwndTabs, &rcTabs);
        RECT rcSpin;
        GetWindowRect(hSpin, &rcSpin);
        MapWindowPoints(HWND_DESKTOP, hwndTabs, reinterpret_cast<LPPOINT>(&rcSpin), 2);
        int newHeight = rcTabs.bottom - rcTabs.top;
        if ((rcSpin.bottom - rcSpin.top) != newHeight || rcSpin.top != 0)
        {
            MoveWindow(hSpin, rcSpin.left, 0, rcSpin.right - rcSpin.left, newHeight, TRUE);
        }
    }
    else if (!hSpin && g_hwndTabSpin)
    {
        TabSpinDetach();
    }
}

void TabSpinDetach()
{
    if (g_hwndTabSpin && g_origTabSpinProc)
        SetWindowLongPtrW(g_hwndTabSpin, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(g_origTabSpinProc));

    g_hwndTabSpin = nullptr;
    g_origTabSpinProc = nullptr;
    g_tabSpinHoverLeft = false;
    g_tabSpinHoverRight = false;
    g_tabSpinPressLeft = false;
    g_tabSpinPressRight = false;
    g_trackingTabSpinMouse = false;
}
