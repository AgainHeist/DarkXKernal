#pragma once

#include <sys/syscall.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <fcntl.h>

namespace Safe {

    // Helper to add complex junk code to confuse signature scanners
    #define JUNK_CODE \
        { \
            volatile int _jx = 0x1A2B; \
            volatile int _jy = 0x3C4D; \
            _jx = (_jx << 2) ^ _jy; \
            _jy = (_jy >> 1) + _jx; \
            _jx = _jx & 0xFFFF; \
            volatile int _jz = _jx ^ _jy; \
            (void)_jz; \
        }

    static inline long syscall6(long n, long a1, long a2, long a3, long a4, long a5, long a6) {
        long r;
        JUNK_CODE
        #if defined(__aarch64__)
        register long x8 __asm__("x8") = n;
        register long x0 __asm__("x0") = a1;
        register long x1 __asm__("x1") = a2;
        register long x2 __asm__("x2") = a3;
        register long x3 __asm__("x3") = a4;
        register long x4 __asm__("x4") = a5;
        register long x5 __asm__("x5") = a6;
        __asm__ volatile("svc #0" : "+r"(x0) : "r"(x8), "r"(x1), "r"(x2), "r"(x3), "r"(x4), "r"(x5) : "memory", "cc");
        r = x0;
        #elif defined(__arm__)
        register long r7 __asm__("r7") = n;
        register long r0 __asm__("r0") = a1;
        register long r1 __asm__("r1") = a2;
        register long r2 __asm__("r2") = a3;
        register long r3 __asm__("r3") = a4;
        register long r4 __asm__("r4") = a5;
        register long r5 __asm__("r5") = a6;
        __asm__ volatile("svc #0" : "+r"(r0) : "r"(r7), "r"(r1), "r"(r2), "r"(r3), "r"(r4), "r"(r5) : "memory", "cc");
        r = r0;
        #else
        r = syscall(n, a1, a2, a3, a4, a5, a6);
        #endif
        JUNK_CODE
        return r;
    }

    static inline int openat(int dirfd, const char* pathname, int flags, mode_t mode) {
        return (int)syscall6(__NR_openat, dirfd, (long)pathname, flags, mode, 0, 0);
    }

    static inline ssize_t read(int fd, void* buf, size_t count) {
        return (ssize_t)syscall6(__NR_read, fd, (long)buf, count, 0, 0, 0);
    }

    static inline int close(int fd) {
        return (int)syscall6(__NR_close, fd, 0, 0, 0, 0, 0);
    }

    static inline ssize_t process_vm_readv(pid_t pid, const struct iovec* local_iov, unsigned long liovcnt, const struct iovec* remote_iov, unsigned long riovcnt, unsigned long flags) {
        #if defined(__aarch64__)
        return (ssize_t)syscall6(270, (long)pid, (long)local_iov, liovcnt, (long)remote_iov, riovcnt, flags);
        #elif defined(__arm__)
        return (ssize_t)syscall6(376, (long)pid, (long)local_iov, liovcnt, (long)remote_iov, riovcnt, flags);
        #else
        return -1;
        #endif
    }
}
