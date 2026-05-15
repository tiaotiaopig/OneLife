#!/bin/bash
# OneLife Android 构建配置
# 用法: source android/config.sh
# 按本机实际路径调整，或通过环境变量覆盖

# 检测脚本所在目录的上级是否有 android-tools（开发者可能把 NDK 放在项目旁边）
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# 优先级：环境变量 → 项目同级 → $HOME/android-tools → /root 默认路径
if [[ -z "${ANDROID_NDK_ROOT:-}" ]]; then
    if [[ -d "${PROJECT_ROOT}/../android-tools/android-ndk-r25c" ]]; then
        export ANDROID_NDK_ROOT="${PROJECT_ROOT}/../android-tools/android-ndk-r25c"
    elif [[ -d "$HOME/android-tools/android-ndk-r25c" ]]; then
        export ANDROID_NDK_ROOT="$HOME/android-tools/android-ndk-r25c"
    else
        export ANDROID_NDK_ROOT="/root/android-tools/android-ndk-r25c"
    fi
fi

if [[ -z "${ANDROID_SDK_ROOT:-}" ]]; then
    if [[ -d "${PROJECT_ROOT}/../android-tools/android-sdk" ]]; then
        export ANDROID_SDK_ROOT="${PROJECT_ROOT}/../android-tools/android-sdk"
    elif [[ -d "$HOME/android-tools/android-sdk" ]]; then
        export ANDROID_SDK_ROOT="$HOME/android-tools/android-sdk"
    else
        export ANDROID_SDK_ROOT="/root/android-tools/android-sdk"
    fi
fi
export ANDROID_API_LEVEL=29
# build.sh 会遍历此列表编译多 ABI（x86_64 用于模拟器）。
# 可通过外部环境变量覆盖，例如真机出包用 ANDROID_ABIS="arm64-v8a" ./build.sh release
export ANDROID_ABIS="${ANDROID_ABIS:-arm64-v8a armeabi-v7a x86_64}"
export ANDROID_BUILD_TOOLS_VERSION=33.0.2

# 构建目录：优先用本地硬盘 /data1/（避免 JuiceFS 小文件 I/O 瓶颈，aapt 打包加速 ~15-20x）
# 可通过 ANDROID_BUILD_DIR 环境变量强制覆盖
if [[ -z "${ANDROID_BUILD_DIR:-}" ]]; then
    if [[ -d "/data1" && -w "/data1" ]]; then
        export ANDROID_BUILD_DIR="/data1/build/OneLife-android/$(basename "$SCRIPT_DIR")-${USER:-root}"
    fi
    # 否则留空，build.sh 会 fallback 到 $SCRIPT_DIR/build/$BUILD_TYPE
fi

export AAPT="${ANDROID_SDK_ROOT}/build-tools/${ANDROID_BUILD_TOOLS_VERSION}/aapt"
export APKSIGNER="${ANDROID_SDK_ROOT}/build-tools/${ANDROID_BUILD_TOOLS_VERSION}/apksigner"
export ZIPALIGN="${ANDROID_SDK_ROOT}/build-tools/${ANDROID_BUILD_TOOLS_VERSION}/zipalign"
export ANDROID_JAR="${ANDROID_SDK_ROOT}/platforms/android-${ANDROID_API_LEVEL}/android.jar"

# 验证关键路径存在
if [[ ! -d "$ANDROID_NDK_ROOT" ]]; then
    echo "错误：ANDROID_NDK_ROOT 路径不存在: $ANDROID_NDK_ROOT" >&2
    return 1
fi
if [[ ! -d "$ANDROID_SDK_ROOT" ]]; then
    echo "错误：ANDROID_SDK_ROOT 路径不存在: $ANDROID_SDK_ROOT" >&2
    return 1
fi

