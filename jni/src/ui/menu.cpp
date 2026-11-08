#include <stdlib.h>
#include <math.h>
#include <cmath>

#define IMGUI_DEFINE_MATH_OPERATORS
#include "menu.hpp"
#include "bar.hpp"
#include "cfg.hpp"
#include "Icon.h"
#include "Icon1.h"
#include "theme/theme.hpp"
#include "widgets/widgets.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include "Android_draw/draw.h"
#include <cmath>
#include <ctime>
#include <cstdlib>
#include <string>
#include <vector>
#include "protect/oxorany.h"
#include "protect/oxorany_include.h"
#include "protect/SymbolHider.hpp"

extern std::string EXP;
extern std::string g_UserKey;
extern std::string g_Seller;

extern char g_auth_key[128];
extern char g_auth_expiry[128];
extern char g_auth_seller[128];
extern bool g_UpdateRequired;
extern bool g_KeyExpired;
extern time_t g_expiry_timestamp;

bool aim_show_fov = true;

namespace ui::menu {
    using namespace style;
    using namespace widgets;

    namespace cyber_theme {
        constexpr ImVec4 primary() { return ImVec4(0.00f, 0.55f, 0.95f, 1.0f); }         
        constexpr ImVec4 primary_dark() { return ImVec4(0.00f, 0.25f, 0.55f, 1.0f); }     
        constexpr ImVec4 primary_light() { return ImVec4(0.30f, 0.80f, 1.00f, 1.0f); }     
        constexpr ImVec4 primary_dim() { return ImVec4(0.00f, 0.45f, 0.75f, 0.7f); }       
        constexpr ImVec4 secondary() { return ImVec4(0.85f, 0.20f, 0.95f, 1.0f); }         
        constexpr ImVec4 bg_main() { return ImVec4(0.03f, 0.03f, 0.06f, 1.0f); }           
        constexpr ImVec4 bg_sidebar() { return ImVec4(0.02f, 0.02f, 0.04f, 1.0f); }        
        constexpr ImVec4 bg_panel() { return ImVec4(0.05f, 0.05f, 0.09f, 1.0f); }          
        constexpr ImVec4 bg_header() { return ImVec4(0.04f, 0.06f, 0.12f, 1.0f); }         
        constexpr ImVec4 text_main() { return ImVec4(0.92f, 0.94f, 1.00f, 1.0f); }          
        constexpr ImVec4 text_dim() { return ImVec4(0.65f, 0.70f, 0.85f, 1.0f); }          
        constexpr ImVec4 text_muted() { return ImVec4(0.40f, 0.45f, 0.60f, 1.0f); }        
        constexpr ImVec4 text_neon() { return ImVec4(0.00f, 0.80f, 1.00f, 1.0f); }         
        constexpr ImVec4 border() { return ImVec4(0.12f, 0.18f, 0.28f, 1.0f); }            
        constexpr ImVec4 border_light() { return ImVec4(0.20f, 0.30f, 0.45f, 1.0f); }      
        constexpr ImVec4 border_accent() { return ImVec4(0.00f, 0.55f, 0.95f, 0.8f); }     
        constexpr ImVec4 glow() { return ImVec4(0.00f, 0.55f, 0.95f, 0.35f); }             
        constexpr ImVec4 glow_purple() { return ImVec4(0.85f, 0.20f, 0.95f, 0.25f); }      
    }

    static float ma = 0.f;
    static int tab = 0;
    static bool drag = false;
    static ImVec2 doff = ImVec2(0, 0);
    static float scr_tgt = 0.f;
    static float scr_cur = 0.f;
    static float ta = 1.f;
    static bool tsw = false;
    static int ttab = 0;

    static bool sbd = false;
    static float sbds = 0.f;

    static float mw = 880.f;
    static float mh = 640.f;
    static float sw = 220.f;       // Reduced sidebar width to match smaller panel
    static float th = 64.f;

    static const char* tab_icons[] = { ICON_FA_CROSSHAIRS, ICON_FA_EYE, ICON_FA_MAGIC, ICON_FA_COG, ICON_FA_INFO_CIRCLE };
    static constexpr int tc = sizeof(tab_icons) / sizeof(tab_icons[0]);

    static float lrp(float a, float b, float t) { return a + (b - a) * t; }

    static ImU32 col_with_alpha(ImVec4 color, float alpha) {
        return IM_COL32((int)(color.x * 255), (int)(color.y * 255), (int)(color.z * 255), (int)(color.w * alpha * 255));
    }

    static void shadow(ImVec2 p, ImVec2 s, float a) {
        if (a < 0.01f) return;
        ImDrawList* bg = ImGui::GetBackgroundDrawList();
        bg->AddRectFilled(ImVec2(p.x - 8.f, p.y - 8.f), ImVec2(p.x + s.x + 8.f, p.y + s.y + 8.f), IM_COL32(0, 30, 60, (int)(180 * a)), 16.f);
        bg->AddRectFilled(ImVec2(p.x - 4.f, p.y - 4.f), ImVec2(p.x + s.x + 4.f, p.y + s.y + 4.f), IM_COL32(0, 20, 40, (int)(100 * a)), 14.f);
        bg->AddRectFilled(ImVec2(p.x - 2.f, p.y - 2.f), ImVec2(p.x + s.x + 2.f, p.y + s.y + 2.f), IM_COL32(0, 15, 30, (int)(60 * a)), 12.f);
    }

    static void add_ornament(ImDrawList* dl, ImVec2 pos, float width, float a, bool horizontal = true) {
        if (horizontal) {
            dl->AddRectFilled(ImVec2(pos.x, pos.y), ImVec2(pos.x + width, pos.y + 2.f), col_with_alpha(cyber_theme::primary(), a * 0.6f), 1.f);
            dl->AddRectFilled(ImVec2(pos.x, pos.y + 2.f), ImVec2(pos.x + width * 0.7f, pos.y + 3.f), col_with_alpha(cyber_theme::secondary(), a * 0.4f), 1.f);
            dl->AddRectFilled(ImVec2(pos.x, pos.y - 1.f), ImVec2(pos.x + width, pos.y), col_with_alpha(cyber_theme::glow(), a * 0.5f), 1.f);
        } else {
            dl->AddRectFilled(ImVec2(pos.x, pos.y), ImVec2(pos.x + 2.f, pos.y + width), col_with_alpha(cyber_theme::primary(), a * 0.6f), 1.f);
            dl->AddRectFilled(ImVec2(pos.x + 2.f, pos.y), ImVec2(pos.x + 3.f, pos.y + width), col_with_alpha(cyber_theme::glow(), a * 0.4f), 1.f);
        }
    }


    static void aim_tab(float a) {
        if (checkbox(oxorany("Silent Aim"), &cfg::aim::silent, a)) {
            if (cfg::aim::silent) {
                cfg::aim::aimflick = false;
            }
        }

        if (cfg::aim::silent) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.0f, a));
            ImGui::Text("%s", oxorany("  [!] Use < 70% for safety"));
            ImGui::PopStyleColor();
            slider(oxorany("Headshot Rate"), &cfg::aim::silent_headshot_rate, 0.f, 100.f, a, oxorany("%.0f%%"));
        }

        checkbox(oxorany("Ignore Knocked"), &cfg::aim::ignore_knock, a);
        checkbox(oxorany("Visibility Check"), &cfg::aim::visibility_check, a);
        slider(oxorany("FOV Radius"), &cfg::aim::fov, 0.f, 360.f, a, oxorany("%.0f"));
        checkbox(oxorany("Draw FOV"), &cfg::aim::show_fov, a);
        checkbox(oxorany("Draw Aim Line"), &cfg::aim::show_aimline, a);
    }

    static void esp_tab(float a) {
        checkbox(oxorany("Enable ESP"), &cfg::esp::enable, a);
        checkbox(oxorany("Enable Box"), &cfg::esp::box, a);
        checkbox(oxorany("Draw Line"), &cfg::esp::line, a);
        checkbox(oxorany("Player Count"), &cfg::esp::count, a);
        checkbox(oxorany("Draw Skeleton"), &cfg::esp::skeleton, a);
        checkbox(oxorany("Health Bar"), &cfg::esp::health, a);
        checkbox(oxorany("Health Text"), &cfg::esp::health_text, a);
        checkbox(oxorany("Show Name"), &cfg::esp::name, a);
        checkbox(oxorany("Distance"), &cfg::esp::distance, a);
    }

    static void config_tab(float a) {
        colorpick(oxorany("Box Color"), &cfg::esp::box_color, a);
        slider(oxorany("Box Thickness"), &cfg::esp::box_thickness, 1.f, 5.f, a, oxorany("%.1f"));

        separator(a);

        colorpick(oxorany("Line Color"), &cfg::esp::line_color, a);
        slider(oxorany("Line Thickness"), &cfg::esp::line_thickness, 1.f, 5.f, a, oxorany("%.1f"));

        const char* style_name = oxorany("BOX: NORMAL");
        switch(cfg::esp::box_style) {
            case 0: style_name = oxorany("BOX: NORMAL"); break;
            case 1: style_name = oxorany("BOX: FILLED"); break;
            case 2: style_name = oxorany("BOX: OUTLINE"); break;
            case 3: style_name = oxorany("BOX: TRANSLUCENT"); break;
            case 4: style_name = oxorany("BOX: CORNER"); break;
            case 6: style_name = oxorany("BOX: DASHED"); break;
            case 8: style_name = oxorany("BOX: ROUNDED"); break;
        }

        if (button(style_name, a)) {
            if (cfg::esp::box_style == 0) cfg::esp::box_style = 1;
            else if (cfg::esp::box_style == 1) cfg::esp::box_style = 2;
            else if (cfg::esp::box_style == 2) cfg::esp::box_style = 3;
            else if (cfg::esp::box_style == 3) cfg::esp::box_style = 4;
            else if (cfg::esp::box_style == 4) cfg::esp::box_style = 6;
            else if (cfg::esp::box_style == 6) cfg::esp::box_style = 8;
            else cfg::esp::box_style = 0;
        }

        separator(a);
        colorpick(oxorany("Skeleton Color"), &cfg::esp::skeleton_color, a);

        separator(a);
        colorpick(oxorany("Name Color"), &cfg::esp::name_color, a);
        slider(oxorany("Name Size"), &cfg::esp::name_size, 10.f, 40.f, a, oxorany("%.1f"));

        separator(a);
        colorpick(oxorany("Health Color"), &cfg::esp::hp_text_color, a);
        slider(oxorany("Health Size"), &cfg::esp::hp_text_size, 10.f, 40.f, a, oxorany("%.1f"));

        separator(a);
        colorpick(oxorany("Distance Color"), &cfg::esp::distance_color, a);
        slider(oxorany("Distance Size"), &cfg::esp::distance_size, 10.f, 40.f, a, oxorany("%.1f"));
    }
    
    /*static void memory_tab(float a) {
        // checkbox(oxorany("Fire From Vehicle"), &cfg::mods::fire_vehicle, a); // don't uncomment
        // checkbox(oxorany("Drive On Water"), &cfg::mods::water_drive, a); // don't uncomment
        // checkbox(oxorany("144 FPS"), &cfg::mods::fps_144, a); // don't uncomment
       // checkbox(oxorany("120 FPS"), &cfg::mods::fps_120, a); // don't uncomment
       // checkbox(oxorany("PC LOGO"), &cfg::mods::pc_logo, a); // don't uncomment
    }*/

    static void mods_tab(float a) {
        checkbox(oxorany("Speed Hack"), &cfg::mods::speed_hack, a);
        slider(oxorany("Speed Multiplier"), &cfg::mods::speed_multiplier, 1.0f, 3.5f, a, oxorany("%.1fx"));
        checkbox(oxorany("Speed Overlay"), &cfg::mods::show_speed_overlay, a);
    }

    static void info_tab(float a) {
        ImGui::Dummy(ImVec2(0, 10.f));
        
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, a));
        ImGui::Text("%s", oxorany("   Developer:"));
        ImGui::SameLine(220.f);
        ImGui::TextColored(ImVec4(1, 1, 1, a), "%s", oxorany("KEBLU"));

        ImGui::Text("%s", oxorany("   Version:"));
        ImGui::SameLine(220.f);
        ImGui::TextColored(ImVec4(0.0f, 0.8f, 1.0f, a), "%s", oxorany("2.0"));
        
        ImGui::Text("%s", oxorany("   OB VER:"));
        ImGui::SameLine(220.f);
        ImGui::TextColored(ImVec4(0.0f, 0.8f, 1.0f, a), "%s", oxorany("OB54"));
        
        ImGui::Text("%s", oxorany("   GAME:"));
        ImGui::SameLine(220.f);
        ImGui::TextColored(ImVec4(0.0f, 0.8f, 1.0f, a), "%s", oxorany("FF MAX"));

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, a));
        ImGui::Text("%s", oxorany("   Key Seller:"));
        ImGui::SameLine(220.f);
        ImGui::TextColored(ImVec4(1, 1, 1, a), oxorany("%s"), g_auth_seller);

        ImGui::Dummy(ImVec2(0, 5.f));
        
        ImGui::Text("%s", oxorany("   License Key:"));
        ImGui::SameLine(220.f);
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.8f, a), oxorany("%s"), g_auth_key);

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, a));
        ImGui::Text("%s", oxorany("   Official Owner :"));
        ImGui::SameLine(220.f);
        ImGui::TextColored(ImVec4(1, 1, 1, a), "%s", oxorany("@Kebluuu_On_Top"));
        separator(a);
        checkbox(oxorany("ACTIVATE MENU"), &cfg::other::activate_menu, a);

        // ImGui::Text("%s", oxorany("   Expiry Date:"));
        // ImGui::SameLine(220.f);
        // ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, a), oxorany("%s"), g_auth_expiry);

        ImGui::PopStyleColor(2);

        separator(a);
        ImGui::Dummy(ImVec2(0, 20.f));
        button(oxorany("Exit Menu"), a, []() { Hidden::Exit(0); });
    }

    void render() {
        static const std::string logo_title_secured = oxorany("DARK x STREAMER");
        
        static const std::string tabs_secured[] = { 
            oxorany("Aimbot"), oxorany("Visuals"), oxorany("Mods"), oxorany("Config"), oxorany("Info")
        };
        static const std::string tab_desc_secured[] = {
            oxorany("Precision targeting systems"),
            oxorany("Enhanced visual perception"),
            oxorany("Gameplay modifications"),
            oxorany("Visual appearance settings"),
            oxorany("System configuration & Info")
        };

        ImGuiIO& io = ImGui::GetIO();
        float dt = io.DeltaTime;

        // 🔥 OPTIMIZED: Faster transition (22.f instead of 16.f) for smoother touch release
        ma = lrp(ma, bar::g_open ? 1.f : 0.f, ImClamp(22.f * dt, 0.f, 1.f));
        bar::g_alpha = ma; // Sync animated alpha for touch blocking logic

        if (ma > 0.01f) {
            ImDrawList* bg = ImGui::GetBackgroundDrawList();
            // Reduced alpha from 200 to 75 for a much smoother and less intrusive dimming effect
            int da = (int)(35 * ma * bar::game_alpha());
            bg->AddRectFilled(ImVec2(0, 0), ImVec2(g_sw, g_sh), IM_COL32(0, 0, 0, da));
        }

        if (ma < 0.01f) return;
        tick();

        if (tsw) {
            ta = lrp(ta, 0.f, ImClamp(25.f * dt, 0.f, 1.f));
            if (ta < 0.05f) { tab = ttab; tsw = false; scr_tgt = 0.f; scr_cur = 0.f; }
        } else {
            ta = lrp(ta, 1.f, ImClamp(18.f * dt, 0.f, 1.f));
        }

        float ca = ma * ta;
        content_alpha = ta;

        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ma);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 14.f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));

        ImVec2 wsz(mw, mh);
        ImGui::SetNextWindowSize(wsz, ImGuiCond_Always);
        ImGui::SetNextWindowPos(ImVec2((g_sw - mw) * 0.5f, (g_sh - mh) * 0.5f), ImGuiCond_Once);

        ImGuiWindowFlags wf = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove;

        if (ImGui::Begin(oxorany("##m"), nullptr, wf)) {
            ImVec2 wp = ImGui::GetWindowPos();
            ImDrawList* dl = ImGui::GetWindowDrawList();
            ImVec2 mp = io.MousePos;

            if (g_expiry_timestamp > 0 && time(NULL) > g_expiry_timestamp) g_KeyExpired = true;

            if (g_UpdateRequired || g_KeyExpired) {
                shadow(wp, wsz, ma); 
                dl->AddRectFilled(wp, ImVec2(wp.x + wsz.x, wp.y + wsz.y), IM_COL32(20, 10, 10, (int)(250 * ma)), 14.f);
                dl->AddRect(wp, ImVec2(wp.x + wsz.x, wp.y + wsz.y), IM_COL32(250, 25, 65, 255), 14.f, 0, 3.f);
                
                const char* title = g_UpdateRequired ? oxorany("VERSION OUTDATED") : oxorany("LICENSE EXPIRED");
                const char* sub = g_UpdateRequired ? oxorany("PLEASE DOWNLOAD THE LATEST UPDATE") : oxorany("CONTACT SELLER TO RENEW KEY");

                ImGui::PushFont(fontBold);
                ImVec2 tSize = ImGui::CalcTextSize(title);
                dl->AddText(ImVec2(wp.x + (wsz.x - tSize.x)/2.f, wp.y + wsz.y/2.f - 60.f), IM_COL32(255, 50, 50, (int)(255 * ma)), title);
                ImGui::PopFont();

                ImGui::PushFont(fontMedium);
                ImVec2 sSize = ImGui::CalcTextSize(sub);
                dl->AddText(ImVec2(wp.x + (wsz.x - sSize.x)/2.f, wp.y + wsz.y/2.f - 10.f), IM_COL32(200, 200, 200, (int)(255 * ma)), sub);
                ImGui::PopFont();

                ImGui::SetCursorPos(ImVec2((wsz.x - 200.f) / 2.f, wsz.y / 2.f + 40.f));
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.1f, 0.1f, 1.0f)); 
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.2f, 0.2f, 1.0f)); 
                if (ImGui::Button(oxorany("EXIT GAME"), ImVec2(200.f, 45.f))) Hidden::Exit(0);
                ImGui::PopStyleColor(2);
                ImGui::End();
                ImGui::PopStyleColor(1);
                ImGui::PopStyleVar(4);
                return;
            }

            shadow(wp, wsz, ma);
            dl->AddRectFilled(wp, ImVec2(wp.x + wsz.x, wp.y + wsz.y), col_with_alpha(cyber_theme::bg_main(), ma), 14.f);
            
            // ================== DARK BLUE GLOW EFFECT (CHARO TARAF) ==================
            dl->AddRect(wp, ImVec2(wp.x + wsz.x, wp.y + wsz.y), IM_COL32(0, 40, 150, (int)(55 * ma)), 14.f, 0, 6.0f);
            dl->AddRect(wp, ImVec2(wp.x + wsz.x, wp.y + wsz.y), IM_COL32(0, 60, 200, (int)(100 * ma)), 14.f, 0, 4.0f);
            dl->AddRect(wp, ImVec2(wp.x + wsz.x, wp.y + wsz.y), IM_COL32(0, 80, 255, (int)(170 * ma)), 14.f, 0, 2.0f);
            dl->AddRect(wp, ImVec2(wp.x + wsz.x, wp.y + wsz.y), IM_COL32(0, 50, 180, (int)(255 * ma)), 14.f, 0, 1.2f);
            // =========================================================================
            
            dl->AddRectFilled(wp, ImVec2(wp.x + sw, wp.y + wsz.y), col_with_alpha(cyber_theme::bg_sidebar(), ma), 14.f, ImDrawFlags_RoundCornersLeft);
            dl->AddLine(ImVec2(wp.x + sw, wp.y + 5.f), ImVec2(wp.x + sw, wp.y + wsz.y - 5.f), col_with_alpha(cyber_theme::border_accent(), ma * 0.7f));
            add_ornament(dl, ImVec2(wp.x + 20.f, wp.y + 5.f), sw - 40.f, ma, true);

            float logo_y = wp.y + 32.f;
            ImGui::PushFont(fontMedium);
            dl->AddRectFilled(ImVec2(wp.x + 15.f, logo_y - 8.f), ImVec2(wp.x + sw - 15.f, logo_y + 38.f), col_with_alpha(cyber_theme::primary(), ma * 0.1f), 8.f);
            dl->AddRectFilled(ImVec2(wp.x + 15.f, logo_y - 8.f), ImVec2(wp.x + sw - 15.f, logo_y + 38.f), col_with_alpha(cyber_theme::glow(), ma * 0.15f), 8.f);
            
            const char* logoText = logo_title_secured.c_str();
            ImVec2 textSize = ImGui::CalcTextSize(logoText);
            float textX = wp.x + (sw - textSize.x) * 0.5f;
            dl->AddText(ImVec2(textX, logo_y), col_with_alpha(cyber_theme::primary_light(), ma), logoText);
            
            float underlineWidth = textSize.x * 0.8f; 
            float underlineX = textX + (textSize.x - underlineWidth) * 0.5f;
            dl->AddRectFilled(ImVec2(underlineX, logo_y + 32.f), ImVec2(underlineX + underlineWidth, logo_y + 34.f), col_with_alpha(cyber_theme::primary(), ma * 0.8f), 2.f);
            dl->AddRectFilled(ImVec2(underlineX, logo_y + 33.f), ImVec2(underlineX + underlineWidth, logo_y + 35.f), col_with_alpha(cyber_theme::glow(), ma * 0.4f), 2.f);
            ImGui::PopFont();

            dl->AddLine(ImVec2(wp.x + 20.f, logo_y + 48.f), ImVec2(wp.x + sw - 20.f, logo_y + 48.f), col_with_alpha(cyber_theme::border(), ma));
            dl->AddLine(ImVec2(wp.x + 20.f, logo_y + 49.f), ImVec2(wp.x + sw - 20.f, logo_y + 49.f), col_with_alpha(cyber_theme::glow(), ma * 0.3f));

            float tsy = logo_y + 70.f;
            float tg = 6.f;

            for (int i = 0; i < tc; i++) {
                float ty = tsy + i * (th + tg);
                float tx = wp.x;
                float tw = sw;
                ImVec2 tmin(tx, ty);
                ImVec2 tmax(tx + tw, ty + th);

                bool sel = (tab == i);
                bool hov = ImGui::IsMouseHoveringRect(tmin, tmax);
                float sa = anim((std::string("t_") + std::to_string(i)).c_str(), sel ? 1.f : 0.f, 16.f);
                float ha = anim((std::string("th_") + std::to_string(i)).c_str(), hov && !sel ? 0.5f : 0.f, 16.f);

                if (sa > 0.01f) {
                    dl->AddRectFilled(tmin, tmax, col_with_alpha(cyber_theme::primary(), sa * ma * 0.12f));
                    dl->AddRectFilled(ImVec2(tmin.x, tmin.y + 10.f), ImVec2(tmin.x + 4.f, tmax.y - 10.f), col_with_alpha(cyber_theme::primary(), sa * ma));
                    dl->AddRectFilled(ImVec2(tmax.x - 2.f, tmin.y + 10.f), ImVec2(tmax.x, tmax.y - 10.f), col_with_alpha(cyber_theme::secondary(), sa * ma * 0.6f));
                    dl->AddRectFilled(ImVec2(tmin.x, tmin.y + 5.f), ImVec2(tmax.x, tmin.y + 2.f), col_with_alpha(cyber_theme::glow(), sa * ma * 0.5f), 0.f);
                } else if (ha > 0.01f) {
                    dl->AddRectFilled(tmin, tmax, col_with_alpha(cyber_theme::bg_panel(), ha * ma));
                }

                ImVec4 ttc = anim_col((std::string("tt_") + std::to_string(i)).c_str(), sel ? cyber_theme::text_neon() : cyber_theme::text_dim(), 14.f);
                ImVec4 itc = anim_col((std::string("it_") + std::to_string(i)).c_str(), sel ? cyber_theme::primary() : cyber_theme::text_muted(), 14.f);

                ImGui::PushFont(fontMedium);
                float ny = ty + (th - fontMedium->FontSize) * 0.5f;
                dl->AddText(ImVec2(tx + 28.f, ny), col_with_alpha(itc, ma), tab_icons[i]);
                ImVec2 isz = ImGui::CalcTextSize(tab_icons[i]);
                
                dl->AddText(ImVec2(tx + 28.f + isz.x + 16.f, ny), col_with_alpha(ttc, ma), tabs_secured[i].c_str());
                ImGui::PopFont();

                if (hov && ImGui::IsMouseClicked(0) && ma > 0.5f && !tsw) {
                    if (!popup() && tab != i) { ttab = i; tsw = true; }
                    close();
                }
            }

            float csx = wp.x + sw;
            float cp = 30.f;
            float scr_w = 10.f;
            float scr_pad = 18.f;
            content_w = wsz.x - sw - cp * 2.f - scr_w - scr_pad;
            float cw = content_w;

            ImGui::PushFont(fontMedium);
            ImVec2 header_icn_sz = ImGui::CalcTextSize(tab_icons[tab]);
            dl->AddText(ImVec2(csx + cp, wp.y + cp), col_with_alpha(cyber_theme::primary(), ma), tab_icons[tab]);
            char headerTitle[64];
            
            snprintf(headerTitle, sizeof(headerTitle), "%s", tabs_secured[tab].c_str());
            for(int i=0; headerTitle[i]; i++) { if(headerTitle[i]>='a' && headerTitle[i]<='z') headerTitle[i]-=32; }
            ImVec2 header_txt_sz = ImGui::CalcTextSize(headerTitle);
            dl->AddText(ImVec2(csx + cp + header_icn_sz.x + 14.f, wp.y + cp), col_with_alpha(cyber_theme::text_neon(), ma), headerTitle);
            ImGui::PopFont();

            ImGui::PushFont(fontDesc);
            ImVec2 desc_sz = ImGui::CalcTextSize(tab_desc_secured[tab].c_str());
            
            dl->AddText(ImVec2(csx + cp + header_icn_sz.x + header_txt_sz.x + 24.f, wp.y + cp + (header_icn_sz.y - desc_sz.y)*0.5f), col_with_alpha(cyber_theme::text_muted(), ma), tab_desc_secured[tab].c_str());
            ImGui::PopFont();

            dl->AddLine(ImVec2(csx + cp, wp.y + cp + header_icn_sz.y + 18.f), ImVec2(wp.x + wsz.x - cp, wp.y + cp + header_icn_sz.y + 18.f), col_with_alpha(cyber_theme::border_accent(), ma));
            float mid_x = csx + cp + (wp.x + wsz.x - cp - csx - cp) * 0.5f;
            dl->AddRectFilled(ImVec2(mid_x - 20.f, wp.y + cp + header_icn_sz.y + 16.f), ImVec2(mid_x + 20.f, wp.y + cp + header_icn_sz.y + 20.f), col_with_alpha(cyber_theme::primary(), ma * 0.5f), 4.f);
            dl->AddRectFilled(ImVec2(mid_x - 18.f, wp.y + cp + header_icn_sz.y + 17.f), ImVec2(mid_x + 18.f, wp.y + cp + header_icn_sz.y + 19.f), col_with_alpha(cyber_theme::glow(), ma * 0.4f), 4.f);

            float csy = wp.y + cp + header_icn_sz.y + 40.f;
            float ch = wsz.y - (csy - wp.y) - 25.f;

            ImVec2 cpos = ImVec2(csx + cp, csy);
            ImVec2 cmax = ImVec2(cpos.x + cw, cpos.y + ch);

            ImGui::SetCursorScreenPos(cpos);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ca);
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));
            ImGui::PushClipRect(cpos, cmax, true);

            ImGui::BeginChild(oxorany("##c"), ImVec2(cw, ch), false, ImGuiWindowFlags_NoScrollbar);

            float sm = ImGui::GetScrollMaxY();
            bool chov = ImGui::IsMouseHoveringRect(cpos, cmax);
            if (chov && !popup()) {
                float wh = io.MouseWheel;
                if (wh != 0.f) scr_tgt -= wh * 70.f;
                if (ImGui::IsMouseDragging(0)) scr_tgt -= io.MouseDelta.y;
            }

            scr_tgt = ImClamp(scr_tgt, 0.f, ImMax(sm, 0.f));
            scr_cur = lrp(scr_cur, scr_tgt, ImClamp(20.f * dt, 0.f, 1.f));
            if (fabsf(scr_cur - scr_tgt) < 0.5f) scr_cur = scr_tgt;
            ImGui::SetScrollY(scr_cur);

            switch (tab) {
                case 0: aim_tab(ca); break;
                case 1: esp_tab(ca); break;
                case 2: mods_tab(ca); break;
                case 3: config_tab(ca); break;
                case 4: info_tab(ca); break;
            }

            sm = ImGui::GetScrollMaxY();
            scr_tgt = ImClamp(scr_tgt, 0.f, ImMax(sm, 0.f));

            ImGui::EndChild();
            ImGui::PopClipRect();
            ImGui::PopStyleColor(1);
            ImGui::PopStyleVar(2);

            float sbx = cmax.x + scr_pad;
            float sby = cpos.y;
            float sbh = ch;

            if (sm > 0.f) {
                dl->AddRectFilled(ImVec2(sbx, sby), ImVec2(sbx + scr_w, sby + sbh), col_with_alpha(cyber_theme::bg_panel(), ma), scr_w * 0.5f);
                float sr = ch / (ch + sm);
                float gh = ImMax(sbh * sr, 45.f);
                float sn = scr_tgt / sm;
                float gy = sby + (sbh - gh) * sn;
                
                dl->AddRectFilled(ImVec2(sbx, gy), ImVec2(sbx + scr_w, gy + gh), col_with_alpha(cyber_theme::primary(), ma), scr_w * 0.5f);
                dl->AddRectFilled(ImVec2(sbx, gy), ImVec2(sbx + scr_w * 0.7f, gy + gh), col_with_alpha(cyber_theme::secondary(), ma * 0.5f), scr_w * 0.5f);
                dl->AddRectFilled(ImVec2(sbx - 1.f, gy), ImVec2(sbx, gy + gh), col_with_alpha(cyber_theme::glow(), ma * 0.6f), 0.f);

                ImRect sbr(ImVec2(sbx - 20.f, sby), ImVec2(sbx + scr_w + 20.f, sby + sbh));
                bool sbhov = ImGui::IsMouseHoveringRect(sbr.Min, sbr.Max);
                if (sbhov && ImGui::IsMouseClicked(0) && !popup()) { sbd = true; sbds = mp.y - gy; }
                if (!ImGui::IsMouseDown(0)) sbd = false;
                if (sbd && sm > 0.f) {
                    float ngy = mp.y - sbds;
                    float nn = (ngy - sby) / (sbh - gh);
                    scr_tgt = ImClamp(nn, 0.f, 1.f) * sm;
                }
            } else {
                sbd = false;
            }

            bool inm = (mp.x >= wp.x && mp.x <= wp.x + wsz.x && mp.y >= wp.y && mp.y <= wp.y + wsz.y);
            bool inca = (mp.x >= cpos.x && mp.x <= cmax.x && mp.y >= cpos.y && mp.y <= cmax.y);

            if (inm && !popup() && ImGui::IsMouseClicked(0)) {
                if (!inca && !sbd) { drag = true; doff = ImVec2(mp.x - wp.x, mp.y - wp.y); }
            }
            if (drag) {
                if (ImGui::IsMouseDown(0)) {
                    ImVec2 np = ImVec2(mp.x - doff.x, mp.y - doff.y);
                    np.x = ImClamp(np.x, 0.f, g_sw - wsz.x);
                    np.y = ImClamp(np.y, 0.f, g_sh - wsz.y);
                    ImGui::SetWindowPos(oxorany("##m"), np);
                } else { drag = false; }
            }
            if (popup()) drag = false;
        }
        ImGui::End();
        ImGui::PopStyleColor(1);
        ImGui::PopStyleVar(4);

        popups();
    }

    void render_speed_overlay() {
        if (!cfg::mods::show_speed_overlay) return;

        ImGui::SetNextWindowBgAlpha(0.95f);
        ImGuiWindowFlags overlay_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;

        ImVec4 bgColor = cfg::mods::speed_hack ? ImVec4(0.85f, 0.10f, 0.10f, 0.95f) : ImVec4(0.20f, 0.50f, 0.90f, 0.95f);

        ImGui::PushStyleColor(ImGuiCol_WindowBg, bgColor);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.0f, 12.0f));

        if (ImGui::Begin(oxorany("PremiumSpeedOverlay"), nullptr, overlay_flags)) {
            ImGui::SetWindowFontScale(1.4f);
            ImGui::PushFont(fontBold);
            ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize(oxorany("SPEED")).x) * 0.5f);
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", oxorany("SPEED"));
            ImGui::PopFont();

            ImGui::Dummy(ImVec2(0, 6.0f));

            float height = 68.0f;
            float width = 136.0f;
            float radius = height * 0.50f;

            ImGui::SetCursorPosX((ImGui::GetWindowWidth() - width) * 0.5f);
            ImVec2 p = ImGui::GetCursorScreenPos();
            ImDrawList* draw_list = ImGui::GetWindowDrawList();

            ImU32 bg_col = cfg::mods::speed_hack ? IM_COL32(200, 30, 30, 255) : IM_COL32(50, 130, 230, 255);
            draw_list->AddRectFilled(p, ImVec2(p.x + width, p.y + height), bg_col, radius);

            if (cfg::mods::speed_hack) draw_list->AddRect(p, ImVec2(p.x + width, p.y + height), IM_COL32(255, 60, 60, 180), radius, 0, 3.0f);
            else draw_list->AddRect(p, ImVec2(p.x + width, p.y + height), IM_COL32(80, 180, 255, 180), radius, 0, 3.0f);

            float circle_radius = radius - 6.0f;
            float circle_x = p.x + (cfg::mods::speed_hack ? width - radius : radius);
            float circle_y = p.y + radius;

            draw_list->AddCircleFilled(ImVec2(circle_x, circle_y), circle_radius, IM_COL32(255, 255, 255, 255));
            draw_list->AddCircle(ImVec2(circle_x, circle_y), circle_radius, IM_COL32(150, 150, 150, 180), 0, 2.0f);

            float dot_radius = circle_radius * 0.35f;
            if (cfg::mods::speed_hack) draw_list->AddCircleFilled(ImVec2(circle_x, circle_y), dot_radius, IM_COL32(200, 30, 30, 255));
            else draw_list->AddCircleFilled(ImVec2(circle_x, circle_y), dot_radius, IM_COL32(50, 130, 230, 255));

            if (ImGui::InvisibleButton(oxorany("##speed_toggle_big"), ImVec2(width, height))) {
                cfg::mods::speed_hack = !cfg::mods::speed_hack;
            }
            ImGui::SetWindowFontScale(1.0f);
        }
        ImGui::End();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor();
    }
}