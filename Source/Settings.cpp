#include "Settings.h"
#include "Globals.h"
#include "Config.h"
#include "Classes.h"
#include "Player.h"
#include "Console.h"
#include "QuickCast.h"
#include "AutoLoot.h"
#include <gdiplus.h>
#include <vector>
#include <algorithm>

using namespace Gdiplus;

HWND g_SettingsHwnd = nullptr;
bool g_SettingsRequested = false;
static bool g_SettingsClassRegistered = false;

// ============================================
// UI STATE
// ============================================

enum SettingsTab { TAB_BUFFS = 0, TAB_RAID = 1, TAB_OPTIONS = 2, TAB_ENTITIES = 3, TAB_MISC = 4 };
static SettingsTab g_Tab = TAB_OPTIONS;

static int g_BuffScrollOffset = 0;
static int g_RaidScrollOffset = 0;
static int g_NearbyScrollOffset = 0;
static int g_EntityScrollOffset = 0;

static std::string g_RaidInputText = "";
static bool g_RaidInputFocused = false;

static std::vector<std::string> g_NearbyEntities;
static std::vector<std::pair<std::string, BuffEntry>> g_SortedBuffs;

static int g_RaidSelectedIndex = -1;
static int g_NearbySelectedIndex = -1;
static int g_EntitySelectedIndex = -1;

// Entity picker
struct EntityPickerEntry {
    std::string name;
    void* entityObj;
    void* modelObj;
    bool isCurrent;
    int nameCount;
};
static std::vector<EntityPickerEntry> g_EntityPickerList;
static bool g_EntityPickerRequested = false;

// Rate limiting
static DWORD g_LastNearbyRefresh = 0;
static DWORD g_LastEntityRefresh = 0;
static const DWORD REFRESH_COOLDOWN = 1000;

// Status message
static std::string g_StatusMessage = "";
static DWORD g_StatusTime = 0;
static int g_StatusColor = 0;

// Layout
static const int SETTINGS_W = 520;
static const int SETTINGS_H = 510;
static const int TAB_H = 32;
static const int BTN_H = 28;
static const int S_ROW_H = 24;
static const int LIST_H = 280;
static const int MAX_VIS = LIST_H / S_ROW_H;
static const int STATUS_H = 22;

// ============================================
// HELPERS
// ============================================

static bool HitTest(int px, int py, int x, int y, int w, int h) {
    return px >= x && px < x + w && py >= y && py < y + h;
}

static void SetStatus(const std::string& msg, int color = 0) {
    g_StatusMessage = msg;
    g_StatusTime = GetTickCount();
    g_StatusColor = color;
}

static void RebuildBuffCache() {
    std::lock_guard<std::mutex> lock(g_ConfigMutex);
    g_SortedBuffs.assign(g_BuffConfig.begin(), g_BuffConfig.end());
    std::sort(g_SortedBuffs.begin(), g_SortedBuffs.end(),
        [](const std::pair<std::string, BuffEntry>& a, const std::pair<std::string, BuffEntry>& b) {
            return a.second.displayName < b.second.displayName;
        });
}

static bool RebuildNearbyCache() {
    DWORD now = GetTickCount();
    if (now - g_LastNearbyRefresh < REFRESH_COOLDOWN) {
        SetStatus("Wait 1 second between refreshes", 1);
        return false;
    }
    g_LastNearbyRefresh = now;

    g_NearbyEntities.clear();
    if (!g_LivingEntityModelClass.IsValid()) return false;
    Method findAll = UnityObject.GetMethod("FindObjectsOfType", 1);
    if (!findAll.IsValid()) return false;
    void* mt = Functions.type_get_object(Functions.class_get_type(g_LivingEntityModelClass.ptr));
    if (!mt) return false;
    void* args[] = { mt };
    void* exc = nullptr;
    void* arr = Functions.runtime_invoke(findAll.ptr, nullptr, args, &exc);
    if (!arr || exc) return false;
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
        if (eo == g_PlayerEntity) continue;
        g_NearbyEntities.push_back(name);
    }
    std::sort(g_NearbyEntities.begin(), g_NearbyEntities.end());
    return true;
}

static bool RebuildEntityPicker() {
    DWORD now = GetTickCount();
    if (now - g_LastEntityRefresh < REFRESH_COOLDOWN) {
        SetStatus("Wait 1 second between refreshes", 1);
        return false;
    }
    g_LastEntityRefresh = now;

    g_EntityPickerList.clear();
    g_EntitySelectedIndex = -1;
    g_EntityScrollOffset = 0;

    if (!g_LivingEntityModelClass.IsValid()) return false;
    Method findAll = UnityObject.GetMethod("FindObjectsOfType", 1);
    if (!findAll.IsValid()) return false;
    void* mt = Functions.type_get_object(Functions.class_get_type(g_LivingEntityModelClass.ptr));
    if (!mt) return false;
    void* args[] = { mt };
    void* exc = nullptr;
    void* arr = Functions.runtime_invoke(findAll.ptr, nullptr, args, &exc);
    if (!arr || exc) return false;
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

        EntityPickerEntry entry;
        entry.name = name;
        entry.entityObj = eo;
        entry.modelObj = mo;
        entry.isCurrent = (eo == g_PlayerEntity);
        entry.nameCount = 0;
        g_EntityPickerList.push_back(entry);
    }

    std::map<std::string, int> nameCounts;
    for (auto& e : g_EntityPickerList) nameCounts[e.name]++;
    for (auto& e : g_EntityPickerList) e.nameCount = nameCounts[e.name];

    std::sort(g_EntityPickerList.begin(), g_EntityPickerList.end(),
        [](const EntityPickerEntry& a, const EntityPickerEntry& b) {
            if (a.isCurrent != b.isCurrent) return a.isCurrent > b.isCurrent;
            if ((a.nameCount == 1) != (b.nameCount == 1)) return a.nameCount < b.nameCount;
            return a.name < b.name;
        });

    char msg[128];
    sprintf_s(msg, "Found %d entities", (int)g_EntityPickerList.size());
    SetStatus(msg, 0);
    return true;
}

// ============================================
// DRAW HELPERS
// ============================================

static void DrawButton(Graphics& gfx, const wchar_t* text, int x, int y, int w, int h, bool active = false, bool green = false) {
    Color bg, border;
    if (green) { bg = Color(255, 30, 100, 40); border = Color(255, 60, 180, 80); }
    else if (active) { bg = Color(255, 80, 40, 40); border = Color(255, 180, 80, 80); }
    else { bg = Color(255, 40, 50, 80); border = Color(255, 70, 90, 130); }
    SolidBrush bgBrush(bg);
    Pen borderPen(border, 1.0f);
    Rect r(x, y, w, h);
    gfx.FillRectangle(&bgBrush, r);
    gfx.DrawRectangle(&borderPen, r);

    Font f(L"Segoe UI", 9, FontStyleBold, UnitPoint);
    SolidBrush textBrush(Color(255, 220, 220, 240));
    StringFormat sf;
    sf.SetAlignment(StringAlignmentCenter);
    sf.SetLineAlignment(StringAlignmentCenter);
    RectF rf((float)x, (float)y, (float)w, (float)h);
    gfx.DrawString(text, -1, &f, rf, &sf, &textBrush);
}

static void DrawCheckbox(Graphics& gfx, const wchar_t* text, int x, int y, int w, bool checked) {
    Color boxBg = checked ? Color(255, 40, 140, 60) : Color(255, 50, 40, 40);
    SolidBrush boxBrush(boxBg);
    Pen boxPen(Color(255, 100, 100, 120), 1.0f);
    Rect boxR(x, y + 2, 18, 18);
    gfx.FillRectangle(&boxBrush, boxR);
    gfx.DrawRectangle(&boxPen, boxR);

    if (checked) {
        Font cf(L"Segoe UI", 10, FontStyleBold, UnitPoint);
        SolidBrush checkBrush(Color(255, 255, 255, 255));
        StringFormat csf;
        csf.SetAlignment(StringAlignmentCenter);
        csf.SetLineAlignment(StringAlignmentCenter);
        RectF checkRF((float)x, (float)(y + 2), 18.0f, 18.0f);
        gfx.DrawString(L"\u2713", -1, &cf, checkRF, &csf, &checkBrush);
    }

    Font f(L"Segoe UI", 9, FontStyleRegular, UnitPoint);
    SolidBrush textBrush(Color(255, 200, 200, 220));
    RectF textRF((float)(x + 24), (float)y, (float)(w - 24), 22.0f);
    StringFormat sf;
    sf.SetAlignment(StringAlignmentNear);
    sf.SetLineAlignment(StringAlignmentCenter);
    gfx.DrawString(text, -1, &f, textRF, &sf, &textBrush);
}

static void DrawTab(Graphics& gfx, const wchar_t* text, int x, int y, int w, int h, bool active) {
    Color bg = active ? Color(255, 50, 60, 100) : Color(255, 30, 35, 55);
    SolidBrush bgBrush(bg);
    Rect r(x, y, w, h);
    gfx.FillRectangle(&bgBrush, r);
    if (active) {
        Pen activePen(Color(255, 100, 160, 255), 2.0f);
        gfx.DrawLine(&activePen, x, y + h - 1, x + w, y + h - 1);
    }
    Font f(L"Segoe UI", 9, active ? FontStyleBold : FontStyleRegular, UnitPoint);
    SolidBrush textBrush(active ? Color(255, 220, 230, 255) : Color(255, 140, 140, 160));
    StringFormat sf;
    sf.SetAlignment(StringAlignmentCenter);
    sf.SetLineAlignment(StringAlignmentCenter);
    RectF rf((float)x, (float)y, (float)w, (float)h);
    gfx.DrawString(text, -1, &f, rf, &sf, &textBrush);
}

static void DrawStatusBar(Graphics& gfx, int w) {
    if (g_StatusMessage.empty()) return;
    DWORD elapsed = GetTickCount() - g_StatusTime;
    if (elapsed > 5000) { g_StatusMessage = ""; return; }

    int alpha = 255;
    if (elapsed > 3500) alpha = 255 - (int)((elapsed - 3500) * 255 / 1500);
    if (alpha < 0) alpha = 0;

    Color bg, text;
    if (g_StatusColor == 0) { bg = Color(alpha, 20, 80, 30); text = Color(alpha, 100, 255, 120); }
    else if (g_StatusColor == 1) { bg = Color(alpha, 80, 70, 20); text = Color(alpha, 255, 220, 80); }
    else { bg = Color(alpha, 80, 20, 20); text = Color(alpha, 255, 100, 100); }

    int barY = TAB_H;
    SolidBrush bgBrush(bg);
    Rect barR(0, barY, w, STATUS_H);
    gfx.FillRectangle(&bgBrush, barR);

    Font f(L"Segoe UI", 8, FontStyleBold, UnitPoint);
    SolidBrush textBrush(text);
    StringFormat sf;
    sf.SetAlignment(StringAlignmentCenter);
    sf.SetLineAlignment(StringAlignmentCenter);
    RectF barRF(0.0f, (float)barY, (float)w, (float)STATUS_H);
    std::wstring wMsg = StringToWString(g_StatusMessage);
    gfx.DrawString(wMsg.c_str(), -1, &f, barRF, &sf, &textBrush);
}

// ============================================
// PAINT: BUFFS TAB
// ============================================

static void PaintBuffsTab(Graphics& gfx, int w) {
    Font headerFont(L"Segoe UI", 9, FontStyleBold, UnitPoint);
    Font rowFont(L"Segoe UI", 8, FontStyleRegular, UnitPoint);
    SolidBrush headerText(Color(255, 180, 200, 255));
    SolidBrush dimText(Color(255, 120, 120, 140));
    StringFormat lsf;
    lsf.SetAlignment(StringAlignmentNear);
    lsf.SetLineAlignment(StringAlignmentCenter);

    int baseY = TAB_H + STATUS_H + 4;

    RectF modeRF(10.0f, (float)baseY, 200.0f, 24.0f);
    std::wstring modeStr = L"Filter: ";
    if (g_FilterMode == FILTER_SHOW_ALL) modeStr += L"Show All";
    else if (g_FilterMode == FILTER_WHITELIST) modeStr += L"Whitelist";
    else modeStr += L"Blacklist";
    gfx.DrawString(modeStr.c_str(), -1, &headerFont, modeRF, &lsf, &headerText);

    DrawButton(gfx, L"Cycle Mode", 220, baseY, 100, 24);
    DrawButton(gfx, L"All ON", 340, baseY, 70, 24, false, true);
    DrawButton(gfx, L"All OFF", 415, baseY, 70, 24, true);

    int colY = baseY + 28;
    SolidBrush colBg(Color(255, 30, 40, 65));
    Rect colR(10, colY, w - 20, S_ROW_H);
    gfx.FillRectangle(&colBg, colR);
    RectF nameHdrRF(40.0f, (float)colY, 250.0f, (float)S_ROW_H);
    gfx.DrawString(L"Buff Name", -1, &headerFont, nameHdrRF, &lsf, &headerText);
    RectF idHdrRF(300.0f, (float)colY, 180.0f, (float)S_ROW_H);
    gfx.DrawString(L"Unique ID", -1, &headerFont, idHdrRF, &lsf, &dimText);

    int listY = colY + S_ROW_H;
    SolidBrush listBg(Color(255, 20, 25, 40));
    Rect listR(10, listY, w - 20, LIST_H);
    gfx.FillRectangle(&listBg, listR);

    int visStart = g_BuffScrollOffset;
    int visEnd = min((int)g_SortedBuffs.size(), visStart + MAX_VIS);
    int drawY = listY;

    for (int i = visStart; i < visEnd; i++) {
        auto& p = g_SortedBuffs[i];
        if ((i - visStart) % 2 == 1) {
            SolidBrush altBg(Color(255, 25, 30, 50));
            Rect altR(10, drawY, w - 20, S_ROW_H);
            gfx.FillRectangle(&altBg, altR);
        }
        std::wstring wName = StringToWString(p.second.displayName);
        DrawCheckbox(gfx, wName.c_str(), 14, drawY + 1, 280, p.second.enabled);
        std::wstring wId = StringToWString(p.second.uniqueName);
        RectF idRF(300.0f, (float)drawY, 180.0f, (float)S_ROW_H);
        gfx.DrawString(wId.c_str(), -1, &rowFont, idRF, &lsf, &dimText);
        drawY += S_ROW_H;
    }

    int bottomY = listY + LIST_H + 4;
    if ((int)g_SortedBuffs.size() > MAX_VIS) {
        wchar_t scrollText[64];
        swprintf_s(scrollText, L"Scroll: %d-%d of %d", visStart + 1, visEnd, (int)g_SortedBuffs.size());
        RectF scrollRF(10.0f, (float)bottomY, 300.0f, 20.0f);
        gfx.DrawString(scrollText, -1, &rowFont, scrollRF, &lsf, &dimText);
    }

    DrawButton(gfx, L"Save Config", 390, bottomY, 95, BTN_H, false, true);
}

// ============================================
// PAINT: RAID TAB
// ============================================

static void PaintRaidTab(Graphics& gfx, int w) {
    Font headerFont(L"Segoe UI", 9, FontStyleBold, UnitPoint);
    Font rowFont(L"Segoe UI", 8, FontStyleRegular, UnitPoint);
    SolidBrush headerText(Color(255, 180, 200, 255));
    SolidBrush dimText(Color(255, 120, 120, 140));
    SolidBrush whiteText(Color(255, 220, 220, 240));
    StringFormat lsf;
    lsf.SetAlignment(StringAlignmentNear);
    lsf.SetLineAlignment(StringAlignmentCenter);

    int baseY = TAB_H + STATUS_H + 4;

    RectF trackedLabelRF(10.0f, (float)baseY, 200.0f, 20.0f);
    gfx.DrawString(L"Tracked Players:", -1, &headerFont, trackedLabelRF, &lsf, &headerText);

    SolidBrush listBg(Color(255, 20, 25, 40));
    Pen listBorder(Color(255, 50, 60, 90), 1.0f);
    int listY = baseY + 22;
    Rect trackedR(10, listY, 220, 180);
    gfx.FillRectangle(&listBg, trackedR);
    gfx.DrawRectangle(&listBorder, trackedR);

    {
        std::lock_guard<std::mutex> lock(g_RaidMutex);
        int drawY = listY;
        int maxShow = 180 / S_ROW_H;
        int start = g_RaidScrollOffset;
        int end = min((int)g_RaidTrackNames.size(), start + maxShow);
        for (int i = start; i < end; i++) {
            bool sel = (i == g_RaidSelectedIndex);
            if (sel) {
                SolidBrush selBg(Color(255, 50, 70, 120));
                Rect selR(11, drawY, 218, S_ROW_H);
                gfx.FillRectangle(&selBg, selR);
            }
            std::wstring wn = StringToWString(g_RaidTrackNames[i]);
            RectF nameRF(16.0f, (float)drawY, 210.0f, (float)S_ROW_H);
            gfx.DrawString(wn.c_str(), -1, &rowFont, nameRF, &lsf, sel ? &whiteText : &dimText);
            drawY += S_ROW_H;
        }
    }

    int inputLabelY = listY + 186;
    RectF addLabelRF(10.0f, (float)inputLabelY, 100.0f, 20.0f);
    gfx.DrawString(L"Add player:", -1, &rowFont, addLabelRF, &lsf, &dimText);

    int inputY = inputLabelY + 20;
    Color inputBg = g_RaidInputFocused ? Color(255, 40, 45, 70) : Color(255, 25, 30, 50);
    Color inputBorderC = g_RaidInputFocused ? Color(255, 100, 140, 220) : Color(255, 60, 70, 100);
    SolidBrush ibBrush(inputBg);
    Pen ibPen(inputBorderC, 1.0f);
    Rect inputR(10, inputY, 165, 24);
    gfx.FillRectangle(&ibBrush, inputR);
    gfx.DrawRectangle(&ibPen, inputR);

    std::wstring wInput = StringToWString(g_RaidInputText);
    if (wInput.empty() && !g_RaidInputFocused) wInput = L"Type name...";
    SolidBrush inputTextBrush(wInput == L"Type name..." ? Color(255, 80, 80, 100) : Color(255, 220, 220, 240));
    RectF inputRF(14.0f, (float)inputY, 160.0f, 24.0f);
    gfx.DrawString(wInput.c_str(), -1, &rowFont, inputRF, &lsf, &inputTextBrush);

    DrawButton(gfx, L"Add", 180, inputY, 50, 24, false, true);

    int removeY = inputY + 30;
    DrawButton(gfx, L"Remove", 10, removeY, 80, 26, true);

    RectF nearbyLabelRF(250.0f, (float)baseY, 200.0f, 20.0f);
    gfx.DrawString(L"Nearby Players:", -1, &headerFont, nearbyLabelRF, &lsf, &headerText);

    Rect nearbyR(250, listY, 245, 180);
    gfx.FillRectangle(&listBg, nearbyR);
    gfx.DrawRectangle(&listBorder, nearbyR);

    {
        int drawY = listY;
        int maxShow = 180 / S_ROW_H;
        int start = g_NearbyScrollOffset;
        int end = min((int)g_NearbyEntities.size(), start + maxShow);
        for (int i = start; i < end; i++) {
            bool sel = (i == g_NearbySelectedIndex);
            if (sel) {
                SolidBrush selBg(Color(255, 50, 70, 120));
                Rect selR(251, drawY, 243, S_ROW_H);
                gfx.FillRectangle(&selBg, selR);
            }
            bool tracked = false;
            {
                std::lock_guard<std::mutex> lock(g_RaidMutex);
                for (auto& n : g_RaidTrackNames)
                    if (ToLower(n) == ToLower(g_NearbyEntities[i])) { tracked = true; break; }
            }
            std::wstring wn = StringToWString(g_NearbyEntities[i]);
            if (tracked) wn += L" \u2713";
            SolidBrush* brush = tracked ? &headerText : (sel ? &whiteText : &dimText);
            RectF nameRF(255.0f, (float)drawY, 235.0f, (float)S_ROW_H);
            gfx.DrawString(wn.c_str(), -1, &rowFont, nameRF, &lsf, brush);
            drawY += S_ROW_H;
        }
    }

    int nearbyBtnY = listY + 186;
    DrawButton(gfx, L"Refresh", 250, nearbyBtnY, 90, 26);
    DrawButton(gfx, L"Add Selected", 345, nearbyBtnY, 110, 26, false, true);

    DrawButton(gfx, L"Save Raid Config", 345, removeY, 135, BTN_H, false, true);
}

// ============================================
// PAINT: OPTIONS TAB
// ============================================

static void PaintOptionsTab(Graphics& gfx, int w) {
    Font headerFont(L"Segoe UI", 10, FontStyleBold, UnitPoint);
    Font smallFont(L"Segoe UI", 8, FontStyleRegular, UnitPoint);
    SolidBrush headerText(Color(255, 180, 200, 255));
    SolidBrush dimText(Color(255, 140, 140, 160));
    StringFormat lsf;
    lsf.SetAlignment(StringAlignmentNear);
    lsf.SetLineAlignment(StringAlignmentCenter);
    Pen sepPen(Color(255, 50, 60, 90), 1.0f);

    int cy = TAB_H + STATUS_H + 6;

    RectF secLabel(10.0f, (float)cy, 200.0f, 22.0f);
    gfx.DrawString(L"\u2694 PLAYER", -1, &headerFont, secLabel, &lsf, &headerText);
    cy += 24;
    gfx.DrawLine(&sepPen, 10, cy, w - 10, cy);
    cy += 6;

    std::wstring playerStr = L"Current: ";
    playerStr += g_PlayerName.empty() ? L"(none - select below)" : StringToWString(g_PlayerName);
    Color plrColor = g_PlayerName.empty() ? Color(255, 180, 100, 100) : Color(255, 100, 220, 100);
    SolidBrush plrBrush(plrColor);
    RectF plrRF(10.0f, (float)cy, 400.0f, 22.0f);
    gfx.DrawString(playerStr.c_str(), -1, &smallFont, plrRF, &lsf, &plrBrush);
    cy += 24;

    DrawButton(gfx, L"Auto-Find Player", 10, cy, 140, BTN_H, false, true);
    DrawButton(gfx, L"Pick from List \u25B6", 160, cy, 150, BTN_H);
    DrawButton(gfx, L"Reset Player", 320, cy, 120, BTN_H, true);
    cy += 38;

    RectF trackSecLabel(10.0f, (float)cy, 200.0f, 22.0f);
    gfx.DrawString(L"\u26A1 TRACKING", -1, &headerFont, trackSecLabel, &lsf, &headerText);
    cy += 24;
    gfx.DrawLine(&sepPen, 10, cy, w - 10, cy);
    cy += 8;

    DrawCheckbox(gfx, L"Tracking Active", 10, cy, 200, g_TrackingActive);

    std::wstring trackStatus = g_TrackingActive ? L"\u25CF LIVE" : L"\u25CB STOPPED";
    Color trackColor = g_TrackingActive ? Color(255, 80, 255, 80) : Color(255, 180, 80, 80);
    SolidBrush trackBrush(trackColor);
    Font statusFont(L"Segoe UI", 9, FontStyleBold, UnitPoint);
    RectF trackStatusRF(220.0f, (float)cy, 150.0f, 22.0f);
    gfx.DrawString(trackStatus.c_str(), -1, &statusFont, trackStatusRF, &lsf, &trackBrush);
    cy += 34;

    RectF overlaySecLabel(10.0f, (float)cy, 200.0f, 22.0f);
    gfx.DrawString(L"\u2728 OVERLAYS", -1, &headerFont, overlaySecLabel, &lsf, &headerText);
    cy += 24;
    gfx.DrawLine(&sepPen, 10, cy, w - 10, cy);
    cy += 8;

    DrawCheckbox(gfx, L"Buff Overlay", 10, cy, 200, g_OverlayActive);
    DrawCheckbox(gfx, L"Stacks Overlay", 260, cy, 200, g_StacksOverlayActive);
    cy += 28;
    DrawCheckbox(gfx, L"Raid Overlay", 10, cy, 200, g_RaidOverlayActive);
    cy += 38;

    RectF actSecLabel(10.0f, (float)cy, 200.0f, 22.0f);
    gfx.DrawString(L"\u2699 ACTIONS", -1, &headerFont, actSecLabel, &lsf, &headerText);
    cy += 24;
    gfx.DrawLine(&sepPen, 10, cy, w - 10, cy);
    cy += 8;

    DrawButton(gfx, L"Clear Stacks History", 10, cy, 160, BTN_H);
    DrawButton(gfx, L"Save All Configs", 180, cy, 140, BTN_H, false, true);
}

// ============================================
// PAINT: ENTITY PICKER TAB
// ============================================

static void PaintEntitiesTab(Graphics& gfx, int w) {
    Font headerFont(L"Segoe UI", 9, FontStyleBold, UnitPoint);
    Font rowFont(L"Segoe UI", 8, FontStyleRegular, UnitPoint);
    SolidBrush headerText(Color(255, 180, 200, 255));
    SolidBrush dimText(Color(255, 120, 120, 140));
    SolidBrush whiteText(Color(255, 220, 220, 240));
    SolidBrush greenText(Color(255, 100, 255, 120));
    SolidBrush cyanText(Color(255, 100, 200, 255));
    StringFormat lsf;
    lsf.SetAlignment(StringAlignmentNear);
    lsf.SetLineAlignment(StringAlignmentCenter);

    int baseY = TAB_H + STATUS_H + 4;

    wchar_t countText[64];
    swprintf_s(countText, L"Select your character (%d entities):", (int)g_EntityPickerList.size());
    RectF headerRF(10.0f, (float)baseY, 380.0f, 22.0f);
    gfx.DrawString(countText, -1, &headerFont, headerRF, &lsf, &headerText);

    DrawButton(gfx, L"Refresh List", 400, baseY, 100, 22);

    int colY = baseY + 26;
    SolidBrush colBg(Color(255, 30, 40, 65));
    Rect colR(10, colY, w - 20, S_ROW_H);
    gfx.FillRectangle(&colBg, colR);
    RectF nameColRF(14.0f, (float)colY, 300.0f, (float)S_ROW_H);
    gfx.DrawString(L"Entity Name", -1, &headerFont, nameColRF, &lsf, &headerText);
    RectF countColRF(350.0f, (float)colY, 80.0f, (float)S_ROW_H);
    gfx.DrawString(L"Copies", -1, &headerFont, countColRF, &lsf, &dimText);
    RectF statusColRF(430.0f, (float)colY, 60.0f, (float)S_ROW_H);
    gfx.DrawString(L"Status", -1, &headerFont, statusColRF, &lsf, &dimText);

    int listY = colY + S_ROW_H;
    SolidBrush listBgBrush(Color(255, 20, 25, 40));
    int entityListH = 300;
    Rect listR(10, listY, w - 20, entityListH);
    gfx.FillRectangle(&listBgBrush, listR);

    int maxVisEntity = entityListH / S_ROW_H;
    int visStart = g_EntityScrollOffset;
    int visEnd = min((int)g_EntityPickerList.size(), visStart + maxVisEntity);
    int drawY = listY;

    for (int i = visStart; i < visEnd; i++) {
        auto& e = g_EntityPickerList[i];
        bool sel = (i == g_EntitySelectedIndex);

        if (sel) {
            SolidBrush selBg(Color(255, 50, 70, 120));
            Rect selR(10, drawY, w - 20, S_ROW_H);
            gfx.FillRectangle(&selBg, selR);
        }
        else if (e.isCurrent) {
            SolidBrush curBg(Color(255, 25, 60, 30));
            Rect curR(10, drawY, w - 20, S_ROW_H);
            gfx.FillRectangle(&curBg, curR);
        }
        else if ((i - visStart) % 2 == 1) {
            SolidBrush altBg(Color(255, 25, 30, 50));
            Rect altR(10, drawY, w - 20, S_ROW_H);
            gfx.FillRectangle(&altBg, altR);
        }

        std::wstring wName = StringToWString(e.name);
        SolidBrush* nameBrush = e.isCurrent ? &greenText : (e.nameCount == 1 ? &cyanText : &dimText);
        RectF nameRF(14.0f, (float)drawY, 330.0f, (float)S_ROW_H);
        gfx.DrawString(wName.c_str(), -1, &rowFont, nameRF, &lsf, nameBrush);

        wchar_t copyText[16];
        swprintf_s(copyText, L"%d", e.nameCount);
        SolidBrush* copyBrush = e.nameCount == 1 ? &cyanText : &dimText;
        RectF copyRF(350.0f, (float)drawY, 80.0f, (float)S_ROW_H);
        gfx.DrawString(copyText, -1, &rowFont, copyRF, &lsf, copyBrush);

        if (e.isCurrent) {
            RectF statusRF(430.0f, (float)drawY, 60.0f, (float)S_ROW_H);
            gfx.DrawString(L"\u2605 YOU", -1, &rowFont, statusRF, &lsf, &greenText);
        }
        else if (e.nameCount == 1) {
            RectF statusRF(430.0f, (float)drawY, 60.0f, (float)S_ROW_H);
            SolidBrush uniqueBrush(Color(255, 180, 180, 100));
            gfx.DrawString(L"unique", -1, &rowFont, statusRF, &lsf, &uniqueBrush);
        }

        drawY += S_ROW_H;
    }

    int bottomY = listY + entityListH + 4;

    if ((int)g_EntityPickerList.size() > maxVisEntity) {
        wchar_t scrollText[64];
        swprintf_s(scrollText, L"Scroll: %d-%d of %d", visStart + 1, visEnd, (int)g_EntityPickerList.size());
        RectF scrollRF(10.0f, (float)bottomY, 300.0f, 20.0f);
        gfx.DrawString(scrollText, -1, &rowFont, scrollRF, &lsf, &dimText);
    }

    DrawButton(gfx, L"\u2714 Select This Entity", 300, bottomY, 190, BTN_H, false, true);

    Font hintFont(L"Segoe UI", 8, FontStyleItalic, UnitPoint);
    SolidBrush hintBrush(Color(255, 100, 100, 130));
    RectF hintRF(10.0f, (float)(bottomY + 30), 450.0f, 18.0f);
    gfx.DrawString(L"Tip: Your character has a unique name (copies=1, shown in cyan)", -1, &hintFont, hintRF, &lsf, &hintBrush);
}

// ============================================
// PAINT: MISC TAB
// ============================================

static void PaintMiscTab(Graphics& gfx, int w) {
    Font headerFont(L"Segoe UI", 10, FontStyleBold, UnitPoint);
    Font smallFont(L"Segoe UI", 8, FontStyleRegular, UnitPoint);
    Font hintFont(L"Segoe UI", 8, FontStyleItalic, UnitPoint);
    SolidBrush headerText(Color(255, 180, 200, 255));
    SolidBrush dimText(Color(255, 140, 140, 160));
    SolidBrush hintBrush(Color(255, 100, 100, 130));
    StringFormat lsf;
    lsf.SetAlignment(StringAlignmentNear);
    lsf.SetLineAlignment(StringAlignmentCenter);
    Pen sepPen(Color(255, 50, 60, 90), 1.0f);

    int cy = TAB_H + STATUS_H + 6;

    RectF qcSecLabel(10.0f, (float)cy, 300.0f, 22.0f);
    gfx.DrawString(L"\u26A1 QUICK CAST", -1, &headerFont, qcSecLabel, &lsf, &headerText);
    cy += 24;
    gfx.DrawLine(&sepPen, 10, cy, w - 10, cy);
    cy += 8;

    DrawCheckbox(gfx, L"Enable Quick Cast", 10, cy, 250, g_QuickCastEnabled);

    std::wstring qcStatus = g_QuickCastEnabled ? L"\u25CF ACTIVE" : L"\u25CB OFF";
    Color qcColor = g_QuickCastEnabled ? Color(255, 80, 255, 80) : Color(255, 180, 80, 80);
    SolidBrush qcBrush(qcColor);
    Font statusFont(L"Segoe UI", 9, FontStyleBold, UnitPoint);
    RectF qcStatusRF(270.0f, (float)cy, 150.0f, 22.0f);
    gfx.DrawString(qcStatus.c_str(), -1, &statusFont, qcStatusRF, &lsf, &qcBrush);
    cy += 26;

    RectF qcDesc1RF(10.0f, (float)cy, (float)(w - 20), 18.0f);
    gfx.DrawString(L"Ground-target spells auto-place at your mouse cursor.", -1, &smallFont, qcDesc1RF, &lsf, &dimText);
    cy += 16;
    RectF qcDesc2RF(10.0f, (float)cy, (float)(w - 20), 18.0f);
    gfx.DrawString(L"Press the keybind and it fires instantly. Hotkey: CTRL+Q", -1, &smallFont, qcDesc2RF, &lsf, &dimText);
    cy += 26;

    bool qcReady = g_LocalPlayerInputClass.IsValid();
    std::wstring qcInitStr = qcReady ? L"\u2713 QuickCast initialized" : L"\u2717 Not initialized (will init on enable)";
    Color qcInitColor = qcReady ? Color(255, 100, 220, 100) : Color(255, 220, 180, 80);
    SolidBrush qcInitBrush(qcInitColor);
    RectF qcInitRF(10.0f, (float)cy, (float)(w - 20), 18.0f);
    gfx.DrawString(qcInitStr.c_str(), -1, &smallFont, qcInitRF, &lsf, &qcInitBrush);
    cy += 30;

    RectF alSecLabel(10.0f, (float)cy, 300.0f, 22.0f);
    gfx.DrawString(L"\U0001F4E6 AUTO LOOT", -1, &headerFont, alSecLabel, &lsf, &headerText);
    cy += 24;
    gfx.DrawLine(&sepPen, 10, cy, w - 10, cy);
    cy += 8;

    DrawCheckbox(gfx, L"Enable Auto-Loot Corpses", 10, cy, 280, g_AutoLootEnabled);

    std::wstring alStatus = g_AutoLootEnabled ? L"\u25CF ACTIVE" : L"\u25CB OFF";
    Color alColor = g_AutoLootEnabled ? Color(255, 80, 255, 80) : Color(255, 180, 80, 80);
    SolidBrush alBrush(alColor);
    RectF alStatusRF(300.0f, (float)cy, 150.0f, 22.0f);
    gfx.DrawString(alStatus.c_str(), -1, &statusFont, alStatusRF, &lsf, &alBrush);
    cy += 26;

    RectF alDesc1RF(10.0f, (float)cy, (float)(w - 20), 18.0f);
    gfx.DrawString(L"Automatically loots all items when a corpse window opens.", -1, &smallFont, alDesc1RF, &lsf, &dimText);
    cy += 16;
    RectF alDesc2RF(10.0f, (float)cy, (float)(w - 20), 18.0f);
    gfx.DrawString(L"Use your in-game 'Open Nearby Corpses' keybind as normal.", -1, &smallFont, alDesc2RF, &lsf, &dimText);
    cy += 16;
    RectF alDesc3RF(10.0f, (float)cy, (float)(w - 20), 18.0f);
    gfx.DrawString(L"The loot window will open and instantly collect everything. Hotkey: F10", -1, &smallFont, alDesc3RF, &lsf, &dimText);
    cy += 26;

    extern bool g_AutoLootEnabled;
    std::wstring alInitStr = L"\u2713 AutoLoot system initialized";
    Color alInitColor = Color(255, 100, 220, 100);
    SolidBrush alInitBrush(alInitColor);
    RectF alInitRF(10.0f, (float)cy, (float)(w - 20), 18.0f);
    gfx.DrawString(alInitStr.c_str(), -1, &smallFont, alInitRF, &lsf, &alInitBrush);
    cy += 30;

    RectF howLabel(10.0f, (float)cy, 300.0f, 22.0f);
    gfx.DrawString(L"\u2139 HOW IT WORKS", -1, &headerFont, howLabel, &lsf, &headerText);
    cy += 24;
    gfx.DrawLine(&sepPen, 10, cy, w - 10, cy);
    cy += 8;

    const wchar_t* steps[] = {
        L"  QuickCast:  Press spell key \u2192 fires at mouse instantly",
        L"  AutoLoot:   Open corpses \u2192 loot grabbed automatically",
        nullptr
    };
    for (int i = 0; steps[i]; i++) {
        RectF stepRF(10.0f, (float)cy, (float)(w - 20), 18.0f);
        gfx.DrawString(steps[i], -1, &smallFont, stepRF, &lsf, &dimText);
        cy += 18;
    }
    cy += 12;

    RectF tipRF(10.0f, (float)cy, (float)(w - 20), 18.0f);
    gfx.DrawString(L"Tip: Both features can also be toggled via hotkeys while in-game", -1, &hintFont, tipRF, &lsf, &hintBrush);
}

// ============================================
// MAIN PAINT
// ============================================

static void PaintSettings(HWND hwnd) {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    if (!hdc) return;

    RECT cr;
    GetClientRect(hwnd, &cr);
    int w = cr.right, h = cr.bottom;
    if (w <= 0 || h <= 0) { EndPaint(hwnd, &ps); return; }

    HDC mem = CreateCompatibleDC(hdc);
    if (!mem) { EndPaint(hwnd, &ps); return; }
    HBITMAP bmp = CreateCompatibleBitmap(hdc, w, h);
    if (!bmp) { DeleteDC(mem); EndPaint(hwnd, &ps); return; }
    HBITMAP old = (HBITMAP)SelectObject(mem, bmp);

    try {
        Graphics gfx(mem);
        gfx.SetSmoothingMode(SmoothingModeAntiAlias);
        gfx.SetTextRenderingHint(TextRenderingHintAntiAlias);

        SolidBrush bgBrush(Color(255, 18, 22, 36));
        Rect bgR(0, 0, w, h);
        gfx.FillRectangle(&bgBrush, bgR);

        SolidBrush titleBg(Color(255, 25, 30, 50));
        Rect titleR(0, 0, w, TAB_H);
        gfx.FillRectangle(&titleBg, titleR);

        int tabW = w / 5;
        DrawTab(gfx, L"\u2699 Options", 0, 0, tabW, TAB_H, g_Tab == TAB_OPTIONS);
        DrawTab(gfx, L"\u2728 Buffs", tabW, 0, tabW, TAB_H, g_Tab == TAB_BUFFS);
        DrawTab(gfx, L"\u2694 Raid", tabW * 2, 0, tabW, TAB_H, g_Tab == TAB_RAID);
        DrawTab(gfx, L"\u263A Entities", tabW * 3, 0, tabW, TAB_H, g_Tab == TAB_ENTITIES);
        DrawTab(gfx, L"\u2606 Misc", tabW * 4, 0, w - tabW * 4, TAB_H, g_Tab == TAB_MISC);

        switch (g_Tab) {
        case TAB_OPTIONS: PaintOptionsTab(gfx, w); break;
        case TAB_BUFFS: PaintBuffsTab(gfx, w); break;
        case TAB_RAID: PaintRaidTab(gfx, w); break;
        case TAB_ENTITIES: PaintEntitiesTab(gfx, w); break;
        case TAB_MISC: PaintMiscTab(gfx, w); break;
        }

        DrawStatusBar(gfx, w);

        BitBlt(hdc, 0, 0, w, h, mem, 0, 0, SRCCOPY);
    }
    catch (...) {}

    SelectObject(mem, old);
    DeleteObject(bmp);
    DeleteDC(mem);
    EndPaint(hwnd, &ps);
}

// ============================================
// CLICK HANDLING
// ============================================

static void HandleClick(HWND hwnd, int mx, int my) {
    int w = SETTINGS_W;
    int tabW = w / 5;

    if (my < TAB_H) {
        if (mx < tabW) { g_Tab = TAB_OPTIONS; }
        else if (mx < tabW * 2) { g_Tab = TAB_BUFFS; RebuildBuffCache(); g_BuffScrollOffset = 0; }
        else if (mx < tabW * 3) { g_Tab = TAB_RAID; g_RaidScrollOffset = 0; }
        else if (mx < tabW * 4) { g_Tab = TAB_ENTITIES; RebuildEntityPicker(); }
        else { g_Tab = TAB_MISC; }
        InvalidateRect(hwnd, nullptr, FALSE);
        return;
    }

    int baseY = TAB_H + STATUS_H + 4;

    // === BUFFS ===
    if (g_Tab == TAB_BUFFS) {
        if (HitTest(mx, my, 220, baseY, 100, 24)) {
            g_FilterMode = (FilterMode)(((int)g_FilterMode + 1) % 3);
            std::string modes[] = { "Show All", "Whitelist", "Blacklist" };
            SetStatus("Filter: " + modes[(int)g_FilterMode], 1);
            InvalidateRect(hwnd, nullptr, FALSE);
            return;
        }
        if (HitTest(mx, my, 340, baseY, 70, 24)) {
            std::lock_guard<std::mutex> lock(g_ConfigMutex);
            for (auto& p : g_BuffConfig) p.second.enabled = true;
            RebuildBuffCache();
            SetStatus("All buffs enabled", 0);
            InvalidateRect(hwnd, nullptr, FALSE);
            return;
        }
        if (HitTest(mx, my, 415, baseY, 70, 24)) {
            std::lock_guard<std::mutex> lock(g_ConfigMutex);
            for (auto& p : g_BuffConfig) p.second.enabled = false;
            RebuildBuffCache();
            SetStatus("All buffs disabled", 1);
            InvalidateRect(hwnd, nullptr, FALSE);
            return;
        }

        int colY = baseY + 28;
        int listY = colY + S_ROW_H;
        int bottomY = listY + LIST_H + 4;

        if (HitTest(mx, my, 390, bottomY, 95, BTN_H)) {
            SaveConfig();
            SetStatus("Buff config saved!", 0);
            InvalidateRect(hwnd, nullptr, FALSE);
            return;
        }
        if (my >= listY && my < listY + LIST_H) {
            int row = (my - listY) / S_ROW_H;
            int idx = row + g_BuffScrollOffset;
            if (idx >= 0 && idx < (int)g_SortedBuffs.size()) {
                std::string key = g_SortedBuffs[idx].first;
                std::lock_guard<std::mutex> lock(g_ConfigMutex);
                auto it = g_BuffConfig.find(key);
                if (it != g_BuffConfig.end()) {
                    it->second.enabled = !it->second.enabled;
                    g_SortedBuffs[idx].second.enabled = it->second.enabled;
                }
            }
            InvalidateRect(hwnd, nullptr, FALSE);
            return;
        }
    }

    // === RAID ===
    if (g_Tab == TAB_RAID) {
        int listY = baseY + 22;
        int inputLabelY = listY + 186;
        int inputY = inputLabelY + 20;
        int removeY = inputY + 30;
        int nearbyBtnY = listY + 186;

        if (HitTest(mx, my, 10, listY, 220, 180)) {
            int row = (my - listY) / S_ROW_H + g_RaidScrollOffset;
            std::lock_guard<std::mutex> lock(g_RaidMutex);
            if (row >= 0 && row < (int)g_RaidTrackNames.size()) g_RaidSelectedIndex = row;
            InvalidateRect(hwnd, nullptr, FALSE);
            return;
        }
        if (HitTest(mx, my, 250, listY, 245, 180)) {
            int row = (my - listY) / S_ROW_H + g_NearbyScrollOffset;
            if (row >= 0 && row < (int)g_NearbyEntities.size()) g_NearbySelectedIndex = row;
            InvalidateRect(hwnd, nullptr, FALSE);
            return;
        }
        g_RaidInputFocused = HitTest(mx, my, 10, inputY, 165, 24);
        if (HitTest(mx, my, 180, inputY, 50, 24)) {
            if (!g_RaidInputText.empty()) {
                std::lock_guard<std::mutex> lock(g_RaidMutex);
                bool exists = false;
                for (auto& n : g_RaidTrackNames) if (ToLower(n) == ToLower(g_RaidInputText)) { exists = true; break; }
                if (!exists) { g_RaidTrackNames.push_back(g_RaidInputText); SetStatus("Added: " + g_RaidInputText, 0); }
                else SetStatus("Already tracking!", 1);
                g_RaidInputText = "";
            }
            InvalidateRect(hwnd, nullptr, FALSE);
            return;
        }
        if (HitTest(mx, my, 10, removeY, 80, 26)) {
            std::lock_guard<std::mutex> lock(g_RaidMutex);
            if (g_RaidSelectedIndex >= 0 && g_RaidSelectedIndex < (int)g_RaidTrackNames.size()) {
                SetStatus("Removed: " + g_RaidTrackNames[g_RaidSelectedIndex], 1);
                g_RaidTrackNames.erase(g_RaidTrackNames.begin() + g_RaidSelectedIndex);
                g_RaidSelectedIndex = -1;
            }
            InvalidateRect(hwnd, nullptr, FALSE);
            return;
        }
        if (HitTest(mx, my, 250, nearbyBtnY, 90, 26)) {
            if (RebuildNearbyCache()) {
                g_NearbySelectedIndex = -1;
                g_NearbyScrollOffset = 0;
                char msg[64]; sprintf_s(msg, "Found %d nearby", (int)g_NearbyEntities.size());
                SetStatus(msg, 0);
            }
            InvalidateRect(hwnd, nullptr, FALSE);
            return;
        }
        if (HitTest(mx, my, 345, nearbyBtnY, 110, 26)) {
            if (g_NearbySelectedIndex >= 0 && g_NearbySelectedIndex < (int)g_NearbyEntities.size()) {
                std::string name = g_NearbyEntities[g_NearbySelectedIndex];
                std::lock_guard<std::mutex> lock(g_RaidMutex);
                bool exists = false;
                for (auto& n : g_RaidTrackNames) if (ToLower(n) == ToLower(name)) { exists = true; break; }
                if (!exists) { g_RaidTrackNames.push_back(name); SetStatus("Added: " + name, 0); }
                else SetStatus("Already tracking!", 1);
            }
            InvalidateRect(hwnd, nullptr, FALSE);
            return;
        }
        if (HitTest(mx, my, 345, removeY, 135, BTN_H)) {
            SaveRaidConfig();
            SetStatus("Raid config saved!", 0);
            InvalidateRect(hwnd, nullptr, FALSE);
            return;
        }
    }

    // === OPTIONS ===
    if (g_Tab == TAB_OPTIONS) {
        int cy = TAB_H + STATUS_H + 6;
        cy += 24 + 6 + 24;

        if (HitTest(mx, my, 10, cy, 140, BTN_H)) {
            bool found = FindPlayerEntity();
            g_Tab = TAB_ENTITIES;
            g_LastEntityRefresh = 0;
            RebuildEntityPicker();
            if (found) {
                g_TrackingActive = true;
                SetStatus("Found: " + g_PlayerName + " - Tracking!", 0);
            }
            else {
                SetStatus("Auto-detect failed. Pick from list.", 2);
            }
            InvalidateRect(hwnd, nullptr, FALSE);
            return;
        }
        if (HitTest(mx, my, 160, cy, 150, BTN_H)) {
            g_Tab = TAB_ENTITIES;
            g_LastEntityRefresh = 0;
            RebuildEntityPicker();
            InvalidateRect(hwnd, nullptr, FALSE);
            return;
        }
        if (HitTest(mx, my, 320, cy, 120, BTN_H)) {
            g_TrackingActive = false;
            g_PlayerEntity = nullptr;
            g_PlayerModel = nullptr;
            g_PlayerName = "";
            { std::lock_guard<std::mutex> l(g_EffectsMutex); g_CurrentEffects.clear(); }
            SetStatus("Player reset.", 1);
            InvalidateRect(hwnd, nullptr, FALSE);
            return;
        }
        cy += 38;

        cy += 24 + 8;
        if (HitTest(mx, my, 10, cy, 200, 24)) {
            if (g_PlayerEntity) { g_TrackingActive = !g_TrackingActive; SetStatus(g_TrackingActive ? "Tracking ON" : "Tracking OFF", g_TrackingActive ? 0 : 1); }
            else SetStatus("No player selected!", 2);
            InvalidateRect(hwnd, nullptr, FALSE);
            return;
        }
        cy += 34;

        cy += 24 + 8;
        // Buff Overlay checkbox — just toggle the bool, unified overlay handles visibility
        if (HitTest(mx, my, 10, cy, 200, 24)) {
            g_OverlayActive = !g_OverlayActive;
            SetStatus(g_OverlayActive ? "Buff panel ON" : "Buff panel OFF", g_OverlayActive ? 0 : 1);
            InvalidateRect(hwnd, nullptr, FALSE);
            return;
        }
        // Stacks Overlay checkbox — just toggle the bool
        if (HitTest(mx, my, 260, cy, 200, 24)) {
            g_StacksOverlayActive = !g_StacksOverlayActive;
            SetStatus(g_StacksOverlayActive ? "Stacks panel ON" : "Stacks panel OFF", g_StacksOverlayActive ? 0 : 1);
            InvalidateRect(hwnd, nullptr, FALSE);
            return;
        }
        cy += 28;
        // Raid Overlay checkbox — just toggle the bool
        if (HitTest(mx, my, 10, cy, 200, 24)) {
            g_RaidOverlayActive = !g_RaidOverlayActive;
            SetStatus(g_RaidOverlayActive ? "Raid panel ON" : "Raid panel OFF", g_RaidOverlayActive ? 0 : 1);
            InvalidateRect(hwnd, nullptr, FALSE);
            return;
        }
        cy += 38;

        cy += 24 + 8;
        if (HitTest(mx, my, 10, cy, 160, BTN_H)) {
            { std::lock_guard<std::mutex> l(g_StacksMutex); g_TrackedStacks.clear(); }
            SetStatus("Stacks cleared!", 0);
            InvalidateRect(hwnd, nullptr, FALSE);
            return;
        }
        if (HitTest(mx, my, 180, cy, 140, BTN_H)) {
            SaveConfig();
            SaveRaidConfig();
            SetStatus("All configs saved!", 0);
            InvalidateRect(hwnd, nullptr, FALSE);
            return;
        }
    }

    // === ENTITIES ===
    if (g_Tab == TAB_ENTITIES) {
        if (HitTest(mx, my, 400, baseY, 100, 22)) {
            RebuildEntityPicker();
            InvalidateRect(hwnd, nullptr, FALSE);
            return;
        }
        int colY = baseY + 26;
        int listY = colY + S_ROW_H;
        int entityListH = 300;
        int bottomY = listY + entityListH + 4;

        if (my >= listY && my < listY + entityListH) {
            int row = (my - listY) / S_ROW_H;
            int idx = row + g_EntityScrollOffset;
            if (idx >= 0 && idx < (int)g_EntityPickerList.size()) {
                g_EntitySelectedIndex = idx;
            }
            InvalidateRect(hwnd, nullptr, FALSE);
            return;
        }
        if (HitTest(mx, my, 300, bottomY, 190, BTN_H)) {
            if (g_EntitySelectedIndex >= 0 && g_EntitySelectedIndex < (int)g_EntityPickerList.size()) {
                auto& e = g_EntityPickerList[g_EntitySelectedIndex];
                g_PlayerEntity = e.entityObj;
                g_PlayerModel = e.modelObj;
                g_PlayerName = e.name;
                g_TrackingActive = true;
                for (auto& ent : g_EntityPickerList) ent.isCurrent = (ent.entityObj == g_PlayerEntity);
                SetStatus("Selected: " + e.name + " - Tracking!", 0);
                LogSuccess(("UI Selected: " + e.name).c_str());
            }
            else {
                SetStatus("Click an entity first!", 2);
            }
            InvalidateRect(hwnd, nullptr, FALSE);
            return;
        }
    }

    // === MISC ===
    if (g_Tab == TAB_MISC) {
        int cy = TAB_H + STATUS_H + 6;
        cy += 24;
        cy += 8;

        if (HitTest(mx, my, 10, cy, 250, 24)) {
            ToggleQuickCast();
            if (g_QuickCastEnabled) {
                SetStatus("QuickCast ENABLED - ground spells auto-place!", 0);
            }
            else {
                SetStatus("QuickCast DISABLED - normal casting", 1);
            }
            InvalidateRect(hwnd, nullptr, FALSE);
            return;
        }
        cy += 26;
        cy += 16;
        cy += 16;
        cy += 26;
        cy += 18;
        cy += 30;

        cy += 24;
        cy += 8;

        if (HitTest(mx, my, 10, cy, 280, 24)) {
            ToggleAutoLoot();
            if (g_AutoLootEnabled) {
                SetStatus("AutoLoot ENABLED - corpses will auto-loot!", 0);
            }
            else {
                SetStatus("AutoLoot DISABLED - manual looting", 1);
            }
            InvalidateRect(hwnd, nullptr, FALSE);
            return;
        }
    }

    InvalidateRect(hwnd, nullptr, FALSE);
}

// ============================================
// WINDOW PROC
// ============================================

static LRESULT CALLBACK SettingsWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_PAINT:
        PaintSettings(hwnd);
        return 0;

    case WM_LBUTTONDOWN:
        HandleClick(hwnd, LOWORD(lParam), HIWORD(lParam));
        return 0;

    case WM_MOUSEWHEEL: {
        int delta = GET_WHEEL_DELTA_WPARAM(wParam);
        int scroll = (delta > 0) ? -2 : 2;
        if (g_Tab == TAB_BUFFS) {
            g_BuffScrollOffset = max(0, min((int)g_SortedBuffs.size() - MAX_VIS, g_BuffScrollOffset + scroll));
        }
        else if (g_Tab == TAB_RAID) {
            POINT pt; GetCursorPos(&pt); ScreenToClient(hwnd, &pt);
            if (pt.x < 240) {
                std::lock_guard<std::mutex> lock(g_RaidMutex);
                int maxS = max(0, (int)g_RaidTrackNames.size() - (180 / S_ROW_H));
                g_RaidScrollOffset = max(0, min(maxS, g_RaidScrollOffset + scroll));
            }
            else {
                int maxS = max(0, (int)g_NearbyEntities.size() - (180 / S_ROW_H));
                g_NearbyScrollOffset = max(0, min(maxS, g_NearbyScrollOffset + scroll));
            }
        }
        else if (g_Tab == TAB_ENTITIES) {
            int maxS = max(0, (int)g_EntityPickerList.size() - (300 / S_ROW_H));
            g_EntityScrollOffset = max(0, min(maxS, g_EntityScrollOffset + scroll));
        }
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    }

    case WM_CHAR:
        if (g_Tab == TAB_RAID && g_RaidInputFocused) {
            char c = (char)wParam;
            if (c == '\b') {
                if (!g_RaidInputText.empty()) g_RaidInputText.pop_back();
            }
            else if (c == '\r' || c == '\n') {
                if (!g_RaidInputText.empty()) {
                    std::lock_guard<std::mutex> lock(g_RaidMutex);
                    bool exists = false;
                    for (auto& n : g_RaidTrackNames) if (ToLower(n) == ToLower(g_RaidInputText)) { exists = true; break; }
                    if (!exists) { g_RaidTrackNames.push_back(g_RaidInputText); SetStatus("Added: " + g_RaidInputText, 0); }
                    else SetStatus("Already tracking!", 1);
                    g_RaidInputText = "";
                }
            }
            else if (c >= 32 && c < 127 && g_RaidInputText.length() < 30) {
                g_RaidInputText += c;
            }
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        break;

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) { ShowWindow(hwnd, SW_HIDE); return 0; }
        break;

    case WM_ERASEBKGND:
        return 1;

    case WM_CLOSE:
        ShowWindow(hwnd, SW_HIDE);
        return 0;

    case WM_DESTROY:
        g_SettingsHwnd = nullptr;
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

// ============================================
// PUBLIC API
// ============================================

void CreateSettingsIfRequested() {
    if (!g_SettingsRequested && !g_EntityPickerRequested) return;

    bool openToEntities = g_EntityPickerRequested;
    g_SettingsRequested = false;
    g_EntityPickerRequested = false;

    if (g_SettingsHwnd && IsWindow(g_SettingsHwnd)) {
        if (openToEntities) { g_Tab = TAB_ENTITIES; g_LastEntityRefresh = 0; RebuildEntityPicker(); }
        ShowWindow(g_SettingsHwnd, SW_SHOW);
        SetForegroundWindow(g_SettingsHwnd);
        InvalidateRect(g_SettingsHwnd, nullptr, FALSE);
        return;
    }

    if (!g_SettingsClassRegistered) {
        WNDCLASSEXA wc = {};
        wc.cbSize = sizeof(wc);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = SettingsWndProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = nullptr;
        wc.lpszClassName = "EthyrialSettings";
        if (RegisterClassExA(&wc) || GetLastError() == ERROR_CLASS_ALREADY_EXISTS)
            g_SettingsClassRegistered = true;
        else { LogError("[SETTINGS] Class register failed!"); return; }
    }

    if (openToEntities) { g_Tab = TAB_ENTITIES; g_LastEntityRefresh = 0; RebuildEntityPicker(); }
    else { g_Tab = TAB_OPTIONS; }
    RebuildBuffCache();

    g_SettingsHwnd = CreateWindowExA(
        WS_EX_TOPMOST,
        "EthyrialSettings",
        "Ethyrial Buff Tracker",
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT, SETTINGS_W, SETTINGS_H,
        nullptr, nullptr, GetModuleHandle(nullptr), nullptr);

    if (g_SettingsHwnd) {
        ShowWindow(g_SettingsHwnd, SW_SHOW);
        UpdateWindow(g_SettingsHwnd);
        LogSuccess("[SETTINGS] Window opened!");
    }
    else {
        LogError("[SETTINGS] CreateWindow failed!");
    }
}

void RequestOpenSettings() { g_SettingsRequested = true; }
void OpenSettingsWindow() { RequestOpenSettings(); }
void RequestEntityPicker() { g_EntityPickerRequested = true; }

void CloseSettingsWindow() {
    if (g_SettingsHwnd && IsWindow(g_SettingsHwnd))
        ShowWindow(g_SettingsHwnd, SW_HIDE);
}

bool IsSettingsOpen() {
    return g_SettingsHwnd && IsWindow(g_SettingsHwnd) && IsWindowVisible(g_SettingsHwnd);
}