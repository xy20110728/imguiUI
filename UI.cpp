#include "SetoUI.h"
#include "SetoLogo.h"
#include <GLES3/gl3.h>
#include "imgui_internal.h"
#include <cstdio>
#include <cstring>
#include <cmath>
#include <cstdlib>
#include <string>

// ---- jni 功能全局 ----
extern bool DrawIo[100];
extern int  自瞄按钮[100];
extern bool 自瞄开关[50];
extern float NumIo[100];
extern bool 追踪[100];
extern bool 物资开关, 载具开关;
extern bool show556, show762, showRifle, showSubmachine, showSniper, showMirror,
    showExpansion, showOtherParts, showMirro, showExpansio, hzhz, tzw;
extern bool cshzt;
extern int PlayerCount, RobotCount, FPS;
extern int screen_x, screen_y;
extern int DrawInt();
extern void NumIoSave(const char* name);
extern void shutdown();
extern void SetMenuVisible(bool show);
extern void 重置配置();

namespace SetoUI {

State g_state;
static float g_s = 1.0f;

// 3D 翻页
static int   g_pendingPage = -1;
static float g_flipT = 1.0f;
static bool  g_switched = true;

static ImDrawList* g_bgDl = nullptr;
static ImDrawList* g_mainDl = nullptr;
static ImDrawList* g_bodyDl = nullptr;

void RequestPage(int page) {
    if (page == g_state.page || g_flipT < 1.0f) return;
    g_pendingPage = page;
    g_flipT = 0.0f;
    g_switched = false;
} // 当前缩放

static ImU32 C(ImU32 rgb, int a = 255) { return (rgb & 0x00FFFFFF) | ((ImU32)a << 24); }
static const ImU32 kShell   = IM_COL32(247, 248, 250, 248);
static const ImU32 kCard    = IM_COL32(242, 243, 246, 255);
static const ImU32 kCard2   = IM_COL32(236, 237, 241, 255);
static const ImU32 kBorder  = IM_COL32(228, 230, 235, 255);
static const ImU32 kText    = IM_COL32(20, 20, 22, 255);
static const ImU32 kDim     = IM_COL32(128, 132, 140, 255);
static const ImU32 kMuted   = IM_COL32(168, 171, 178, 255);
static const ImU32 kBlack   = IM_COL32(18, 18, 20, 255);
static const ImU32 kWhite   = IM_COL32(255, 255, 255, 255);
static const ImU32 kAccent  = IM_COL32(110, 160, 255, 255);

static ImVec2 V(float x, float y) { return ImVec2(x, y); }
static float S(float v) { return v * g_s; }

static void Txt(ImDrawList* dl, ImVec2 p, float size, ImU32 col, const char* s) {
    dl->AddText(ImGui::GetFont(), size, p, col, s);
}
static ImVec2 TxtSize(float size, const char* s) {
    return ImGui::GetFont()->CalcTextSizeA(size, FLT_MAX, 0.0f, s);
}
static void CenterTxt(ImDrawList* dl, ImVec2 c, float size, ImU32 col, const char* s) {
    ImVec2 ts = TxtSize(size, s);
    Txt(dl, V(c.x - ts.x * 0.5f, c.y - ts.y * 0.5f), size, col, s);
}

static void TxtGlow(ImDrawList* dl, ImVec2 p, float size, ImU32 core, const char* s) {
    // 纯白字 + 冷色发光
    for (int i = 3; i >= 1; --i) {
        float o = S((float)i * 1.1f);
        int a = 26 + (3 - i) * 18;
        ImU32 g = IM_COL32(150, 215, 255, a);
        Txt(dl, V(p.x - o, p.y), size, g, s);
        Txt(dl, V(p.x + o, p.y), size, g, s);
        Txt(dl, V(p.x, p.y - o), size, g, s);
        Txt(dl, V(p.x, p.y + o), size, g, s);
        Txt(dl, V(p.x - o * 0.7f, p.y - o * 0.7f), size, g, s);
        Txt(dl, V(p.x + o * 0.7f, p.y - o * 0.7f), size, g, s);
        Txt(dl, V(p.x - o * 0.7f, p.y + o * 0.7f), size, g, s);
        Txt(dl, V(p.x + o * 0.7f, p.y + o * 0.7f), size, g, s);
    }
    Txt(dl, p, size, core, s);
}
static void Shadow(ImDrawList* dl, ImVec2 p0, ImVec2 p1, float round) {
    for (int i = 4; i >= 1; --i) {
        float o = S((float)i * 4.0f);
        int a = 4 + i * 3;
        dl->AddRectFilled(V(p0.x - o * 0.15f, p0.y + o * 0.35f),
                          V(p1.x + o * 0.15f, p1.y + o * 0.55f),
                          IM_COL32(30, 35, 50, a), round + o * 0.2f);
    }
}

static bool Hit(const char* id, ImVec2 p, ImVec2 size) {
    ImGui::SetCursorScreenPos(p);
    ImGui::InvisibleButton(id, size);
    return ImGui::IsItemClicked(0);
}
static bool Hover(const char* id, ImVec2 p, ImVec2 size) {
    ImGui::SetCursorScreenPos(p);
    ImGui::InvisibleButton(id, size);
    return ImGui::IsItemHovered();
}

static void DrawSwitch(ImDrawList* dl, ImVec2 p, ImVec2 size, bool on) {
    float h = size.y, w = size.x, r = h * 0.5f;
    dl->AddRectFilled(p, V(p.x + w, p.y + h), on ? kBlack : IM_COL32(214, 216, 222, 255), r);
    float kr = h * 0.36f;
    float cx = on ? (p.x + w - h * 0.5f) : (p.x + h * 0.5f);
    dl->AddCircleFilled(V(cx, p.y + h * 0.5f), kr, kWhite);
}

static bool Toggle(const char* id, ImVec2 p, float w, bool* v, const char* label) {
    float h = S(24.0f);
    ImGui::SetCursorScreenPos(p);
    ImGui::InvisibleButton(id, V(w, h));
    if (ImGui::IsItemClicked(0)) *v = !*v;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    float tw = S(40.0f);
    DrawSwitch(dl, V(p.x + w - tw, p.y + (h - S(24.0f)) * 0.5f), V(tw, S(24.0f)), *v);
    Txt(dl, V(p.x, p.y + S(1.0f)), S(13.0f), kText, label);
    return false;
}

static bool Slider(const char* id, ImVec2 p, float w, const char* label,
                   float* v, float lo, float hi, const char* suffix) {
    float labelY = p.y;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    Txt(dl, V(p.x, labelY), S(12.5f), kText, label);
    char buf[64];
    if (strcmp(label, "界面大小") == 0)
        snprintf(buf, sizeof(buf), "%.0f%%", (*v) * 100.0f);
    else if (suffix && suffix[0])
        snprintf(buf, sizeof(buf), "%.0f%s", *v, suffix);
    else
        snprintf(buf, sizeof(buf), "%.0f", *v);
    ImVec2 ts = TxtSize(S(12.0f), buf);
    Txt(dl, V(p.x + w - ts.x, labelY), S(12.0f), kDim, buf);

    ImVec2 track = V(p.x, p.y + S(24.0f));
    float tw = w, th = S(6.0f);
    dl->AddRectFilled(track, V(track.x + tw, track.y + th), IM_COL32(222, 224, 230, 255), th * 0.5f);
    float t = (*v - lo) / (hi - lo);
    if (t < 0) t = 0; if (t > 1) t = 1;
    dl->AddRectFilled(track, V(track.x + tw * t, track.y + th), kBlack, th * 0.5f);
    float kx = track.x + tw * t;
    dl->AddCircleFilled(V(kx, track.y + th * 0.5f), S(8.0f), kBlack);
    dl->AddCircle(V(kx, track.y + th * 0.5f), S(8.0f), kWhite, 0, S(1.5f));

    ImGui::SetCursorScreenPos(V(p.x, p.y + S(14.0f)));
    ImGui::InvisibleButton(id, V(w, S(28.0f)));
    if (ImGui::IsItemActive()) {
        float mx = ImGui::GetIO().MousePos.x - p.x;
        float nt = mx / (w > 1 ? w : 1);
        if (nt < 0) nt = 0; if (nt > 1) nt = 1;
        *v = lo + nt * (hi - lo);
    }
    return false;
}

static void Card(ImDrawList* dl, ImVec2 p, ImVec2 sz, float round) {
    dl->AddRectFilled(p, V(p.x + sz.x, p.y + sz.y), kCard, round);
}
static void CardBorder(ImDrawList* dl, ImVec2 p, ImVec2 sz, float round) {
    dl->AddRect(p, V(p.x + sz.x, p.y + sz.y), kBorder, round, 0, S(1.0f));
}

static bool Button(const char* id, ImVec2 p, ImVec2 sz, const char* label, bool primary) {
    ImGui::SetCursorScreenPos(p);
    ImGui::InvisibleButton(id, sz);
    bool clicked = ImGui::IsItemClicked(0);
    bool held = ImGui::IsItemActive();
    bool hov = ImGui::IsItemHovered();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImU32 fill = primary ? (held ? IM_COL32(40, 40, 44, 255) : kBlack) : (held ? kCard2 : kCard);
    dl->AddRectFilled(p, V(p.x + sz.x, p.y + sz.y), fill, S(12.0f));
    if (!primary) dl->AddRect(p, V(p.x + sz.x, p.y + sz.y), kBorder, S(12.0f), 0, S(1.0f));
    CenterTxt(dl, V(p.x + sz.x * 0.5f, p.y + sz.y * 0.5f), S(13.0f),
              primary ? kWhite : (hov ? kBlack : kText), label);
    return clicked;
}

static void NavIcon(ImDrawList* dl, ImVec2 c, int type, ImU32 col) {
    float r = S(8.0f);
    if (type == 0) { // home
        dl->AddRectFilled(V(c.x - r, c.y - r * 0.2f), V(c.x + r, c.y + r), col, S(2.0f));
        dl->AddTriangleFilled(V(c.x - r * 1.2f, c.y - r * 0.2f), V(c.x + r * 1.2f, c.y - r * 0.2f), V(c.x, c.y - r * 1.2f), col);
    } else if (type == 1) { // draw
        dl->AddRect(V(c.x - r, c.y - r), V(c.x + r, c.y + r), col, S(2.0f), 0, S(1.6f));
    } else if (type == 2) { // aim
        dl->AddCircle(c, r, col, 0, S(1.6f));
        dl->AddLine(V(c.x - r * 1.4f, c.y), V(c.x - r * 0.3f, c.y), col, S(1.6f));
        dl->AddLine(V(c.x + r * 0.3f, c.y), V(c.x + r * 1.4f, c.y), col, S(1.6f));
        dl->AddLine(V(c.x, c.y - r * 1.4f), V(c.x, c.y - r * 0.3f), col, S(1.6f));
        dl->AddLine(V(c.x, c.y + r * 0.3f), V(c.x, c.y + r * 1.4f), col, S(1.6f));
    } else { // settings gear
        dl->AddCircle(c, r * 0.55f, col, 0, S(1.6f));
        dl->AddCircleFilled(c, S(2.0f), col);
        for (int k = 0; k < 4; k++) {
            float a = (float)k * 1.5707963f;
            dl->AddLine(V(c.x + cosf(a) * r * 0.75f, c.y + sinf(a) * r * 0.75f),
                        V(c.x + cosf(a) * r * 1.35f, c.y + sinf(a) * r * 1.35f), col, S(1.8f));
        }
    }
}

static GLuint g_logo = 0;
static bool EnsureLogo() {
    if (g_logo != 0) return true;
    glGenTextures(1, &g_logo);
    if (g_logo == 0) return false;
    glBindTexture(GL_TEXTURE_2D, g_logo);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kSetoLogoW, kSetoLogoH, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, kSetoLogoRgba);
    glBindTexture(GL_TEXTURE_2D, 0);
    return true;
}

static void LiquidGlass(ImDrawList* dl, ImVec2 g0, ImVec2 g1, float round) {
    for (int i = 3; i >= 1; --i) {
        float o = S(3.0f * i);
        dl->AddRectFilled(V(g0.x - o * 0.25f, g0.y - o * 0.12f),
                          V(g1.x + o * 0.25f, g1.y + o * 0.20f),
                          IM_COL32(255, 255, 255, 3 + i * 2), round + o);
    }
    dl->AddRectFilled(g0, g1, IM_COL32(226, 238, 252, 46), round);
    dl->AddRectFilledMultiColor(V(g0.x + S(2.0f), g0.y + S(2.0f)),
                                V(g1.x - S(2.0f), g1.y - S(2.0f)),
                                IM_COL32(255, 255, 255, 92), IM_COL32(255, 255, 255, 58),
                                IM_COL32(198, 220, 248, 22), IM_COL32(214, 231, 250, 30));
    float midY = g0.y + (g1.y - g0.y) * 0.44f;
    dl->AddRectFilledMultiColor(V(g0.x + S(4.0f), g0.y + S(4.0f)),
                                V(g1.x - S(4.0f), midY),
                                IM_COL32(255, 255, 255, 76), IM_COL32(255, 255, 255, 46),
                                IM_COL32(255, 255, 255, 6), IM_COL32(255, 255, 255, 10));
    dl->AddRectFilledMultiColor(V(g0.x + S(4.0f), g0.y + (g1.y - g0.y) * 0.66f),
                                V(g1.x - S(4.0f), g1.y - S(4.0f)),
                                IM_COL32(190, 214, 245, 8), IM_COL32(190, 214, 245, 6),
                                IM_COL32(160, 190, 230, 24), IM_COL32(170, 198, 236, 18));
    dl->PushClipRect(g0, g1, true);
    float t = fmodf((float)ImGui::GetTime() * 0.16f, 1.5f) - 0.25f;
    float bx = g0.x + t * (g1.x - g0.x);
    dl->AddQuadFilled(V(bx, g0.y), V(bx + S(28.0f), g0.y),
                      V(bx + S(8.0f), g1.y), V(bx - S(20.0f), g1.y),
                      IM_COL32(255, 255, 255, 17));
    dl->AddQuadFilled(V(bx + S(32.0f), g0.y), V(bx + S(42.0f), g0.y),
                      V(bx + S(22.0f), g1.y), V(bx + S(12.0f), g1.y),
                      IM_COL32(255, 255, 255, 9));
    dl->PopClipRect();
    dl->AddRect(g0, g1, IM_COL32(255, 255, 255, 155), round, 0, S(1.2f));
    dl->AddRect(V(g0.x + S(1.0f), g0.y + S(1.0f)), V(g1.x - S(1.0f), g1.y - S(1.0f)),
                IM_COL32(255, 255, 255, 62), round - S(2.0f), 0, S(1.0f));
    dl->AddLine(V(g0.x + round * 0.55f, g0.y + S(1.4f)),
                V(g1.x - round * 0.55f, g0.y + S(1.4f)), IM_COL32(255, 255, 255, 135), S(1.3f));
    dl->AddLine(V(g0.x + round * 0.55f, g1.y - S(1.4f)),
                V(g1.x - round * 0.55f, g1.y - S(1.4f)), IM_COL32(170, 200, 238, 75), S(1.0f));
    dl->AddCircleFilled(V(g1.x - S(25.0f), g0.y + S(24.0f)), S(2.8f), IM_COL32(255, 255, 255, 100));
    dl->AddCircleFilled(V(g1.x - S(37.0f), g0.y + S(16.0f)), S(1.5f), IM_COL32(255, 255, 255, 65));
}

static void Sidebar(ImDrawList* dl, ImVec2 p0, ImVec2 p1) {
    // 液态玻璃侧栏
    ImVec2 g0 = V(p0.x + S(10.0f), p0.y + S(10.0f));
    ImVec2 g1 = V(p1.x - S(10.0f), p1.y - S(2.0f));
    LiquidGlass(dl, g0, g1, S(22.0f));

    float x = p0.x + S(22.0f);
    float y = p0.y + S(26.0f);
    // logo
    ImVec2 lc = V(x + S(28.0f), y + S(28.0f));
    if (EnsureLogo() && g_logo != 0) {
        ImTextureID tid = (ImTextureID)(intptr_t)g_logo;
        dl->AddImage(tid, V(lc.x - S(28.0f), lc.y - S(28.0f)),
                     V(lc.x + S(28.0f), lc.y + S(28.0f)),
                     V(0, 0), V(1, 1), IM_COL32(255, 255, 255, 255));
    } else {
        dl->AddCircleFilled(lc, S(28.0f), kBlack);
        CenterTxt(dl, lc, S(24.0f), kWhite, "S");
    }
    Txt(dl, V(lc.x + S(40.0f), y + S(10.0f)), S(23.0f), kText, "LYNX");
    Txt(dl, V(lc.x + S(40.0f), y + S(40.0f)), S(10.0f), kMuted, "Simple clean");

    const char* labels[4] = {"主页", "绘制", "自瞄", "设置"};
    float navY = y + S(96.0f);
    float itemH = S(46.0f), gap = S(8.0f), itemW = p1.x - p0.x - S(44.0f);
    for (int i = 0; i < 4; i++) {
        ImVec2 ip = V(x, navY + i * (itemH + gap));
        bool active = g_state.page == i;
        ImGui::SetCursorScreenPos(ip);
        ImGui::InvisibleButton((std::string("##nav") + std::to_string(i)).c_str(), V(itemW, itemH));
        if (ImGui::IsItemClicked(0)) RequestPage(i);
        bool hov = ImGui::IsItemHovered();
        if (active)
            dl->AddRectFilled(ip, V(ip.x + itemW, ip.y + itemH), kBlack, itemH * 0.5f);
        else if (hov)
            dl->AddRectFilled(ip, V(ip.x + itemW, ip.y + itemH), IM_COL32(236, 237, 241, 255), itemH * 0.5f);
        ImU32 col = active ? kWhite : kText;
        NavIcon(dl, V(ip.x + S(24.0f), ip.y + itemH * 0.5f), i, col);
        Txt(dl, V(ip.x + S(46.0f), ip.y + itemH * 0.5f - S(8.0f)), S(13.0f), col, labels[i]);
    }

    // 底部
    float by = p1.y - S(86.0f);
    TxtGlow(dl, V(x, by + S(16.0f)), S(17.0f), kWhite, "@LYNXNB686");
}

static void Home(ImDrawList* dl, ImVec2 p, float w) {
    Txt(dl, p, S(26.0f), kText, "主页");
    Txt(dl, V(p.x, p.y + S(36.0f)), S(12.0f), kDim, "你的空间，保持整洁。");

    // 黑色横幅
    ImVec2 bp = V(p.x, p.y + S(66.0f));
    ImVec2 bs = V(w, S(110.0f));
    dl->AddRectFilled(bp, V(bp.x + bs.x, bp.y + bs.y), kBlack, S(18.0f));
    Txt(dl, V(bp.x + S(22.0f), bp.y + S(16.0f)), S(9.5f), IM_COL32(150, 152, 158, 255), "WELCOME TO LYNX");
    Txt(dl, V(bp.x + S(22.0f), bp.y + S(32.0f)), S(36.0f), kWhite, "LYNXNB686");
    Txt(dl, V(bp.x + S(24.0f), bp.y + S(82.0f)), S(10.0f), IM_COL32(145, 147, 154, 255), "始于热爱，一切极简");
    // 装饰线
    for (int i = 0; i < 4; i++) {
        float yy = bp.y + S(22.0f + i * 18.0f);
        dl->AddBezierCubic(V(bp.x + bs.x - S(190.0f), yy),
                           V(bp.x + bs.x - S(140.0f), yy - S(12.0f)),
                           V(bp.x + bs.x - S(90.0f), yy + S(12.0f)),
                           V(bp.x + bs.x - S(30.0f), yy),
                           IM_COL32(90, 92, 98, 150), S(1.0f));
    }
    // 载入按钮
    if (Button("##attach", V(bp.x + bs.x - S(148.0f), bp.y + S(68.0f)), V(S(124.0f), S(30.0f)),
               cshzt ? "已载入" : "载入游戏", true)) {
        if (!cshzt) {
            if (DrawInt()) {
                cshzt = true;
                DrawIo[0] = true;
                g_state.attached = true;
            }
        }
    }

    // 三张入口卡
    float gap = S(12.0f);
    float cw = (w - gap * 2.0f) / 3.0f;
    float cy = bp.y + bs.y + S(18.0f);
    const char* titles[3] = {"控制设置", "自瞄设置", "设置"};
    const char* subs[3] = {"开关与交互", "瞄准与手感", "外观与系统"};
    for (int i = 0; i < 3; i++) {
        ImVec2 cp = V(p.x + i * (cw + gap), cy);
        Card(dl, cp, V(cw, S(78.0f)), S(14.0f));
        CardBorder(dl, cp, V(cw, S(78.0f)), S(14.0f));
        dl->AddCircleFilled(V(cp.x + S(24.0f), cp.y + S(24.0f)), S(12.0f), i == 1 ? kBlack : IM_COL32(220, 222, 228, 255));
        Txt(dl, V(cp.x + S(44.0f), cp.y + S(14.0f)), S(13.0f), kText, titles[i]);
        Txt(dl, V(cp.x + S(44.0f), cp.y + S(36.0f)), S(10.0f), kDim, subs[i]);
        Txt(dl, V(cp.x + cw - S(24.0f), cp.y + S(18.0f)), S(16.0f), kDim, "›");
        if (Hit((std::string("##entry") + std::to_string(i)).c_str(), cp, V(cw, S(78.0f))))
            RequestPage(i == 0 ? 1 : (i == 1 ? 2 : 3));
    }

    // 界面与配置
    float lowerY = cy + S(96.0f);
    float leftW = w * 0.56f;
    ImVec2 lp = V(p.x, lowerY);
    Card(dl, lp, V(leftW, S(190.0f)), S(16.0f));
    CardBorder(dl, lp, V(leftW, S(190.0f)), S(16.0f));
    Txt(dl, V(lp.x + S(18.0f), lp.y + S(14.0f)), S(13.0f), kText, "界面与配置");
    Txt(dl, V(lp.x + S(18.0f), lp.y + S(36.0f)), S(9.5f), kDim, "所有修改会自动保存");
    Slider("##uizoom", V(lp.x + S(18.0f), lp.y + S(58.0f)), leftW - S(36.0f), "界面大小", &g_state.uiZoom, 0.75f, 1.4f, "");
    if (Button("##save", V(lp.x + S(18.0f), lp.y + S(122.0f)), V((leftW - S(44.0f)) * 0.6f, S(36.0f)), "保存配置", true))
        NumIoSave("LYNX配置.ini");
    if (Button("##reset", V(lp.x + S(18.0f) + (leftW - S(44.0f)) * 0.6f + S(8.0f), lp.y + S(122.0f)),
               V((leftW - S(44.0f)) * 0.4f, S(36.0f)), "还原", false)) {
        重置配置();
    }
    Txt(dl, V(lp.x + S(18.0f), lp.y + S(166.0f)), S(8.5f), kMuted, "支持手机 / 平板，自动适配分辨率");

    // 设计与交互
    float rightX = p.x + leftW + gap;
    float rightW = w - leftW - gap;
    ImVec2 rp = V(rightX, lowerY);
    Card(dl, rp, V(rightW, S(190.0f)), S(16.0f));
    CardBorder(dl, rp, V(rightW, S(190.0f)), S(16.0f));
    Txt(dl, V(rp.x + S(18.0f), rp.y + S(14.0f)), S(13.0f), kText, "设计与交互");
    Toggle("##snd", V(rp.x + S(18.0f), rp.y + S(48.0f)), rightW - S(36.0f), &g_state.soundFeedback, "声音反馈");
    Toggle("##numpad", V(rp.x + S(18.0f), rp.y + S(78.0f)), rightW - S(36.0f), &g_state.numpadNav, "数字键盘导航");
    Toggle("##canim", V(rp.x + S(18.0f), rp.y + S(108.0f)), rightW - S(36.0f), &g_state.closeAnim, "关闭动画");
    Toggle("##fast", V(rp.x + S(18.0f), rp.y + S(138.0f)), rightW - S(36.0f), &g_state.animSpeedFast, "快速动画");

    Txt(dl, V(p.x, lowerY + S(206.0f)), S(9.5f), kMuted, "所有设置会自动保存 · 查看所有");
}

static void EspPage(ImDrawList* dl, ImVec2 p, float w) {
    Txt(dl, p, S(26.0f), kText, "绘制");
    Txt(dl, V(p.x, p.y + S(36.0f)), S(12.0f), kDim, "透视显示 / ESP");
    float y = p.y + S(68.0f);
    float gap = S(10.0f);
    float cw = (w - gap * 2.0f) / 3.0f;
    float ch = S(58.0f);

    struct Item { const char* label; const char* sub; bool* v; };
    Item items[] = {
        {"方框", "2D / 3D", &DrawIo[2]},
        {"射线", "准心连线", &DrawIo[1]},
        {"骨骼", "火柴人", &DrawIo[3]},
        {"名字", "昵称", &DrawIo[7]},
        {"血量", "血条", &DrawIo[4]},
        {"距离", "米数", &DrawIo[5]},
        {"背敌", "背对提示", &DrawIo[6]},
        {"手持", "当前武器", &DrawIo[8]},
        {"雷达", "视角雷达", &DrawIo[9]},
        {"手雷预警", "投掷物", &DrawIo[10]},
        {"绘制世界", "世界物资", &DrawIo[50]},
        {"物资总开关", "全部物资", &物资开关},
    };
    int n = (int)(sizeof(items) / sizeof(items[0]));
    for (int i = 0; i < n; i++) {
        int col = i % 3, row = i / 3;
        ImVec2 cp = V(p.x + col * (cw + gap), y + row * (ch + gap));
        Card(dl, cp, V(cw, ch), S(12.0f));
        CardBorder(dl, cp, V(cw, ch), S(12.0f));
        Txt(dl, V(cp.x + S(14.0f), cp.y + S(12.0f)), S(12.5f), kText, items[i].label);
        Txt(dl, V(cp.x + S(14.0f), cp.y + S(32.0f)), S(9.5f), kDim, items[i].sub);
        float tw = S(36.0f);
        ImVec2 tp = V(cp.x + cw - tw - S(14.0f), cp.y + ch * 0.5f - S(11.0f));
        ImGui::SetCursorScreenPos(tp);
        ImGui::InvisibleButton((std::string("##esp") + std::to_string(i)).c_str(), V(tw, S(22.0f)));
        if (ImGui::IsItemClicked(0)) *items[i].v = !*items[i].v;
        DrawSwitch(dl, tp, V(tw, S(22.0f)), *items[i].v);
    }
    float sy = y + 4 * (ch + gap) + S(6.0f);
    Slider("##bonethick", V(p.x, sy), w * 0.48f, "骨骼粗细", &NumIo[3], 0.5f, 4.0f, "");
    Slider("##line", V(p.x + w * 0.52f, sy), w * 0.48f, "射线粗细", &NumIo[4], 0.5f, 4.0f, "");
    Slider("##space", V(p.x, sy + S(46.0f)), w * 0.48f, "信息间距", &NumIo[5], 0.0f, 30.0f, "");
    Txt(dl, V(p.x + w * 0.52f, sy + S(46.0f)), S(11.5f), kText, "方框样式");
    const char* styles[3] = {"2D", "3D", "四角"};
    for (int i = 0; i < 3; i++) {
        ImVec2 bp = V(p.x + w * 0.52f + i * S(46.0f), sy + S(68.0f));
        bool on = ((int)NumIo[22]) == i;
        if (Button((std::string("##bs") + std::to_string(i)).c_str(), bp, V(S(40.0f), S(28.0f)), styles[i], on))
            NumIo[22] = (float)i;
    }
}

static void AimPage(ImDrawList* dl, ImVec2 p, float w) {
    Txt(dl, p, S(26.0f), kText, "自瞄");
    Txt(dl, V(p.x, p.y + S(36.0f)), S(12.0f), kDim, "触摸跟枪 / 手感参数");
    float y = p.y + S(68.0f);
    float gap = S(10.0f);
    float cw = (w - gap) * 0.5f;
    float ch = S(58.0f);
    struct Item { const char* label; const char* sub; int idx; };
    Item items[] = {
        {"开启自瞄", "开火跟枪", 0},
        {"显示触摸区", "长按拖动校准", 1},
        {"动态自瞄", "自适应范围", 2},
        {"喷子自瞄", "霰弹枪", 3},
        {"忽略倒地", "不锁倒地", 4},
        {"忽略人机", "不锁人机", 5},
    };
    int n = (int)(sizeof(items) / sizeof(items[0]));
    for (int i = 0; i < n; i++) {
        int col = i % 2, row = i / 2;
        ImVec2 cp = V(p.x + col * (cw + gap), y + row * (ch + gap));
        Card(dl, cp, V(cw, ch), S(12.0f));
        CardBorder(dl, cp, V(cw, ch), S(12.0f));
        Txt(dl, V(cp.x + S(14.0f), cp.y + S(12.0f)), S(12.5f), kText, items[i].label);
        Txt(dl, V(cp.x + S(14.0f), cp.y + S(32.0f)), S(9.5f), kDim, items[i].sub);
        float tw = S(36.0f);
        ImVec2 tp = V(cp.x + cw - tw - S(14.0f), cp.y + ch * 0.5f - S(11.0f));
        ImGui::SetCursorScreenPos(tp);
        ImGui::InvisibleButton((std::string("##aimt") + std::to_string(i)).c_str(), V(tw, S(22.0f)));
        if (ImGui::IsItemClicked(0)) {
            if (items[i].idx == 0) 自瞄按钮[0] = 自瞄按钮[0] ? 0 : 1;
            else 自瞄开关[items[i].idx] = !自瞄开关[items[i].idx];
        }
        bool on = items[i].idx == 0 ? (自瞄按钮[0] != 0) : 自瞄开关[items[i].idx];
        DrawSwitch(dl, tp, V(tw, S(22.0f)), on);
    }
    float sy = y + 3 * (ch + gap) + S(4.0f);
    Slider("##aimdist", V(p.x, sy), w * 0.48f, "自瞄距离", &NumIo[6], 10.0f, 500.0f, "");
    Slider("##aimrange", V(p.x + w * 0.52f, sy), w * 0.48f, "自瞄范围", &NumIo[9], 10.0f, 800.0f, "");
    Slider("##aimlock", V(p.x, sy + S(46.0f)), w * 0.48f, "锁定强度", &NumIo[10], 0.1f, 30.0f, "");
    Slider("##aimspeed", V(p.x + w * 0.52f, sy + S(46.0f)), w * 0.48f, "瞄准速度", &NumIo[11], 0.0f, 30.0f, "");
    Slider("##recoil", V(p.x, sy + S(92.0f)), w * 0.48f, "站立压枪", &NumIo[14], 0.1f, 10.0f, "");
    Slider("##crouch", V(p.x + w * 0.52f, sy + S(92.0f)), w * 0.48f, "蹲下压枪", &NumIo[15], 0.1f, 10.0f, "");
    Txt(dl, V(p.x, sy + S(140.0f)), S(11.5f), kText, "瞄准部位");
    const char* bones[4] = {"头部", "胸部", "智能", "腹部"};
    float boneVals[4] = {1.0f, 0.0f, 3.0f, 2.0f};
    for (int i = 0; i < 4; i++) {
        ImVec2 bp = V(p.x + i * S(64.0f), sy + S(160.0f));
        bool on = 自瞄按钮[7] == (int)boneVals[i];
        if (Button((std::string("##bone") + std::to_string(i)).c_str(), bp, V(S(58.0f), S(32.0f)), bones[i], on))
            自瞄按钮[7] = (int)boneVals[i];
    }
}

static void TrackPage(ImDrawList* dl, ImVec2 p, float w) {
    Txt(dl, p, S(26.0f), kText, "追踪");
    Txt(dl, V(p.x, p.y + S(36.0f)), S(12.0f), kDim, "子弹追踪 / 目标射线");
    float y = p.y + S(70.0f);
    Card(dl, V(p.x, y), V(w, S(86.0f)), S(14.0f));
    CardBorder(dl, V(p.x, y), V(w, S(86.0f)), S(14.0f));
    Txt(dl, V(p.x + S(16.0f), y + S(14.0f)), S(14.0f), kText, "子弹追踪");
    Txt(dl, V(p.x + S(16.0f), y + S(38.0f)), S(10.0f), kDim, "自动修正弹道 / 仅对开火生效");
    float tw = S(44.0f);
    ImVec2 tp = V(p.x + w - tw - S(16.0f), y + S(28.0f));
    ImGui::SetCursorScreenPos(tp);
    ImGui::InvisibleButton("##trackmain", V(tw, S(26.0f)));
    if (ImGui::IsItemClicked(0)) 追踪[1] = !追踪[1];
    DrawSwitch(dl, tp, V(tw, S(26.0f)), 追踪[1]);

    y += S(100.0f);
    Card(dl, V(p.x, y), V(w, S(70.0f)), S(14.0f));
    CardBorder(dl, V(p.x, y), V(w, S(70.0f)), S(14.0f));
    Txt(dl, V(p.x + S(16.0f), y + S(12.0f)), S(13.0f), kText, "目标射线");
    ImGui::SetCursorScreenPos(V(p.x + w - tw - S(16.0f), y + S(22.0f)));
    ImGui::InvisibleButton("##trackline", V(tw, S(24.0f)));
    if (ImGui::IsItemClicked(0)) 追踪[3] = !追踪[3];
    DrawSwitch(dl, V(p.x + w - tw - S(16.0f), y + S(22.0f)), V(tw, S(24.0f)), 追踪[3]);

    y += S(92.0f);
    Slider("##trange", V(p.x, y), w * 0.48f, "追踪范围", &NumIo[9], 0.0f, 4000.0f, "");
    Slider("##tprob", V(p.x + w * 0.52f, y), w * 0.48f, "触发概率", &NumIo[41], 0.0f, 100.0f, "%");

    if (Button("##trackon", V(p.x, y + S(54.0f)), V(w, S(42.0f)), "开启追踪", true)) {
        追踪[1] = true;
        追踪[2] = true;
    }
}

static void SettingsPage(ImDrawList* dl, ImVec2 p, float w) {
    Txt(dl, p, S(26.0f), kText, "设置");
    Txt(dl, V(p.x, p.y + S(36.0f)), S(12.0f), kDim, "外观与系统");
    float y = p.y + S(70.0f);
    Card(dl, V(p.x, y), V(w, S(64.0f)), S(14.0f));
    CardBorder(dl, V(p.x, y), V(w, S(64.0f)), S(14.0f));
    Txt(dl, V(p.x + S(16.0f), y + S(12.0f)), S(13.0f), kText, "深色模式");
    Txt(dl, V(p.x + S(16.0f), y + S(34.0f)), S(9.5f), kDim, "夜间主题");
    ImGui::SetCursorScreenPos(V(p.x + w - S(56.0f), y + S(20.0f)));
    ImGui::InvisibleButton("##dark", V(S(40.0f), S(24.0f)));
    if (ImGui::IsItemClicked(0)) g_state.darkMode = !g_state.darkMode;
    DrawSwitch(dl, V(p.x + w - S(56.0f), y + S(20.0f)), V(S(40.0f), S(24.0f)), g_state.darkMode);

    y += S(80.0f);
    Card(dl, V(p.x, y), V(w, S(150.0f)), S(14.0f));
    CardBorder(dl, V(p.x, y), V(w, S(150.0f)), S(14.0f));
    Txt(dl, V(p.x + S(16.0f), y + S(12.0f)), S(13.0f), kText, "界面缩放");
    Slider("##zoom2", V(p.x + S(16.0f), y + S(40.0f)), w - S(32.0f), "界面大小", &g_state.uiZoom, 0.75f, 1.4f, "");
    if (Button("##fsave", V(p.x + S(16.0f), y + S(96.0f)), V((w - S(40.0f)) * 0.6f, S(38.0f)), "保存配置", true))
        NumIoSave("LYNX配置.ini");
    if (Button("##fexit", V(p.x + S(16.0f) + (w - S(40.0f)) * 0.6f + S(8.0f), y + S(96.0f)),
               V((w - S(40.0f)) * 0.4f, S(38.0f)), "安全退出", false)) {
        shutdown();
        exit(0);
    }
}

State& GetState() { return g_state; }

void ApplyStyle(float scale) {
    g_s = scale;
    ImGuiStyle& s = ImGui::GetStyle();
    ImGui::StyleColorsLight();
    s.WindowRounding = S(22.0f);
    s.ChildRounding = S(16.0f);
    s.FrameRounding = S(12.0f);
    s.GrabRounding = S(12.0f);
    s.PopupRounding = S(12.0f);
    s.ScrollbarRounding = S(10.0f);
    s.ScrollbarSize = S(6.0f);
    s.WindowBorderSize = 0.0f;
    s.FrameBorderSize = 0.0f;
    s.WindowPadding = V(0, 0);
    s.ItemSpacing = V(S(8.0f), S(6.0f));
    s.Colors[ImGuiCol_WindowBg] = ImVec4(0, 0, 0, 0);
    s.Colors[ImGuiCol_ChildBg] = ImVec4(0, 0, 0, 0);
    s.Colors[ImGuiCol_Text] = ImGui::ColorConvertU32ToFloat4(kText);
    s.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0, 0, 0, 0);
    s.Colors[ImGuiCol_ScrollbarGrab] = ImGui::ColorConvertU32ToFloat4(IM_COL32(200, 202, 208, 255));
    s.Colors[ImGuiCol_ScrollbarGrabHovered] = ImGui::ColorConvertU32ToFloat4(IM_COL32(170, 172, 180, 255));
    s.Colors[ImGuiCol_ScrollbarGrabActive] = ImGui::ColorConvertU32ToFloat4(kBlack);
}

void Draw(float screenW, float screenH, float scale) {
    g_s = scale;
    if (g_flipT < 1.0f) {
        g_flipT += ImGui::GetIO().DeltaTime / 0.34f;
        if (g_flipT > 1.0f) g_flipT = 1.0f;
        if (!g_switched && g_flipT >= 0.5f) {
            g_state.page = g_pendingPage;
            g_switched = true;
        }
    }
    const float winW = S(860.0f);
    const float winH = S(600.0f);
    ImVec2 wp = V((screenW - winW) * 0.5f, (screenH - winH) * 0.5f);
    ImVec2 ws = V(winW, winH);

    ImGui::SetNextWindowPos(wp, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ws, ImGuiCond_Always);
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, S(24.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, V(0, 0));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));

    if (ImGui::Begin("##SetoMain", nullptr, flags)) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImDrawList* bg = ImGui::GetBackgroundDrawList();
        g_bgDl = bg;
        g_mainDl = dl;
        float sideW = S(210.0f);
        ImVec2 shell0 = V(wp.x + sideW, wp.y);
        ImVec2 shell1 = V(wp.x + ws.x, wp.y + ws.y);
        Shadow(bg, shell0, shell1, S(24.0f));
        bg->AddRectFilled(shell0, shell1, kShell, S(24.0f));
        bg->AddRect(shell0, shell1, IM_COL32(255, 255, 255, 255), S(24.0f), 0, S(1.0f));
        bg->AddLine(V(shell0.x, wp.y + S(18.0f)), V(shell0.x, wp.y + ws.y - S(18.0f)),
                    IM_COL32(255, 255, 255, 120), S(1.0f));
        Sidebar(dl, wp, V(wp.x + sideW, wp.y + ws.y - S(64.0f)));

        // 顶栏右侧控件
        float topY = wp.y + S(22.0f);
        float rx = wp.x + ws.x - S(26.0f);
        if (Hit("##setbtn", V(rx - S(26.0f), topY - S(6.0f)), V(S(26.0f), S(26.0f)))) RequestPage(3);
        dl->AddCircle(V(rx - S(13.0f), topY + S(7.0f)), S(7.0f), kDim, 0, S(1.4f));
        if (Hit("##minbtn", V(rx - S(62.0f), topY - S(6.0f)), V(S(26.0f), S(26.0f)))) SetMenuVisible(false);
        dl->AddLine(V(rx - S(56.0f), topY + S(10.0f)), V(rx - S(42.0f), topY + S(10.0f)), kDim, S(1.4f));
        if (Hit("##closebtn", V(rx - S(96.0f), topY - S(6.0f)), V(S(26.0f), S(26.0f)))) g_state.requestExit = true;
        dl->AddLine(V(rx - S(90.0f), topY + S(2.0f)), V(rx - S(78.0f), topY + S(14.0f)), kDim, S(1.4f));
        dl->AddLine(V(rx - S(78.0f), topY + S(2.0f)), V(rx - S(90.0f), topY + S(14.0f)), kDim, S(1.4f));
        Txt(dl, V(rx - S(160.0f), topY - S(1.0f)), S(11.0f), kDim, "30");
        dl->AddCircle(V(rx - S(134.0f), topY + S(7.0f)), S(6.0f), kDim, 0, S(1.2f));

        static int lastPage = -1;
        static float pageAnim = 1.0f;
        if (g_state.page != lastPage) {
            lastPage = g_state.page;
            pageAnim = 0.0f;
        }
        pageAnim += ImGui::GetIO().DeltaTime * 7.0f;
        if (pageAnim > 1.0f) pageAnim = 1.0f;
        float pe = 1.0f - (1.0f - pageAnim) * (1.0f - pageAnim) * (1.0f - pageAnim);
        float slide = (1.0f - pe) * S(26.0f);

        float bodyX = wp.x + sideW + S(26.0f) + slide;
        float bodyY = wp.y + S(26.0f);
        float bodyW = ws.x - sideW - S(52.0f);
        float bodyH = ws.y - S(100.0f);
        ImGui::SetCursorScreenPos(V(bodyX, bodyY));
        ImGui::BeginChild("##setobody", V(bodyW, bodyH), false, ImGuiWindowFlags_None);
        ImVec2 cp = ImGui::GetCursorScreenPos();
        float cw = bodyW - S(6.0f);
        ImDrawList* cdl = ImGui::GetWindowDrawList();
        g_bodyDl = cdl;
        if (g_state.page == 0) Home(cdl, cp, cw);
        else if (g_state.page == 1) EspPage(cdl, cp, cw);
        else if (g_state.page == 2) AimPage(cdl, cp, cw);
        else SettingsPage(cdl, cp, cw);
        ImGui::EndChild();

        // 底部状态
        float dy = wp.y + ws.y - S(26.0f);
        char status[128];
        snprintf(status, sizeof(status), "FPS %d   玩家 %d   人机 %d", FPS, PlayerCount, RobotCount);
        ImVec2 ss = TxtSize(S(9.5f), status);
        ImVec2 sp = V(wp.x + ws.x * 0.5f - ss.x * 0.5f - S(12.0f), dy - S(12.0f));
        dl->AddRectFilled(sp, V(sp.x + ss.x + S(24.0f), sp.y + S(22.0f)), kBlack, S(11.0f));
        Txt(dl, V(sp.x + S(12.0f), sp.y + S(5.0f)), S(9.5f), kWhite, status);
    }
    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
}

void TransformDrawData(ImDrawData* data, float cx, float cy) {
    if (!data) return;

    ImGuiIO& io = ImGui::GetIO();
    float nx = 0.0f, ny = 0.0f;
    if (io.MousePos.x > -1.0e20f && cx > 1.0f && cy > 1.0f) {
        nx = (io.MousePos.x - cx) / cx;
        ny = (io.MousePos.y - cy) / cy;
        if (nx < -1.0f) nx = -1.0f; if (nx > 1.0f) nx = 1.0f;
        if (ny < -1.0f) ny = -1.0f; if (ny > 1.0f) ny = 1.0f;
    }

    static float tiltYaw = 0.0f, tiltPitch = 0.0f;
    float targetYaw = nx * 0.38f;     // 约 21.8°
    float targetPitch = -ny * 0.28f;  // 约 16.0°
    float dt = io.DeltaTime;
    if (dt > 0.05f) dt = 0.05f;
    float k = dt * 8.0f;
    if (k > 1.0f) k = 1.0f;
    tiltYaw += (targetYaw - tiltYaw) * k;
    tiltPitch += (targetPitch - tiltPitch) * k;

    bool flipping = (g_flipT < 1.0f);
    if (!flipping && fabsf(tiltYaw) < 0.0002f && fabsf(tiltPitch) < 0.0002f)
        return;

    float yaw = tiltYaw;
    if (flipping) {
        float t = g_flipT;
        yaw += (t < 0.5f) ? (t * 3.14159265f) : ((1.0f - t) * 3.14159265f);
    }
    float pitch = tiltPitch;
    float cyaw = cosf(yaw), syaw = sinf(yaw);
    float cpit = cosf(pitch), spit = sinf(pitch);
    float persp = 1.0f / (1.0f + 0.38f * fabsf(syaw) + 0.22f * fabsf(spit));
    int alphaMul = flipping ? (int)(255.0f * (1.0f - 0.35f * fabsf(syaw))) : 255;

    float tiltMag = fabsf(tiltYaw) / 0.38f + fabsf(tiltPitch) / 0.28f;
    if (tiltMag > 1.0f) tiltMag = 1.0f;
    float sep = tiltMag;
    if (flipping) sep = 1.0f;

    for (int i = 0; i < data->CmdListsCount; i++) {
        ImDrawList* dl = data->CmdLists[i];
        float z = 0.0f;
        if (dl == g_bodyDl) z = 48.0f;
        else if (dl == g_bgDl) z = -36.0f;
        else if (dl == g_mainDl) z = 10.0f;

        float zoff = z * sep;
        float scale = 1.0f + zoff * 0.0007f;
        float ox = syaw * zoff * 0.45f;
        float oy = spit * zoff * 0.38f;

        for (int k2 = 0; k2 < dl->VtxBuffer.Size; k2++) {
            ImDrawVert& v = dl->VtxBuffer[k2];
            float dx = v.pos.x - cx;
            float dy = v.pos.y - cy;
            v.pos.x = cx + dx * cyaw * persp * scale + dy * syaw * 0.34f + ox;
            v.pos.y = cy + dy * cpit * persp * scale + dx * spit * 0.30f + oy;
            if (alphaMul != 255) {
                int a = (v.col >> 24) & 0xFF;
                a = a * alphaMul / 255;
                v.col = (v.col & 0x00FFFFFF) | ((ImU32)a << 24);
            }
        }
        for (int k2 = 0; k2 < dl->CmdBuffer.Size; k2++)
            dl->CmdBuffer[k2].ClipRect = ImVec4(0, 0, cx * 2.0f, cy * 2.0f);
    }
}

}
