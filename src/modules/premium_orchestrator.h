/*
  Solum

  Premium header render orchestration extracted from main window procedure.
  Owns Direct2D resources/timer lifecycle and keeps paint scope clipped.
*/

#pragma once

#include <windows.h>

namespace PremiumOrchestrator
{
constexpr UINT_PTR kTimerId = 0x4C4E02;

void Initialize(HWND hwndMain);
void Shutdown(HWND hwndMain);

void OnPaint(HWND hwndMain, const RECT &paintRect);
void OnSize(UINT width, UINT height);
void OnDpiChanged(HWND hwndMain);
bool OnTimer(HWND hwndMain, WPARAM timerId);

void SetEnabled(HWND hwndMain, bool enabled);
bool IsEnabled();
} // namespace PremiumOrchestrator

