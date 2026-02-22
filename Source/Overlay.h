#pragma once

#include <Windows.h>
#include <gdiplus.h>

using namespace Gdiplus;

// ============================================
// SINGLE WINDOW — 3 independently-positioned panels
// One GDI bitmap, one window, one font cache
// ============================================

// Panel size calculators
int CalcBuffPanelWidth();
int CalcBuffPanelHeight();
int CalcStacksPanelWidth();
int CalcStacksPanelHeight();
int CalcRaidPanelWidth();
int CalcRaidPanelHeight();
int CalcTotalOverlayWidth();
int CalcTotalOverlayHeight();

void CalcWindowOrigin(int& outX, int& outY);

LRESULT CALLBACK UnifiedWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);