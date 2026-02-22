#include "Overlay.h"
#include "Globals.h"
#include "Config.h"
#include "Console.h"
#include <algorithm>

// ============================================
// CACHED FONTS
// ============================================

static Font* g_FontName = nullptr;
static Font* g_FontTime = nullptr;
static Font* g_FontHeader = nullptr;
static Font* g_FontIcon = nullptr;
static Font* g_FontStack = nullptr;

static Font* g_StkFontName = nullptr;
static Font* g_StkFontHeader = nullptr;
static Font* g_StkFontStack = nullptr;
static Font* g_StkFontPeak = nullptr;

static Font* g_RaidFontHeader = nullptr;
static Font* g_RaidFontName = nullptr;
static Font* g_RaidFontEffect = nullptr;
static Font* g_RaidFontTime = nullptr;
static Font* g_RaidFontStatus = nullptr;

static bool g_FontCacheInit = false;

static void InitFontCache() {
    if (g_FontCacheInit) return;

    g_FontName = new Font(L"Segoe UI", 9, FontStyleBold, UnitPoint);
    g_FontTime = new Font(L"Segoe UI", 9, FontStyleBold, UnitPoint);
    g_FontHeader = new Font(L"Segoe UI", 10, FontStyleBold, UnitPoint);
    g_FontIcon = new Font(L"Segoe UI", 12, FontStyleBold, UnitPoint);
    g_FontStack = new Font(L"Segoe UI", 7, FontStyleBold, UnitPoint);

    g_StkFontName = new Font(L"Segoe UI", 9, FontStyleBold, UnitPoint);
    g_StkFontHeader = new Font(L"Segoe UI", 10, FontStyleBold, UnitPoint);
    g_StkFontStack = new Font(L"Segoe UI", 14, FontStyleBold, UnitPoint);
    g_StkFontPeak = new Font(L"Segoe UI", 7, FontStyleRegular, UnitPoint);

    g_RaidFontHeader = new Font(L"Segoe UI", 10, FontStyleBold, UnitPoint);
    g_RaidFontName = new Font(L"Segoe UI", 9, FontStyleBold, UnitPoint);
    g_RaidFontEffect = new Font(L"Segoe UI", 8, FontStyleRegular, UnitPoint);
    g_RaidFontTime = new Font(L"Segoe UI", 8, FontStyleBold, UnitPoint);
    g_RaidFontStatus = new Font(L"Segoe UI", 8, FontStyleItalic, UnitPoint);

    g_FontCacheInit = true;
}

// ============================================
// SIZE CALCULATORS
// ============================================

int CalcBuffPanelWidth() { return BAR_WIDTH + ICON_SIZE + PADDING * 3; }

int CalcBuffPanelHeight() {
    if (!g_OverlayActive) return 0;
    int count = 0;
    { std::lock_guard<std::mutex> lock(g_EffectsMutex); for (auto& e : g_CurrentEffects) if (IsBuffVisible(e)) count++; }
    if (count == 0) return 0;
    return HEADER_HEIGHT + count * ROW_HEIGHT + PADDING * 2;
}

int CalcStacksPanelWidth() { return BAR_WIDTH + PADDING * 2; }

int CalcStacksPanelHeight() {
    if (!g_StacksOverlayActive) return 0;
    int count = 0;
    { std::lock_guard<std::mutex> lock(g_StacksMutex); count = (int)g_TrackedStacks.size(); }
    if (count == 0) return 0;
    return HEADER_HEIGHT + count * ROW_HEIGHT + PADDING * 2;
}

int CalcRaidPanelWidth() { return RAID_BAR_WIDTH + PADDING * 2; }

int CalcRaidPanelHeight() {
    if (!g_RaidOverlayActive) return 0;
    int totalRows = 0;
    {
        std::lock_guard<std::mutex> lock(g_RaidMutex);
        if (g_RaidMembers.empty()) return 0;
        for (auto& m : g_RaidMembers) {
            totalRows++;
            if (m.isNearby) { int v = 0; for (auto& e : m.effects) if (!e.removed && !e.hideFromInfobar) v++; totalRows += (v > 0) ? v : 1; }
            else totalRows++;
        }
    }
    if (totalRows == 0) return 0;
    return RAID_HEADER_HEIGHT + totalRows * RAID_ROW_HEIGHT + PADDING * 2;
}

// ============================================
// BOUNDING BOX — window must cover all panels
// ============================================

void CalcWindowOrigin(int& outX, int& outY) {
    int minX = 99999, minY = 99999;
    if (CalcBuffPanelHeight() > 0) { if (g_OverlayX < minX) minX = g_OverlayX; if (g_OverlayY < minY) minY = g_OverlayY; }
    if (CalcStacksPanelHeight() > 0) { if (g_StacksX < minX) minX = g_StacksX; if (g_StacksY < minY) minY = g_StacksY; }
    if (CalcRaidPanelHeight() > 0) { if (g_RaidX < minX) minX = g_RaidX; if (g_RaidY < minY) minY = g_RaidY; }
    outX = (minX == 99999) ? 0 : minX;
    outY = (minY == 99999) ? 0 : minY;
}

int CalcTotalOverlayWidth() {
    int winX, winY; CalcWindowOrigin(winX, winY);
    int maxR = 0;
    if (CalcBuffPanelHeight() > 0) { int r = g_OverlayX + CalcBuffPanelWidth();  if (r > maxR) maxR = r; }
    if (CalcStacksPanelHeight() > 0) { int r = g_StacksX + CalcStacksPanelWidth(); if (r > maxR) maxR = r; }
    if (CalcRaidPanelHeight() > 0) { int r = g_RaidX + CalcRaidPanelWidth();    if (r > maxR) maxR = r; }
    return maxR - winX;
}

int CalcTotalOverlayHeight() {
    int winX, winY; CalcWindowOrigin(winX, winY);
    int maxB = 0;
    int bh = CalcBuffPanelHeight();   if (bh > 0) { int b = g_OverlayY + bh; if (b > maxB) maxB = b; }
    int sh = CalcStacksPanelHeight(); if (sh > 0) { int b = g_StacksY + sh; if (b > maxB) maxB = b; }
    int rh = CalcRaidPanelHeight();   if (rh > 0) { int b = g_RaidY + rh;   if (b > maxB) maxB = b; }
    return maxB - winY;
}

// ============================================
// DRAG STATE
// ============================================

enum DragTarget { DRAG_NONE = 0, DRAG_BUFF, DRAG_STACKS, DRAG_RAID };
static DragTarget g_DragTarget = DRAG_NONE;
static POINT g_DragStartMouse = {};
static int g_DragStartPanelX = 0;
static int g_DragStartPanelY = 0;

// Hit test: which panel header is at screen position (sx, sy)?
static DragTarget HitTestPanel(int sx, int sy) {
    int bh = CalcBuffPanelHeight();
    if (bh > 0 && sx >= g_OverlayX && sx < g_OverlayX + CalcBuffPanelWidth()
        && sy >= g_OverlayY && sy < g_OverlayY + HEADER_HEIGHT)
        return DRAG_BUFF;

    int sh = CalcStacksPanelHeight();
    if (sh > 0 && sx >= g_StacksX && sx < g_StacksX + CalcStacksPanelWidth()
        && sy >= g_StacksY && sy < g_StacksY + HEADER_HEIGHT)
        return DRAG_STACKS;

    int rh = CalcRaidPanelHeight();
    if (rh > 0 && sx >= g_RaidX && sx < g_RaidX + CalcRaidPanelWidth()
        && sy >= g_RaidY && sy < g_RaidY + RAID_HEADER_HEIGHT)
        return DRAG_RAID;

    return DRAG_NONE;
}

// ============================================
// PAINT: BUFF PANEL
// ============================================

static void PaintBuffPanel(Graphics& gfx, int px, int py) {
    std::lock_guard<std::mutex> lock(g_EffectsMutex);
    std::vector<StatusEffectInfo*> vis;
    for (auto& e : g_CurrentEffects) if (IsBuffVisible(e)) vis.push_back(&e);
    if (vis.empty()) return;

    int tw = CalcBuffPanelWidth();
    int th = HEADER_HEIGHT + (int)vis.size() * ROW_HEIGHT + PADDING * 2;

    SolidBrush bgBrush(Color(200, 20, 20, 30));
    Pen borderPen(Color(200, 60, 60, 80), 1.0f);
    Rect bgR(px, py, tw, th);
    gfx.FillRectangle(&bgBrush, bgR);
    gfx.DrawRectangle(&borderPen, bgR);

    SolidBrush headerBrush(Color(255, 200, 200, 220));
    StringFormat hsf; hsf.SetAlignment(StringAlignmentCenter); hsf.SetLineAlignment(StringAlignmentCenter);
    std::wstring ht = g_PlayerName.empty() ? L"BUFFS" : StringToWString(g_PlayerName) + L" - Buffs";
    RectF headerRect((REAL)(px + PADDING), (REAL)(py + 2), (REAL)(tw - PADDING * 2), (REAL)HEADER_HEIGHT);
    gfx.DrawString(ht.c_str(), -1, g_FontHeader, headerRect, &hsf, &headerBrush);

    Pen sepPen(Color(150, 80, 80, 100), 1.0f);
    gfx.DrawLine(&sepPen, px + PADDING, py + HEADER_HEIGHT, px + tw - PADDING, py + HEADER_HEIGHT);

    SolidBrush whiteBrush(Color(255, 255, 255, 255));
    StringFormat nfmt; nfmt.SetAlignment(StringAlignmentNear); nfmt.SetLineAlignment(StringAlignmentCenter); nfmt.SetTrimming(StringTrimmingEllipsisCharacter);
    StringFormat cfmt; cfmt.SetAlignment(StringAlignmentCenter); cfmt.SetLineAlignment(StringAlignmentCenter);

    int y = py + HEADER_HEIGHT + PADDING;
    for (auto* info : vis) {
        std::wstring wn = StringToWString(info->name.empty() ? info->uniqueName : info->name);
        float rem = info->GetRemainingTime();
        bool perm = info->IsPermanent();
        float prog = (!perm && info->maxDuration > 0) ? max(0.0f, min(1.0f, rem / info->maxDuration)) : 1.0f;
        int bx = px + PADDING + ICON_SIZE + PADDING, by = y, bh = ROW_HEIGHT - 4;

        SolidBrush iconBg(Color(180, 40, 40, 60));
        Pen iconBorder(Color(200, 80, 80, 100), 1.0f);
        Rect iconR(px + PADDING, by, ICON_SIZE, bh);
        gfx.FillRectangle(&iconBg, iconR);
        gfx.DrawRectangle(&iconBorder, iconR);
        wchar_t ic[2] = { wn.empty() ? L'?' : wn[0], 0 };
        RectF iconRect((REAL)(px + PADDING), (REAL)by, (REAL)ICON_SIZE, (REAL)bh);
        gfx.DrawString(ic, 1, g_FontIcon, iconRect, &cfmt, &whiteBrush);

        int nw = (int)(BAR_WIDTH * 0.65f);
        Color nc = perm ? Color(220, 30, 120, 50) : rem > 120 ? Color(220, 30, 140, 50) : rem > 30 ? Color(220, 160, 140, 20) : Color(220, 180, 40, 30);
        SolidBrush nameBrush(nc);
        Rect nameR(bx, by, nw, bh);
        gfx.FillRectangle(&nameBrush, nameR);
        RectF nameRect((REAL)(bx + 4), (REAL)by, (REAL)(nw - 8), (REAL)bh);
        gfx.DrawString(wn.c_str(), -1, g_FontName, nameRect, &nfmt, &whiteBrush);

        int tx = bx + nw + 2, tw2 = BAR_WIDTH - nw - 2;
        SolidBrush timeBg(Color(200, 80, 20, 20));
        Rect timeR(tx, by, tw2, bh);
        gfx.FillRectangle(&timeBg, timeR);
        if (!perm && prog > 0) {
            Color fc = rem > 120 ? Color(200, 180, 40, 30) : rem > 30 ? Color(200, 200, 80, 20) : Color(220, 220, 30, 30);
            SolidBrush fillBrush(fc);
            Rect fillR(tx, by, (int)(tw2 * prog), bh);
            gfx.FillRectangle(&fillBrush, fillR);
        }

        wchar_t tb[32];
        if (perm) wcscpy_s(tb, L"\u221E");
        else if (rem >= 60) swprintf_s(tb, L"%dm %02ds", (int)(rem / 60), (int)rem % 60);
        else swprintf_s(tb, L"%.1fs", rem);
        RectF timeRect((REAL)tx, (REAL)by, (REAL)tw2, (REAL)bh);
        gfx.DrawString(tb, -1, g_FontTime, timeRect, &cfmt, &whiteBrush);

        if (info->currentStacks > 1.0f) {
            wchar_t sb[16]; swprintf_s(sb, L"x%.0f", info->currentStacks);
            SolidBrush stackBrush(Color(255, 255, 200, 100));
            RectF stackRect((REAL)(px + PADDING), (REAL)(by + bh - 12), (REAL)ICON_SIZE, 12);
            gfx.DrawString(sb, -1, g_FontStack, stackRect, &cfmt, &stackBrush);
        }
        y += ROW_HEIGHT;
    }
}

// ============================================
// PAINT: STACKS PANEL
// ============================================

static void PaintStacksPanel(Graphics& gfx, int px, int py) {
    std::lock_guard<std::mutex> lock(g_StacksMutex);
    if (g_TrackedStacks.empty()) return;

    std::vector<TrackedStackEffect*> sorted;
    for (auto& p : g_TrackedStacks) sorted.push_back(&p.second);
    std::sort(sorted.begin(), sorted.end(), [](auto* a, auto* b) {
        if (a->isActive != b->isActive) return a->isActive > b->isActive;
        if (a->isActive) return a->currentStacks > b->currentStacks;
        return a->lastDropTime > b->lastDropTime;
        });

    int tw = CalcStacksPanelWidth();
    int th = HEADER_HEIGHT + (int)sorted.size() * ROW_HEIGHT + PADDING * 2;
    DWORD now = (DWORD)GetTickCount64();

    SolidBrush bgBrush(Color(200, 25, 15, 40));
    Pen borderPen(Color(200, 80, 50, 120), 1.0f);
    Rect bgR(px, py, tw, th);
    gfx.FillRectangle(&bgBrush, bgR);
    gfx.DrawRectangle(&borderPen, bgR);

    SolidBrush headerBrush(Color(255, 220, 180, 255));
    StringFormat csf; csf.SetAlignment(StringAlignmentCenter); csf.SetLineAlignment(StringAlignmentCenter);
    RectF headerRect((REAL)(px + PADDING), (REAL)(py + 2), (REAL)(tw - PADDING * 2), (REAL)HEADER_HEIGHT);
    gfx.DrawString(L"\u26A1 STACKS", -1, g_StkFontHeader, headerRect, &csf, &headerBrush);

    Pen sepPen(Color(150, 100, 60, 130), 1.0f);
    gfx.DrawLine(&sepPen, px + PADDING, py + HEADER_HEIGHT, px + tw - PADDING, py + HEADER_HEIGHT);

    StringFormat nfmt; nfmt.SetAlignment(StringAlignmentNear); nfmt.SetLineAlignment(StringAlignmentCenter); nfmt.SetTrimming(StringTrimmingEllipsisCharacter);
    StringFormat tfmt; tfmt.SetAlignment(StringAlignmentCenter); tfmt.SetLineAlignment(StringAlignmentCenter);

    int y = py + HEADER_HEIGHT + PADDING;
    for (auto* t : sorted) {
        int by = y, bh = ROW_HEIGHT - 4, bx = px + PADDING, bw = BAR_WIDTH;
        int alpha = 255;
        if (!t->isActive && t->lastDropTime > 0) {
            float fp = (float)(now - t->lastDropTime) / (float)STACKS_FADE_TIME;
            if (fp > 1.0f) fp = 1.0f;
            alpha = 255 - (int)(fp * 180);
            if (alpha < 75) alpha = 75;
        }

        int nw = (int)(bw * 0.55f), sw = (int)(bw * 0.20f), timerW = bw - nw - sw - 4;

        SolidBrush nameBg(t->isActive ? Color(alpha, 50, 30, 90) : Color(alpha / 2, 40, 30, 50));
        Rect nameR(bx, by, nw, bh);
        gfx.FillRectangle(&nameBg, nameR);
        SolidBrush nameText(t->isActive ? Color(alpha, 255, 255, 255) : Color(alpha, 150, 150, 160));
        RectF nameRect((REAL)(bx + 6), (REAL)by, (REAL)(nw - 10), (REAL)bh);
        std::wstring wName = StringToWString(t->displayName);
        gfx.DrawString(wName.c_str(), -1, g_StkFontName, nameRect, &nfmt, &nameText);

        int sx = bx + nw + 2;
        Color sbg = !t->isActive ? Color(alpha / 2, 60, 20, 20) :
            (t->currentStacks >= t->peakStacks && t->peakStacks > 1) ? Color(alpha, 40, 160, 40) : Color(alpha, 120, 120, 30);
        SolidBrush stackBg(sbg);
        Rect stackR(sx, by, sw, bh);
        gfx.FillRectangle(&stackBg, stackR);
        wchar_t st2[8]; swprintf_s(st2, L"%d", t->isActive ? (int)t->currentStacks : 0);
        SolidBrush stackText(t->isActive ? Color(alpha, 255, 255, 255) : Color(alpha, 120, 80, 80));
        RectF stackRect((REAL)sx, (REAL)by, (REAL)sw, (REAL)bh);
        gfx.DrawString(st2, -1, g_StkFontStack, stackRect, &csf, &stackText);

        wchar_t pk[16]; swprintf_s(pk, L"pk:%.0f", t->peakStacks);
        SolidBrush peakBrush(Color(alpha / 2, 180, 180, 200));
        RectF peakRect((REAL)sx, (REAL)(by + bh - 11), (REAL)sw, 11);
        gfx.DrawString(pk, -1, g_StkFontPeak, peakRect, &csf, &peakBrush);

        int tx = sx + sw + 2;
        SolidBrush timerBg(t->isActive ? Color(alpha, 40, 25, 50) : Color(alpha / 3, 50, 20, 20));
        Rect timerR(tx, by, timerW, bh);
        gfx.FillRectangle(&timerBg, timerR);
        wchar_t tb[32];
        if (!t->isActive) swprintf_s(tb, L"DROP %.0fs", (float)(now - t->lastDropTime) / 1000.0f);
        else if (t->maxDuration > 0) { float r = t->maxDuration - t->currentDuration; if (r < 0) r = 0; if (r >= 60) swprintf_s(tb, L"%dm %02ds", (int)(r / 60), (int)r % 60); else swprintf_s(tb, L"%.1fs", r); }
        else wcscpy_s(tb, L"\u221E");
        SolidBrush timerText(t->isActive ? Color(alpha, 255, 255, 255) : Color(alpha, 180, 100, 100));
        RectF timerRect((REAL)tx, (REAL)by, (REAL)timerW, (REAL)bh);
        gfx.DrawString(tb, -1, g_StkFontName, timerRect, &tfmt, &timerText);

        Pen rowBorder(t->isActive ? Color(alpha / 2, 100, 70, 140) : Color(alpha / 4, 60, 40, 60), 1.0f);
        Rect rowR(bx, by, bw, bh);
        gfx.DrawRectangle(&rowBorder, rowR);
        y += ROW_HEIGHT;
    }
}

// ============================================
// PAINT: RAID PANEL
// ============================================

static void PaintRaidPanel(Graphics& gfx, int px, int py) {
    std::lock_guard<std::mutex> lock(g_RaidMutex);
    if (g_RaidMembers.empty()) return;

    int totalRows = 0;
    for (auto& m : g_RaidMembers) {
        totalRows++;
        if (m.isNearby) { int v = 0; for (auto& e : m.effects) if (!e.removed && !e.hideFromInfobar) v++; totalRows += (v > 0) ? v : 1; }
        else totalRows++;
    }

    int tw = CalcRaidPanelWidth();
    int th = RAID_HEADER_HEIGHT + totalRows * RAID_ROW_HEIGHT + PADDING * 2;

    SolidBrush bgBrush(Color(210, 15, 25, 45));
    Pen borderPen(Color(200, 40, 80, 140), 1.0f);
    Rect bgR(px, py, tw, th);
    gfx.FillRectangle(&bgBrush, bgR);
    gfx.DrawRectangle(&borderPen, bgR);

    SolidBrush headerBrush(Color(255, 160, 200, 255));
    StringFormat csf; csf.SetAlignment(StringAlignmentCenter); csf.SetLineAlignment(StringAlignmentCenter);
    RectF headerRect((REAL)(px + PADDING), (REAL)(py + 2), (REAL)(tw - PADDING * 2), (REAL)RAID_HEADER_HEIGHT);
    gfx.DrawString(L"\u2694 RAID TRACKER", -1, g_RaidFontHeader, headerRect, &csf, &headerBrush);

    Pen sepPen(Color(150, 50, 80, 130), 1.0f);
    gfx.DrawLine(&sepPen, px + PADDING, py + RAID_HEADER_HEIGHT, px + tw - PADDING, py + RAID_HEADER_HEIGHT);

    StringFormat nfmt; nfmt.SetAlignment(StringAlignmentNear); nfmt.SetLineAlignment(StringAlignmentCenter);
    StringFormat rfmt; rfmt.SetAlignment(StringAlignmentFar); rfmt.SetLineAlignment(StringAlignmentCenter);
    StringFormat efmt; efmt.SetAlignment(StringAlignmentNear); efmt.SetLineAlignment(StringAlignmentCenter); efmt.SetTrimming(StringTrimmingEllipsisCharacter);
    StringFormat tfmt; tfmt.SetAlignment(StringAlignmentFar); tfmt.SetLineAlignment(StringAlignmentCenter);

    int y = py + RAID_HEADER_HEIGHT + PADDING;
    for (auto& member : g_RaidMembers) {
        int bx = px + PADDING, rw = RAID_BAR_WIDTH;

        Color nameBgColor = member.isNearby ? Color(200, 30, 60, 100) : Color(150, 50, 30, 30);
        SolidBrush nameBgBrush(nameBgColor);
        Pen nameBorderPen(Color(180, 50, 80, 130), 1.0f);
        Rect nameR(bx, y, rw, RAID_ROW_HEIGHT - 2);
        gfx.FillRectangle(&nameBgBrush, nameR);
        gfx.DrawRectangle(&nameBorderPen, nameR);

        Color dotColor = member.isNearby ? Color(255, 80, 220, 80) : Color(255, 150, 60, 60);
        SolidBrush dotBrush(dotColor);
        Rect dotR(bx + 4, y + 6, 8, 8);
        gfx.FillEllipse(&dotBrush, dotR);

        std::wstring wName = StringToWString(member.name);
        SolidBrush nameTextBrush(member.isNearby ? Color(255, 220, 230, 255) : Color(200, 150, 120, 120));
        RectF nameTextRect((REAL)(bx + 16), (REAL)y, (REAL)(rw - 20), (REAL)(RAID_ROW_HEIGHT - 2));
        gfx.DrawString(wName.c_str(), -1, g_RaidFontName, nameTextRect, &nfmt, &nameTextBrush);

        if (member.isNearby) {
            int visCount = 0; for (auto& e : member.effects) if (!e.removed && !e.hideFromInfobar) visCount++;
            wchar_t countText[16]; swprintf_s(countText, L"(%d)", visCount);
            SolidBrush countBrush(Color(180, 150, 180, 220));
            RectF countRect((REAL)bx, (REAL)y, (REAL)(rw - 6), (REAL)(RAID_ROW_HEIGHT - 2));
            gfx.DrawString(countText, -1, g_RaidFontEffect, countRect, &rfmt, &countBrush);
        }
        y += RAID_ROW_HEIGHT;

        if (!member.isNearby) {
            SolidBrush offBg(Color(100, 30, 20, 20));
            Rect offR(bx + 10, y, rw - 20, RAID_ROW_HEIGHT - 2);
            gfx.FillRectangle(&offBg, offR);
            SolidBrush offText(Color(180, 150, 100, 100));
            RectF offRect((REAL)(bx + 10), (REAL)y, (REAL)(rw - 20), (REAL)(RAID_ROW_HEIGHT - 2));
            gfx.DrawString(L"Not nearby / Out of range", -1, g_RaidFontStatus, offRect, &csf, &offText);
            y += RAID_ROW_HEIGHT;
            continue;
        }

        int visCount = 0;
        for (auto& e : member.effects) {
            if (e.removed || e.hideFromInfobar) continue;
            visCount++;
            std::wstring eName = StringToWString(e.name.empty() ? e.uniqueName : e.name);
            float rem = e.GetRemainingTime(); bool perm = e.IsPermanent();
            Color eBgColor = perm ? Color(140, 20, 50, 35) : rem > 60 ? Color(140, 20, 55, 35) : rem > 15 ? Color(140, 55, 50, 15) : Color(140, 60, 25, 20);
            SolidBrush eBgBrush(eBgColor);
            Rect eR(bx + 10, y, rw - 20, RAID_ROW_HEIGHT - 2);
            gfx.FillRectangle(&eBgBrush, eR);

            std::wstring displayStr = eName;
            if (e.currentStacks > 1) { wchar_t sp[16]; swprintf_s(sp, L"x%.0f ", e.currentStacks); displayStr = sp + eName; }
            SolidBrush eTextBrush(Color(230, 220, 220, 240));
            RectF eNameRect((REAL)(bx + 14), (REAL)y, (REAL)(rw - 100), (REAL)(RAID_ROW_HEIGHT - 2));
            gfx.DrawString(displayStr.c_str(), -1, g_RaidFontEffect, eNameRect, &efmt, &eTextBrush);

            wchar_t tb[32];
            if (perm) wcscpy_s(tb, L"\u221E");
            else if (rem >= 60) swprintf_s(tb, L"%dm %02ds", (int)(rem / 60), (int)rem % 60);
            else swprintf_s(tb, L"%.1fs", rem);
            Color timeColor = perm ? Color(200, 150, 180, 200) : rem > 60 ? Color(220, 120, 200, 120) : rem > 15 ? Color(220, 220, 200, 80) : Color(230, 220, 80, 80);
            SolidBrush timeBrush(timeColor);
            RectF timeRect((REAL)(bx + 10), (REAL)y, (REAL)(rw - 24), (REAL)(RAID_ROW_HEIGHT - 2));
            gfx.DrawString(tb, -1, g_RaidFontTime, timeRect, &tfmt, &timeBrush);
            y += RAID_ROW_HEIGHT;
        }

        if (visCount == 0) {
            SolidBrush noBg(Color(100, 20, 30, 25));
            Rect noR(bx + 10, y, rw - 20, RAID_ROW_HEIGHT - 2);
            gfx.FillRectangle(&noBg, noR);
            SolidBrush noText(Color(150, 120, 140, 120));
            RectF noRect((REAL)(bx + 10), (REAL)y, (REAL)(rw - 20), (REAL)(RAID_ROW_HEIGHT - 2));
            gfx.DrawString(L"No active buffs", -1, g_RaidFontStatus, noRect, &csf, &noText);
            y += RAID_ROW_HEIGHT;
        }
    }
}

// ============================================
// WINDOW PROC — single window, per-panel drag
// ============================================

LRESULT CALLBACK UnifiedWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {

    case WM_LBUTTONDOWN: {
        POINT pt; GetCursorPos(&pt);
        DragTarget target = HitTestPanel(pt.x, pt.y);
        if (target != DRAG_NONE) {
            g_DragTarget = target;
            g_DragStartMouse = pt;
            if (target == DRAG_BUFF) { g_DragStartPanelX = g_OverlayX; g_DragStartPanelY = g_OverlayY; }
            if (target == DRAG_STACKS) { g_DragStartPanelX = g_StacksX;  g_DragStartPanelY = g_StacksY; }
            if (target == DRAG_RAID) { g_DragStartPanelX = g_RaidX;    g_DragStartPanelY = g_RaidY; }
            SetCapture(hwnd);
        }
        return 0;
    }

    case WM_MOUSEMOVE: {
        if (g_DragTarget != DRAG_NONE && (wParam & MK_LBUTTON)) {
            POINT pt; GetCursorPos(&pt);
            int dx = pt.x - g_DragStartMouse.x;
            int dy = pt.y - g_DragStartMouse.y;
            if (g_DragTarget == DRAG_BUFF) { g_OverlayX = g_DragStartPanelX + dx; g_OverlayY = g_DragStartPanelY + dy; }
            if (g_DragTarget == DRAG_STACKS) { g_StacksX = g_DragStartPanelX + dx;  g_StacksY = g_DragStartPanelY + dy; }
            if (g_DragTarget == DRAG_RAID) { g_RaidX = g_DragStartPanelX + dx;    g_RaidY = g_DragStartPanelY + dy; }
            // Window will reposition on next repaint cycle
        }
        return 0;
    }

    case WM_LBUTTONUP: {
        if (g_DragTarget != DRAG_NONE) {
            g_DragTarget = DRAG_NONE;
            ReleaseCapture();
        }
        return 0;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        if (!hdc) return 0;

        RECT cr; GetClientRect(hwnd, &cr);
        int w = cr.right - cr.left, h = cr.bottom - cr.top;
        if (w <= 0 || h <= 0) { EndPaint(hwnd, &ps); return 0; }

        HDC mem = CreateCompatibleDC(hdc);
        if (!mem) { EndPaint(hwnd, &ps); return 0; }
        HBITMAP bmp = CreateCompatibleBitmap(hdc, w, h);
        if (!bmp) { DeleteDC(mem); EndPaint(hwnd, &ps); return 0; }
        HBITMAP old = (HBITMAP)SelectObject(mem, bmp);

        {
            Graphics gfx(mem);
            gfx.SetSmoothingMode(SmoothingModeAntiAlias);
            gfx.SetTextRenderingHint(TextRenderingHintAntiAlias);
            gfx.Clear(Color(0, 0, 0, 0));
            InitFontCache();

            // Window origin — all panel positions are relative to this
            int winX, winY;
            CalcWindowOrigin(winX, winY);

            if (CalcBuffPanelHeight() > 0)
                PaintBuffPanel(gfx, g_OverlayX - winX, g_OverlayY - winY);
            if (CalcStacksPanelHeight() > 0)
                PaintStacksPanel(gfx, g_StacksX - winX, g_StacksY - winY);
            if (CalcRaidPanelHeight() > 0)
                PaintRaidPanel(gfx, g_RaidX - winX, g_RaidY - winY);
        }

        BitBlt(hdc, 0, 0, w, h, mem, 0, 0, SRCCOPY);
        SelectObject(mem, old);
        DeleteObject(bmp);
        DeleteDC(mem);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_ERASEBKGND:
        return 1;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}