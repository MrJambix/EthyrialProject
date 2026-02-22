#pragma once

#include <Windows.h>
#include <vector>
#include <string>

struct StatusEffectInfo;

void UpdateTrackedStacks(const std::vector<StatusEffectInfo>& effects);