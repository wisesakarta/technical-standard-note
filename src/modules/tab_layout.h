/*
  Otso

  Tab layout and DPI/font metrics controller.
*/

#pragma once

#include <windows.h>

int TabScalePx(int px);
void TabDestroyFonts();
void TabRefreshDpi();
void TabRefreshVisualMetrics();
HFONT TabGetRegularFont();
HFONT TabGetActiveFont();
