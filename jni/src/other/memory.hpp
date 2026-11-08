#pragma once

#include <cstdint>
#include <cstring>
#include <sys/uio.h>
#include <fcntl.h>
#include <unistd.h>
#include <dlfcn.h>
#include "../protect/oxorany.h"
#include "../protect/oxorany_include.h"
#include "SafeSyscall.h"
#include "../includes/Memory/KernelTools.h"

namespace proc {
    inline int pid = -1;
    inline uint64_t lib = 0;
    inline int fd = -1; // 🔥 Stealth FD for /proc/[pid]/mem
    inline int pm = -1;
    inline bool use_driver = false;
}

namespace sys {
    #ifndef AT_FDCWD
    #define AT_FDCWD -100
    #endif

    // 🔥 Proxy Syscall Gadget (Advanced 0-Detection)
    static uintptr_t syscall_gadget = 0;

    static void find_gadget() {
        if (syscall_gadget != 0) return;
        // Search in libc.so for: svc #0; ret;
        // This makes anti-cheat think the syscall came from libc.so
        uintptr_t libc = (uintptr_t)dlopen(oxorany("libc.so"), RTLD_LAZY);
        if (!libc) return;
        // Use a generic syscall entry point in libc
        syscall_gadget = (uintptr_t)dlsym((void*)libc, oxorany("syscall"));
    }

    static inline long proxy_syscall(long n, long a1, long a2, long a3, long a4, long a5, long a6) {
        find_gadget();
        if (syscall_gadget) {
            // Call through libc's syscall function
            return ((long (*)(long, long, long, long, long, long, long))syscall_gadget)(n, a1, a2, a3, a4, a5, a6);
        }
        return Safe::syscall6(n, a1, a2, a3, a4, a5, a6);
    }

    static inline int openat(int fd, const char* p, int f) {
        return (int)proxy_syscall(__NR_openat, fd, (long)p, f, 0, 0, 0);
    }

    static inline int close(int fd) {
        return (int)proxy_syscall(__NR_close, fd, 0, 0, 0, 0, 0);
    }

    static inline long read(int fd, void* b, size_t c) {
        return proxy_syscall(__NR_read, fd, (long)b, c, 0, 0, 0);
    }

    static inline long getdents64(int fd, void* d, size_t c) {
        return proxy_syscall(__NR_getdents64, fd, (long)d, c, 0, 0, 0);
    }

    static inline long pread64(int fd, void* buf, size_t count, uint64_t offset) {
        #if defined(__aarch64__)
        return proxy_syscall(67, fd, (long)buf, count, (long)offset, 0, 0);
        #elif defined(__arm__)
        return proxy_syscall(180, fd, (long)buf, count, (long)(offset & 0xFFFFFFFF), (long)(offset >> 32), 0);
        #else
        return -1;
        #endif
    }

    static inline long vm_read(int pid, struct iovec* l, unsigned long lc, struct iovec* r, unsigned long rc) {
        #if defined(__aarch64__)
        return proxy_syscall(270, pid, (long)l, lc, (long)r, rc, 0);
        #elif defined(__arm__)
        return proxy_syscall(376, pid, (long)l, lc, (long)r, rc, 0);
        #else
        return -1;
        #endif
    }

    static inline long vm_write(int pid, struct iovec* l, unsigned long lc, struct iovec* r, unsigned long rc) {
        #if defined(__aarch64__)
        return proxy_syscall(271, pid, (long)l, lc, (long)r, rc, 0);
        #elif defined(__arm__)
        return proxy_syscall(377, pid, (long)l, lc, (long)r, rc, 0);
        #else
        return -1;
        #endif
    }
}


// Baki ka structure same rahega
struct linux_dirent64 {
    uint64_t d_ino;
    int64_t d_off;
    unsigned short d_reclen;
    unsigned char d_type;
    char d_name[];
};

inline bool addr_valid(uint64_t a) { return a >= 0x1000; }

inline bool mem_read(uint64_t a, void* b, size_t l) {
    if (proc::pid < 0 || a < 0x1000 || l == 0 || b == nullptr || driver::fd <= 0) return false;
    return driver::read(a, b, l);
}

inline bool mem_write(uint64_t a, const void* b, size_t l) {
    if (proc::pid < 0 || a == 0 || l == 0 || b == nullptr || driver::fd <= 0) return false;
    return driver::write(a, const_cast<void*>(b), l);
}

template<typename T> inline T rpm(uint64_t a) { T v{}; mem_read(a, &v, sizeof(T)); return v; }
template<typename T> inline bool wpm(uint64_t a, const T& d) { return mem_write(a, &d, sizeof(T)); }

inline int get_pid() {
    int pfd = sys::openat(AT_FDCWD, oxorany("/proc"), O_RDONLY | O_DIRECTORY);
    if (pfd < 0) return -1;
    char buf[4096];
    while (true) {
        long nr = sys::getdents64(pfd, buf, sizeof(buf));
        if (nr <= 0) break;
        char* p = buf;
        while (p < buf + nr) {
            auto* e = reinterpret_cast<linux_dirent64*>(p);
            if (e->d_type == 4) {
                char* n = e->d_name;
                bool in = true;
                for (char* c = n; *c; c++) if (*c < '0' || *c > '9') { in = false; break; }
                if (in && *n != '\0') {
                    int pid = 0; for (char* c = n; *c; c++) pid = pid * 10 + (*c - '0');
                    char cp[256]; int ps = 0;
                    const char* ps_ = oxorany("/proc/"); while (*ps_) cp[ps++] = *ps_++;
                    for (char* c = n; *c; c++) cp[ps++] = *c;
                    const char* cs = oxorany("/cmdline"); while (*cs) cp[ps++] = *cs++;
                    cp[ps] = '\0';
                    int cfd = sys::openat(AT_FDCWD, cp, O_RDONLY);
                    if (cfd >= 0) {
                        char cmd[256] = {0}; long b = sys::read(cfd, cmd, sizeof(cmd) - 1);
                        sys::close(cfd);
                        if (b > 0) {
                            cmd[b] = '\0';
                            const char* tg = oxorany("com.dts.freefiremax");
                            int tl = 0; while (tg[tl]) tl++;
                            if (b >= tl) {
                                for (int i = 0; i <= b - tl; i++) {
                                    bool fd = true;
                                    for (int j = 0; j < tl; j++) if (cmd[i + j] != tg[j]) { fd = false; break; }
                                    if (fd) { sys::close(pfd); return pid; }
                                }
                            }
                        }
                    }
                }
            }
            p += e->d_reclen;
        }
    }
    sys::close(pfd);
    return -1;
}

inline uint64_t get_lib() {
    if (proc::pid <= 0) return 0;
    char mp[64]; int ps = 0;
    const char* ps_ = oxorany("/proc/"); while (*ps_) mp[ps++] = *ps_++;
    int pid = proc::pid; char pds[16]; int pl = 0; int tm = pid;
    while (tm > 0) { pds[pl++] = '0' + (tm % 10); tm /= 10; }
    for (int i = pl - 1; i >= 0; i--) mp[ps++] = pds[i];
    const char* ms = oxorany("/maps"); while (*ms) mp[ps++] = *ms++; mp[ps] = '\0';
    int mfd = sys::openat(AT_FDCWD, mp, O_RDONLY); if (mfd < 0) return 0;
    char buf[8192]; long tr = 0; uint64_t bs = 0;
    while (true) {
        long nr = sys::read(mfd, buf + tr, sizeof(buf) - tr - 1);
        if (nr <= 0) break; tr += nr; buf[tr] = '\0'; char* ls = buf;
        while (true) {
            char* le = ls; while (*le && *le != '\n') le++;
            if (*le == '\0') break; *le = '\0';
            uint64_t st = 0, en = 0; char* pt = ls;
            while (*pt && ((*pt >= '0' && *pt <= '9') || (*pt >= 'a' && *pt <= 'f') || (*pt >= 'A' && *pt <= 'F'))) {
                uint64_t v = (*pt <= '9') ? (*pt - '0') : ((*pt >= 'a') ? (*pt - 'a' + 10) : (*pt - 'A' + 10));
                st = st * 16 + v; pt++;
            }
            if (*pt != '-') goto nl; pt++;
            while (*pt && ((*pt >= '0' && *pt <= '9') || (*pt >= 'a' && *pt <= 'f') || (*pt >= 'A' && *pt <= 'F'))) {
                uint64_t v = (*pt <= '9') ? (*pt - '0') : ((*pt >= 'a') ? (*pt - 'a' + 10) : (*pt - 'A' + 10));
                en = en * 16 + v; pt++;
            }
            while (*pt == ' ') pt++;
            if (pt[0] == 'r' && pt[3] == 'p') {
                while (*pt == ' ') pt++; while (*pt && *pt != '/') pt++;
                if (*pt == '/') {
                    const char* lu = oxorany("libil2cpp.so"); int ll = 0; while (lu[ll]) ll++;
                    char* ph = pt;
                    while (*ph) {
                        bool mt = true;
                        for (int i = 0; i < ll && ph[i]; i++) if (ph[i] != lu[i]) { mt = false; break; }
                        // 🔥 UPDATED: Increased size limit for new Play Store updates
                        if (mt && (en - st) < 0x8000000) { bs = st; sys::close(mfd); return bs; }
                        ph++;
                    }
                }
            }
        nl: ls = le + 1;
        }
        if (ls < buf + tr) { int rm = (buf + tr) - ls; for (int i = 0; i < rm; i++) buf[i] = ls[i]; tr = rm; } else tr = 0;
    }
    sys::close(mfd);
    return bs;
}

inline void init_mem() {
    if (proc::pid <= 0) return;

    if (proc::fd >= 0) sys::close(proc::fd);
    if (proc::pm >= 0) sys::close(proc::pm);
    proc::fd = -1; proc::pm = -1;

    char base_path[64]; int ps = 0;
    const char* ps_ = oxorany("/proc/"); while (*ps_) base_path[ps++] = *ps_++;
    int pid = proc::pid; char pds[16]; int pl = 0; int tm = pid;
    if (tm == 0) pds[pl++] = '0';
    while (tm > 0) { pds[pl++] = '0' + (tm % 10); tm /= 10; }
    for (int i = pl - 1; i >= 0; i--) base_path[ps++] = pds[i];
    base_path[ps] = '\0';

    char path[128];

    // 1. Open /mem
    int temp_ps = ps;
    strcpy(path, base_path);
    const char* s_mem = oxorany("/mem");
    int i = 0; while (s_mem[i]) path[temp_ps++] = s_mem[i++]; path[temp_ps] = '\0';
    proc::fd = sys::openat(AT_FDCWD, path, O_RDONLY);

    // 2. Open /pagemap
    temp_ps = ps;
    strcpy(path, base_path);
    const char* s_pm = oxorany("/pagemap");
    i = 0; while (s_pm[i]) path[temp_ps++] = s_pm[i++]; path[temp_ps] = '\0';
    proc::pm = sys::openat(AT_FDCWD, path, O_RDONLY);
}

inline bool is_pid_alive(int pid) {
    if (pid <= 0) return false;
    char path[64];
    // Fast & Silent check: Check if /proc/[pid] directory still exists
    // Bypasses suspicious process tree scanning
    snprintf(path, sizeof(path), oxorany("/proc/%d"), pid);
    return (sys::openat(AT_FDCWD, path, O_RDONLY | O_DIRECTORY) >= 0);
}

namespace game {
    inline int tick = 0, fail_count = 0, check_tick = 0;
    inline bool valid() {
        if (proc::pid <= 0) {
            proc::pid = get_pid();
            if (proc::pid <= 0) return false;
            if (proc::use_driver) driver::initialize(proc::pid);
        }

        if (proc::lib == 0) {
            proc::lib = get_lib();
            if (proc::lib != 0) {
                init_mem();
                if (proc::use_driver) driver::initialize(proc::pid);
            }
            return false;
        }

        tick++;
        if (tick >= 10) {
            tick = 0;
            // 🔥 LIGHTWEIGHT CHECK: Only check our specific game PID
            // No more lag, No more suspicious scanning!
            if (!is_pid_alive(proc::pid)) {
                if (proc::pm >= 0) { sys::close(proc::pm); proc::pm = -1; }
                proc::pid = -1; proc::lib = 0;
                return false;
            }
        }
        return true;
    }
    inline void check_lib(uint64_t pm) {
        check_tick++;
        if (check_tick >= 60) {
            if (pm == 0) {
                fail_count++;
                if (fail_count >= 5) {
                    uint64_t lib = get_lib();
                    if (lib != 0 && lib != proc::lib) { proc::lib = lib; if (proc::pm >= 0) { sys::close(proc::pm); proc::pm = -1; } init_mem(); }
                    fail_count = 3;
                }
            } else fail_count = 0;
            check_tick = 0;
        }
    }
    inline void init() { proc::pid = get_pid(); if (proc::pid > 0) { proc::lib = get_lib(); if (proc::lib != 0) init_mem(); } }
}