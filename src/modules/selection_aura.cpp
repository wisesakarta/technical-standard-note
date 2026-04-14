#include "selection_aura.h"
#include "core/globals.h"
#include "theme.h"
#include "design_system.h"
#include "editor.h"
#include <richedit.h>
#include <algorithm>

namespace UI {

bool SelectionAura::RegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"TechnicalStandardSelectionAura";
    wc.hCursor = LoadCursor(nullptr, IDC_IBEAM);
    // WS_EX_TRANSPARENT + hbrBackground = NULL allows click-through
    wc.hbrBackground = nullptr;
    return ::RegisterClassExW(&wc) != 0;
}

HWND SelectionAura::Create(HWND hwndParent, HWND hwndEditor)
{
    HWND hwnd = CreateWindowExW(WS_EX_TRANSPARENT | WS_EX_LAYERED, L"TechnicalStandardSelectionAura", nullptr,
        WS_CHILD | WS_VISIBLE, 0, 0, 0, 0,
        hwndParent, nullptr, GetModuleHandle(nullptr), nullptr);
    
    if (hwnd) {
        SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);
        State* state = new State();
        state->hwndEditor = hwndEditor;
        state->engine.Initialize(hwnd);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));
    }
    return hwnd;
}

void SelectionAura::Update(HWND hwnd)
{
    State* state = reinterpret_cast<State*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (!state) return;

    CHARRANGE cr;
    SendMessageW(state->hwndEditor, EM_EXGETSEL, 0, reinterpret_cast<LPARAM>(&cr));
    
    // EM_HIDESELECTION hides the system visual highlight while keeping the selection range active.
    SendMessageW(state->hwndEditor, EM_HIDESELECTION, (WPARAM)TRUE, 0);



    
    if (cr.cpMin == cr.cpMax) {
        if (state->opacity.endValue != 0.0f) {
            state->opacity.Start(state->opacity.GetCurrentValue(), 0.0f, 200.0f);
            SetTimer(hwnd, 0x1001, 16, nullptr);
        }
    } else {
        if (state->opacity.endValue == 0.0f) {
            state->opacity.Start(state->opacity.GetCurrentValue(), 1.0f, 150.0f);
            SetTimer(hwnd, 0x1001, 16, nullptr);
        }

        const int lineStart = static_cast<int>(SendMessageW(state->hwndEditor, EM_EXLINEFROMCHAR, 0, cr.cpMin));
        const int lineEnd = static_cast<int>(SendMessageW(state->hwndEditor, EM_EXLINEFROMCHAR, 0, cr.cpMax));
        
        state->selectionRectCount = 0;
        for (int i = lineStart; i <= lineEnd && state->selectionRectCount < 64; ++i) {
            const int lineIndex = static_cast<int>(SendMessageW(state->hwndEditor, EM_LINEINDEX, i, 0));
            if (lineIndex == -1) continue;

            const int lineLength = static_cast<int>(SendMessageW(state->hwndEditor, EM_LINELENGTH, lineIndex, 0));
            const int selStart = std::max(lineIndex, (int)cr.cpMin);
            const int selEnd = std::min(lineIndex + lineLength, (int)cr.cpMax);

            if (selStart < selEnd) {
                POINT ptStart{}, ptEnd{};
                SendMessageW(state->hwndEditor, EM_POSFROMCHAR, reinterpret_cast<WPARAM>(&ptStart), selStart);
                SendMessageW(state->hwndEditor, EM_POSFROMCHAR, reinterpret_cast<WPARAM>(&ptEnd), selEnd);

                if (selEnd == lineIndex + lineLength && i < lineEnd) {
                    RECT rcClient;
                    GetClientRect(state->hwndEditor, &rcClient);
                    ptEnd.x = rcClient.right;
                }

                // Minimum width for visible selection if identical points
                if (ptEnd.x <= ptStart.x) ptEnd.x = ptStart.x + 8;

                state->selectionRects[state->selectionRectCount++] = { 
                    ptStart.x, ptStart.y, ptEnd.x, ptStart.y + 22 
                };
            }
        }
    }
    
    InvalidateRect(hwnd, nullptr, FALSE);
}

void SelectionAura::SetEditor(HWND hwndAura, HWND hwndEditor)
{
    if (!hwndAura)
        return;
    State* state = reinterpret_cast<State*>(GetWindowLongPtrW(hwndAura, GWLP_USERDATA));
    if (!state)
        return;
    state->hwndEditor = hwndEditor;
    state->selectionRectCount = 0;
    InvalidateRect(hwndAura, nullptr, FALSE);
}

LRESULT CALLBACK SelectionAura::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    State* state = reinterpret_cast<State*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (msg) {
    case WM_SIZE:
        if (state) state->engine.Resize(LOWORD(lParam), HIWORD(lParam));
        return 0;
    case WM_TIMER:
        if (wParam == 0x1001 && state) {
            InvalidateRect(hwnd, nullptr, FALSE);
            if (!state->opacity.active) {
                KillTimer(hwnd, 0x1001);
            }
        }
        return 0;
    case WM_PAINT: {
        if (!state) return 0;
        PAINTSTRUCT ps;
        BeginPaint(hwnd, &ps);
        
        auto context = state->engine.GetDeviceContext();
        const float alpha = state->opacity.GetCurrentValue();
        if (context && alpha > 0.01f) {
            context->BeginDraw();
            context->Clear(nullptr);
            
            if (state->selectionRectCount > 0) {
                // High-Intensity Glow (Librarian's Polish): 0.95f ensures visibility even in bright themes
                D2D1_COLOR_F glowColor = Graphics::Engine::ColorToD2D(DesignSystem::Color::kAccent, 0.95f * alpha);
                
                for (int i = 0; i < state->selectionRectCount; ++i) {
                    RECT& r = state->selectionRects[i];
                    // Geometric definition: Expanding to provide a solid, premium highlight feel
                    D2D1_RECT_F rect = D2D1::RectF((float)r.left, (float)r.top, (float)r.right, (float)r.bottom + 1.0f);
                    state->engine.DrawBlurredRect(rect, glowColor, 1.2f); // Sharp focus for initial verification
                }
            }
            context->EndDraw();
        } else if (context) {
            context->BeginDraw();
            context->Clear(nullptr);
            context->EndDraw();
        }
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_NCHITTEST: return HTTRANSPARENT; // Ensure mouse passes through to editor
    case WM_MOUSEWHEEL:
        if (state && state->hwndEditor)
        {
            ScrollEditorFromMouseWheel(state->hwndEditor, wParam);
            if (g_hwndScrollbar)
                InvalidateRect(g_hwndScrollbar, nullptr, FALSE);
            return 0;
        }
        return 0;
    case WM_DESTROY:
        delete state;
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

} // namespace UI
