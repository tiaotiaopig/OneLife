#!/bin/bash
# OneLife 服务端启动脚本
# 将运行时产物（db/log）隔离到 runtime/ 目录，与二进制和静态数据分离

set -e
cd "$(dirname "$(readlink -f "$0")")"

ROOT="$PWD"
BIN="$ROOT/bin"
RUNTIME="$ROOT/runtime"

mkdir -p "$RUNTIME"

# 在 runtime/ 下建立到数据和配置的符号链接
for d in objects transitions categories animations tutorialMaps contentSettings; do
    [ -e "$RUNTIME/$d" ] || ln -s "../data/$d" "$RUNTIME/$d"
done

[ -e "$RUNTIME/dataVersionNumber.txt" ] || ln -s "../data/dataVersionNumber.txt" "$RUNTIME/dataVersionNumber.txt"
[ -e "$RUNTIME/isAHAP.txt" ]            || ln -s "../data/isAHAP.txt"            "$RUNTIME/isAHAP.txt"
[ -e "$RUNTIME/settings" ]              || ln -s "../settings"                    "$RUNTIME/settings"

# 链接二进制和文本资源到 runtime/
[ -e "$RUNTIME/OneLifeServer" ]         || ln -s "../bin/OneLifeServer"           "$RUNTIME/OneLifeServer"
for f in "$BIN"/*.txt; do
    fname="$(basename "$f")"
    [ -e "$RUNTIME/$fname" ] || ln -s "../bin/$fname" "$RUNTIME/$fname"
done

# 以 runtime/ 为工作目录启动，所有 .db 和 log 都会落在这里
cd "$RUNTIME"
exec ./OneLifeServer "$@"
