#pragma once

#include <Windows.h>
#include <string>
#include <vector>

void UpdateRaidData();
void ShowRaidConfigMenu();
void HandleRaidConfigInput(const std::string& line);
void SaveRaidConfig();
void LoadRaidConfig();