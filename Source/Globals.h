#pragma once

#include <Windows.h>
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <algorithm>
#include <gdiplus.h>

#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

// ============================================
// IL2CPP RESOLVER
// ============================================

#include "IL2CPPResolver.hpp"
using namespace IL2CPP;

// ============================================
// STATUS EFFECT INFO
// ============================================

struct StatusEffectInfo {
    std::string uniqueName;
    std::string name;
    float currentDuration = 0.0f;
    float maxDuration = 0.0f;
    float currentStacks = 0.0f;
    bool removed = false;
    bool hideFromInfobar = false;
    void* objectPtr = nullptr;

    float GetRemainingTime() const {
        if (maxDuration <= 0) return -1.0f;
        float rem = maxDuration - currentDuration;
        return rem > 0 ? rem : 0;
    }

    bool IsPermanent() const {
        return maxDuration <= 0 || maxDuration > 86400.0f;
    }
};

// ============================================
// TRACKED STACK EFFECT
// ============================================

struct TrackedStackEffect {
    std::string uniqueName;
    std::string displayName;
    float currentStacks = 0.0f;
    float peakStacks = 0.0f;
    float currentDuration = 0.0f;
    float maxDuration = 0.0f;
    bool isActive = false;
    DWORD lastSeenTime = 0;
    DWORD lastDropTime = 0;
};

// ============================================
// ENTITY ENTRY
// ============================================

struct EntityEntry {
    std::string name;
    void* entityObj = nullptr;
    void* modelObj = nullptr;
};

// ============================================
// RAID MEMBER DATA
// ============================================

struct RaidMemberData {
    std::string name;
    void* entityObj = nullptr;
    bool isNearby = false;
    std::vector<StatusEffectInfo> effects;
};

// ============================================
// BUFF CONFIG
// ============================================

enum FilterMode {
    FILTER_SHOW_ALL = 0,
    FILTER_WHITELIST = 1,
    FILTER_BLACKLIST = 2
};

struct BuffEntry {
    std::string uniqueName;
    std::string displayName;
    bool enabled = true;
};

// ============================================
// LAYOUT CONSTANTS
// Same values as your originals
// ============================================

static const int BAR_WIDTH = 260;
static const int ICON_SIZE = 28;
static const int PADDING = 6;
static const int HEADER_HEIGHT = 24;
static const int ROW_HEIGHT = 26;
static const int STACKS_FADE_TIME = 15000;

static const int RAID_BAR_WIDTH = 300;
static const int RAID_HEADER_HEIGHT = 26;
static const int RAID_ROW_HEIGHT = 22;

// Gap between panels in unified overlay
static const int PANEL_GAP = 6;

// ============================================
// EXTERN GLOBALS - IL2CPP classes
// ============================================

extern Class g_StatusEffectClass;
extern Class g_LivingEntityClass;
extern Class g_LivingEntityModelClass;
extern Class g_PlayerScriptClass;

// ============================================
// EXTERN GLOBALS - Player state
// ============================================

extern void* g_PlayerEntity;
extern void* g_PlayerModel;
extern std::string g_PlayerName;

// ============================================
// EXTERN GLOBALS - Data collections
// ============================================

extern std::vector<StatusEffectInfo> g_CurrentEffects;
extern std::map<std::string, TrackedStackEffect> g_TrackedStacks;
extern std::vector<EntityEntry> g_EntityList;
extern std::vector<std::string> g_RaidTrackNames;
extern std::vector<RaidMemberData> g_RaidMembers;

// ============================================
// EXTERN GLOBALS - Mutexes
// ============================================

extern std::mutex g_EffectsMutex;
extern std::mutex g_StacksMutex;
extern std::mutex g_RaidMutex;
extern std::mutex g_ConfigMutex;

// ============================================
// EXTERN GLOBALS - Buff config
// ============================================

extern FilterMode g_FilterMode;
extern std::map<std::string, BuffEntry> g_BuffConfig;
extern std::vector<std::string> g_ConfigDisplayOrder;

// ============================================
// EXTERN GLOBALS - Overlay state
// ============================================

extern bool g_OverlayActive;
extern bool g_StacksOverlayActive;
extern bool g_RaidOverlayActive;
extern bool g_TrackingActive;
extern bool g_WaitingForSelection;
extern bool g_WaitingForConfig;
extern bool g_WaitingForRaidConfig;

// ============================================
// EXTERN GLOBALS - Unified overlay window
// ============================================

extern HWND g_OverlayHwnd;
extern ULONG_PTR g_GdiplusToken;
extern int g_OverlayX, g_OverlayY;

// ============================================
// EXTERN GLOBALS - Config files
// ============================================

extern const char* CONFIG_FILE;
extern const char* RAID_CONFIG_FILE;

// ============================================
// SHARED HELPER FUNCTIONS
// ============================================

std::string ReadIL2CppString(void* strPtr);
std::wstring StringToWString(const std::string& str);
std::string ToLower(const std::string& s);

// ============================================
// EXTERN GLOBALS - Single overlay window
// ============================================

extern HWND g_OverlayHwnd;
extern ULONG_PTR g_GdiplusToken;

// ============================================
// EXTERN GLOBALS - Per-panel screen positions
// ============================================

extern int g_OverlayX, g_OverlayY;   // Buff panel
extern int g_StacksX, g_StacksY;     // Stacks panel
extern int g_RaidX, g_RaidY;         // Raid panel