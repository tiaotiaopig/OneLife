#!/usr/bin/env bash
# OneLife dist 部署包一键构建脚本
# 编译客户端/服务端并组装 Linux x64 部署包到 dist/。

set -euo pipefail

ROOT="$(cd "$(dirname "$(readlink -f "$0")")/.." && pwd)"
DATA_ROOT="$ROOT/../OneLifeData7"
MINORGEMS_ROOT="$ROOT/../minorGems"
DIST="$ROOT/dist"
DEPLOY="$ROOT/build/deploy"

SKIP_BUILD=0
ONLY=""

usage() {
    cat <<'EOF'
用法：scripts/build-dist.sh [选项]

选项：
  --skip-build        跳过编译，直接使用现有二进制
  --only server       只构建服务端部署包
  --only client       只构建客户端部署包
  -h, --help          显示此帮助
EOF
}

while [ $# -gt 0 ]; do
    case "$1" in
        --skip-build) SKIP_BUILD=1; shift ;;
        --only)       ONLY="${2:-}"; shift 2 ;;
        -h|--help)    usage; exit 0 ;;
        *) echo "未知参数：$1" >&2; usage; exit 2 ;;
    esac
done

case "$ONLY" in ""|server|client) ;; *)
    echo "--only 只接受 server 或 client" >&2; exit 2 ;;
esac

require_dir()  { [ -d "$1" ] || { echo "缺少目录：$1" >&2; exit 1; }; }
require_file() { [ -f "$1" ] || { echo "缺少文件：$1" >&2; exit 1; }; }

require_dir  "$MINORGEMS_ROOT"
require_dir  "$DATA_ROOT"
require_dir  "$DEPLOY/client-linux-x64"
require_dir  "$DEPLOY/server-linux-x64"
require_file "$ROOT/server/configure"
require_file "$ROOT/configure"

echo "[dist] root=$ROOT skip_build=$SKIP_BUILD only=${ONLY:-all}"

build_server() {
    echo "[dist] 编译服务端"
    (
        cd "$ROOT/server"
        ./configure 1
        make
    )
    require_file "$ROOT/server/OneLifeServer"
}

build_client() {
    echo "[dist] 编译客户端"
    (
        cd "$ROOT"
        ./configure 1
    )
    (
        cd "$ROOT/gameSource"
        make
    )
    require_file "$ROOT/gameSource/OneLife"
}

if [ "$SKIP_BUILD" -eq 0 ]; then
    case "$ONLY" in
        server) build_server ;;
        client) build_client ;;
        "")     build_server; build_client ;;
    esac
fi

SERVER_TEXT_FILES=(
    wordList.txt curseWordList.txt firstNames.txt lastNames.txt
    maleNames.txt femaleNames.txt namesInfo.txt protocol.txt
    serverCodeVersionNumber.txt
)

SERVER_DATA_DIRS=(
    objects transitions categories animations tutorialMaps contentSettings
)

package_server() {
    echo "[dist] 组装服务端部署包"
    local stage="$DIST/server-linux-x64"
    rm -rf "$stage"
    mkdir -p "$stage/bin" "$stage/data" "$stage/settings"

    require_file "$ROOT/server/OneLifeServer"
    cp "$ROOT/server/OneLifeServer" "$stage/bin/"
    for f in "${SERVER_TEXT_FILES[@]}"; do
        require_file "$ROOT/server/$f"
        cp "$ROOT/server/$f" "$stage/bin/"
    done

    for d in "${SERVER_DATA_DIRS[@]}"; do
        require_dir "$DATA_ROOT/$d"
        cp -r "$DATA_ROOT/$d" "$stage/data/"
    done
    require_file "$DATA_ROOT/dataVersionNumber.txt"
    require_file "$DATA_ROOT/isAHAP.txt"
    cp "$DATA_ROOT/dataVersionNumber.txt" "$stage/data/"
    cp "$DATA_ROOT/isAHAP.txt"            "$stage/data/"

    require_dir "$ROOT/server/settings"
    cp -r "$ROOT/server/settings/." "$stage/settings/"

    cp "$DEPLOY/server-linux-x64/start-server.sh" "$stage/"
    cp "$DEPLOY/server-linux-x64/stop-server.sh"  "$stage/"
    cp "$DEPLOY/server-linux-x64/README.md"       "$stage/"
    chmod +x "$stage/start-server.sh" "$stage/stop-server.sh"

    tar -C "$DIST" -czf "$DIST/onelife-server-linux-x64.tar.gz" server-linux-x64
    echo "[dist] 生成 $DIST/onelife-server-linux-x64.tar.gz"
}

CLIENT_DATA_DIRS=(
    animations categories contentSettings faces ground music objects
    overlays scenes sounds sprites transitions tutorialMaps
)

package_client() {
    echo "[dist] 组装客户端部署包"
    local stage="$DIST/client-linux-x64"
    rm -rf "$stage"
    mkdir -p "$stage/bin"

    require_file "$ROOT/gameSource/OneLife"
    cp "$ROOT/gameSource/OneLife" "$stage/bin/"

    for d in graphics languages settings; do
        require_dir "$ROOT/gameSource/$d"
        cp -r "$ROOT/gameSource/$d" "$stage/bin/"
    done

    for d in "${CLIENT_DATA_DIRS[@]}"; do
        require_dir "$DATA_ROOT/$d"
        cp -r "$DATA_ROOT/$d" "$stage/bin/"
    done
    cp "$DATA_ROOT/dataVersionNumber.txt" "$stage/bin/"
    cp "$DATA_ROOT/isAHAP.txt"            "$stage/bin/"

    cp "$DEPLOY/client-linux-x64/start-client.sh" "$stage/"
    cp "$DEPLOY/client-linux-x64/README.md"       "$stage/"
    chmod +x "$stage/start-client.sh"

    tar -C "$DIST" -czf "$DIST/onelife-client-linux-x64.tar.gz" client-linux-x64
    echo "[dist] 生成 $DIST/onelife-client-linux-x64.tar.gz"
}

case "$ONLY" in
    server) package_server ;;
    client) package_client ;;
    "")     package_server; package_client ;;
esac

echo "[dist] 全部完成"
