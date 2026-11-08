#ifndef KERNELTOOLS_H
#define KERNELTOOLS_H

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <pthread.h>
#include <dirent.h>
#include <malloc.h>
#include <thread>
#include <sys/mman.h>
#include <sys/uio.h>
#include <math.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <locale>
#include <string>
#include <dlfcn.h>
#include <regex.h>

namespace driver {

    extern int fd;
    extern pid_t pid;

	typedef struct _COPY_MEMORY {
		pid_t pid;
		uintptr_t addr;
		void* buffer;
		size_t size;
	} COPY_MEMORY, *PCOPY_MEMORY;

	typedef struct _MODULE_BASE {
		pid_t pid;
		char* name;
		uintptr_t base;
	} MODULE_BASE, *PMODULE_BASE;

	enum OPERATIONS {
        OP_INIT_KEY = 0x800,
        OP_READ_MEM = 0x801,
        OP_WRITE_MEM = 0x802,
        OP_MODULE_BASE = 0x803,
        OP_HIDE_PROCESS = 0x804,
    };

    uintptr_t get_module_base(const char* name);
    void c_driver();
    void close_c_driver();
    void initialize(pid_t _pid);
    bool init_key(const char *key);
    bool read(uintptr_t addr, void *buffer, size_t size);
    bool write(uintptr_t addr, void *buffer, size_t size);

    template <typename T>
    T read(uintptr_t addr) {
        T res;
        if (read(addr, &res, sizeof(T)))
            return res;
        return {};
    }

    template <typename T>
    bool write(uintptr_t addr, T value) {
        return write(addr, &value, sizeof(T));
    }
}

#endif //KERNELTOOLS_H
