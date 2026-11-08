#include <cstdio>
#include <cstring>
#include <thread>
#include <dirent.h>
#include <fcntl.h>
#include <linux/input.h>
#include <poll.h>
#include <cmath>
#include <unistd.h>
#include <vector>
#include "imgui.h"
#include "ui/bar.hpp"

#define TOUCH_MODE 2

#ifndef KEY_VOLUMEDOWN
#define KEY_VOLUMEDOWN 114
#endif

namespace touch {
    struct input {
        int fd;
        bool canBeUsed{false};
        bool isKeyboard{false};
        input_absinfo absX{}, absY{};
        float absXMultiplier{0}, absYMultiplier{0};
#if TOUCH_MODE == 2
        bool trackingIDPresent{false};
#endif

        input(const char *_path, int32_t _screen_w, int32_t _screen_h) {
            fd = open(_path, O_RDWR);
            if (fd == -1) return;
                
            bool isTouch = ioctl(fd, EVIOCGABS(ABS_MT_POSITION_X), &absX) != -1 &&
                           ioctl(fd, EVIOCGABS(ABS_MT_POSITION_Y), &absY) != -1 && 
                           absX.maximum > 0 && absY.maximum > 0;

            unsigned long bitmask[EV_MAX/8 + 1];
            memset(bitmask, 0, sizeof(bitmask));
            ioctl(fd, EVIOCGBIT(0, sizeof(bitmask)), bitmask);
            bool hasKeys = (bitmask[EV_KEY/8] & (1 << (EV_KEY % 8))) != 0;

            if (isTouch) {
                canBeUsed = true;
                isKeyboard = false;
#if TOUCH_MODE == 2
                input_absinfo absTrackingID{};
                trackingIDPresent = ioctl(fd, EVIOCGABS(ABS_MT_TRACKING_ID), &absTrackingID) != -1 && absTrackingID.minimum <= 0 && absTrackingID.maximum > 0;
#endif
                calculateMultipliers(_screen_w, _screen_h);
            } else if (hasKeys) {
                canBeUsed = true;
                isKeyboard = true;
            } else { ::close(fd); fd = -1; }
        }

        void close() { if (fd != -1) { ::close(fd); fd = -1; } }

        void calculateMultipliers(int32_t _screen_w, int32_t _screen_h) {
            if (isKeyboard) return;
            auto max = fmax((float) absX.maximum, (float) absY.maximum);
            auto min = fmin((float) absX.maximum, (float) absY.maximum);
            absXMultiplier = (float) _screen_w / (_screen_w > _screen_h ? max : min);
            absYMultiplier = (float) _screen_h / (_screen_w > _screen_h ? min : max);
        }
    };

    uint8_t orientation;
    ImGuiIO *imgui_io;
    std::vector<input> inputs;

    void input_thread(const input &_input) {
        sleep(1);
        if (!imgui_io) imgui_io = &ImGui::GetIO();
        input_event events[64]{0};
        bool isGrabbing = false;
        bool isDown = false;
        float x = 0.0f, y = 0.0f;

        pollfd pfd = {_input.fd, POLLIN, 0};

        while (_input.fd > 0 && imgui_io && imgui_io->BackendRendererUserData) {
            // 🔥 FIXED: Removed EVIOCGRAB to prevent background game freeze
            // This allows the game to receive touches even when the menu is open.
            // We now handle touch filtering in the UI layer.
            isGrabbing = false;

            int ret = poll(&pfd, 1, 60);
            if (ret <= 0) continue;

            auto event_readed_count = read(_input.fd, events, sizeof(events));
            if (event_readed_count <= 0) continue;
            event_readed_count /= (ssize_t) sizeof(input_event);

            for (long j = 0; j < event_readed_count; j++) {
                auto &event = events[j];
                
                if (event.type == EV_KEY) {
                    if (event.code == KEY_VOLUMEDOWN || event.code == 25) {
                        if (event.value == 1) imgui_io->AddKeyEvent(ImGuiKey_F12, true);
                        else if (event.value == 0) imgui_io->AddKeyEvent(ImGuiKey_F12, false);
                    }
                }
                
                if (event.type == EV_ABS && !_input.isKeyboard) {
#if TOUCH_MODE == 2
                    if (_input.trackingIDPresent && event.code == ABS_MT_TRACKING_ID) {
                        isDown = event.value != -1;
                        continue;
                    }
#endif
                    if (event.code == ABS_MT_POSITION_X) { x = (float) event.value; continue; }
                    if (event.code == ABS_MT_POSITION_Y) { y = (float) event.value; continue; }
                }
                if (event.code == SYN_REPORT && imgui_io && !_input.isKeyboard) {
#if TOUCH_MODE == 2
                    imgui_io->MouseDown[0] = _input.trackingIDPresent ? isDown : event_readed_count > 2;
#else
                    imgui_io->MouseDown[0] = isDown;
#endif
                    if (imgui_io->MouseDown[0]) {
                        switch (orientation) {
                            case 1: imgui_io->MousePos.x = y; imgui_io->MousePos.y = _input.absX.maximum - x; break;
                            case 2: imgui_io->MousePos.x = _input.absX.maximum - x; imgui_io->MousePos.y = _input.absY.maximum - y; break;
                            case 3: imgui_io->MousePos.x = _input.absY.maximum - y; imgui_io->MousePos.y = x; break;
                            default: imgui_io->MousePos.x = x; imgui_io->MousePos.y = y; break;
                        }
                        imgui_io->MousePos.x *= _input.absXMultiplier;
                        imgui_io->MousePos.y *= _input.absYMultiplier;
                    }
                }
            }
        }
        if (isGrabbing) ioctl(_input.fd, EVIOCGRAB, 0);
    }

    bool init(int32_t _screen_w, int32_t _screen_h, uint8_t _orientation) {
        orientation = _orientation;
        bool result = false;
        const char *path = "/dev/input/";
        auto dir = opendir(path);
        dirent *ptr;
        while ((ptr = readdir(dir)))
            if (strstr(ptr->d_name, "event")) {
                char event_path[128];
                sprintf(event_path, "%s%s", path, ptr->d_name);
                input _input(event_path, _screen_w, _screen_h);
                if (_input.canBeUsed) {
                    if (!_input.isKeyboard) result = true;
                    inputs.push_back(_input);
                    std::thread(input_thread, _input).detach();
                }
            }
        closedir(dir);
        return result;
    }

    void update(int32_t _screen_w, int32_t _screen_h, uint8_t _orientation) {
        orientation = _orientation;
        for (auto &_input: inputs) _input.calculateMultipliers(_screen_w, _screen_h);
    }

    void updateOrientation(uint8_t _orientation) { orientation = _orientation; }

    void shutdown() {
        for (auto &_input: inputs) _input.close();
        inputs.clear();
        imgui_io = nullptr;
    }

    // 🔥 Implementation of Zero-Write Touch Simulation
    static int touch_fd = -1;
    static int tracking_id = 100;

    void send_event(int fd, uint16_t type, uint16_t code, int32_t value) {
        struct input_event ev;
        memset(&ev, 0, sizeof(ev));
        ev.type = type;
        ev.code = code;
        ev.value = value;
        write(fd, &ev, sizeof(ev));
    }

    void down(float x, float y) {
        if (inputs.empty()) return;
        int fd = inputs[0].fd; // Use first touch device
        send_event(fd, EV_ABS, ABS_MT_TRACKING_ID, tracking_id++);
        send_event(fd, EV_ABS, ABS_MT_POSITION_X, (int)(x / inputs[0].absXMultiplier));
        send_event(fd, EV_ABS, ABS_MT_POSITION_Y, (int)(y / inputs[0].absYMultiplier));
        send_event(fd, EV_ABS, ABS_MT_TOUCH_MAJOR, 1);
        send_event(fd, EV_SYN, SYN_REPORT, 0);
    }

    void move(float x, float y) {
        if (inputs.empty()) return;
        int fd = inputs[0].fd;
        send_event(fd, EV_ABS, ABS_MT_POSITION_X, (int)(x / inputs[0].absXMultiplier));
        send_event(fd, EV_ABS, ABS_MT_POSITION_Y, (int)(y / inputs[0].absYMultiplier));
        send_event(fd, EV_SYN, SYN_REPORT, 0);
    }

    void up() {
        if (inputs.empty()) return;
        int fd = inputs[0].fd;
        send_event(fd, EV_ABS, ABS_MT_TRACKING_ID, -1);
        send_event(fd, EV_SYN, SYN_REPORT, 0);
    }
}
