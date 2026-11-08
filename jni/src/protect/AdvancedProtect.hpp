#pragma once

#include <sys/ptrace.h>
#include <sys/prctl.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "../other/SafeSyscall.h"
#include "oxorany.h"

namespace AdvancedGuard {

    // --- Khatarnak Macros ---

    // Opaque Predicate: Always true but looks complex to AI/Decompilers
    #define OPAQUE_TRUE (((uintptr_t)__builtin_return_address(0) ^ 0x55) != 0xAA)

    // Hard Crash: Null dereference + Trap + Exit
    #define SILENT_DEATH() { \
        volatile int* p = nullptr; \
        *p = 0x1337; \
        __builtin_trap(); \
        _exit(1); \
    }

    // --- Advanced Detection Functions ---

    inline bool AntiDebugPtrace() {
        // Direct syscall ptrace(PTRACE_TRACEME, 0, 1, 0)
        // If a debugger is already attached, this will return -1
        long res = Safe::syscall6(__NR_ptrace, PTRACE_TRACEME, 0, 1, 0, 0, 0);
        if (res < 0) return true;

        // Detach after test if needed, but TRACEME stays until exit or exec
        return false;
    }

    inline bool AntiDump() {
        // Prevent memory dumping (Anti-GameGuardian / Anti-Dumper)
        return Safe::syscall6(__NR_prctl, PR_SET_DUMPABLE, 0, 0, 0, 0, 0) < 0;
    }

    inline bool DetectFridaThreads() {
        DIR* dir = opendir(oxorany("/proc/self/task"));
        if (!dir) return false;

        struct dirent* entry;
        bool found = false;
        while ((entry = readdir(dir)) != nullptr) {
            if (entry->d_name[0] == '.') continue;

            char path[256];
            snprintf(path, sizeof(path), oxorany("/proc/self/task/%s/comm"), entry->d_name);

            int fd = Safe::openat(-100, path, O_RDONLY, 0);
            if (fd >= 0) {
                char comm[64] = {0};
                Safe::read(fd, comm, sizeof(comm) - 1);
                Safe::close(fd);

                // Common Frida thread names
                if (strstr(comm, oxorany("gmain")) ||
                    strstr(comm, oxorany("gdbus")) ||
                    strstr(comm, oxorany("gum-js-loop")) ||
                    strstr(comm, oxorany("pool-frida")) ||
                    strstr(comm, oxorany("frida-helper"))) {
                    found = true;
                    break;
                }
            }
        }
        closedir(dir);
        return found;
    }

    inline bool DetectFridaNamedPipes() {
        // Frida often uses named pipes for communication
        int fd = Safe::openat(-100, oxorany("/proc/self/maps"), O_RDONLY, 0);
        if (fd < 0) return false;

        char buf[1024];
        bool found = false;
        while (Safe::read(fd, buf, sizeof(buf) - 1) > 0) {
            if (strstr(buf, oxorany("linjector")) ||
                strstr(buf, oxorany("frida-agent-")) ||
                strstr(buf, oxorany(".so (deleted)"))) { // Often a sign of library hiders
                found = true;
                break;
            }
        }
        Safe::close(fd);
        return found;
    }

    inline void StartKhatarnakMonitor() {
        std::thread([]() {
            while (true) {
                // Temporarily disabled for debugging
                // if (AntiDebugPtrace()) SILENT_DEATH();
                if (DetectFridaThreads()) SILENT_DEATH();
                if (DetectFridaNamedPipes()) SILENT_DEATH();

                // Random sleep to avoid pattern detection
                usleep(5000000 + (rand() % 5000000));
            }
        }).detach();
    }
}
