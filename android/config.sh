#!/bin/bash
# OneLife Android 构建配置——按本机实际路径调整
export ANDROID_NDK_ROOT="${ANDROID_NDK_ROOT:-$HOME/android-tools/android-ndk-r25c}"
export ANDROID_SDK_ROOT="${ANDROID_SDK_ROOT:-$HOME/android-tools/android-sdk}"
export ANDROID_API_LEVEL=29
export ANDROID_ABIS="arm64-v8a armeabi-v7a"
export ANDROID_BUILD_TOOLS_VERSION=33.0.2

export AAPT="$ANDROID_SDK_ROOT/build-tools/$ANDROID_BUILD_TOOLS_VERSION/aapt"
export APKSIGNER="$ANDROID_SDK_ROOT/build-tools/$ANDROID_BUILD_TOOLS_VERSION/apksigner"
export ZIPALIGN="$ANDROID_SDK_ROOT/build-tools/$ANDROID_BUILD_TOOLS_VERSION/zipalign"
export ANDROID_JAR="$ANDROID_SDK_ROOT/platforms/android-$ANDROID_API_LEVEL/android.jar"
