#include "Config.h"
#include "Globals.h"
#include "Classes.h"
#include "Console.h"
#include <fstream>
#include <iostream>

void SaveConfig() {
    std::lock_guard<std::mutex> lock(g_ConfigMutex);
    std::ofstream f(CONFIG_FILE);
    if (!f.is_open()) return;
    f << "mode=" << (int)g_FilterMode << "\n";
    for (auto& p : g_BuffConfig)
        f << (p.second.enabled ? "+" : "-") << p.second.uniqueName << "|" << p.second.displayName << "\n";
    f.close();
    LogSuccess("Buff config saved!");
}

void LoadConfig() {
    std::lock_guard<std::mutex> lock(g_ConfigMutex);
    std::ifstream f(CONFIG_FILE);
    if (!f.is_open()) { LogInfo("No buff config file. Defaults."); return; }
    std::string ln;
    while (std::getline(f, ln)) {
        if (ln.empty() || ln[0] == '#') continue;
        if (ln.substr(0, 5) == "mode=") {
            int m = std::stoi(ln.substr(5));
            if (m >= 0 && m <= 2) g_FilterMode = (FilterMode)m;
            continue;
        }
        if (ln[0] == '+' || ln[0] == '-') {
            bool en = (ln[0] == '+');
            std::string r = ln.substr(1);
            size_t p = r.find('|');
            BuffEntry e;
            e.uniqueName = (p != std::string::npos) ? r.substr(0, p) : r;
            e.displayName = (p != std::string::npos) ? r.substr(p + 1) : e.uniqueName;
            e.enabled = en;
            g_BuffConfig[e.uniqueName] = e;
        }
    }
    f.close();
    LogSuccess("Buff config loaded!");
}

void RegisterBuff(const std::string& key, const std::string& display) {
    std::lock_guard<std::mutex> lock(g_ConfigMutex);
    if (g_BuffConfig.find(key) == g_BuffConfig.end()) {
        BuffEntry e;
        e.uniqueName = key;
        e.displayName = display;
        e.enabled = true;
        g_BuffConfig[key] = e;
    }
}

bool IsBuffVisible(const StatusEffectInfo& info) {
    if (info.removed || info.hideFromInfobar) return false;
    if (info.currentStacks > 1.0f) return false;
    std::string key = info.uniqueName.empty() ? info.name : info.uniqueName;
    std::lock_guard<std::mutex> lock(g_ConfigMutex);
    if (g_FilterMode == FILTER_SHOW_ALL) return true;
    auto it = g_BuffConfig.find(key);
    if (g_FilterMode == FILTER_WHITELIST) return it != g_BuffConfig.end() && it->second.enabled;
    return it == g_BuffConfig.end() || it->second.enabled;
}

void ShowConfigMenu() {
    LogHeader("BUFF FILTER CONFIG");
    char msg[512];
    sprintf_s(msg, "Mode: %s", g_FilterMode == 0 ? "SHOW ALL" : g_FilterMode == 1 ? "WHITELIST" : "BLACKLIST");
    LogInfo(msg);
    LogInfo("  number=toggle, 'mode 0/1/2', 'save', 'done'");
    g_ConfigDisplayOrder.clear();
    std::lock_guard<std::mutex> lock(g_ConfigMutex);
    std::vector<std::pair<std::string, BuffEntry>> sorted(g_BuffConfig.begin(), g_BuffConfig.end());
    std::sort(sorted.begin(), sorted.end(), [](auto& a, auto& b) { return a.second.displayName < b.second.displayName; });
    int idx = 0;
    for (auto& p : sorted) {
        idx++;
        g_ConfigDisplayOrder.push_back(p.first);
        sprintf_s(msg, "  [%2d] %s %s  (%s)", idx, p.second.enabled ? "[ON] " : "[OFF]",
            p.second.displayName.c_str(), p.second.uniqueName.c_str());
        p.second.enabled ? LogSuccess(msg) : Log(msg, 8);
    }
    g_WaitingForConfig = true;
}

void HandleConfigInput(const std::string& input) {
    if (input == "done" || input == "q") { g_WaitingForConfig = false; LogSuccess("Config closed."); return; }
    if (input == "save") { SaveConfig(); return; }
    if (input == "all on") { std::lock_guard<std::mutex> l(g_ConfigMutex); for (auto& p : g_BuffConfig) p.second.enabled = true; ShowConfigMenu(); return; }
    if (input == "all off") { std::lock_guard<std::mutex> l(g_ConfigMutex); for (auto& p : g_BuffConfig) p.second.enabled = false; ShowConfigMenu(); return; }
    if (input.length() > 5 && input.substr(0, 5) == "mode ") {
        try { int m = std::stoi(input.substr(5)); if (m >= 0 && m <= 2) { g_FilterMode = (FilterMode)m; LogSuccess("Mode changed!"); } }
        catch (...) { LogError("Invalid mode!"); }
        return;
    }
    try {
        int i = std::stoi(input);
        if (i >= 1 && i <= (int)g_ConfigDisplayOrder.size()) {
            std::lock_guard<std::mutex> l(g_ConfigMutex);
            auto it = g_BuffConfig.find(g_ConfigDisplayOrder[i - 1]);
            if (it != g_BuffConfig.end()) {
                it->second.enabled = !it->second.enabled;
                char msg[256];
                sprintf_s(msg, "'%s' %s", it->second.displayName.c_str(), it->second.enabled ? "ON" : "OFF");
                it->second.enabled ? LogSuccess(msg) : LogWarning(msg);
            }
        }
    }
    catch (...) { LogError("Unknown command."); }
}

// ============================================
// RAID CONFIG
// ============================================

void SaveRaidConfig() {
    std::lock_guard<std::mutex> lock(g_RaidMutex);
    std::ofstream f(RAID_CONFIG_FILE);
    if (!f.is_open()) return;
    f << "# Raid Tracker\n";
    for (auto& n : g_RaidTrackNames) f << n << "\n";
    f.close();
    LogSuccess("Raid config saved!");
}

void LoadRaidConfig() {
    std::lock_guard<std::mutex> lock(g_RaidMutex);
    std::ifstream f(RAID_CONFIG_FILE);
    if (!f.is_open()) { LogInfo("No raid config."); return; }
    std::string ln;
    while (std::getline(f, ln)) {
        if (ln.empty() || ln[0] == '#') continue;
        ln.erase(0, ln.find_first_not_of(" \t\r\n"));
        ln.erase(ln.find_last_not_of(" \t\r\n") + 1);
        if (!ln.empty()) g_RaidTrackNames.push_back(ln);
    }
    f.close();
    char msg[256];
    sprintf_s(msg, "Raid config: tracking %zu players", g_RaidTrackNames.size());
    LogSuccess(msg);
}

void ShowRaidConfigMenu() {
    LogHeader("RAID BUFF TRACKER CONFIG");
    char msg[512];
    {
        std::lock_guard<std::mutex> lock(g_RaidMutex);
        if (g_RaidTrackNames.empty()) {
            LogWarning("Not tracking anyone!");
        }
        else {
            LogInfo("Currently tracking:");
            for (size_t i = 0; i < g_RaidTrackNames.size(); i++) {
                sprintf_s(msg, "  [%zu] %s", i + 1, g_RaidTrackNames[i].c_str());
                LogSuccess(msg);
            }
        }
    }
    std::cout << "\n";
    LogInfo("Commands: 'add <name>', 'remove <name/#>', 'list', 'clear', 'save', 'done'");
    g_WaitingForRaidConfig = true;
}

void HandleRaidConfigInput(const std::string& input) {
    char msg[512];
    if (input == "done" || input == "q") { g_WaitingForRaidConfig = false; LogSuccess("Raid config closed."); return; }
    if (input == "save") { SaveRaidConfig(); return; }
    if (input == "clear") { std::lock_guard<std::mutex> l(g_RaidMutex); g_RaidTrackNames.clear(); g_RaidMembers.clear(); LogWarning("All cleared!"); return; }

    if (input == "list") {
        if (!g_LivingEntityModelClass.IsValid()) return;
        Method findAll = UnityObject.GetMethod("FindObjectsOfType", 1);
        if (!findAll.IsValid()) return;
        void* mt = Functions.type_get_object(Functions.class_get_type(g_LivingEntityModelClass.ptr));
        if (!mt) return;
        void* args[] = { mt }; void* exc = nullptr;
        void* arr = Functions.runtime_invoke(findAll.ptr, nullptr, args, &exc);
        if (!arr || exc) return;
        Il2CppArray* array = (Il2CppArray*)arr;
        LogInfo("\nNearby entities:");
        for (size_t i = 0; i < array->max_length; i++) {
            void* mo = array->vector[i]; if (!mo || IsBadReadPtr(mo, sizeof(void*))) continue;
            uintptr_t ma = (uintptr_t)mo; if (IsBadReadPtr((void*)(ma + 0x38), sizeof(void*))) continue;
            void* eo = *(void**)(ma + 0x38); if (!eo || IsBadReadPtr(eo, sizeof(void*))) continue;
            std::string name = GetEntityName(eo); if (name.empty()) continue;
            bool isYou = (eo == g_PlayerEntity);
            bool tracked = false;
            { std::lock_guard<std::mutex> l(g_RaidMutex); for (auto& n : g_RaidTrackNames) if (ToLower(n) == ToLower(name)) { tracked = true; break; } }
            sprintf_s(msg, "  %s%s%s", name.c_str(), isYou ? " (YOU)" : "", tracked ? " [TRACKED]" : "");
            if (isYou) LogInfo(msg); else if (tracked) LogSuccess(msg); else Log(msg, 8);
        }
        return;
    }

    if (input.length() > 4 && input.substr(0, 4) == "add ") {
        std::string name = input.substr(4);
        name.erase(0, name.find_first_not_of(" \t"));
        name.erase(name.find_last_not_of(" \t") + 1);
        if (name.empty()) { LogError("Specify a name!"); return; }
        std::lock_guard<std::mutex> l(g_RaidMutex);
        for (auto& n : g_RaidTrackNames) if (ToLower(n) == ToLower(name)) { LogWarning("Already tracked!"); return; }
        g_RaidTrackNames.push_back(name);
        sprintf_s(msg, "Now tracking: '%s'", name.c_str());
        LogSuccess(msg);
        return;
    }

    if (input.length() > 7 && input.substr(0, 7) == "remove ") {
        std::string arg = input.substr(7);
        arg.erase(0, arg.find_first_not_of(" \t"));
        arg.erase(arg.find_last_not_of(" \t") + 1);
        std::lock_guard<std::mutex> l(g_RaidMutex);
        try { int idx = std::stoi(arg); if (idx >= 1 && idx <= (int)g_RaidTrackNames.size()) { sprintf_s(msg, "Removed: '%s'", g_RaidTrackNames[idx - 1].c_str()); g_RaidTrackNames.erase(g_RaidTrackNames.begin() + (idx - 1)); LogWarning(msg); return; } }
        catch (...) {}
        for (auto it = g_RaidTrackNames.begin(); it != g_RaidTrackNames.end(); ++it) { if (ToLower(*it) == ToLower(arg)) { sprintf_s(msg, "Removed: '%s'", it->c_str()); g_RaidTrackNames.erase(it); LogWarning(msg); return; } }
        LogError("Not found!");
        return;
    }

    LogError("Unknown command.");
}