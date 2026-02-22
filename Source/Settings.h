#pragma once

#include <Windows.h>
#include <string>

extern HWND g_SettingsHwnd;

void RequestOpenSettings();
void RequestEntityPicker();
void CreateSettingsIfRequested();
bool IsSettingsOpen();
void CloseSettingsWindow();