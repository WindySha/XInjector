#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/system_properties.h>
#include <android/api-level.h>
#include "utils.h"
#include "logger.h"


static const char *libc_path;
static const char *linker_path;
static const char *libdl_path;
static int has_init_flag = 0;
static int device_api_level = -1;

static void init_system_lib_paths() {
    if (has_init_flag) return;

    int sdkVersion = xinjector_get_android_sdk_level();

    if (xinjector_is_64_bit_abi() == 1) {
        if (sdkVersion >= __ANDROID_API_R__) { //Android 11
            libc_path = "/apex/com.android.runtime/lib64/bionic/libc.so";
            linker_path = "/apex/com.android.runtime/bin/linker64";
            libdl_path = "/apex/com.android.runtime/lib64/bionic/libdl.so";
        } else if (sdkVersion >= __ANDROID_API_Q__) { //Android 10
            libc_path = "/apex/com.android.runtime/lib64/bionic/libc.so";
            linker_path = "/apex/com.android.runtime/bin/linker64";
            libdl_path = "/apex/com.android.runtime/lib64/bionic/libdl.so";
        } else {
            libc_path = "/system/lib64/libc.so";
            linker_path = "/system/bin/linker64";
            libdl_path = "/system/lib64/libdl.so";
        }
    } else {
        if (sdkVersion >= __ANDROID_API_R__) { //Android 11
            libc_path = "/apex/com.android.runtime/lib/bionic/libc.so";
            linker_path = "/apex/com.android.runtime/bin/linker";
            libdl_path = "/apex/com.android.runtime/lib/bionic/libdl.so";
        } else if (sdkVersion >= __ANDROID_API_Q__) { //Android 10
            libc_path = "/apex/com.android.runtime/lib/bionic/libc.so";
            linker_path = "/apex/com.android.runtime/bin/linker";
            libdl_path = "/apex/com.android.runtime/lib/bionic/libdl.so";
        } else {
            libc_path = "/system/lib/libc.so";
            linker_path = "/system/bin/linker";
            libdl_path = "/system/lib/libdl.so";
        }
    }
    has_init_flag = 1;
}

const char* xinjector_get_libc_path()
{
    init_system_lib_paths();
    return libc_path;
}

const char* xinjector_get_linker_path()
{
    init_system_lib_paths();
    return linker_path;
}

const char* xinjector_get_libdl_path()
{
    init_system_lib_paths();
    return libdl_path;
}

int xinjector_get_android_sdk_level()
{
    if (device_api_level < 0) {
        device_api_level = android_get_device_api_level();
    }
    return device_api_level;
}

int xinjector_is_selinux_enabled() {
    int result = 0;
    FILE* fp = fopen("/proc/filesystems", "r");
    char line[100];
    while(fgets(line, 100, fp)) {
        if (strstr(line, "selinuxfs")) {
            result = 1;
        }
    }
    LOGD(" isSELinuxEnabled, result = %d", result);
    fclose(fp);
    return result;
}

void xinjector_disable_selinux() {
    char line[1024];
    FILE* fp = fopen("/proc/mounts", "r");
    while(fgets(line, 1024, fp)) {
        if (strstr(line, "selinuxfs")) {
            strtok(line, " ");
            char* selinux_dir = strtok(NULL, " ");
            char* selinux_path = strcat(selinux_dir, "/enforce");

            FILE* fp_selinux = fopen(selinux_path, "w");
            char buf[2] = "0"; //0 = Permissive
            fwrite(buf, strlen(buf), 1, fp_selinux);
            fclose(fp_selinux);
            break;
        }
    }
    fclose(fp);
}

pid_t xinjector_get_pid(const char* process_name) {
    if (process_name == NULL) {
        return -1;
    }
    DIR* dir = opendir("/proc");
    if (dir == NULL) {
        return -1;
    }
    struct dirent* entry;
    while((entry = readdir(dir)) != NULL) {
        size_t pid = atoi(entry->d_name);
        if (pid != 0) {
            char file_name[30];
            snprintf(file_name, 30, "/proc/%zu/cmdline", pid);
            FILE *fp = fopen(file_name, "r");
            char temp_name[50];
            if (fp != NULL) {
                fgets(temp_name, 50, fp);
                fclose(fp);
                if (strcmp(process_name, temp_name) == 0) {
                    return pid;
                }
            }
        }
    }
    return -1;
}

void *xinjector_get_module_base_addr(pid_t pid, const char *moduleName) {
    long moduleBaseAddr = 0;
    char mapsPath[50] = {0};
    char szMapFileLine[1024] = {0};

    snprintf(mapsPath, sizeof(mapsPath), "/proc/%d/maps", pid);
    FILE *fp = fopen(mapsPath, "r");

    if (fp != NULL) {
        while (fgets(szMapFileLine, sizeof(szMapFileLine), fp)) {
            if (strstr(szMapFileLine, moduleName)) {
                char *address = strtok(szMapFileLine, "-");
                moduleBaseAddr = strtoul(address, NULL, 16);

                if (moduleBaseAddr == 0x8000) {
                    moduleBaseAddr = 0;
                }

                break;
            }
        }
        fclose(fp);
    }

    return (void *)moduleBaseAddr;
}

//Get the address of the function in the module loaded by the remote process and this process
void *xinjector_get_remote_func_addr(pid_t pid, const char *moduleName, void *localFuncAddr) {
    //Starting address of the module in my process
    void *localModuleAddr = xinjector_get_module_base_addr(getpid(), moduleName);
     //Starting address of module loaded in other process
    void *remoteModuleAddr = xinjector_get_module_base_addr(pid, moduleName);
    //Address of the function in the other process
    void *remoteFuncAddr = (void *)((uintptr_t)localFuncAddr - (uintptr_t)localModuleAddr + (uintptr_t)remoteModuleAddr);
    return remoteFuncAddr;
}
