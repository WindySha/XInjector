#pragma once

#include "logger.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void xinjector_kill_application(const char* packageName)
{
    //am force-stop packageName
    char start_cmd[1024] = "am force-stop ";
    strcat(start_cmd, packageName);
    system(start_cmd);
}

static void xinjector_launch_application(const char* packageName)
{
    // adb shell monkey -p package_name -c android.intent.category.LAUNCHER 1
    char start_cmd[1024];
    char start_cmd_fmt[1024] = "monkey -p %s -c android.intent.category.LAUNCHER 1";

    sprintf(start_cmd, start_cmd_fmt, packageName);
    system(start_cmd);
}

static void xinjector_launch_application_by_activity(const char* packageName)
{
    // adb shell am start $(adb shell cmd package resolve-activity --brief com.ss.android.ugc.aweme | tail -n 1)
    // am start $(cmd package resolve-activity --brief com.ss.android.ugc.aweme | tail -n 1)

    char start_cmd[1024];
    char start_cmd_fmt[1024] = "am start $(cmd package resolve-activity --brief %s | tail -n 1)";

    sprintf(start_cmd, start_cmd_fmt, packageName);
    system(start_cmd);
}

static pid_t fetch_app_process_pid(const char* package_name) {
    pid_t _pid = 0;
    int count = 100;
    for (size_t i = 0; i < count; i++) {
        _pid = xinjector_get_pid(package_name);
        if (_pid > 0) {
            LOGI("getPID iterate count i = %d  target pid = %d", i, _pid);
            break;
        }
        usleep(500);
    }
    return _pid;
}

static pid_t xinjector_restart_app_and_getpid(const char* package_name)
{
    xinjector_kill_application(package_name);
    xinjector_launch_application(package_name);

    pid_t pid = fetch_app_process_pid(package_name);

    if (pid <= 0) {
        xinjector_launch_application_by_activity(package_name);
        pid = fetch_app_process_pid(package_name);
    }

    return pid;
}