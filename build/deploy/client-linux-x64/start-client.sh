#!/bin/bash
# OneLife 客户端启动脚本
set -e
cd "$(dirname "$(readlink -f "$0")")/bin"
exec ./OneLife "$@"
