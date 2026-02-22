#pragma once

#include "Globals.h"

// ============================================
// QUICK CAST SYSTEM
// ============================================

extern bool g_QuickCastEnabled;
extern Class g_LocalPlayerInputClass;
extern Class g_CameraControllerClass;
extern Class g_SpellClass;
extern Class g_QuickBarClass;

bool InitQuickCast();
void QuickCastUpdate();
void ToggleQuickCast();