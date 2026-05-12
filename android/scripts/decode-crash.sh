#!/bin/bash
# 用 ndk-stack symbolicate 最近的 native crash
# 用法: ./scripts/decode-crash.sh [logfile]
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# config.sh 使用 [[ -z "$VAR" ]] 检查空变量，与 set -u 冲突，临时关闭
set +u
source "$SCRIPT_DIR/../config.sh"
set -u

ADB="$ANDROID_SDK_ROOT/platform-tools/adb"
ANDROID_DIR="$(dirname "$SCRIPT_DIR")"
NDK_STACK="$ANDROID_NDK_ROOT/ndk-stack"

LOG="${1:-}"
if [ -z "$LOG" ]; then
    # 从 emulator 抓最近 500 行
    LOG="$ANDROID_DIR/build/test-logs/crash-$(date +%Y%m%d-%H%M%S).log"
    mkdir -p "$(dirname "$LOG")"
    "$ADB" logcat -d > "$LOG"
    echo "抓取当前 logcat 到 $LOG"
fi

# 根据模拟器 ABI 选 symbol 目录
ABI=$("$ADB" shell getprop ro.product.cpu.abi | tr -d '\r')
SYM_DIR="$ANDROID_DIR/build/debug/$ABI"

if [ ! -d "$SYM_DIR" ]; then
    echo "未找到符号目录 $SYM_DIR" >&2
    echo "先运行 ./build.sh debug 生成" >&2
    exit 1
fi

echo "使用符号目录：$SYM_DIR"
echo "解析崩溃："
echo ""
"$NDK_STACK" -sym "$SYM_DIR" -dump "$LOG"
