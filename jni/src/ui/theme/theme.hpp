#pragma once
#include "imgui.h"
#include "imgui_internal.h"
#include <string>
#include <unordered_map>

inline float g_sw, g_sh;

namespace ui {

namespace clr {
    // 🔥 Ultra-Premium Dark Space Theme with Electric Accent
    inline ImVec4 bg           = ImVec4(0.05f, 0.05f, 0.06f, 1.0f);  // Deep Space Dark
    inline ImVec4 bg_two       = ImVec4(0.08f, 0.08f, 0.09f, 1.0f);
    inline ImVec4 sidebar      = ImVec4(0.04f, 0.04f, 0.05f, 1.0f);
    inline ImVec4 panel        = ImVec4(0.09f, 0.09f, 0.11f, 1.0f);
    inline ImVec4 widget       = ImVec4(0.12f, 0.12f, 0.14f, 1.0f);
    
    // Electric Azure Blue (Highly Aesthetic)
    inline ImVec4 accent       = ImVec4(0.12f, 0.56f, 1.00f, 1.0f);
    inline ImVec4 accent_light = ImVec4(0.35f, 0.70f, 1.00f, 1.0f);
    inline ImVec4 accent_dark  = ImVec4(0.05f, 0.35f, 0.80f, 1.0f);
    
    inline ImVec4 text         = ImVec4(0.95f, 0.96f, 0.98f, 1.0f);
    inline ImVec4 text_light   = ImVec4(1.00f, 1.00f, 1.00f, 1.0f);
    inline ImVec4 text_dim     = ImVec4(0.55f, 0.58f, 0.62f, 1.0f);
    
    inline ImVec4 border       = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    inline ImVec4 border_light = ImVec4(1.00f, 1.00f, 1.00f, 0.12f);
    inline ImVec4 border_dark  = ImVec4(0.00f, 0.00f, 0.00f, 0.60f);
    inline ImVec4 subtab_bg    = ImVec4(0.07f, 0.07f, 0.08f, 1.0f);
}

namespace style {
    inline std::unordered_map<std::string, float> anims;
    inline std::unordered_map<std::string, ImVec4> anim_colors;
    inline float content_w = 0.f;
    inline float content_alpha = 1.f;
    inline bool popup_open = false;
    inline std::string active_popup = "";
    inline constexpr float S = 2.5f;

    void tick();
    float anim(const std::string& id, float tgt, float spd = 12.f);
    ImVec4 anim_col(const std::string& id, const ImVec4& tgt, float spd = 12.f);
    ImU32 col(const ImVec4& c, float a = 1.f);
    bool popup();
    void close();
    void popups();
}

}