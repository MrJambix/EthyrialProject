#pragma once
#include "Globals.h"

bool InitAutoLoot();
bool IsAutoLootReady();
void AutoLootUpdate();
void ToggleAutoLoot();

extern bool g_AutoLootEnabled;