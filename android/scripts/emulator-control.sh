#!/bin/bash
# 模拟器控制脚本
# 用法:
#   ./emulator-control.sh start   # 后台启动模拟器并等待 boot
#   ./emulator-control.sh stop    # 停止模拟器
#   ./emulator-control.sh status  # 检查模拟器状态
#   ./emulator-control.sh logs    # 查看模拟器日志
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# config.sh 使用 [[ -z "$VAR" ]] 检查空变量，与 set -u 冲突，临时关闭
set +u
# shellcheck disable=SC1091
source "$SCRIPT_DIR/../config.sh"
set -u

ADB="$ANDROID_SDK_ROOT/platform-tools/adb"
EMULATOR="$ANDROID_SDK_ROOT/emulator/emulator"
LOG_DIR="$HOME/android-tools/emulator-logs"
PID_FILE="$LOG_DIR/emulator.pid"
LOG_FILE="$LOG_DIR/emulator.log"
AVD_NAME="onelife_test"

mkdir -p "$LOG_DIR"

case "${1:-status}" in
    start)
        if [ -f "$PID_FILE" ] && kill -0 "$(cat "$PID_FILE")" 2>/dev/null; then
            echo "Emulator already running (PID $(cat "$PID_FILE"))"
            exit 0
        fi
        echo "Starting emulator..."
        nohup "$EMULATOR" -avd "$AVD_NAME" \
            -no-window -no-audio -no-snapshot \
            -gpu swiftshader_indirect -no-boot-anim -read-only \
            > "$LOG_FILE" 2>&1 &
        echo $! > "$PID_FILE"
        echo "PID: $(cat "$PID_FILE"), waiting for boot..."
        "$ADB" wait-for-device
        for i in $(seq 1 60); do
            B=$("$ADB" shell getprop sys.boot_completed 2>/dev/null | tr -d '\r')
            [ "$B" = "1" ] && { echo "Booted in $((i*10))s"; exit 0; }
            sleep 10
        done
        echo "ERROR: did not boot" >&2
        tail -30 "$LOG_FILE"
        exit 1
        ;;
    stop)
        if [ -f "$PID_FILE" ]; then
            PID=$(cat "$PID_FILE")
            if kill -0 "$PID" 2>/dev/null; then
                kill "$PID"
                echo "Stopped (PID $PID)"
            fi
            rm -f "$PID_FILE"
        fi
        "$ADB" emu kill 2>/dev/null || true
        ;;
    status)
        "$ADB" devices
        if [ -f "$PID_FILE" ] && kill -0 "$(cat "$PID_FILE")" 2>/dev/null; then
            echo "Emulator process: PID $(cat "$PID_FILE")"
        else
            echo "Emulator process: not running"
        fi
        ;;
    logs)
        tail -f "$LOG_FILE"
        ;;
    *)
        echo "Usage: $0 {start|stop|status|logs}" >&2
        exit 1
        ;;
esac
