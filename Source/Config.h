#pragma once

#include "Globals.h"
#include <string>

void SaveConfig();
void LoadConfig();
void RegisterBuff(const std::string& key, const std::string& display);
bool IsBuffVisible(const StatusEffectInfo& info);
void ShowConfigMenu();
void HandleConfigInput(const std::string& input);

void SaveRaidConfig();
void LoadRaidConfig();
void ShowRaidConfigMenu();
void HandleRaidConfigInput(const std::string& input);