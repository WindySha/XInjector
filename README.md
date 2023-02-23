# Xinjector
This is a tool that can be used to inject any elf files into any android application on root device.

# Features
1. Support inject multiple elf files；
2. Support arm and arm64 architecture；
3. Support android 5 and above android os version;
4. Support calling function in the injected elf;

# Build
Clone the project
```
$ git clone https://code.byted.org/xiawanli/Xinjector.git
$ cd Xinjector
```
Use ndk-build in the android sdk tools to build, make sure the ndk-build file path is put into the environment variable.
```
$ ndk-build
```
Output executable file path is:
```
libs/armeabi-v7a/xinjector
libs/arm64-v8a/xinjector
```
# How to use
Push the xinjector and the so to be injected into /data/local/tmp on the android device.
```
$ adb push libs/arm64-v8a/xinjector /data/local/tmp
$ adb push libnative-lib.so /data/local/tmp
```
Execute the xinjector file:
```
$ adb shell
$ su
$ cd /data/local/tmp
$ ./xinjector package_name /data/local/tmp/libnative-lib.so entry_point_function_name entry_point_parameters
```
package_name  --> the package name of the app that you want to inject into.  
/data/local/tmp/libnative-lib.so  --> injected so file path.  
entry_point_function_name  --> the function to be called in the injected so, this is alternative.  
entry_point_parameters   --> the function parameter to be passed in, this is alternative too.  

# Reference
[Android-Ptrace-Injector](https://github.com/reveny/Android-Ptrace-Injector)

# License
Copyright (C) 2023 WindySha

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this work 
except in compliance with the License. You may obtain a copy of the License in the 
LICENSE file, or at:

http://www.apache.org/licenses/LICENSE-2.0
