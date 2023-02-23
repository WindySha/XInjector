#pragma once

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

int xinjector_inject_library_files(pid_t pid, const char* path[], size_t path_len, const char* entrypoint, const char* data);

#ifdef __cplusplus
}
#endif