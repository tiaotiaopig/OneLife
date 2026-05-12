#!/bin/bash
# OneLife Android 自动化烟测
#
# 流程：build → install → launch → 等待 N 秒 → 抓 logcat → 判定
#
# 用法：
#   ./scripts/test-cycle.sh                 # 默认 build debug + 测 5 秒
#   ./scripts/test-cycle.sh 10              # 测 10 秒
#   ./scripts/test-cycle.sh 5 --no-build    # 跳过构建，直接装现有 APK
#
# 退出码：
#   0  PASS
#   1  FAIL（进程挂掉/崩溃/EGL 错误/渲染循环停滞）
#   2  环境问题（emulator 不在 / adb 出错）

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ANDROID_DIR="$(dirname "$SCRIPT_DIR")"
cd "$ANDROID_DIR"

# config.sh 使用 [[ -z "$VAR" ]] 检查空变量，与 set -u 冲突，临时关闭
set +u
source ./config.sh
set -u

ADB="$ANDROID_SDK_ROOT/platform-tools/adb"
PKG="com.fengli.onelife"
ACTIVITY="${PKG}/android.app.NativeActivity"
APK="$ANDROID_DIR/build/OneLife-debug.apk"

WAIT_SECONDS="${1:-5}"
SKIP_BUILD=0
for arg in "$@"; do
    [ "$arg" = "--no-build" ] && SKIP_BUILD=1
done

LOG_DIR="$ANDROID_DIR/build/test-logs"
mkdir -p "$LOG_DIR"
RUN_LOG="$LOG_DIR/run-$(date +%Y%m%d-%H%M%S).log"

print_header() {
    echo "========================================"
    echo "  $1"
    echo "========================================"
}

fail() {
    echo ""
    echo "[FAIL] $1"
    echo ""
    echo "--- 最近 50 行 logcat ---"
    tail -50 "$RUN_LOG" || true
    echo ""
    echo "完整日志：$RUN_LOG"
    exit 1
}

# 1) 检查 emulator 在线
print_header "1. 检查 emulator"
if ! "$ADB" devices | grep -q "emulator-.*device"; then
    echo "emulator 未连接。请先运行 ./scripts/emulator-control.sh start"
    exit 2
fi
"$ADB" devices | grep emulator

# 2) 构建
if [ "$SKIP_BUILD" -eq 0 ]; then
    print_header "2. 构建 APK"
    if ! ./build.sh debug > "$LOG_DIR/build.log" 2>&1; then
        echo "构建失败，最后 30 行："
        tail -30 "$LOG_DIR/build.log"
        exit 1
    fi
    ls -lh "$APK"
else
    print_header "2. 跳过构建"
fi

if [ ! -f "$APK" ]; then
    fail "APK 不存在：$APK"
fi

# 3) 卸载旧版本（防止签名冲突）+ 装新 APK
print_header "3. 安装 APK"
"$ADB" uninstall "$PKG" 2>/dev/null || true
if ! "$ADB" install -r "$APK" 2>&1 | tee "$LOG_DIR/install.log" | grep -q "Success"; then
    echo "安装失败："
    cat "$LOG_DIR/install.log"
    exit 1
fi

# 4) 清 logcat、启动 Activity
print_header "4. 启动 Activity"
"$ADB" logcat -c
"$ADB" shell am start -n "$ACTIVITY"

# 5) 等待，期间采集 logcat
print_header "5. 采集 ${WAIT_SECONDS}s logcat"
"$ADB" logcat -s 'OneLife:V' 'AndroidRuntime:E' 'DEBUG:V' 'libc:E' > "$RUN_LOG" &
LOGCAT_PID=$!
sleep "$WAIT_SECONDS"
kill "$LOGCAT_PID" 2>/dev/null || true
wait "$LOGCAT_PID" 2>/dev/null || true

# 6) 校验
print_header "6. 校验"

# 6a) 进程存活
PID=$("$ADB" shell pidof "$PKG" 2>/dev/null | tr -d '\r' || true)
if [ -z "$PID" ]; then
    fail "进程不在运行"
fi
echo "进程存活 PID=$PID"

# 6b) 无 native crash
if grep -qE "FATAL|backtrace:|Fatal signal" "$RUN_LOG"; then
    fail "检测到 native crash"
fi
echo "无 native crash"

# 6c) 无 EGL 错误
if grep -q "eglSwapBuffers failed" "$RUN_LOG"; then
    fail "eglSwapBuffers 失败"
fi
echo "无 EGL 错误"

# 6d) 渲染循环存活：tickGame 日志递增
#     （Task 2.4 起 android_main 的主循环改名为 tickGame，调 platformTick）
DRAW_COUNT=$(grep -cE "tickGame #|drawFrame #" "$RUN_LOG" || true)
if [ "$DRAW_COUNT" -lt 2 ]; then
    fail "渲染循环未启动（tick 日志数=$DRAW_COUNT，期望>=2）"
fi
echo "渲染循环正常，tick 事件 $DRAW_COUNT 次"

# 6e) EGL 初始化成功
if ! grep -q "EGL ready:" "$RUN_LOG"; then
    fail "EGL 未初始化（未见 'EGL ready:'）"
fi
EGL_SIZE=$(grep "EGL ready:" "$RUN_LOG" | head -1)
echo "$EGL_SIZE"

# 全部通过
echo ""
echo "[PASS] 所有检查通过"
echo "日志：$RUN_LOG"
exit 0
