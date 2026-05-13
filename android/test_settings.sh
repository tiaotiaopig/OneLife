#!/bin/bash
# 配置 Android 客户端连接本地测试服务端
# 用法：./test_settings.sh [server_ip] [port]
#
# 需要先安装 APK 并启动一次（让 settings 目录创建），然后运行此脚本。

set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/config.sh"

ADB="$ANDROID_SDK_ROOT/platform-tools/adb"
PKG="com.fengli.onelife"
SETTINGS_DIR="/data/data/$PKG/files/settings"

SERVER_IP="${1:-10.0.2.2}"
SERVER_PORT="${2:-8005}"

echo "配置测试服务端: $SERVER_IP:$SERVER_PORT"

"$ADB" shell "run-as $PKG sh -c 'echo 1 > $SETTINGS_DIR/useCustomServer.ini'"
"$ADB" shell "run-as $PKG sh -c 'echo $SERVER_IP > $SETTINGS_DIR/customServerAddress.ini'"
"$ADB" shell "run-as $PKG sh -c 'echo $SERVER_PORT > $SETTINGS_DIR/customServerPort.ini'"
"$ADB" shell "run-as $PKG sh -c 'echo 0 > $SETTINGS_DIR/requireClientPassword.ini'"
"$ADB" shell "run-as $PKG sh -c 'echo test@test.com > $SETTINGS_DIR/email.ini'"
"$ADB" shell "run-as $PKG sh -c 'echo TESTKEY123456789 > $SETTINGS_DIR/accountKey.ini'"

echo "Done. 重启客户端后生效。"
