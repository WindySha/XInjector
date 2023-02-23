#pragma once

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

static int xinjector_is_64_bit_abi()
{
#if defined(__aarch64__)
    return 1;
#else
    return 0;
#endif
}

// get device libc file path
const char* xinjector_get_libc_path();

// get device linker file path
const char* xinjector_get_linker_path();

// get device libdl file path
const char* xinjector_get_libdl_path();

// get target device android sdk level
int xinjector_get_android_sdk_level();

// whether selinux is enabled
int xinjector_is_selinux_enabled();

// disable selinux
void xinjector_disable_selinux();

// get the pid of the process
pid_t xinjector_get_pid(const char* process_name);

//Search the base address of the module in the process
void* xinjector_get_module_base_addr(pid_t pid, const char* moduleName);

//Get the address of the function in the module loaded by the remote process and this process
void* xinjector_get_remote_func_addr(pid_t pid, const char* moduleName, void* localFuncAddr);

#ifdef __cplusplus
}
#endif