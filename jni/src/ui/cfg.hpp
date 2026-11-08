#pragma once
#include "imgui.h"

namespace cfg {

namespace esp {
    inline bool enable = false; 
    inline bool box = false;
    inline bool name = false;
    inline bool health = false;
    inline bool health_text = false;
    inline bool distance = false;
    inline bool line = false;
    inline bool weapon = false;
    inline bool skeleton = false;
    inline bool head_dot = false;
    inline bool count = false;
    inline bool alert_360 = false;
    inline bool show_timer = false;

    // Advanced Styles
    inline int box_style = 1;
    inline float box_thickness = 2.3f;
    inline float box_corner_round = 4.0f;
    inline float box_glow_intensity = 0.7f;
    inline float box_fill_alpha = 0.5f;
    inline int box_color_mode = 0;

    inline int line_style = 0;
    inline float line_thickness = 2.3f;
    inline float line_glow_intensity = 0.8f;
    inline int line_color_mode = 0;

    inline int health_style = 0;
    inline float health_width = 6.0f;
    inline float health_rounding = 2.0f;
    inline bool health_gradient = true;

    inline float head_dot_radius = 5.0f;
    inline int color_index = 1; 

    inline ImVec4 box_color = ImVec4(0.0f, 1.0f, 1.0f, 1.0f);
    inline ImVec4 line_color = ImVec4(0.0f, 1.0f, 1.0f, 1.0f);
    inline ImVec4 skeleton_color = ImVec4(0.0f, 1.0f, 1.0f, 1.0f);

    inline ImVec4 name_color = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
    inline float name_size = 24.0f;
    inline ImVec4 hp_text_color = ImVec4(0.0f, 1.0f, 1.0f, 1.0f);
    inline float hp_text_size = 25.0f;
    inline ImVec4 distance_color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    inline float distance_size = 22.0f;
}

namespace aim {
    inline bool aimflick = false;         // <-- ADDED
    inline bool silent = false;
    inline bool ignore_knock = true;      // <-- ADDED
    inline bool visibility_check = true;  // <-- CHANGED TO TRUE BY DEFAULT
    inline bool show_fov = false;
    inline bool show_aimline = false;
    inline float silent_headshot_rate = 45.0f; // Default 45% Head, 55% Chest (SAFETIER)

    inline int headshot_rate = 50;
    inline float fov = 60.0f;
    inline float range = 100.0f;
    inline ImVec4 fov_color = ImVec4(0.0f, 1.0f, 1.0f, 1.0f);
    inline int color_index = 1;
}

namespace other {
    inline bool activate_menu = false; // 🔥 GLOBAL MASTER SWITCH 🔥
    inline bool stream_mode = false;
}

namespace mods {
    inline bool speed_hack = false;
    inline bool show_speed_overlay = false;
    inline float speed_multiplier = 3.0f;
}

}