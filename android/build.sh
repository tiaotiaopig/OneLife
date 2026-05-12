#!/bin/bash
# OneLife Android 一键构建脚本
# 用法：./build.sh [debug|release]
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

source ./config.sh

BUILD_TYPE="${1:-debug}"
# 构建目录优先级：
#   1. 显式环境变量 ANDROID_BUILD_DIR（可能由 config.sh 自动设为 /data1/ 下的路径）
#   2. 默认回退到 $SCRIPT_DIR/build/$BUILD_TYPE
if [[ -n "${ANDROID_BUILD_DIR:-}" ]]; then
    BUILD_DIR="$ANDROID_BUILD_DIR/$BUILD_TYPE"
else
    BUILD_DIR="$SCRIPT_DIR/build/$BUILD_TYPE"
fi
OUT_APK="$SCRIPT_DIR/build/OneLife-$BUILD_TYPE.apk"
mkdir -p "$SCRIPT_DIR/build"  # 确保 OUT_APK 所在目录存在

mkdir -p "$BUILD_DIR"

# 1) 用 CMake + NDK 编译每个 ABI 的 .so
for ABI in $ANDROID_ABIS; do
    ABI_BUILD="$BUILD_DIR/$ABI"
    mkdir -p "$ABI_BUILD"
    cmake -B "$ABI_BUILD" -S "$SCRIPT_DIR" \
        -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK_ROOT/build/cmake/android.toolchain.cmake" \
        -DANDROID_ABI="$ABI" \
        -DANDROID_PLATFORM="android-$ANDROID_API_LEVEL" \
        -DCMAKE_BUILD_TYPE=$( [ "$BUILD_TYPE" = "debug" ] && echo Debug || echo Release ) \
        -DANDROID_STL=c++_static
    cmake --build "$ABI_BUILD" -j
done

# 2) 收集每个 ABI 的 .so 到 lib/
LIB_DIR="$BUILD_DIR/apk-staging/lib"
rm -rf "$LIB_DIR"
mkdir -p "$LIB_DIR"
for ABI in $ANDROID_ABIS; do
    mkdir -p "$LIB_DIR/$ABI"
    cp "$BUILD_DIR/$ABI/libonelife.so" "$LIB_DIR/$ABI/"
done

# 3) aapt 打包：Manifest + res + lib + assets（assets 在 Phase 0 为空目录）
APK_UNSIGNED="$BUILD_DIR/OneLife-unsigned.apk"
mkdir -p "$SCRIPT_DIR/assets"
"$AAPT" package -f \
    -M "$SCRIPT_DIR/AndroidManifest.xml" \
    -S "$SCRIPT_DIR/res" \
    -A "$SCRIPT_DIR/assets" \
    -I "$ANDROID_JAR" \
    -F "$APK_UNSIGNED"

# 4) 把 lib/ 加进 APK
cd "$BUILD_DIR/apk-staging"
"$AAPT" add "$APK_UNSIGNED" $(find lib -type f)
cd "$SCRIPT_DIR"

# 5) zipalign + 签名
APK_ALIGNED="$BUILD_DIR/OneLife-aligned.apk"
"$ZIPALIGN" -f 4 "$APK_UNSIGNED" "$APK_ALIGNED"

KEYSTORE="$SCRIPT_DIR/build/debug.keystore"
if [ ! -f "$KEYSTORE" ]; then
    keytool -genkeypair -v -keystore "$KEYSTORE" -storepass android -keypass android \
        -alias androiddebugkey -dname "CN=Android Debug,O=Android,C=US" \
        -keyalg RSA -keysize 2048 -validity 10000
fi

"$APKSIGNER" sign --ks "$KEYSTORE" --ks-pass pass:android --key-pass pass:android \
    --out "$OUT_APK" "$APK_ALIGNED"

echo "==> Built: $OUT_APK"
ls -lh "$OUT_APK"
