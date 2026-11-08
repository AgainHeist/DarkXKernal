#define IMGUI_DEFINE_MATH_OPERATORS
#include "widgets.hpp"
#include "Android_draw/draw.h"
#include <cmath>
#include <algorithm>
#include <functional>
#include <unordered_map>
#include <string>
#include <vector>

#ifndef IM_PI
#define IM_PI 3.14159265358979323846f
#endif

namespace ui {

namespace style {
    static float dt = 0.016f;
    static bool clicked = false;

    static float lrp(float a, float b, float t) { return a + (b - a) * t; }
    static ImVec4 lrp_col(const ImVec4& a, const ImVec4& b, float t) {
        return ImVec4(lrp(a.x, b.x, t), lrp(a.y, b.y, t), lrp(a.z, b.z, t), lrp(a.w, b.w, t));
    }

    void tick() {
        clicked = false;
        static float lt = 0.f;
        float ct = ImGui::GetTime();
        if (lt > 0.f) {
            dt = ct - lt;
            dt = ImClamp(dt, 0.001f, 0.1f);
        }
        lt = ct;
    }

    float anim(const std::string& id, float tgt, float spd) {
        auto it = anims.find(id);
        if (it == anims.end()) {
            anims[id] = tgt;
            return tgt;
        }
        float t = ImClamp(spd * dt, 0.f, 1.f);
        it->second = lrp(it->second, tgt, t * (1.5f - t * 0.5f)); 
        if (fabsf(it->second - tgt) < 0.001f) it->second = tgt;
        return it->second;
    }

    ImVec4 anim_col(const std::string& id, const ImVec4& tgt, float spd) {
        auto it = anim_colors.find(id);
        if (it == anim_colors.end()) {
            anim_colors[id] = tgt;
            return tgt;
        }
        float t = ImClamp(spd * dt, 0.f, 1.f);
        it->second = lrp_col(it->second, tgt, t);
        return it->second;
    }

    ImU32 col(const ImVec4& c, float a) {
        return IM_COL32((int)(c.x * 255), (int)(c.y * 255), (int)(c.z * 255), (int)(c.w * 255 * a * content_alpha));
    }

    bool popup() { return popup_open || !active_popup.empty(); }
    void close() { active_popup = ""; popup_open = false; }

    void popups() {
        if (active_popup.empty()) return;
        float pa = anim(active_popup + "_pa", 1.f, 18.f);
        if (pa < 0.01f) { active_popup = ""; return; }
        if (pa > 0.15f && ImGui::IsMouseClicked(0) && !clicked) {
            active_popup = "";
            popup_open = false;
        }
    }
}

namespace widgets {
    using namespace style;

    // ==================== TOGGLE CHECKBOX (Blue Accent) ====================
    bool checkbox(const char* name, bool* v, float a) {
        if (a < 0.01f) return false;
        ImGuiWindow* w = ImGui::GetCurrentWindow();
        if (w->SkipItems) return false;

        std::string ids = std::string("cb_") + name;
        ImGuiID id = w->GetID(ids.c_str());

        float ww = content_w > 0 ? content_w : ImGui::GetContentRegionAvail().x;
        float h = 36.f * S;
        float pad = 12.f * S;

        ImVec2 pos = w->DC.CursorPos;
        ImRect r(pos, ImVec2(pos.x + ww, pos.y + h));

        ImGui::ItemSize(r);
        if (!ImGui::ItemAdd(r, id)) return false;

        bool hov = ImGui::IsMouseHoveringRect(r.Min, r.Max) && !popup();

        // Toggle Background area
        float sw_w = 44.f * S;
        float sw_h = 22.f * S;
        float sw_x = r.Max.x - sw_w - pad;
        float sw_y = r.GetCenter().y - sw_h * 0.5f;
        ImVec2 sw_min(sw_x, sw_y);
        ImVec2 sw_max(sw_x + sw_w, sw_y + sw_h);

        bool sw_hov = ImGui::IsMouseHoveringRect(sw_min, sw_max) && !popup();

        // Only toggle if the switch area specifically is clicked
        bool press = sw_hov && ImGui::IsMouseClicked(0) && !clicked;
        if (press) { *v = !*v; clicked = true; }

        float ha = anim(ids + "_h", hov ? 1.f : 0.f, 10.f);
        float va = anim(ids + "_v", *v ? 1.f : 0.f, 15.f);

        ImDrawList* dl = w->DrawList;

        if (ha > 0.01f) dl->AddRectFilled(r.Min, r.Max, col(clr::widget, a * ha * 0.4f), 8.f * S);

        // Toggle Background using precalculated sw_min/sw_max
        ImU32 bg_col = col(lrp_col(clr::widget, clr::accent, va), a);
        dl->AddRectFilled(sw_min, sw_max, bg_col, sw_h * 0.5f);
        dl->AddRect(sw_min, sw_max, col(clr::border_light, a), sw_h * 0.5f, 0, 1.5f * S);

        // Thumb
        float t_r = (sw_h * 0.5f) - 3.0f * S;
        float t_x = sw_min.x + (sw_h * 0.5f) + va * (sw_w - sw_h);
        dl->AddCircleFilled(ImVec2(t_x, sw_min.y + sw_h * 0.5f), t_r, col(clr::text_light, a), 32);

        // Text
        ImGui::PushFont(fontMedium);
        float tx = r.Min.x + pad;
        float ty = r.GetCenter().y - fontMedium->FontSize * 0.5f;
        ImVec4 tc = anim_col(ids + "_tc", *v ? clr::text_light : clr::text_dim, 12.f);
        dl->AddText(ImVec2(tx, ty), col(tc, a), name);
        ImGui::PopFont();

        return press;
    }

    static std::unordered_map<ImGuiID, bool> sl_active;
    static std::unordered_map<ImGuiID, bool> sl_locked;

    // ==================== PREMIUM BLUE SLIDER (Smooth & Responsive) ====================
    bool slider(const char* name, float* v, float mn, float mx, float a, const char* fmt) {
        if (a < 0.01f) return false;
        ImGuiWindow* w = ImGui::GetCurrentWindow();
        if (w->SkipItems) return false;

        std::string ids = std::string("sl_") + name;
        ImGuiID id = w->GetID(ids.c_str());

        float ww = content_w > 0 ? content_w : ImGui::GetContentRegionAvail().x;
        float h = 55.f * S; 
        float pad = 12.f * S;

        ImVec2 pos = w->DC.CursorPos;
        ImRect r(pos, ImVec2(pos.x + ww, pos.y + h));

        float bh = 6.f * S; 
        float by = r.Max.y - pad - bh - 2.f * S;
        ImRect bar(ImVec2(r.Min.x + pad, by), ImVec2(r.Max.x - pad, by + bh));

        ImGui::ItemSize(r);
        if (!ImGui::ItemAdd(r, id)) return false;

        bool hov = ImGui::IsMouseHoveringRect(r.Min, r.Max) && !popup();
        static std::unordered_map<ImGuiID, bool> active_sliders;

        if (hov && ImGui::IsMouseDown(0) && !clicked) {
            active_sliders[id] = true;
        }

        if (active_sliders[id]) {
            if (ImGui::IsMouseDown(0)) {
                float mx_pos = ImGui::GetIO().MousePos.x;
                float nn = ImClamp((mx_pos - bar.Min.x) / bar.GetWidth(), 0.f, 1.f);
                *v = mn + nn * (mx - mn);
            } else {
                active_sliders[id] = false;
            }
        }

        float norm = (*v - mn) / (mx - mn);
        float na = anim(ids + "_n", norm, 30.f); // Increased speed for ultra-smoothness
        ImDrawList* dl = w->DrawList;

        // Panel background
        dl->AddRectFilled(r.Min, r.Max, col(clr::panel, a), 8.f * S);
        if (hov || active_sliders[id])
            dl->AddRect(r.Min, r.Max, col(clr::accent, a * 0.3f), 8.f * S, 0, 1.5f * S);

        ImGui::PushFont(fontMedium);
        float ty = r.Min.y + 10.f * S;
        dl->AddText(ImVec2(r.Min.x + pad, ty), col(clr::text, a), name);

        char vb[32];
        snprintf(vb, sizeof(vb), fmt, *v);
        ImVec2 vs = ImGui::CalcTextSize(vb);
        dl->AddText(ImVec2(r.Max.x - pad - vs.x, ty), col(clr::accent_light, a), vb);
        ImGui::PopFont();

        // Track Background
        dl->AddRectFilled(bar.Min, bar.Max, col(clr::widget, a), bh * 0.5f);
        
        // Track Fill (Electric Blue)
        float fw = bar.GetWidth() * na;
        if (fw > 0.f) {
            dl->AddRectFilled(bar.Min, ImVec2(bar.Min.x + fw, bar.Max.y), col(clr::accent, a), bh * 0.5f);
            // Glow effect for the fill
            dl->AddRectFilled(bar.Min, ImVec2(bar.Min.x + fw, bar.Max.y), col(clr::accent_light, a * 0.2f), bh * 0.5f);
        }

        // Circular Thumb (Blue with white center)
        float gx = bar.Min.x + fw;
        ImVec2 thumb_pos = ImVec2(gx, bar.GetCenter().y);
        float thumb_r = 8.5f * S;
        
        // Outer glow
        dl->AddCircleFilled(thumb_pos, thumb_r + 2.f * S, col(clr::accent, a * 0.2f), 32);
        // Main thumb body
        dl->AddCircleFilled(thumb_pos, thumb_r, col(clr::accent, a), 32);
        // Inner white dot for premium look
        dl->AddCircleFilled(thumb_pos, 3.f * S, col(clr::text_light, a), 32);
        // Border
        dl->AddCircle(thumb_pos, thumb_r, col(clr::text_light, a * 0.5f), 32, 1.5f * S);

        return active_sliders[id];
    }

    static ImVec2 cmb_pos;
    static float cmb_w;
    static std::vector<const char*> cmb_items;
    static int* cmb_ptr = nullptr;

    // ==================== NORMAL COMBO BOX ====================
    bool combo(const char* name, int* v, const std::vector<const char*>& items, float a) {
        if (a < 0.01f) return false;
        ImGuiWindow* w = ImGui::GetCurrentWindow();
        if (w->SkipItems) return false;

        std::string ids = std::string("cmb_") + name;
        ImGuiID id = w->GetID(ids.c_str());

        float ww = content_w > 0 ? content_w : ImGui::GetContentRegionAvail().x;
        float h = 55.f * S;
        float pad = 12.f * S;

        ImVec2 pos = w->DC.CursorPos;
        ImRect r(pos, ImVec2(pos.x + ww, pos.y + h));

        float bh = 28.f * S;
        ImRect btn(ImVec2(r.Min.x + pad, r.Max.y - pad - bh), ImVec2(r.Max.x - pad, r.Max.y - pad));

        ImGui::ItemSize(r);
        if (!ImGui::ItemAdd(r, id)) return false;

        bool hov = ImGui::IsMouseHoveringRect(btn.Min, btn.Max);
        bool other = !active_popup.empty() && active_popup != ids;
        bool press = hov && ImGui::IsMouseClicked(0) && !other && !clicked;

        bool open = (active_popup == ids);

        if (press) {
            if (open) { active_popup = ""; popup_open = false; }
            else {
                active_popup = ids;
                popup_open = true;
                cmb_pos = ImVec2(btn.Min.x, btn.Max.y + 6.f * S);
                cmb_w = btn.GetWidth();
                cmb_items = items;
                cmb_ptr = v;
            }
            clicked = true;
        }

        float oa = anim(ids + "_o", open ? 1.f : 0.f, 12.f);
        bool act = open || (hov && ImGui::IsMouseDown(0) && !popup());
        float ha = anim(ids + "_h", act ? 1.f : 0.f, act ? 20.f : 12.f);
        float arrow_rot = anim(ids + "_ar", open ? 1.f : 0.f, 15.f);

        ImDrawList* dl = w->DrawList;

        dl->AddRectFilled(r.Min, r.Max, col(clr::panel, a), 8.f * S);
        dl->AddRect(r.Min, r.Max, col(clr::border, a), 8.f * S);

        ImGui::PushFont(fontMedium);
        float ty = r.Min.y + 8.f * S;
        dl->AddText(ImVec2(r.Min.x + pad, ty), col(clr::text, a), name);
        ImGui::PopFont();

        ImVec4 btn_col = lrp_col(clr::widget, clr::bg_two, ha);
        dl->AddRectFilled(btn.Min, btn.Max, col(btn_col, a), 6.f * S);
        dl->AddRect(btn.Min, btn.Max, col(open ? clr::accent : clr::border, a), 6.f * S);

        ImGui::PushFont(fontMedium);
        const char* prev = (*v >= 0 && *v < (int)items.size()) ? items[*v] : "";
        float py = btn.GetCenter().y - fontMedium->FontSize * 0.5f;
        dl->AddText(ImVec2(btn.Min.x + 10.f * S, py), col(clr::text, a), prev);
        ImGui::PopFont();

        float ax = btn.Max.x - 14.f * S;
        float ay = btn.GetCenter().y;
        float ar_sz = 5.f * S;
        float ar_off = 2.f * S * (1.f - arrow_rot * 2.f);
        
        ImVec2 p1(ax - ar_sz, ay - ar_off);
        ImVec2 p2(ax + ar_sz, ay - ar_off);
        ImVec2 p3(ax, ay + ar_off + ar_sz * 0.5f * (1.f - arrow_rot * 2.f));
        if (arrow_rot > 0.5f) {
            p1 = ImVec2(ax - ar_sz, ay + ar_off);
            p2 = ImVec2(ax + ar_sz, ay + ar_off);
            p3 = ImVec2(ax, ay - ar_off - ar_sz * 0.5f * (arrow_rot * 2.f - 1.f));
        }
        dl->AddTriangleFilled(p1, p2, p3, col(clr::accent, a));

        if (oa > 0.01f && cmb_ptr == v) {
            ImDrawList* fg = ImGui::GetForegroundDrawList();
            float ih = 32.f * S;
            float ph = items.size() * ih;
            float visible_h = ph * oa;
            ImVec2 pmin = cmb_pos;
            ImVec2 pmax(pmin.x + cmb_w, pmin.y + ph);

            fg->AddRectFilled(ImVec2(pmin.x - 4, pmin.y - 4), ImVec2(pmax.x + 4, pmin.y + visible_h + 4), IM_COL32(0,0,0, (int)(80 * oa)), 8.f * S);
            fg->PushClipRect(pmin, ImVec2(pmax.x, pmin.y + visible_h), true);
            fg->AddRectFilled(pmin, pmax, col(clr::panel, oa), 8.f * S);
            fg->AddRect(pmin, pmax, col(clr::accent, oa * 0.5f), 8.f * S);

            float iy = pmin.y;
            for (int i = 0; i < (int)items.size(); i++) {
                ImRect ir(ImVec2(pmin.x, iy), ImVec2(pmax.x, iy + ih));
                bool ihov = ImGui::IsMouseHoveringRect(ir.Min, ir.Max, false) && oa > 0.7f;
                bool isel = (i == *v);

                std::string item_id = ids + "_i" + std::to_string(i);
                float item_ha = anim(item_id + "_h", ihov ? 1.f : 0.f, 16.f);
                float item_sa = anim(item_id + "_s", isel ? 1.f : 0.f, 12.f);

                if (item_ha > 0.01f) fg->AddRectFilled(ir.Min, ir.Max, col(clr::widget, oa * item_ha));
                if (item_sa > 0.01f) {
                    fg->AddRectFilled(ir.Min, ImVec2(ir.Min.x + 4.f * S, ir.Max.y), col(clr::accent, oa * item_sa));
                    fg->AddRectFilled(ir.Min, ir.Max, col(clr::accent, oa * item_sa * 0.15f));
                }

                ImGui::PushFont(fontMedium);
                float tty = ir.GetCenter().y - fontMedium->FontSize * 0.5f;
                ImVec4 txt_col = lrp_col(clr::text, clr::accent, item_sa);
                fg->AddText(ImVec2(ir.Min.x + 12.f * S, tty), col(txt_col, oa), items[i]);
                ImGui::PopFont();

                if (ihov && ImGui::IsMouseClicked(0) && !clicked && open && oa > 0.7f) {
                    *v = i;
                    active_popup = "";
                    popup_open = false;
                    clicked = true;
                }
                iy += ih;
            }
            fg->PopClipRect();
        }
        return false;
    }

    static std::string cp_id = "";
    static ImVec4* cp_ptr = nullptr;
    static std::unordered_map<std::string, ImVec4> hsv_cache;
    static std::string picker = "";

    // ==================== NORMAL COLOR PICKER ====================
    void colorpick(const char* name, ImVec4* v, float a) {
        if (a < 0.01f) return;
        ImGuiWindow* w = ImGui::GetCurrentWindow();
        if (w->SkipItems) return;

        std::string ids = std::string("cp_") + name;
        ImGuiID id = w->GetID(ids.c_str());

        float ww = content_w > 0 ? content_w : ImGui::GetContentRegionAvail().x;
        float h = 36.f * S;
        float pad = 12.f * S;

        ImVec2 pos = w->DC.CursorPos;
        ImRect r(pos, ImVec2(pos.x + ww, pos.y + h));

        ImGui::ItemSize(r);
        if (!ImGui::ItemAdd(r, id)) return;

        bool hov = ImGui::IsMouseHoveringRect(r.Min, r.Max);
        bool other = !active_popup.empty() && active_popup != ids;
        bool press = hov && ImGui::IsMouseClicked(0) && !other && !clicked;

        bool open = (active_popup == ids);

        if (press) {
            if (open) { active_popup = ""; popup_open = false; }
            else {
                active_popup = ids;
                popup_open = true;
                cp_id = ids;
                cp_ptr = v;
                if (hsv_cache.find(ids) == hsv_cache.end()) {
                    float hh, ss, vv;
                    ImGui::ColorConvertRGBtoHSV(v->x, v->y, v->z, hh, ss, vv);
                    hsv_cache[ids] = ImVec4(hh, ss, vv, 1.f);
                }
            }
            clicked = true;
        }

        float oa = anim(ids + "_o", open ? 1.f : 0.f, 18.f);
        bool act = open || (hov && ImGui::IsMouseDown(0) && !popup());
        float ha = anim(ids + "_h", act ? 1.f : 0.f, act ? 20.f : 12.f);

        ImDrawList* dl = w->DrawList;

        if (ha > 0.01f) dl->AddRectFilled(r.Min, r.Max, col(clr::border, a * ha * 0.5f), 8.f * S);

        ImGui::PushFont(fontMedium);
        float ty = r.GetCenter().y - fontMedium->FontSize * 0.5f;
        dl->AddText(ImVec2(r.Min.x + pad, ty), col(clr::text, a), name);
        ImGui::PopFont();

        float bw = 32.f * S;
        float bh = 16.f * S;
        float bx = r.Max.x - bw - pad;
        float by = r.GetCenter().y - bh * 0.5f;

        dl->AddRectFilled(ImVec2(bx + 2, by + 2), ImVec2(bx + bw - 2, by + bh - 2), IM_COL32((int)(v->x*255), (int)(v->y*255), (int)(v->z*255), (int)(255*a*content_alpha)), 6.f * S);
        ImVec4 bc = anim_col(ids + "_bc", open ? clr::accent : clr::border_light, 14.f);
        dl->AddRect(ImVec2(bx, by), ImVec2(bx + bw, by + bh), col(bc, a), 6.f * S, 0, 1.5f * S);

        if (oa > 0.01f && cp_ptr == v) {
            ImDrawList* fg = ImGui::GetForegroundDrawList();

            if (hsv_cache.find(ids) == hsv_cache.end()) {
                float hh, ss, vv;
                ImGui::ColorConvertRGBtoHSV(v->x, v->y, v->z, hh, ss, vv);
                hsv_cache[ids] = ImVec4(hh, ss, vv, 1.f);
            }
            ImVec4& hsv = hsv_cache[ids];

            float sv_sz = 110.f * S;
            float hue_w = 20.f * S;
            float p = 10.f * S;
            float gap = 8.f * S;
            float pw = p + sv_sz + gap + hue_w + p;
            float ph = p + sv_sz + p;

            ImVec2 pmin(r.Max.x - pw, r.Max.y + 6.f * S);
            ImVec2 pmax(pmin.x + pw, pmin.y + ph);

            fg->AddRectFilled(ImVec2(pmin.x - 5, pmin.y - 5), ImVec2(pmax.x + 5, pmax.y + 5), IM_COL32(0,0,0, (int)(100*oa)), 10.f * S);
            fg->AddRectFilled(pmin, pmax, col(clr::bg, oa), 10.f * S);
            fg->AddRect(pmin, pmax, col(clr::accent, oa * 0.5f), 10.f * S);

            if (oa > 0.1f) {
                ImVec2 sv_min(pmin.x + p, pmin.y + p);
                ImVec2 sv_max(sv_min.x + sv_sz, sv_min.y + sv_sz);
                ImVec2 hue_min(sv_max.x + gap, sv_min.y);
                ImVec2 hue_max(hue_min.x + hue_w, sv_max.y);

                ImVec4 hc;
                ImGui::ColorConvertHSVtoRGB(hsv.x, 1.f, 1.f, hc.x, hc.y, hc.z);
                ImU32 hu32 = IM_COL32((int)(hc.x * 255), (int)(hc.y * 255), (int)(hc.z * 255), (int)(255 * oa));

                fg->AddRectFilled(sv_min, sv_max, col(clr::widget, oa));
                fg->AddRectFilledMultiColor(sv_min, sv_max, IM_COL32(255, 255, 255, (int)(255 * oa)), hu32, hu32, IM_COL32(255, 255, 255, (int)(255 * oa)));
                fg->AddRectFilledMultiColor(sv_min, sv_max, IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, (int)(255 * oa)), IM_COL32(0, 0, 0, (int)(255 * oa)));
                fg->AddRect(sv_min, sv_max, col(clr::border_dark, oa), 0, 0, 1.f);
                
                fg->AddRectFilled(hue_min, hue_max, col(clr::widget, oa));
                float seg = (hue_max.y - hue_min.y) / 6.f;
                for (int i = 0; i < 6; i++) {
                    ImVec4 c1, c2;
                    ImGui::ColorConvertHSVtoRGB(i / 6.f, 1.f, 1.f, c1.x, c1.y, c1.z);
                    ImGui::ColorConvertHSVtoRGB((i + 1) / 6.f, 1.f, 1.f, c2.x, c2.y, c2.z);
                    fg->AddRectFilledMultiColor(
                        ImVec2(hue_min.x + 2, hue_min.y + seg * i),
                        ImVec2(hue_max.x - 2, hue_min.y + seg * (i + 1)),
                        IM_COL32((int)(c1.x * 255), (int)(c1.y * 255), (int)(c1.z * 255), (int)(255 * oa)),
                        IM_COL32((int)(c1.x * 255), (int)(c1.y * 255), (int)(c1.z * 255), (int)(255 * oa)),
                        IM_COL32((int)(c2.x * 255), (int)(c2.y * 255), (int)(c2.z * 255), (int)(255 * oa)),
                        IM_COL32((int)(c2.x * 255), (int)(c2.y * 255), (int)(c2.z * 255), (int)(255 * oa))
                    );
                }
                fg->AddRect(hue_min, hue_max, col(clr::border_dark, oa), 0, 0, 1.f);

                ImGuiIO& io = ImGui::GetIO();
                ImVec2 mp = io.MousePos;

                if (io.MouseDown[0]) {
                    if (picker.empty()) {
                        if (mp.x >= sv_min.x && mp.x <= sv_max.x && mp.y >= sv_min.y && mp.y <= sv_max.y) picker = ids + "_sv";
                        else if (mp.x >= hue_min.x && mp.x <= hue_max.x && mp.y >= hue_min.y && mp.y <= hue_max.y) picker = ids + "_hue";
                    }
                    if (picker == ids + "_sv") {
                        hsv.y = ImClamp((mp.x - sv_min.x) / (sv_max.x - sv_min.x), 0.f, 1.f);
                        hsv.z = 1.f - ImClamp((mp.y - sv_min.y) / (sv_max.y - sv_min.y), 0.f, 1.f);
                        ImGui::ColorConvertHSVtoRGB(hsv.x, hsv.y, hsv.z, v->x, v->y, v->z);
                        clicked = true;
                    } else if (picker == ids + "_hue") {
                        hsv.x = ImClamp((mp.y - hue_min.y) / (hue_max.y - hue_min.y), 0.f, 1.f);
                        ImGui::ColorConvertHSVtoRGB(hsv.x, hsv.y, hsv.z, v->x, v->y, v->z);
                        clicked = true;
                    }
                } else {
                    picker = "";
                }

                float cx = sv_min.x + hsv.y * (sv_max.x - sv_min.x);
                float cy = sv_min.y + (1.f - hsv.z) * (sv_max.y - sv_min.y);
                fg->AddCircleFilled(ImVec2(cx, cy), 6.f * S, IM_COL32(0,0,0, (int)(150*oa)));
                fg->AddCircleFilled(ImVec2(cx, cy), 5.f * S, IM_COL32(255,255,255, (int)(255*oa)));
                fg->AddCircle(ImVec2(cx, cy), 5.f * S, IM_COL32(0,0,0, (int)(255*oa)), 0, 1.5f);

                float hy = hue_min.y + hsv.x * (hue_max.y - hue_min.y);
                fg->AddTriangleFilled(ImVec2(hue_min.x - 5.f * S, hy - 4.f * S), ImVec2(hue_min.x - 5.f * S, hy + 4.f * S), ImVec2(hue_min.x + 1.f * S, hy), IM_COL32(255, 255, 255, (int)(255 * oa)));
                fg->AddTriangleFilled(ImVec2(hue_max.x + 5.f * S, hy - 4.f * S), ImVec2(hue_max.x + 5.f * S, hy + 4.f * S), ImVec2(hue_max.x - 1.f * S, hy), IM_COL32(255, 255, 255, (int)(255 * oa)));
            }
        }
    }

    void separator(float a) {
        if (a < 0.01f) return;
        ImGuiWindow* w = ImGui::GetCurrentWindow();
        ImVec2 pos = w->DC.CursorPos;
        float ww = content_w > 0 ? content_w : ImGui::GetContentRegionAvail().x;
        ImDrawList* dl = w->DrawList;
        float y = pos.y + 6.f * S;

        dl->AddRectFilledMultiColor(
            ImVec2(pos.x, y), ImVec2(pos.x + ww * 0.5f, y + 1.5f),
            col(clr::border, 0.f), col(clr::border, a * 0.6f),
            col(clr::border, a * 0.6f), col(clr::border, 0.f)
        );
        dl->AddRectFilledMultiColor(
            ImVec2(pos.x + ww * 0.5f, y), ImVec2(pos.x + ww, y + 1.5f),
            col(clr::border, a * 0.6f), col(clr::border, 0.f),
            col(clr::border, 0.f), col(clr::border, a * 0.6f)
        );
        ImGui::Dummy(ImVec2(0, 12.f * S));
    }

    // ==================== NORMAL BUTTON (No RGB, No Neon, No Flowers) ====================
    bool button(const char* name, float a, std::function<void()> on_click) {
        if (a < 0.01f) return false;
        ImGuiWindow* w = ImGui::GetCurrentWindow();
        if (w->SkipItems) return false;

        std::string ids = std::string("btn_") + name;
        ImGuiID id = w->GetID(ids.c_str());

        float ww = content_w > 0 ? content_w : ImGui::GetContentRegionAvail().x;
        float h = 44.f * S;

        ImVec2 pos = w->DC.CursorPos;
        ImRect r(pos, ImVec2(pos.x + ww, pos.y + h));

        ImGui::ItemSize(r);
        if (!ImGui::ItemAdd(r, id)) return false;

        bool hov = ImGui::IsMouseHoveringRect(r.Min, r.Max) && !popup();
        bool press = hov && ImGui::IsMouseClicked(0) && !clicked;
        if (press) {
            clicked = true;
            if (on_click) on_click();
        }

        float ha = anim(ids + "_h", hov ? 1.f : 0.f, 14.f);
        float pa = anim(ids + "_p", press ? 1.f : 0.f, 20.f);
        
        ImDrawList* dl = w->DrawList;

        // Background - normal flat grayscale
        ImU32 bg_color;
        if (press) {
            bg_color = IM_COL32(60, 60, 60, (int)(255 * a));
        } else if (hov) {
            bg_color = IM_COL32(100, 100, 100, (int)(255 * a));
        } else {
            bg_color = IM_COL32(70, 70, 70, (int)(255 * a));
        }

        dl->AddRectFilled(r.Min, r.Max, bg_color, 8.f * S);
        
        // Border
        ImU32 border_color = hov ? IM_COL32(180, 180, 180, (int)(200 * a)) : IM_COL32(90, 90, 90, (int)(200 * a));
        dl->AddRect(r.Min, r.Max, border_color, 8.f * S, 0, 1.5f * S);

        // Text
        ImGui::PushFont(fontBold);
        ImVec2 text_size = ImGui::CalcTextSize(name);
        float tx = r.Min.x + (ww - text_size.x) * 0.5f;
        float ty = r.GetCenter().y - text_size.y * 0.5f;
        
        ImU32 text_color = (press || hov) ? IM_COL32(255, 255, 255, (int)(255 * a)) : IM_COL32(200, 200, 200, (int)(255 * a));
        dl->AddText(ImVec2(tx, ty), text_color, name);
        ImGui::PopFont();

        return press;
    }

    // ==================== NORMAL TOGGLE BUTTON (Black & White, no RGB) ====================
    bool toggle_button(const char* name, bool* v, float a) {
        if (a < 0.01f) return false;
        ImGuiWindow* w = ImGui::GetCurrentWindow();
        if (w->SkipItems) return false;

        std::string ids = std::string("tog_") + name;
        ImGuiID id = w->GetID(ids.c_str());

        float ww = content_w > 0 ? content_w : ImGui::GetContentRegionAvail().x;
        float h = 36.f * S;
        float pad = 12.f * S;

        ImVec2 pos = w->DC.CursorPos;
        ImRect r(pos, ImVec2(pos.x + ww, pos.y + h));

        ImGui::ItemSize(r);
        if (!ImGui::ItemAdd(r, id)) return false;

        bool hov = ImGui::IsMouseHoveringRect(r.Min, r.Max) && !popup();

        // Switch area calculation
        float switch_w = 44.f * S;
        float switch_h = 22.f * S;
        float switch_x = r.Max.x - switch_w - pad;
        float switch_y = r.GetCenter().y - switch_h * 0.5f;
        ImVec2 sw_min(switch_x, switch_y);
        ImVec2 sw_max(switch_x + switch_w, switch_y + switch_h);

        bool sw_hov = ImGui::IsMouseHoveringRect(sw_min, sw_max) && !popup();

        // Only toggle if the switch area specifically is clicked
        bool press = sw_hov && ImGui::IsMouseClicked(0) && !clicked;
        if (press) {
            *v = !*v;
            clicked = true;
        }

        float ha = anim(ids + "_h", hov ? 1.f : 0.f, 14.f);
        
        ImDrawList* dl = w->DrawList;

        // Background panel
        dl->AddRectFilled(r.Min, r.Max, col(clr::panel, a), 6.f * S);
        
        if (ha > 0.01f) {
            dl->AddRect(r.Min, r.Max, col(clr::border_light, a * ha), 6.f * S);
        }

        // Label
        ImGui::PushFont(fontMedium);
        float ty = r.GetCenter().y - fontMedium->FontSize * 0.5f;
        dl->AddText(ImVec2(r.Min.x + pad, ty), col(clr::text, a), name);
        ImGui::PopFont();

        // === PLAIN B&W TOGGLE SWITCH ===
        // Track: white when ON, dark gray when OFF
        ImU32 track_color = *v ? IM_COL32(220, 220, 220, (int)(220 * a)) : IM_COL32(50, 50, 50, (int)(220 * a));
        dl->AddRectFilled(sw_min, sw_max, track_color, switch_h * 0.5f);
        
        // Track border
        dl->AddRect(sw_min, sw_max, IM_COL32(100, 100, 100, (int)(200 * a)), switch_h * 0.5f, 0, 1.5f * S);

        // Thumb: black when ON, white when OFF
        float thumb_r = switch_h * 0.4f;
        float thumb_offset = *v ? (switch_w - thumb_r * 2.5f) : thumb_r * 0.5f;
        ImVec2 thumb_pos(sw_min.x + thumb_offset, sw_min.y + switch_h * 0.5f);
        
        ImU32 thumb_color = *v ? IM_COL32(40, 40, 40, (int)(255 * a)) : IM_COL32(240, 240, 240, (int)(255 * a));
        dl->AddCircleFilled(thumb_pos, thumb_r, thumb_color, 32);
        dl->AddCircle(thumb_pos, thumb_r, IM_COL32(80, 80, 80, (int)(200 * a)), 32, 1.2f);

        return press;
    }
}
}