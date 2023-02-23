#include "logger.h"
#include "launch_app.h"
#include "xinjector.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

// ./xinjector  package_name  lib_path(s)  entry_point_name  params
int main(int argc, char const* argv[])
{
    const char* package_name = NULL;
    const char* entry_point_function_name = NULL;
    const char* entry_point_function_parameters = NULL;
    const char* library_paths = NULL;

    if (argc < 3 || argc > 5) {
        goto bad_usage;
    }

    package_name = argv[1];
    library_paths = argv[2];
    if (argc > 3) {
        entry_point_function_name = argv[3];
    }
    if (argc > 4) {
        entry_point_function_parameters = argv[4];
    }

    if (library_paths == NULL) {
        goto bad_usage;
    }

    const char* path_array[50] = { NULL };
    const char separator[] = ":";

    char* sep_result = strtok(library_paths, separator);

    size_t num = 0;
    path_array[num] = sep_result;
    while (sep_result != NULL) {
        path_array[num] = sep_result;
        sep_result = strtok(NULL, separator);
        num++;
    }
    size_t total_count = num;

    for (size_t i = 0; i < total_count; i++) {
        if (path_array[i] && path_array[i][0] != '/') {
            char buf[256];
            getcwd(buf, 256);
            if (buf == NULL) {
                perror("get_current_dir_name");
                return 0;
            }
            char* abs_file_path = (char*)malloc(256);
            memset(abs_file_path, 0, 256);
            sprintf(abs_file_path, "%s%s%s", buf, "/", path_array[i]);
            path_array[i] = abs_file_path;
        }
        printf("to be injected so file path: %s \n", path_array[i]);

        if ((access(path_array[i], F_OK)) == -1) {
            printf("Injection Failed, file %s not exist \n", path_array[i]);
            goto bad_usage;
        }
    }

    pid_t target_pid = xinjector_restart_app_and_getpid(package_name);
    if (target_pid <= 0) {
        printf("Get pid of %s failed.", package_name);
        return 0;
    }
    printf("package name: %s, library path: %s, pid: %d \n", package_name, path_array[total_count - 1], target_pid);
    xinjector_inject_library_files(target_pid, path_array, total_count, entry_point_function_name, entry_point_function_parameters);
    return 1;

bad_usage: {
    printf("Usage: %s <package name> <so path(s)>  <entry point function name> <entry point function parameters>\n 1. multi so paths must be seperated by ':' , they are loaded in order;\n 2. entry point function name and parameters are alternative;\n", argv[0]);
    return 0;
    }
}