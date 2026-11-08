#include "bar.hpp"
#include "theme/theme.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include <cmath>
#include "protect/oxorany.h"
#include "protect/oxorany_include.h"

namespace ui::bar {
    void set_game_alpha(float a) { g_game_alpha = a; }
    float game_alpha() { return g_game_alpha; }

    void render() {
        ImGuiIO& io = ImGui::GetIO();
        static float lt = 0.f;
        static float last_vol_press = 0.f;
        static bool vol_down_last = false;

        float ct = ImGui::GetTime();

        // --- Volume Down Double Press Logic (Simplified) ---
        bool vol_down_now = ImGui::IsKeyDown(ImGuiKey_F12);
        if (vol_down_now && !vol_down_last) {
            if (ct - last_vol_press < 0.6f) { // 600ms window
                g_open = !g_open;
                g_closed = !g_open;
                last_vol_press = 0.f;
            } else {
                last_vol_press = ct;
            }
        }
        vol_down_last = vol_down_now;

        if (g_game_alpha < 0.001f) return;

        // --- White Bar Logic ---
        float sc = g_sw / 1920.f;
        float bw = 50.f * sc;
        float bh = 7.f;
        ImVec2 bar_pos_min((g_sw - bw) * 0.5f, g_sh - bh - 7.f);
        ImVec2 bar_pos_max((g_sw + bw) * 0.5f, g_sh - 7.f);
        ImVec2 click_size(bw + 80.f * sc, bh + 80.f * sc);
        ImVec2 click_pos(bar_pos_min.x - 40.f * sc, bar_pos_min.y - 40.f * sc);

        ImGui::SetNextWindowPos(click_pos);
        ImGui::SetNextWindowSize(click_size);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin(oxorany("##bar_handle"), nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing);
        if (ImGui::InvisibleButton(oxorany("##b_btn"), click_size)) {
            if (ct - lt >= 0.20f) { g_open = !g_open; g_closed = !g_open; lt = ct; }
        }
        ImGui::End();
        ImGui::PopStyleVar();

        ImDrawList* dl = ImGui::GetForegroundDrawList();
        int alpha_val = static_cast<int>(200 * g_game_alpha);
        if (alpha_val > 0) {
            dl->AddRectFilled(bar_pos_min, bar_pos_max, IM_COL32(255, 255, 255, alpha_val), 12.f);
        }
    }
}
