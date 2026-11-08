#include "../../includes/Memory/KernelTools.h"
#include <sys/prctl.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <sys/stat.h>
#include <string.h>

namespace driver {
    int fd = -1;
    pid_t pid = -1;

    static char *driver_path() {
        const char *dev_path = "/dev";
        DIR *dir = opendir(dev_path);
        if (dir == NULL){
            return NULL;
        }

        char *files[] = { "wanbai", "CheckMe", "Ckanri", "lanran","video188"};
        struct dirent *entry;
        char *file_path = NULL;
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }

            size_t path_length = strlen(dev_path) + strlen(entry->d_name) + 2;
            file_path = (char *)malloc(path_length);
            snprintf(file_path, path_length, "%s/%s", dev_path, entry->d_name);
            for (int i = 0; i < 5; i++) {
                if (strcmp(entry->d_name, files[i]) == 0) {
                    closedir(dir);
                    return file_path;
                }
            }

            struct stat file_info;
            if (stat(file_path, &file_info) < 0) {
                free(file_path);
                file_path = NULL;
                continue;
            }

            if ((S_ISCHR(file_info.st_mode) || S_ISBLK(file_info.st_mode))
                && strchr(entry->d_name, '_') == NULL && strchr(entry->d_name, '-') == NULL && strchr(entry->d_name, ':') == NULL) {

                if (strcmp(entry->d_name, "lpm") == 0 || strcmp(entry->d_name, "stdin") == 0 || strcmp(entry->d_name, "stdout") == 0
                    || strcmp(entry->d_name, "stderr") == 0) {
                    free(file_path);
                    file_path = NULL;
                    continue;
                }

                int file_year = localtime(&file_info.st_ctime)->tm_year + 1900;
                if (file_year <= 1980) {
                    free(file_path);
                    file_path = NULL;
                    continue;
                }

                if (file_info.st_atime == file_info.st_ctime) {
                    if ((file_info.st_mode & S_IFMT) == 8192 && file_info.st_size == 0
                        && file_info.st_gid == 0 && file_info.st_uid == 0 && strlen(entry->d_name) <= 9) {
                        closedir(dir);
                        return file_path;
                    }
                }
            }
            free(file_path);
            file_path = NULL;
        }
        closedir(dir);
        return NULL;
    }

    void c_driver() {
        char *device_name = driver_path();
        if (device_name) {
            fd = open(device_name, O_RDWR);
            free(device_name);
        }
    }

    void close_c_driver() {
        if (fd > 0) {
            close(fd);
            fd = -1;
        }
    }

    void initialize(pid_t _pid) {
        pid = _pid;
    }

    bool init_key(const char *key) {
        if (fd == -1) return false;
        char buf[0x100];
        strncpy(buf, key, sizeof(buf));
        return ioctl(fd, OP_INIT_KEY, buf) == 0;
    }

    bool read(uintptr_t addr, void *buffer, size_t size) {
        if (fd == -1 || pid == -1) return false;
        COPY_MEMORY cm;
        // Basic sanitization similar to original
        addr = addr % 0x10000000000;
        cm.pid = pid;
        cm.addr = addr;
        cm.buffer = buffer;
        cm.size = size;

        return ioctl(fd, OP_READ_MEM, &cm) == 0;
    }

    bool write(uintptr_t addr, void *buffer, size_t size) {
        if (fd == -1 || pid == -1) return false;
        COPY_MEMORY cm;
        addr = addr % 0x10000000000;
        cm.pid = pid;
        cm.addr = addr;
        cm.buffer = buffer;
        cm.size = size;

        return ioctl(fd, OP_WRITE_MEM, &cm) == 0;
    }

    uintptr_t get_module_base(const char *name) {
        if (pid == -1) return 0;
        char filename[64];
        snprintf(filename, sizeof(filename), "/proc/%d/maps", pid);
        FILE *fp = fopen(filename, "rt");
        uintptr_t addr = 0;
        if (fp) {
            char buffer[1024];
            while (fgets(buffer, sizeof(buffer), fp)) {
                if (strstr(buffer, name)) {
                    addr = (uintptr_t)strtoul(buffer, NULL, 16);
                    break;
                }
            }
            fclose(fp);
        }
        return addr;
    }
}
