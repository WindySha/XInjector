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

## Method 1: Using build script (Recommended)
Use the provided build script which automatically handles NDK_TOOLCHAIN environment variable:
```
$ ./build.sh
```

## Method 2: Using ndk-build directly
If you encounter the error "NDK_TOOLCHAIN is defined to the unsupported value", you need to unset the NDK_TOOLCHAIN environment variable first:
```
$ unset NDK_TOOLCHAIN
$ ndk-build
```

Or specify the NDK path explicitly:
```
$ unset NDK_TOOLCHAIN && /path/to/ndk/ndk-build
```

## Method 3: Using GitHub Actions (Automated Build)
The project includes a GitHub Actions workflow that automatically builds the project on push, pull request, or when creating a tag.

**Features:**
- Automatically builds for both `armeabi-v7a` and `arm64-v8a` architectures
- Uploads build artifacts that can be downloaded from the Actions tab
- Automatically creates a GitHub Release when you push a tag starting with `v*` (e.g., `v1.0.0`)

**To use:**
1. Push your code to GitHub
2. Go to the "Actions" tab in your GitHub repository
3. Download the build artifacts from the completed workflow run

**To create a release:**
```bash
git tag v1.0.0
git push origin v1.0.0
```
This will automatically create a GitHub Release with the compiled binaries.

**Output executable file path:**
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

Or with `-r` flag to restart the app before injection:
```
$ ./xinjector -r package_name /data/local/tmp/libnative-lib.so entry_point_function_name entry_point_parameters
```

**Parameters:**
- `-r` (optional): Restart app (kill and restart) before injection. Without this flag, xinjector will wait for the app to start (infinite loop).
- `package_name`  --> the package name of the app that you want to inject into.  
- `/data/local/tmp/libnative-lib.so`  --> injected so file path.  
- `entry_point_function_name`  --> the function to be called in the injected so, this is alternative.  
- `entry_point_parameters`   --> the function parameter to be passed in, this is alternative too.  

# Reference
[Android-Ptrace-Injector](https://github.com/reveny/Android-Ptrace-Injector)

# License
Copyright (C) 2023 WindySha

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this work 
except in compliance with the License. You may obtain a copy of the License in the 
LICENSE file, or at:

http://www.apache.org/licenses/LICENSE-2.0
