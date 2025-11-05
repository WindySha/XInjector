APP_ABI := armeabi-v7a arm64-v8a  # all
APP_PLATFORM = android-21   # 使用SDK的最低等级

# Override NDK_TOOLCHAIN to let NDK auto-select the toolchain
# This fixes the error: NDK_TOOLCHAIN is defined to the unsupported value
# Modern NDK (27+) doesn't support setting NDK_TOOLCHAIN to a path, it should be empty
override NDK_TOOLCHAIN :=
