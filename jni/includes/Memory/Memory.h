#pragma once
// Made By @AKNoryx28
// Fixed & Optimized by ERROR MODZ

#include <unistd.h>
#include <string>
#include <vector>
#include <inttypes.h>
#include <sys/types.h> 
#include <cstddef>     

#ifdef __LP64__
typedef uint64_t APTR;
#else
typedef uint32_t APTR;
#endif

typedef int Prot;
enum Prot_ {
    Prot_NONE = 0,
    Prot_READ = 1 << 1,
    Prot_WRITE = 1 << 2,
    Prot_EXEC = 1 << 3,
    Prot_PRIV = 1 << 4,
};

inline Prot parsePerms(const char* perms_str) {
    Prot perms = Prot_NONE;
    if (perms_str[0] == 'r') {
        perms |= Prot_READ;
    }
    if (perms_str[1] == 'w') {
        perms |= Prot_WRITE;
    }
    if (perms_str[2] == 'x') {
        perms |= Prot_EXEC;
    }
    if (perms_str[3] == 'p') {
        perms |= Prot_PRIV;
    }
    return perms;
}

struct PMap {
    pid_t pid;
    unsigned long long startAddress;
    unsigned long long endAddress;
    size_t length;
    Prot perms;
    unsigned long long offset;
    std::string dev;
    unsigned long inode;
    std::string pathname;
};

namespace Memory {
    inline pid_t g_pid = -1; // default pid
    
    pid_t FindPid(const char* procName);
    
    uintptr_t GetBase(const char *libName, pid_t pid = g_pid);
    uintptr_t GetEnd(const char *libName, pid_t pid = g_pid);
    
    uintptr_t GetBaseInSplit(const char *splitName, const char *libName, pid_t pid = g_pid);
    uintptr_t GetEndInSplit(const char *splitName, const char *libName, pid_t pid = g_pid);
    
    std::vector<PMap> GetMaps(const char *name, pid_t pid = g_pid);
    
    // ✅ FIXED: Removed Ambiguous Overloads. Only using uintptr_t for Memory Addresses.
    int ProcessRead(uintptr_t address, void *out, size_t size, pid_t pid = g_pid);
    int ProcessWrite(uintptr_t address, void *data, size_t size, pid_t pid = g_pid);
    
    // Process IO R/W
    bool WriteIO(uintptr_t adresss, void *write_buffer, size_t size, pid_t pid = g_pid);
    bool ReadIO(uintptr_t adresss, void *read_buffer, size_t size, pid_t pid = g_pid);
    void CleanIO();
    
    uintptr_t FindPattern(uintptr_t start, size_t length, const unsigned char* pattern, const char* mask);
    
    std::string Exec(const char *cmd);
    bool IsInvalidAddress(uintptr_t address);
    bool IsInvalidAddress(void *address);
    
    template<typename T>
    inline T Read(uintptr_t addr);
    
    template<typename T>
    inline T Read(const std::vector<uintptr_t> &address);
    
    template<typename T>
    inline std::vector<T> ReadArray(uintptr_t address, unsigned int size);
    
    inline uintptr_t ReadPtr(uintptr_t address);
    inline uintptr_t ReadPtr(const std::vector<uintptr_t> &address);
    
    template<typename T>
    inline bool Write(uintptr_t addr, T data);
}

// ==========================================
// TEMPLATE IMPLEMENTATIONS
// ==========================================

template<typename T>
inline T Memory::Read(uintptr_t addr) {
    T data{};
    // ✅ FIXED: ProcessRead expects uintptr_t directly, no need to cast to void*
    int result = Memory::ProcessRead(addr, &data, sizeof(T), Memory::g_pid);
    if (result != 0) {
        return {};
    }
    return data;
}

template<typename T>
inline T Memory::Read(const std::vector<uintptr_t> &address) {
    uintptr_t tmpAddress = 0;
    T result{};
    for (size_t i = 0; i < address.size(); ++i) {
        auto adr = address[i];
        int rval = -1;
        if (i == address.size() - 1) {
            // ✅ FIXED: Safe pointer arithmetic and passing as uintptr_t
            rval = Memory::ProcessRead(tmpAddress + adr, &result, sizeof(T), Memory::g_pid);
        } else {
            rval = Memory::ProcessRead(tmpAddress + adr, &tmpAddress, sizeof(uintptr_t), Memory::g_pid);
        }
        if (rval != 0) return {};
    }
    return result;
}

template<typename T>
inline bool Memory::Write(uintptr_t addr, T data) {
    // ✅ FIXED: ProcessWrite expects uintptr_t
    int result = Memory::ProcessWrite(addr, &data, sizeof(T), Memory::g_pid);
    return result == 0;
}

template<typename T>
inline std::vector<T> Memory::ReadArray(uintptr_t address, unsigned int size) {
    std::vector<T> data(size);
    // ✅ FIXED: ProcessRead expects uintptr_t
    Memory::ProcessRead(address, data.data(), sizeof(T) * size, Memory::g_pid);
    return data;
}

inline uintptr_t Memory::ReadPtr(uintptr_t address) {
    uintptr_t data = 0;
    // ✅ FIXED: ProcessRead expects uintptr_t
    int result = Memory::ProcessRead(address, &data, sizeof(uintptr_t), Memory::g_pid);
    if (result != 0) {
        return 0;
    }
    return data;
}

inline uintptr_t Memory::ReadPtr(const std::vector<uintptr_t> &address) {
    uintptr_t tmpAddress = 0;
    for (const auto adr : address) {
        // ✅ FIXED: ProcessRead signature matches exactly
        if (Memory::ProcessRead(tmpAddress + adr, &tmpAddress, sizeof(uintptr_t), Memory::g_pid) != 0)
            return 0;
    }
    return tmpAddress;
}