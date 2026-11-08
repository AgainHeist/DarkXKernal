#pragma once
#include <vector>
#include <mutex>
#include "game/player.hpp"
#include "imgui.h"

namespace data {
    void update();
    void RenderESP(ImDrawList* draw, float sw, float sh); // NAYA: Exact Hook.h Drawing yaha se hogi
    
    extern std::vector<PlayerData> players;
    extern std::mutex players_mutex;
    extern int realPlayerCount;
    extern int botCount;
}
