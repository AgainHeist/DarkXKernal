#include "Android_draw/draw.h"
#include "ui/theme/theme.hpp"
#include "ui/menu.hpp"
#include "ui/bar.hpp"
#include "other/memory.hpp"
#include "protect/oxorany.h"
#include "protect/oxorany_include.h"
#include "game/data.hpp"
#include "external/json.hpp"  
#include "Login/Jni_Stuff.h" 
#include "integrity.hpp"
#include "protect/SymbolHider.hpp"
#include "protect/AdvancedProtect.hpp"
#include "Tools.h"
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <thread>
#include <fstream> 
#include <iostream>
#include <string>
#include <cstring>

using json = nlohmann::json;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-security"

// --- Modern 24-bit RGB Color Palette & UI Macros ---
#define COLOR_PRIMARY   "\033[38;2;138;43;226m"  // Electric Violet
#define COLOR_ACCENT    "\033[38;2;0;255;204m"    // Cyan Neon
#define COLOR_SUCCESS   "\033[38;2;46;204;113m"   // Mint Green
#define COLOR_ERROR     "\033[38;2;231;76;60m"    // Soft Red
#define COLOR_WARN      "\033[38;2;241;196;15m"   // Amber Yellow
#define COLOR_MUTED     "\033[38;2;127;140;141m"  // Slate Gray
#define COLOR_RESET     "\033[0m"
#define BOLD            "\033[1m"

// Status Badges & Icons
#define ICON_INFO    COLOR_ACCENT  "ℹ " COLOR_RESET
#define ICON_SUCCESS COLOR_SUCCESS "✓ " COLOR_RESET
#define ICON_ERROR   COLOR_ERROR   "✗ " COLOR_RESET
#define ICON_WARN    COLOR_WARN    "⚠ " COLOR_RESET

std::string EXP = "";
std::string g_UserKey = "";
std::string g_Seller = "";
bool bValid = false;

char g_auth_key[128] = "Not Logged In";
char g_auth_expiry[128] = "Checking...";
char g_auth_seller[128] = "Official"; 
bool g_UpdateRequired = false;
bool g_KeyExpired = false;
time_t g_expiry_timestamp = 0;

// Dummy variables to satisfy Jni_Stuff.h references
std::string g_encrypted_offsets_json;
time_t g_server_timestamp = 0;

// --- LOADER HANDSHAKE LOGIC ---
bool perform_handshake() {
    // 1. Tell loader we are ready
    printf("ENGINE_READY\n");
    fflush(stdout);

    char challenge[256];
    if (fgets(challenge, sizeof(challenge), stdin)) {
        std::string line(challenge);
        if (line.find("INIT_CHALLENGE:") != std::string::npos) {
            std::string nonce = line.substr(15);
            nonce.erase(nonce.find_last_not_of(" \n\r\t") + 1);

            // Salt matches Loader's salt: OG_PROJECT_SECRET_SALT_777
            std::string salt = oxorany("OG_PROJECT_SECRET_SALT_777");
            std::string response = Tools::CalcMD5(nonce + salt);

            printf("SESSION_VERIFIED:%s\n", response.c_str());
            fflush(stdout);
            return true;
        }
    }
    return false;
}

void print_simple_banner() {
    printf((const char*)oxorany("\033[2J\033[H")); // Clear screen using ANSI escape code instead of system("clear")
    printf((const char*)oxorany("\n"));
    printf((const char*)oxorany("%s"), (const char*)oxorany(COLOR_PRIMARY));
    printf((const char*)oxorany("  ╭─────────────────────────────────────────────────────────╮\n"));
    printf((const char*)oxorany("  │                                                         │\n"));
    printf((const char*)oxorany("  │    " BOLD COLOR_ACCENT "D A R K   x   S T R E A M E R" COLOR_RESET COLOR_PRIMARY "                  │\n"));
    printf((const char*)oxorany("  │    " COLOR_MUTED "System Authentication & Loader" COLOR_PRIMARY "               │\n"));
    printf((const char*)oxorany("  │                                                         │\n"));
    printf((const char*)oxorany("  ╰─────────────────────────────────────────────────────────╯\n"));
    printf((const char*)oxorany("%s\n"), (const char*)oxorany(COLOR_RESET));
    fflush(stdout);
}

static void print_status(const char* status, const char* color = COLOR_ACCENT) {
    printf((const char*)oxorany("  %s[●]%s %s%s%s\n"), (const char*)oxorany(COLOR_MUTED), (const char*)oxorany(COLOR_RESET), color, status, (const char*)oxorany(COLOR_RESET));
    fflush(stdout);
}

static std::string get_hidden_path(const char* seed) {
    char serial[128] = {0};
    Hidden::SystemPropertyGet(oxorany("ro.serialno"), serial);
    std::string s = serial;
    if (s.empty()) s = oxorany("global_fallback");

    unsigned int hash = 0x811C9DC5;
    for (char c : s + seed) {
        hash ^= (unsigned char)c;
        hash *= 0x01000193;
    }

    char buf[64];
    snprintf(buf, sizeof(buf), oxorany("/data/local/tmp/.system_%08X_data"), hash);
    return std::string(buf);
}

static std::string get_hwid() {
    std::string hwid_path = get_hidden_path(oxorany("hwid_v3"));
    int fd = Safe::openat(-100, hwid_path.c_str(), O_RDONLY, 0);
    if (fd >= 0) {
        char buf[128] = {0};
        ssize_t n = Safe::read(fd, buf, sizeof(buf) - 1);
        Safe::close(fd);
        if (n > 0) {
            std::string hwid(buf);
            hwid.erase(hwid.find_last_not_of(" \n\r\t") + 1);
            if (!hwid.empty()) return hwid;
        }
    }

    char buffer[128] = {0};
    // Note: Using __system_property_get is safer than popen for android_id
    Hidden::SystemPropertyGet(oxorany("ro.serialno"), buffer);
    std::string hwid = buffer;
    if (hwid.empty() || hwid == "null") {
        Hidden::SystemPropertyGet(oxorany("ro.build.display.id"), buffer);
        hwid = buffer;
    }
    
    if (hwid.empty()) hwid = oxorany("unknown_device");
    
    unsigned long hash = 5381;
    for (char c : hwid) hash = ((hash << 5) + hash) + (unsigned char)c;
    char final_hwid[32]; snprintf(final_hwid, sizeof(final_hwid), oxorany("%08lX"), hash);
    
    int wfd = Safe::openat(-100, hwid_path.c_str(), O_WRONLY | O_CREAT, 0666);
    if (wfd >= 0) {
        char hbuf[64];
        int len = snprintf(hbuf, sizeof(hbuf), "%s", final_hwid);
        Safe::syscall6(__NR_write, wfd, (long)hbuf, len, 0, 0, 0);
        Safe::close(wfd);
    }
    
    return std::string(final_hwid);
}

bool perform_login(const std::string& key) {
    if (check_vpn() == 1) {
        print_status(oxorany("Please turn off active VPN connection."), COLOR_ERROR);
        return false;
    }

    print_status(oxorany("Authenticating session with server..."), COLOR_WARN);
    
    try {
        std::string err = Login(key.c_str());
        
        if (err == oxorany("OK") && bValid) {
            // Modern Account Details Box
            printf((const char*)oxorany("\n"));
            printf((const char*)oxorany("  %s╭── " BOLD "ACCOUNT SESSION DETAILS" COLOR_RESET " ──────────────────╮\n"), (const char*)oxorany(COLOR_SUCCESS));
            printf((const char*)oxorany("  %s│%s Status   : %sACTIVE%s\n"), (const char*)oxorany(COLOR_SUCCESS), (const char*)oxorany(COLOR_RESET), (const char*)oxorany(COLOR_SUCCESS), (const char*)oxorany(COLOR_RESET));
            printf((const char*)oxorany("  %s│%s License  : %s%s%s\n"), (const char*)oxorany(COLOR_SUCCESS), (const char*)oxorany(COLOR_RESET), (const char*)oxorany(COLOR_ACCENT), key.c_str(), (const char*)oxorany(COLOR_RESET));
            
            strncpy(g_auth_key, key.c_str(), sizeof(g_auth_key));
            if (!EXP.empty() && EXP != oxorany(".....")) {
                strncpy(g_auth_expiry, EXP.c_str(), sizeof(g_auth_expiry));
                printf((const char*)oxorany("  %s│%s Expires  : %s%s%s\n"), (const char*)oxorany(COLOR_SUCCESS), (const char*)oxorany(COLOR_RESET), (const char*)oxorany(COLOR_MUTED), EXP.c_str(), (const char*)oxorany(COLOR_RESET));
                printf("LOADER_DATA_EXP:%s\n", EXP.c_str());
            } else {
                strncpy(g_auth_expiry, oxorany("Lifetime"), sizeof(g_auth_expiry));
                printf((const char*)oxorany("  %s│%s Expires  : %sLifetime%s\n"), (const char*)oxorany(COLOR_SUCCESS), (const char*)oxorany(COLOR_RESET), (const char*)oxorany(COLOR_MUTED), (const char*)oxorany(COLOR_RESET));
                printf("LOADER_DATA_EXP:Lifetime\n");
            }
            
            if (!g_Seller.empty()) {
                strncpy(g_auth_seller, g_Seller.c_str(), sizeof(g_auth_seller));
                printf((const char*)oxorany("  %s│%s seller : %s%s%s\n"), (const char*)oxorany(COLOR_SUCCESS), (const char*)oxorany(COLOR_RESET), (const char*)oxorany(COLOR_MUTED), g_Seller.c_str(), (const char*)oxorany(COLOR_RESET));
                printf("LOADER_DATA_SEL:%s\n", g_Seller.c_str());
            }
            printf((const char*)oxorany("  %s╰──────────────────────────────────────────────────╯%s\n\n"), (const char*)oxorany(COLOR_SUCCESS), (const char*)oxorany(COLOR_RESET));

            printf("ACTIVE\n");
            fflush(stdout);

            print_status(oxorany("Offsets loaded from internal source."), COLOR_SUCCESS);
            return true;
        } else {
            printf((const char*)oxorany("\n  " ICON_ERROR "%sAuthentication Failed:%s %s\n\n"), (const char*)oxorany(COLOR_ERROR), (const char*)oxorany(COLOR_RESET), err.c_str());
            return false;
        }
    } catch (...) {
        print_status(oxorany("Network or internal error during login!"), COLOR_ERROR);
        return false;
    }
}

#include <sys/prctl.h>

// ============================================================
// EXIT BEHAVIOUR CONFIG
//   1 = ON, 0 = OFF
// ============================================================
#define AUTO_EXIT_ON_GAME_CLOSE   0   // game band hone par app band ho jaye
#define AUTO_EXIT_IF_NO_GAME      0   // game start na ho to timeout par band ho jaye
#define NO_GAME_TIMEOUT_TICKS     1000 // ~30 sec (sirf AUTO_EXIT_IF_NO_GAME = 1 par asar)

int main(int argc, char* argv[]) {
    // 🔥 STEALTH 1: Change process name in RAM to look like a Kernel Thread
    // Isse anti-cheat ko binary ka sahi naam nahi dikhega
    prctl(PR_SET_NAME, oxorany("[kworker/u16:1]"), 0, 0, 0);

    // 🔥 GHOST MODE: Delete ourselves from disk immediately after launch
    if (argv[0]) {
        unlink(argv[0]);
    }

    bool auth_only = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--auth-only") == 0) {
            auth_only = true;
            break;
        }
    }

    // 🔥 SMART HANDSHAKE 🔥
    if (argc >= 4) {
        // Background Service Mode: Validate via direct arguments (Fast & No-Blocking)
        std::string key = argv[1];
        std::string nonce = argv[2];
        std::string provided_hash = argv[3];
        std::string salt = oxorany("OG_PROJECT_SECRET_SALT_777");

        if (provided_hash == Tools::CalcMD5(nonce + salt)) {
            perform_login(key);
            if (auth_only) return 0;
        } else {
            return -1;
        }
    } else {
        // Interactive Handshake Mode (For initial Login Page check)
        if (perform_handshake()) {
            char key_buf[256];
            if (fgets(key_buf, sizeof(key_buf), stdin)) {
                std::string key(key_buf);
                key.erase(key.find_last_not_of(" \n\r\t") + 1);
                perform_login(key);
            }
            if (auth_only) return 0;
        } else {
            return -1;
        }
    }

    AdvancedGuard::InitIntegrityCheck();
    AdvancedGuard::StartKhatarnakMonitor(); // Start the advanced background thread
    std::thread(AdvancedGuard::BackgroundTamperMonitor).detach();

    print_simple_banner();

    std::string hwid = get_hwid();
    std::string hwid_display = oxorany("Hardware ID : ") + hwid;
    print_status(hwid_display.c_str(), COLOR_ACCENT);
    printf("\n");

    std::string key;
    std::string key_path = get_hidden_path(oxorany("auth_v3_cache"));
    bool login_success = false;
    
    // Check command line argument
    if (argc >= 2) {
        key = argv[1];
        g_UserKey = key;
        
        print_status(oxorany("Key detected from arguments, initiating login..."), COLOR_PRIMARY);
        
        login_success = perform_login(key);
        
        if (login_success) {
            std::ofstream outFile(key_path);
            if(outFile.is_open()) {
                outFile << key;
                outFile.close();
            }
        }
    }
    
    // Check saved key if no command line or login failed
    if (!login_success) {
        int fd = Safe::openat(-100, key_path.c_str(), O_RDONLY, 0);
        if (fd >= 0) {
            char buf[256] = {0};
            ssize_t n = Safe::read(fd, buf, sizeof(buf) - 1);
            Safe::close(fd);
            if (n > 0) {
                key = buf;
                key.erase(key.find_last_not_of(" \n\r\t") + 1);
                if (!key.empty()) {
                    g_UserKey = key;
                    print_status(oxorany("Key loaded from local storage, authenticating..."), COLOR_PRIMARY);
                    login_success = perform_login(key);
                    if (!login_success) {
                        Safe::syscall6(__NR_unlinkat, -100, (long)key_path.c_str(), 0, 0, 0, 0);
                        print_status(oxorany("Stored key is invalid or expired. Please enter a new key."), COLOR_WARN);
                        Hidden::Sleep(2);
                    }
                }
            }
        }
    }
    
    // Prompt loop for valid key
    while (!login_success) {
        print_simple_banner();
        
        std::string hwid_display2 = oxorany("Hardware ID : ") + hwid;
        print_status(hwid_display2.c_str(), COLOR_ACCENT);
        printf((const char*)oxorany("\n"));
        
        printf((const char*)oxorany("  %s[>] Enter License Key: %s"), (const char*)oxorany(COLOR_ACCENT), (const char*)oxorany(COLOR_RESET));
        fflush(stdout);
        std::cin >> key;
        
        if (key.empty()) {
            print_status(oxorany("Error: Empty key string entered. Please try again."), COLOR_ERROR);
            Hidden::Sleep(2);
            continue;
        }
        
        g_UserKey = key;
        printf("%s", (const char*)oxorany("\n"));
        
        login_success = perform_login(key);
        
        if (login_success) {
            std::ofstream outFile(key_path);
            if(outFile.is_open()) {
                outFile << key;
                outFile.close();
            }
        } else {
            print_status(oxorany("Invalid key or unregistered user!"), COLOR_ERROR);
            print_status(oxorany("Retrying in 3 seconds..."), COLOR_WARN);
            Hidden::Sleep(3);
        }
    }

    if (!bValid) {
        print_status(oxorany("Session validation failed! Exiting..."), COLOR_ERROR);
        return 1;
    }

    printf("\n");
    print_status(oxorany("INITIALIZING ENGINE & OVERLAY..."), COLOR_ACCENT);
    Hidden::Sleep(2);

    screen_config();

    int max_size = (displayInfo.height > displayInfo.width ? displayInfo.height : displayInfo.width);
    int min_size = (displayInfo.height < displayInfo.width ? displayInfo.height : displayInfo.width);

    g_sw = static_cast<float>(max_size);
    g_sh = static_cast<float>(min_size);

    // 🔥 OPTIMIZED: Use actual screen dimensions instead of a square to prevent "rotation glitch"
    native_window_screen_x = max_size;
    native_window_screen_y = min_size;

    if (!initGUI_draw(native_window_screen_x, native_window_screen_y, true)) return -1;

    touch::init(max_size, min_size, (uint8_t)displayInfo.orientation);
    game::init();

    // 🔥 FIXED: Run memory update in a separate thread to prevent UI & Game freeze
    std::thread([]() {
        while (true) {
            if (game::valid() && proc::lib != 0) {
                data::update();
                game::check_lib(data::players.empty() ? 0 : 1);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5)); // ~200Hz update rate
        }
    }).detach();

    // Initialize Driver if possible
    driver::c_driver();
    if (driver::fd > 0) {
        proc::use_driver = true;
        driver::initialize(proc::pid);
        print_status(oxorany("Kernel Driver connected successfully."), COLOR_SUCCESS);
    } else {
        proc::use_driver = false;
        print_status(oxorany("CRITICAL ERROR: Kernel Driver not found! Project will not function."), COLOR_ERROR);
        // We don't exit here to allow UI to show, but memory functions will return false.
    }

    static float alpha = 0.f;
    static bool prev = false;
    static int game_not_found_ticks = 0;

    while (true) {
        drawBegin();
        bool run = game::valid();

        if (run) {
            game_not_found_ticks = 0;
            prev = true;
        } else {
            game_not_found_ticks++;

            bool should_exit = false;

#if AUTO_EXIT_IF_NO_GAME
            // Game kabhi start hi nahi hua -> timeout ke baad band
            if (!prev && game_not_found_ticks > NO_GAME_TIMEOUT_TICKS) should_exit = true;
#endif
#if AUTO_EXIT_ON_GAME_CLOSE
            // Game chal raha tha aur band ho gaya -> turant band
            if (prev && game_not_found_ticks > 2) should_exit = true;
#endif

            if (should_exit) {
                shutdown();
                exit(0);
            }
        }

#if defined(__x86_64__)
        bool is_landscape = (displayInfo.orientation == 0 || displayInfo.orientation == 2);
#else
        bool is_landscape = (displayInfo.orientation == 1 || displayInfo.orientation == 3);
#endif

        if (run && !prev) {
            print_status(oxorany("Target game process detected."), COLOR_SUCCESS);
            prev = true;
        } else if (!run && prev) {
            print_status(oxorany("Target game process closed."), COLOR_WARN);
            prev = false;
        }

        if (is_landscape) {
            ImGuiIO& io = ImGui::GetIO();
            float dt = io.DeltaTime;
            if (dt <= 0.f || dt > 0.1f) dt = 0.016f;

            // 🔥 ALWAYS SHOW MENU (For testing & Reliability)
            // Even if game is not running, the menu will stay at 0.6 alpha
            // When game starts, it will become 1.0 (Solid)
            float target = run ? 1.0f : 0.6f;
            float spd = 4.f;

            if (alpha < target) {
                alpha += dt * spd;
                if (alpha > target) alpha = target;
            } else if (alpha > target) {
                alpha -= dt * spd;
                if (alpha < target) alpha = target;
            }

            ui::bar::set_game_alpha(alpha);
            ui::bar::render();
            ui::menu::render();
            ui::menu::render_speed_overlay();

            if (run && proc::lib != 0) {
                data::RenderESP(ImGui::GetBackgroundDrawList(), g_sw, g_sh);
            }
        }

        bool vis = ui::bar::g_open;
        drawEnd();

        // 🔥 FIXED: Increased sleep time to prevent background freeze and CPU hogging
        // 16000us = ~60 FPS (Smooth and Battery Friendly)
        Hidden::USleep(vis ? 16000 : 33000);
    }

    shutdown();
    return 0;
}

#pragma clang diagnostic pop
