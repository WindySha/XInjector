#!/bin/bash

# Build script for XInjector
# This script unset NDK_TOOLCHAIN environment variable before calling ndk-build
# to fix the error: NDK_TOOLCHAIN is defined to the unsupported value

# Unset NDK_TOOLCHAIN to let NDK auto-select the toolchain
unset NDK_TOOLCHAIN

# Get NDK path from environment or use default
NDK_PATH="${NDK_PATH:-${ANDROID_NDK_HOME}}"

if [ -z "$NDK_PATH" ]; then
    # Try to find NDK in common locations
    if [ -d "$HOME/Library/Android/sdk/ndk" ]; then
        # Check if NDK_VERSION is specified
        if [ -n "$NDK_VERSION" ]; then
            # Try to find the specified version
            NDK_PATH="$HOME/Library/Android/sdk/ndk/$NDK_VERSION"
            if [ ! -d "$NDK_PATH" ]; then
                echo "Warning: Specified NDK version $NDK_VERSION not found, using latest..."
                NDK_PATH=""
            fi
        fi
        
        # If still not found, use the latest NDK version
        if [ -z "$NDK_PATH" ]; then
            NDK_PATH=$(ls -td "$HOME/Library/Android/sdk/ndk"/*/ 2>/dev/null | head -1)
            NDK_PATH=${NDK_PATH%/}
        fi
    fi
fi

if [ -z "$NDK_PATH" ]; then
    echo "Error: NDK path not found. Please set NDK_PATH or ANDROID_NDK_HOME environment variable."
    exit 1
fi

echo "Using NDK: $NDK_PATH"
echo "Building XInjector..."

# Change to project root directory
cd "$(dirname "$0")" || exit 1

# Run ndk-build with all arguments passed to this script
# NDK will automatically look for jni/Android.mk
"$NDK_PATH/ndk-build" "$@"

