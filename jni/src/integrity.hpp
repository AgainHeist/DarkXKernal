#pragma once
#include "protect/oxorany.h"
#include "protect/oxorany_include.h"
#include "protect/AdvancedProtect.hpp"
#include <string>
#include <vector>
#include <iostream>
#include <thread>
#include <chrono>
#include <cstdio>

namespace AdvancedGuard {

    template <unsigned int N>
    struct RollingXorStr {
        char data[N];
        static constexpr char KeySequence[] = {0x7A, 0x3F, 0x1B, 0x4C, 0x6E, 0x5D, 0x2A, 0x1F};

        constexpr RollingXorStr(const char* str) : data{0} {
            for (unsigned int i = 0; i < N - 1; ++i) {
                data[i] = str[i] ^ KeySequence[i % sizeof(KeySequence)];
            }
        }

        std::string decrypt() const {
            std::string res;
            res.reserve(N);
            for (unsigned int i = 0; i < N - 1; ++i) {
                res += (char)(data[i] ^ KeySequence[i % sizeof(KeySequence)]);
            }
            return res;
        }
    };

    #define DECRYPT_SECURE(str) (AdvancedGuard::RollingXorStr<sizeof(str)>(str).decrypt())

    inline std::string GetPanelURL() {
        return oxorany("https://ogmods.myvippanel.shop/bihari");
    }

    inline void FORCE_TAMPER_CRASH() { 
        volatile int* poison_ptr = nullptr; 
        *poison_ptr = 0xDEADBEEF; 
        __builtin_trap(); 
    }

    static uintptr_t g_base_addr = 0;
    static size_t g_code_size = 0;
    static uint32_t g_initial_hash = 0;

    inline uint32_t CalculateMemoryHash(uintptr_t start, size_t size) {
        uint32_t hash = 0x811C9DC5;
        const unsigned char* p = reinterpret_cast<const unsigned char*>(start);
        for (size_t i = 0; i < size; ++i) {
            hash ^= p[i];
            hash *= 0x01000193;
        }
        return hash;
    }

    inline bool InitIntegrityCheck() {
        int fd = Safe::openat(-100, oxorany("/proc/self/maps"), O_RDONLY, 0);
        if (fd < 0) return false;

        char buf[4096];
        ssize_t n = Safe::read(fd, buf, sizeof(buf) - 1);
        Safe::close(fd);

        if (n > 0) {
            buf[n] = '\0';
            char* line = strtok(buf, "\n");
            while (line) {
                // Find the first executable segment of our binary
                if (strstr(line, oxorany(" r-xp ")) && strstr(line, oxorany("android.system.svc"))) {
                    uintptr_t start, end;
                    if (sscanf(line, oxorany("%lx-%lx"), &start, &end) == 2) {
                        g_base_addr = start;
                        g_code_size = end - start;
                        g_initial_hash = CalculateMemoryHash(g_base_addr, g_code_size);
                        return true;
                    }
                }
                line = strtok(NULL, "\n");
            }
        }
        return false;
    }

    inline void BackgroundTamperMonitor() {
        // Anti-Dump immediately
        AdvancedGuard::AntiDump();

        // Give some time for initial loads
        std::this_thread::sleep_for(std::chrono::seconds(5));

        while (true) {
            // 1. Ptrace & Frida Checks (Khatarnak)
            // if (AdvancedGuard::AntiDebugPtrace()) SILENT_DEATH();
            if (AdvancedGuard::DetectFridaThreads() || AdvancedGuard::DetectFridaNamedPipes()) SILENT_DEATH();

            // 2. URL Integrity Check
            std::string liveUrl = GetPanelURL();
            if (liveUrl.length() != 37 || liveUrl[0] != 'h' || liveUrl.find(oxorany("ogmods")) == std::string::npos) {
                SILENT_DEATH();
            }

            // 3. Memory Integrity Check (Detect Runtime Patching/Hooks)
            if (g_base_addr != 0 && g_code_size != 0) {
                uint32_t current_hash = CalculateMemoryHash(g_base_addr, g_code_size);
                if (current_hash != g_initial_hash) {
                    SILENT_DEATH();
                }
            }

            // 4. Simple PTrace Check (Legacy)
            if (Tools::IsBeingDebugged()) {
                SILENT_DEATH();
            }

            std::this_thread::sleep_for(std::chrono::seconds(10));
        }
    }

    inline std::string GenerateServerSignature(const std::string& key, const std::string& hwid, const std::string& timestamp) {
        std::string secret = oxorany("TERI_MA_DI_FUDDI");
        std::string data = key + hwid + timestamp + secret;
        
        uint32_t hash = 0x811C9DC5;
        for (char c : data) {
            hash ^= static_cast<unsigned char>(c);
            hash *= 0x01000193;
        }
        char buf[32];
        snprintf(buf, sizeof(buf), "%08X", hash);
        return std::string(buf);
    }
}