#pragma once

#include "Globals.h"

bool FindAllClasses();
StatusEffectInfo ReadStatusEffect(void* effectPtr);
std::vector<StatusEffectInfo> ReadEffectsFromEntity(void* entityObj);
std::string GetEntityName(void* entityObj);
void SortEffects(std::vector<StatusEffectInfo>& effects);
void UpdateTrackedStacks(const std::vector<StatusEffectInfo>& effects);
void UpdateRaidData();