#include "Player.h"
#include "Classes.h"
#include "Console.h"
#include <iostream>
#include <algorithm>

// Rate limiting
static DWORD g_LastEntityScan = 0;
static const DWORD SCAN_COOLDOWN = 1000; // 1 second

struct ScanEntry {
    std::string name;
    int effectCount;
    void* entityObj;
    void* modelObj;
};

static std::vector<ScanEntry> ScanAllEntities() {
    std::vector<ScanEntry> results;

    // Rate limit
    DWORD now = GetTickCount();
    if (now - g_LastEntityScan < SCAN_COOLDOWN) {
        LogWarning("Scan cooldown - wait 1 second.");
        return results;
    }
    g_LastEntityScan = now;

    if (!g_LivingEntityModelClass.IsValid()) return results;
    Method findAll = UnityObject.GetMethod("FindObjectsOfType", 1);
    if (!findAll.IsValid()) return results;
    void* mt = Functions.type_get_object(Functions.class_get_type(g_LivingEntityModelClass.ptr));
    if (!mt) return results;

    void* args[] = { mt };
    void* exc = nullptr;
    void* arr = Functions.runtime_invoke(findAll.ptr, nullptr, args, &exc);
    if (!arr || exc) return results;
    Il2CppArray* array = (Il2CppArray*)arr;

    for (size_t i = 0; i < array->max_length; i++) {
        void* mo = array->vector[i];
        if (!mo || IsBadReadPtr(mo, sizeof(void*))) continue;
        uintptr_t ma = (uintptr_t)mo;
        if (IsBadReadPtr((void*)(ma + 0x38), sizeof(void*))) continue;
        void* eo = *(void**)(ma + 0x38);
        if (!eo || IsBadReadPtr(eo, sizeof(void*))) continue;
        std::string name = GetEntityName(eo);
        if (name.empty()) continue;

        auto fx = ReadEffectsFromEntity(eo);

        ScanEntry entry;
        entry.name = name;
        entry.effectCount = (int)fx.size();
        entry.entityObj = eo;
        entry.modelObj = mo;
        results.push_back(entry);
    }

    return results;
}

bool FindPlayerEntity() {
    LogHeader("FINDING PLAYER");

    // Force allow scan for auto-detect
    g_LastEntityScan = 0;

    auto entities = ScanAllEntities();
    if (entities.empty()) {
        LogError("No entities found!");
        return false;
    }

    char msg[256];
    sprintf_s(msg, "Scanned %d entities", (int)entities.size());
    LogInfo(msg);

    // Strategy 1: Count name occurrences. Player names are unique, mob names repeat.
    std::map<std::string, int> nameCounts;
    for (auto& e : entities) {
        nameCounts[e.name]++;
    }

    // Strategy 2: Find entities with unique names AND effects > 0 (strongest signal)
    ScanEntry* bestCandidate = nullptr;
    int bestScore = -1;

    for (auto& e : entities) {
        int count = nameCounts[e.name];
        int score = 0;

        // Unique name = very likely a player (mobs have duplicates)
        if (count == 1) score += 100;

        // Has active effects = very likely a player
        score += e.effectCount * 50;

        // Name doesn't match known mob patterns (simple heuristic)
        bool looksLikeMob = false;
        std::string lower = ToLower(e.name);
        // Common mob-like names
        const char* mobPatterns[] = {
            "hound", "knight", "manifestation", "paragon", "wolf", "spider",
            "skeleton", "zombie", "golem", "elemental", "guard", "bandit",
            "bear", "boar", "rat", "bat", "slime", "goblin", "troll",
            "drake", "wyvern", "demon", "imp", "wraith", "ghost",
            "shade", "spirit", "construct", "sentinel", "watcher",
            "crawler", "lurker", "stalker", "prowler", "beast", nullptr
        };
        for (int p = 0; mobPatterns[p]; p++) {
            if (lower.find(mobPatterns[p]) != std::string::npos) {
                looksLikeMob = true;
                break;
            }
        }

        // If name has a space and first word is capitalized like a proper name, bonus
        // (Player names like "MichaelScott" or "Halvardt" vs "Shadow Knight")
        bool hasMultipleWords = (e.name.find(' ') != std::string::npos);
        if (!hasMultipleWords && !looksLikeMob) score += 30;

        // Penalize mob-like names
        if (looksLikeMob) score -= 50;

        // Penalize duplicate names heavily
        if (count > 1) score -= count * 30;

        sprintf_s(msg, "  '%s' score=%d (count=%d, fx=%d, mob=%s)",
            e.name.c_str(), score, count, e.effectCount, looksLikeMob ? "yes" : "no");
        LogInfo(msg);

        if (score > bestScore) {
            bestScore = score;
            bestCandidate = &e;
        }
    }

    // Accept if score is reasonably high
    if (bestCandidate && bestScore >= 50) {
        g_PlayerEntity = bestCandidate->entityObj;
        g_PlayerModel = bestCandidate->modelObj;
        g_PlayerName = bestCandidate->name;

        sprintf_s(msg, "Auto-detected player: '%s' (score=%d, %d effects)",
            g_PlayerName.c_str(), bestScore, bestCandidate->effectCount);
        LogSuccess(msg);
        return true;
    }

    // Strategy 3: If all else fails, pick the unique-named entity with most effects
    ScanEntry* fallback = nullptr;
    int fallbackFx = -1;
    for (auto& e : entities) {
        if (nameCounts[e.name] == 1 && e.effectCount > fallbackFx) {
            fallbackFx = e.effectCount;
            fallback = &e;
        }
    }

    if (fallback) {
        g_PlayerEntity = fallback->entityObj;
        g_PlayerModel = fallback->modelObj;
        g_PlayerName = fallback->name;

        sprintf_s(msg, "Fallback detected: '%s' (%d effects)",
            g_PlayerName.c_str(), fallback->effectCount);
        LogSuccess(msg);
        return true;
    }

    LogWarning("Auto-detect failed. Opening entity picker...");
    return false;
}

void ListAllLivingEntities() {
    // Force allow scan
    g_LastEntityScan = 0;

    auto entities = ScanAllEntities();
    if (entities.empty()) {
        LogError("No entities found!");
        return;
    }

    // Store for selection
    g_EntityList.clear();
    for (auto& e : entities) {
        EntityEntry entry;
        entry.name = e.name;
        entry.entityObj = e.entityObj;
        entry.modelObj = e.modelObj;
        g_EntityList.push_back(entry);
    }

    LogHeader("ALL LIVING ENTITIES");
    char msg[256];
    for (int i = 0; i < (int)entities.size(); i++) {
        auto& e = entities[i];
        bool isCurrent = (e.entityObj == g_PlayerEntity);
        sprintf_s(msg, "  [%d] %s '%s' (%d fx)",
            i + 1,
            isCurrent ? ">>>" : "   ",
            e.name.c_str(),
            e.effectCount);

        if (isCurrent) LogSuccess(msg);
        else if (e.effectCount > 0) LogInfo(msg);
        else std::cout << "  " << msg << "\n";
    }

    std::cout << "\nType number to select:\n";
    g_WaitingForSelection = true;
}

void SelectEntity(int index) {
    g_WaitingForSelection = false;

    if (index < 1 || index >(int)g_EntityList.size()) {
        LogError("Invalid selection!");
        return;
    }

    auto& entry = g_EntityList[index - 1];
    g_PlayerEntity = entry.entityObj;
    g_PlayerModel = entry.modelObj;
    g_PlayerName = entry.name;
    g_TrackingActive = true;

    char msg[256];
    sprintf_s(msg, ">>> SELECTED: '%s' <<<", g_PlayerName.c_str());
    LogSuccess(msg);
    LogSuccess("Tracking started!");
}