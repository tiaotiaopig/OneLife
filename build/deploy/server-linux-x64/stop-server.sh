#!/bin/bash
# 优雅停止本机上运行的 OneLifeServer
PIDS=$(pgrep -f "/OneLifeServer" || true)
if [ -z "$PIDS" ]; then
    echo "未发现运行中的 OneLifeServer"
    exit 0
fi
echo "发送 SIGTERM 给：$PIDS"
kill $PIDS
