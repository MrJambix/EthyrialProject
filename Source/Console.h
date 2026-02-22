#pragma once

#include <Windows.h>
#include <string>
#include <csignal>

void InitConsole();
void Log(const char* msg);
void Log(const char* msg, const char* arg);
void Log(const char* msg, int arg);
void LogInfo(const char* msg);
void LogSuccess(const char* msg);
void LogWarning(const char* msg);
void LogError(const char* msg);
void LogHeader(const char* msg);
void LogDebug(const char* msg);
void PrintMemoryUsage();
void CheckMemoryPeriodic();
void InstallCrashHandlers();