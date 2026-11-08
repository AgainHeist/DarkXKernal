#pragma once

#include <dlfcn.h>
#include <string>
#include <stdarg.h>
#include "oxorany.h"

namespace Hidden {

    template <typename T>
    inline T GetSymbol(const char* lib_name, const char* sym_name) {
        void* handle = dlopen(lib_name, RTLD_LAZY);
        if (!handle) return nullptr;
        T sym = reinterpret_cast<T>(dlsym(handle, sym_name));
        return sym;
    }

    #define CALL_HIDDEN(lib, sym, ret_type, ...) \
        ((ret_type (*)(__VA_ARGS__))Hidden::GetSymbol(oxorany(lib), oxorany(#sym)))

    // --- Common Wrapper Functions ---

    inline int SystemPropertyGet(const char* name, char* value) {
        typedef int (*get_prop_t)(const char*, char*);
        static auto fn = GetSymbol<get_prop_t>(oxorany("libc.so"), oxorany("__system_property_get"));
        return fn ? fn(name, value) : 0;
    }

    inline void Sleep(unsigned int seconds) {
        typedef unsigned int (*sleep_t)(unsigned int);
        static auto fn = GetSymbol<sleep_t>(oxorany("libc.so"), oxorany("sleep"));
        if (fn) fn(seconds);
    }

    inline void USleep(unsigned int useconds) {
        typedef int (*usleep_t)(unsigned int);
        static auto fn = GetSymbol<usleep_t>(oxorany("libc.so"), oxorany("usleep"));
        if (fn) fn(useconds);
    }

    inline void Exit(int status) {
        typedef void (*exit_t)(int);
        static auto fn = GetSymbol<exit_t>(oxorany("libc.so"), oxorany("exit"));
        if (fn) fn(status); else _exit(status);
    }

    inline char* StrTok(char* str, const char* delim) {
        typedef char* (*strtok_t)(char*, const char*);
        static auto fn = GetSymbol<strtok_t>(oxorany("libc.so"), oxorany("strtok"));
        return fn ? fn(str, delim) : nullptr;
    }

    inline int Atoi(const char* str) {
        typedef int (*atoi_t)(const char*);
        static auto fn = GetSymbol<atoi_t>(oxorany("libc.so"), oxorany("atoi"));
        return fn ? fn(str) : 0;
    }
}
