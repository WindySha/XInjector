#include "xinjector.h"
#include "logger.h"
#include "ptrace_utils.h"
#include "utils.h"
#include <dlfcn.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

static int call_remote_mmap(pid_t pid, struct pt_regs* regs)
{
    long parameters[6];

    void* mmapAddr = xinjector_get_mmap_addr(pid);
    LOGI("Mmap Function Address: 0x%lx\n", (uintptr_t)mmapAddr);

    //void *mmap(void *start, size_t length, int prot, int flags, int fd, off_t offsize);
    parameters[0] = 0; //Not needed
    parameters[1] = 0x3000;
    parameters[2] = PROT_READ | PROT_WRITE | PROT_EXEC;
    parameters[3] = MAP_ANONYMOUS | MAP_PRIVATE;
    parameters[4] = 0; //Not needed
    parameters[5] = 0; //Not needed

    //Call the mmap function of the target process
    return xinjector_ptrace_call(pid, (uintptr_t)mmapAddr, parameters, 6, regs);
}

static void* call_remote_dlopen(pid_t pid, struct pt_regs* regs, void* remoteMmapAddr)
{
    long parameters[2];

    void* dlopen_addr = xinjector_get_dlopen_addr(pid);

    //Get address for dlerror
    void* dlErrorAddr = xinjector_get_dlerror_addr(pid);

    //Return value of dlopen is the start address of the loaded module
    //void *dlopen(const char *filename, int flag);
    parameters[0] = (uintptr_t)remoteMmapAddr;
    parameters[1] = RTLD_NOW | RTLD_GLOBAL;

    //Calls dlopen which loads the lib
    if (xinjector_ptrace_call(pid, (uintptr_t)dlopen_addr, parameters, 2, regs) == -1) {
        LOGE("Call dlopen Failed");
        return NULL;
    }

    void* remoteModuleAddr = (void*)xinjector_ptrace_getret(regs);
    LOGI("ptrace_call dlopen success, Remote module Address: 0x%lx", (long)remoteModuleAddr);

    //dlopen error
    if ((long)remoteModuleAddr == 0x0) {
        LOGE("dlopen error");
        if (xinjector_ptrace_call(pid, (uintptr_t)dlErrorAddr, parameters, 0, regs) == -1) {
            LOGE("Call dlerror failed");
            return NULL;
        }
        char* error = (char*)xinjector_ptrace_getret(regs);
        char localErrorInfo[1024] = { 0 };
        xinjector_ptrace_read_data(pid, (uint8_t*)error, (uint8_t*)localErrorInfo, 1024);
        LOGE("dlopen error: %s\n", localErrorInfo);
        return NULL;
    }
    return remoteModuleAddr;
}

static void* call_remote_dlsym(pid_t pid, struct pt_regs* regs, void* remoteHandle, void* remoteMmapAddr)
{
    long parameters[2];

    void* dlsym_addr = xinjector_get_dlsym_addr(pid);

    parameters[0] = (uintptr_t)remoteHandle;
    parameters[1] = (uintptr_t)remoteMmapAddr;

    if (xinjector_ptrace_call(pid, (uintptr_t)dlsym_addr, parameters, 2, regs) == -1) {
        LOGE("Call dlsym Failed");
        return NULL;
    }

    void* remoteModuleAddr = (void*)xinjector_ptrace_getret(regs);
    LOGI("ptrace_call dlsym success, Remote module Address: 0x%lx", (long)remoteModuleAddr);

    //dlopen error
    if ((long)remoteModuleAddr == 0x0) {
        LOGE("dlsym error");
        return NULL;
    }
    return remoteModuleAddr;
}

static int call_remote_function_by_addr(pid_t pid, struct pt_regs* regs, void* remoteFunc, void* remoteData)
{
    long parameters[1];
    parameters[0] = (uintptr_t)remoteData;

    if (xinjector_ptrace_call(pid, (uintptr_t)remoteFunc, parameters, 1, regs) == -1) {
        LOGE("Call remote func Failed");
        return -1;
    }

    return 0;
}

static void* get_remote_mmapped_string_addr(pid_t pid, struct pt_regs* regs, const char* content)
{
    if (call_remote_mmap(pid, regs) == -1) {
        return NULL;
    }

    // Return value is the starting address of the memory map
    void* remoteMapMemoryAddr = (void*)xinjector_ptrace_getret(regs);
    LOGI("Remote Process Map Address: 0x%lx", (uintptr_t)remoteMapMemoryAddr);

    //Params:            pid,             start addr,                      content,          size
    if (xinjector_ptrace_write_data(pid, (uint8_t*)remoteMapMemoryAddr, (uint8_t*)content, strlen(content) + 1) == -1) {
        LOGE("writing %s to process failed", content);
        return NULL;
    }
    return remoteMapMemoryAddr;
}

int xinjector_inject_library_files(pid_t pid, const char* so_path[], size_t so_path_len, const char* entrypoint, const char* data)
{
    if (xinjector_is_selinux_enabled() == 1) {
        xinjector_disable_selinux();
    }

    int returnValue = 0;

    if (xinjector_ptrace_attach(pid) != 0) {
        return -1;
    }

    struct pt_regs currentRegs;
    struct pt_regs originalRegs;

    if (xinjector_ptrace_getregs(pid, &currentRegs) != 0) {
        LOGE("Ptrace getregs failed");
        return -1;
    }

    //Backup the Original Register
    memcpy(&originalRegs, &currentRegs, sizeof(currentRegs));
    void* main_so_address = NULL;

    for (size_t i = 0; i < so_path_len; i++) {
        void* remoteMapMemoryAddr = get_remote_mmapped_string_addr(pid, &currentRegs, so_path[i]);

        void* remoteModuleAddr = call_remote_dlopen(pid, &currentRegs, remoteMapMemoryAddr);
        if (!remoteModuleAddr) {
            LOGE("Call dlopen Failed, path: %s ", so_path[i]);
            returnValue = -1;
        }
        if (i == so_path_len - 1) {
            main_so_address = remoteModuleAddr;
        }
    }

    if (entrypoint && main_so_address) {
        void* remoteMapSymbleAddr = get_remote_mmapped_string_addr(pid, &currentRegs, entrypoint);
        void* symble_address = call_remote_dlsym(pid, &currentRegs, main_so_address, remoteMapSymbleAddr);

        if (symble_address) {
            void* remoteMapDataAddr = NULL;
            if (data) {
                remoteMapDataAddr = get_remote_mmapped_string_addr(pid, &currentRegs, data);
            }
            call_remote_function_by_addr(pid, &currentRegs, symble_address, remoteMapDataAddr);
        } else {
            LOGE("Cannot found address of symbol: [%s] !!!", entrypoint);
        }
    }

    if (xinjector_ptrace_setregs(pid, &originalRegs) == -1) {
        LOGE("Could not recover reges");
        returnValue = -1;
    }

    xinjector_ptrace_getregs(pid, &currentRegs);
    if (memcmp(&originalRegs, &currentRegs, sizeof(currentRegs)) != 0) {
        LOGE("Set Regs Error");
        returnValue = -1;
    }

    xinjector_ptrace_detach(pid);

    return returnValue;
}