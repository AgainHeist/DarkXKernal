#pragma once

namespace touch {
    bool init(int32_t _screen_w, int32_t _screen_h, uint8_t _orientation);
    void update(int32_t _screen_w, int32_t _screen_h, uint8_t _orientation);
    void updateOrientation(uint8_t _orientation);
    void shutdown();

    // 🔥 ADDED: Touch Simulation for Zero-Write Aimbot
    void move(float x, float y);
    void down(float x, float y);
    void up();
}

