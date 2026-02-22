#include "Classes.h"
#include "Config.h"
#include "Console.h"

bool FindAllClasses() {
    LogInfo("Finding game classes...");
    void* domain = Resolver::GetDomain();
    if (!domain) { LogError("Failed to get domain!"); return false; }

    size_t ac = 0;
    void** assemblies = Functions.domain_get_assemblies(domain, &ac);

    char msg[256];
    sprintf_s(msg, "Found %zu assemblies", ac);
    LogInfo(msg);

    for (size_t i = 0; i < ac; i++) {
        void* image = Functions.assembly_get_image(assemblies[i]);
        if (!image) continue;
        const char* name = Functions.image_get_name(image);
        if (!name || strcmp(name, "Game.dll") != 0) continue;

        LogInfo("Found Game.dll assembly!");

        void* k;
        k = Functions.class_from_name(image, "", "StatusEffect");
        if (k) { g_StatusEffectClass = Class(k); LogSuccess("Found StatusEffect"); }
        else LogWarning("StatusEffect NOT found!");

        k = Functions.class_from_name(image, "", "LivingEntity");
        if (k) { g_LivingEntityClass = Class(k); LogSuccess("Found LivingEntity"); }
        else LogWarning("LivingEntity NOT found!");

        k = Functions.class_from_name(image, "", "LivingEntityModel");
        if (k) { g_LivingEntityModelClass = Class(k); LogSuccess("Found LivingEntityModel"); }
        else LogWarning("LivingEntityModel NOT found!");

        k = Functions.class_from_name(image, "", "PlayerScript");
        if (k) { g_PlayerScriptClass = Class(k); LogSuccess("Found PlayerScript"); }
        else LogWarning("PlayerScript NOT found (will use manual select)");

        break;
    }

    LogInfo("Class search complete.");
    return g_StatusEffectClass.IsValid() && g_LivingEntityClass.IsValid();
}

StatusEffectInfo ReadStatusEffect(void* p) {
    StatusEffectInfo info;
    info.objectPtr = p;
    info.currentDuration = -1.0f;
    info.maxDuration = -1.0f;
    info.currentStacks = 0.0f;
    info.removed = false;
    info.hideFromInfobar = false;

    if (!p || IsBadReadPtr(p, 0x70)) return info;
    uintptr_t a = (uintptr_t)p;

    if (!IsBadReadPtr((void*)(a + 0x10), sizeof(void*)))
        info.uniqueName = ReadIL2CppString(*(void**)(a + 0x10));
    if (!IsBadReadPtr((void*)(a + 0x18), sizeof(void*)))
        info.name = ReadIL2CppString(*(void**)(a + 0x18));
    if (!IsBadReadPtr((void*)(a + 0x40), sizeof(float)))
        info.currentDuration = *(float*)(a + 0x40);
    if (!IsBadReadPtr((void*)(a + 0x44), sizeof(float)))
        info.currentStacks = *(float*)(a + 0x44);
    if (!IsBadReadPtr((void*)(a + 0x48), sizeof(float)))
        info.maxDuration = *(float*)(a + 0x48);
    if (!IsBadReadPtr((void*)(a + 0x58), sizeof(bool)))
        info.removed = *(bool*)(a + 0x58);
    if (!IsBadReadPtr((void*)(a + 0x68), sizeof(bool)))
        info.hideFromInfobar = *(bool*)(a + 0x68);

    std::string key = info.uniqueName.empty() ? info.name : info.uniqueName;
    if (!key.empty()) RegisterBuff(key, info.name);

    return info;
}

std::vector<StatusEffectInfo> ReadEffectsFromEntity(void* entityObj) {
    std::vector<StatusEffectInfo> results;
    if (!entityObj || IsBadReadPtr(entityObj, sizeof(void*))) return results;

    uintptr_t ea = (uintptr_t)entityObj;
    if (IsBadReadPtr((void*)(ea + 0x268), sizeof(void*))) return results;
    void* dl = *(void**)(ea + 0x268);
    if (!dl || IsBadReadPtr(dl, 0x20)) return results;

    uintptr_t da = (uintptr_t)dl;
    if (IsBadReadPtr((void*)(da + 0x18), sizeof(void*))) return results;
    void* vl = *(void**)(da + 0x18);
    if (!vl || IsBadReadPtr(vl, 0x20)) return results;

    uintptr_t lb = (uintptr_t)vl;
    if (IsBadReadPtr((void*)(lb + 0x10), sizeof(void*)) ||
        IsBadReadPtr((void*)(lb + 0x18), sizeof(int))) return results;

    void* ia = *(void**)(lb + 0x10);
    int sz = *(int*)(lb + 0x18);
    if (sz <= 0 || sz > 50 || !ia || IsBadReadPtr(ia, sizeof(void*))) return results;

    uintptr_t ab = (uintptr_t)ia;
    if (IsBadReadPtr((void*)(ab + 0x20), sizeof(void*) * sz)) return results;
    void** items = (void**)(ab + 0x20);

    for (int j = 0; j < sz; j++) {
        void* item = items[j];
        if (!item || IsBadReadPtr(item, 0x70)) continue;
        StatusEffectInfo si = ReadStatusEffect(item);
        if (!si.name.empty() || !si.uniqueName.empty())
            results.push_back(si);
    }
    return results;
}

std::string GetEntityName(void* entityObj) {
    if (!entityObj || IsBadReadPtr(entityObj, sizeof(void*))) return "";
    uintptr_t ea = (uintptr_t)entityObj;
    if (IsBadReadPtr((void*)(ea + 0x90), sizeof(void*))) return "";
    void* si = *(void**)(ea + 0x90);
    if (!si || IsBadReadPtr(si, 0x48)) return "";
    uintptr_t sa = (uintptr_t)si;
    if (IsBadReadPtr((void*)(sa + 0x40), sizeof(void*))) return "";
    return ReadIL2CppString(*(void**)(sa + 0x40));
}

void SortEffects(std::vector<StatusEffectInfo>& effects) {
    std::sort(effects.begin(), effects.end(),
        [](const StatusEffectInfo& a, const StatusEffectInfo& b) {
            bool pa = a.IsPermanent(), pb = b.IsPermanent();
            if (pa && !pb) return true;
            if (!pa && pb) return false;
            if (pa && pb) {
                std::string nA = a.name.empty() ? a.uniqueName : a.name;
                std::string nB = b.name.empty() ? b.uniqueName : b.name;
                return nA < nB;
            }
            return a.GetRemainingTime() > b.GetRemainingTime();
        });
}

void UpdateTrackedStacks(const std::vector<StatusEffectInfo>& effects) {
    std::lock_guard<std::mutex> lock(g_StacksMutex);
    DWORD now = (DWORD)GetTickCount64();

    for (auto& p : g_TrackedStacks) p.second.isActive = false;

    for (const auto& e : effects) {
        if (e.removed || e.hideFromInfobar || e.currentStacks <= 1.0f) continue;
        std::string key = e.uniqueName.empty() ? e.name : e.uniqueName;
        if (key.empty()) continue;

        auto it = g_TrackedStacks.find(key);
        if (it == g_TrackedStacks.end()) {
            TrackedStackEffect t;
            t.uniqueName = key;
            t.displayName = e.name.empty() ? e.uniqueName : e.name;
            t.currentStacks = e.currentStacks;
            t.peakStacks = e.currentStacks;
            t.currentDuration = e.currentDuration;
            t.maxDuration = e.maxDuration;
            t.isActive = true;
            t.lastSeenTime = now;
            t.lastDropTime = 0;
            g_TrackedStacks[key] = t;
        }
        else {
            it->second.currentStacks = e.currentStacks;
            if (e.currentStacks > it->second.peakStacks) it->second.peakStacks = e.currentStacks;
            it->second.currentDuration = e.currentDuration;
            it->second.maxDuration = e.maxDuration;
            it->second.displayName = e.name.empty() ? e.uniqueName : e.name;
            it->second.isActive = true;
            it->second.lastSeenTime = now;
            it->second.lastDropTime = 0;
        }
    }

    for (auto& p : g_TrackedStacks)
        if (!p.second.isActive && p.second.lastDropTime == 0)
        {
            p.second.lastDropTime = now; p.second.currentStacks = 0;
        }

    std::vector<std::string> toRemove;
    for (auto& p : g_TrackedStacks)
        if (!p.second.isActive && p.second.lastDropTime > 0 && (now - p.second.lastDropTime) > (DWORD)STACKS_FADE_TIME)
            toRemove.push_back(p.first);
    for (auto& k : toRemove) g_TrackedStacks.erase(k);
}

void UpdateRaidData() {
    std::lock_guard<std::mutex> lock(g_RaidMutex);
    if (g_RaidTrackNames.empty()) { g_RaidMembers.clear(); return; }
    if (!g_LivingEntityModelClass.IsValid()) return;

    Method findAll = UnityObject.GetMethod("FindObjectsOfType", 1);
    if (!findAll.IsValid()) return;

    void* typeObj = Functions.class_get_type(g_LivingEntityModelClass.ptr);
    if (!typeObj) return;
    void* mt = Functions.type_get_object(typeObj);
    if (!mt) return;

    void* args[] = { mt };
    void* exc = nullptr;
    void* arr = Functions.runtime_invoke(findAll.ptr, nullptr, args, &exc);
    if (!arr || exc) return;

    Il2CppArray* array = (Il2CppArray*)arr;
    if (IsBadReadPtr(array, sizeof(Il2CppArray))) return;
    if (array->max_length <= 0 || array->max_length > 500) return;

    std::map<std::string, std::pair<void*, std::vector<StatusEffectInfo>>> entityMap;
    for (size_t i = 0; i < array->max_length; i++) {
        try {
            void* mo = array->vector[i];
            if (!mo || IsBadReadPtr(mo, sizeof(void*))) continue;
            uintptr_t ma = (uintptr_t)mo;
            if (IsBadReadPtr((void*)(ma + 0x38), sizeof(void*))) continue;
            void* eo = *(void**)(ma + 0x38);
            if (!eo || IsBadReadPtr(eo, sizeof(void*))) continue;
            std::string name = GetEntityName(eo);
            if (name.empty()) continue;

            std::string lower = ToLower(name);
            bool isTracked = false;
            for (auto& tn : g_RaidTrackNames) {
                if (ToLower(tn) == lower) { isTracked = true; break; }
            }

            if (isTracked) {
                entityMap[lower] = { eo, ReadEffectsFromEntity(eo) };
            }
            else {
                entityMap[lower] = { eo, {} };
            }
        }
        catch (...) {
            continue;
        }
    }

    g_RaidMembers.clear();
    for (auto& trackName : g_RaidTrackNames) {
        RaidMemberData rd;
        rd.name = trackName;
        rd.entityObj = nullptr;
        rd.isNearby = false;

        auto it = entityMap.find(ToLower(trackName));
        if (it != entityMap.end()) {
            rd.entityObj = it->second.first;
            rd.effects = it->second.second;
            rd.isNearby = true;
            SortEffects(rd.effects);
        }
        g_RaidMembers.push_back(rd);
    }
}