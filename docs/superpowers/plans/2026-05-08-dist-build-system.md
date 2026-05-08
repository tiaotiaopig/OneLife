# dist 部署包构建体系实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 建立 `build/deploy/` 模板目录与 `scripts/build-dist.sh` 统一构建入口，让 `dist/` 成为可重建的纯产物目录。

**Architecture:** 模板与产物分离；`build/deploy/` 保存手写脚本和包内 README；`scripts/build-dist.sh` 负责编译客户端/服务端二进制并从模板组装 `dist/` 下的两个 Linux 部署包；`.gitignore` 精细化只跟踪模板、构建脚本和 `dist/README.md`。

**Tech Stack:** Bash、GNU make、tar、现有 OneLife configure 脚本

**注：** 本项目以 Shell 脚本为主，缺乏成熟单元测试框架。验证以“脚本运行结果”作为黑盒断言，而不是单元测试 Runner。

---

## 前置说明

- 所有路径相对 `/jfs/fengli16/Projects/CProjects/OneLife`
- 参考规格：`docs/superpowers/specs/2026-05-08-dist-build-system-design.md`
- 现有模板源文件：`dist/client-linux-x64/start-client.sh`、`dist/server-linux-x64/start-server.sh`、`dist/server-linux-x64/stop-server.sh`、`dist/*/README.md`
- 构建依赖：`server/configure`、`./configure`（根目录）、`gameSource/Makefile`、`../minorGems`、`../OneLifeData7`

---

### Task 1: 调整 .gitignore，精细化忽略 dist

**Files:**
- Modify: `.gitignore:32`

- [ ] **Step 1: 改写 `.gitignore` 中 dist 规则**

将 `.gitignore` 末尾的 `dist/` 一行替换为：

```gitignore
dist/client-linux-x64/
dist/server-linux-x64/
dist/*.tar.gz
!dist/README.md
```

- [ ] **Step 2: 验证 git 跟踪状态**

```bash
git check-ignore -v dist/README.md || echo "README tracked"
git check-ignore -v dist/client-linux-x64/bin/OneLife
git check-ignore -v dist/onelife-client-linux-x64.tar.gz
```

期望：`dist/README.md` 未被忽略；后两者被忽略。

- [ ] **Step 3: 提交**

```bash
git add .gitignore
git commit -m "chore(dist): refine gitignore to track templates only"
```

---

### Task 2: 建立 build/deploy 模板目录结构

**Files:**
- Create: `build/deploy/client-linux-x64/start-client.sh`
- Create: `build/deploy/client-linux-x64/README.md`
- Create: `build/deploy/server-linux-x64/start-server.sh`
- Create: `build/deploy/server-linux-x64/stop-server.sh`
- Create: `build/deploy/server-linux-x64/README.md`

- [ ] **Step 1: 拷贝现有模板到 build/deploy**

```bash
mkdir -p build/deploy/client-linux-x64 build/deploy/server-linux-x64
cp dist/client-linux-x64/start-client.sh  build/deploy/client-linux-x64/
cp dist/client-linux-x64/README.md        build/deploy/client-linux-x64/
cp dist/server-linux-x64/start-server.sh  build/deploy/server-linux-x64/
cp dist/server-linux-x64/stop-server.sh   build/deploy/server-linux-x64/
cp dist/server-linux-x64/README.md        build/deploy/server-linux-x64/
chmod +x build/deploy/client-linux-x64/start-client.sh \
         build/deploy/server-linux-x64/start-server.sh \
         build/deploy/server-linux-x64/stop-server.sh
```

- [ ] **Step 2: 校验模板目录**

```bash
ls build/deploy/client-linux-x64 build/deploy/server-linux-x64
```

期望列出上述 5 个文件。

- [ ] **Step 3: 提交模板**

```bash
git add build/deploy
git commit -m "chore(build): add deploy templates under build/deploy"
```

---

### Task 3: 新增 build/deploy/README.md 说明模板目录职责

**Files:**
- Create: `build/deploy/README.md`

- [ ] **Step 1: 写入 README**

```markdown
# build/deploy 部署模板

本目录保存 `dist/` 部署包的手写模板，构建产物不落在本目录。

## 内容

- `client-linux-x64/` — 客户端部署包模板（启动脚本、包内 README）
- `server-linux-x64/` — 服务端部署包模板（启动脚本、停止脚本、包内 README）

## 构建入口

在项目根目录执行：

```bash
scripts/build-dist.sh
```

该脚本会：

1. 编译服务端与客户端二进制
2. 组装 `dist/client-linux-x64/` 与 `dist/server-linux-x64/`
3. 生成 `dist/onelife-client-linux-x64.tar.gz` 与 `dist/onelife-server-linux-x64.tar.gz`

## 修改流程

1. 修改本目录下模板文件
2. 重新执行 `scripts/build-dist.sh`
3. 验证 `dist/` 下新产物
4. 提交模板改动

`dist/` 下的生成内容不需要提交。
```

- [ ] **Step 2: 提交**

```bash
git add build/deploy/README.md
git commit -m "docs(build): document deploy template directory"
```

---

### Task 4: 从 dist 移除已迁移的模板文件

**Files:**
- Delete: `dist/client-linux-x64/start-client.sh`
- Delete: `dist/client-linux-x64/README.md`
- Delete: `dist/server-linux-x64/start-server.sh`
- Delete: `dist/server-linux-x64/stop-server.sh`
- Delete: `dist/server-linux-x64/README.md`

- [ ] **Step 1: 删除 dist 下模板文件**

```bash
rm -f dist/client-linux-x64/start-client.sh dist/client-linux-x64/README.md
rm -f dist/server-linux-x64/start-server.sh dist/server-linux-x64/stop-server.sh dist/server-linux-x64/README.md
```

- [ ] **Step 2: 验证**

```bash
ls dist/client-linux-x64 dist/server-linux-x64
```

期望 `dist/client-linux-x64/` 只剩 `bin/`；`dist/server-linux-x64/` 只剩 `bin/ data/ settings/`。

- [ ] **Step 3: 无需 git add**

这些文件本就未被跟踪，无需提交。继续下一个任务。

---

### Task 5: 创建 scripts/build-dist.sh 主干与选项解析

**Files:**
- Create: `scripts/build-dist.sh`

- [ ] **Step 1: 创建骨架脚本**

写入 `scripts/build-dist.sh`：

```bash
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
```

- [ ] **Step 2: 赋可执行权限**

```bash
chmod +x scripts/build-dist.sh
```

- [ ] **Step 3: 冒烟测试帮助输出**

```bash
scripts/build-dist.sh --help
```

期望打印用法。

- [ ] **Step 4: 提交骨架**

```bash
git add scripts/build-dist.sh
git commit -m "feat(build): add scripts/build-dist.sh skeleton"
```

---

### Task 6: 为 build-dist.sh 增加编译阶段

**Files:**
- Modify: `scripts/build-dist.sh`

- [ ] **Step 1: 在脚本末尾追加编译函数**

在 `scripts/build-dist.sh` 末尾追加：

```bash
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
```

- [ ] **Step 2: 验证 --skip-build 路径不触发编译**

```bash
scripts/build-dist.sh --skip-build --only server
```

期望脚本执行不报错即停（后续任务会补打包，此处允许在组包处失败）。

- [ ] **Step 3: 提交**

```bash
git add scripts/build-dist.sh
git commit -m "feat(build): compile server and client in build-dist.sh"
```

---

### Task 7: 在 build-dist.sh 中实现服务端组包

**Files:**
- Modify: `scripts/build-dist.sh`

- [ ] **Step 1: 在脚本末尾追加服务端组包函数**

在 `scripts/build-dist.sh` 末尾追加：

```bash
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

if [ "$ONLY" = "server" ] || [ -z "$ONLY" ]; then
    package_server
fi
```

- [ ] **Step 2: 验证服务端组包**

```bash
scripts/build-dist.sh --only server
ls dist/server-linux-x64 dist/onelife-server-linux-x64.tar.gz
tar tzf dist/onelife-server-linux-x64.tar.gz | head -20
```

期望：`dist/server-linux-x64/` 下含 `bin/ data/ settings/ start-server.sh stop-server.sh README.md`；tar 列表以 `server-linux-x64/` 开头。

- [ ] **Step 3: 提交**

```bash
git add scripts/build-dist.sh
git commit -m "feat(build): package server deployment tarball"
```

---

### Task 8: 在 build-dist.sh 中实现客户端组包

**Files:**
- Modify: `scripts/build-dist.sh`

- [ ] **Step 1: 在脚本末尾追加客户端组包函数**

在 `scripts/build-dist.sh` 末尾追加：

```bash
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

if [ "$ONLY" = "client" ] || [ -z "$ONLY" ]; then
    package_client
fi
```

- [ ] **Step 2: 验证客户端组包**

```bash
scripts/build-dist.sh --skip-build --only client
ls dist/client-linux-x64 dist/onelife-client-linux-x64.tar.gz
```

注：若当前 `gameSource/OneLife` 不存在，去掉 `--skip-build` 重新跑。

- [ ] **Step 3: 提交**

```bash
git add scripts/build-dist.sh
git commit -m "feat(build): package client deployment tarball"
```

---

### Task 9: 更新 dist/README.md

**Files:**
- Modify: `dist/README.md:140`
- Modify: `dist/README.md` 首尾相关段落

- [ ] **Step 1: 修正运行时产物描述**

将 `dist/README.md` 中第 140 行左右的：

```markdown
4. **运行时产物**：服务端运行后会在 `bin/` 下生成 `*.db`、`*Log/`、`log.txt` 等文件
```

改为：

```markdown
4. **运行时产物**：服务端运行后会在 `runtime/` 下生成 `*.db`、`*Log/`、`log.txt` 等文件；备份世界只需 `tar czf backup.tar.gz runtime/`，重置世界 `rm -rf runtime/` 即可
```

- [ ] **Step 2: 在顶部补充构建方式段落**

在 `dist/README.md` 顶部 `## 部署包列表` 之前插入：

```markdown
## 构建方式

本目录下的展开目录与 `*.tar.gz` 均为构建产物，不纳入版本管理。可在项目根目录执行：

```bash
scripts/build-dist.sh
```

重新生成。模板位于 `build/deploy/`，详见 `build/deploy/README.md`。
```

- [ ] **Step 3: 提交**

```bash
git add dist/README.md
git commit -m "docs(dist): clarify runtime layout and build pipeline"
```

---

### Task 10: 端到端验证

**Files:**
- Verify only

- [ ] **Step 1: 清理并全量构建**

```bash
rm -rf dist/client-linux-x64 dist/server-linux-x64 dist/*.tar.gz
scripts/build-dist.sh
```

期望：两个 tar.gz 均生成成功，脚本以 0 码退出。

- [ ] **Step 2: 校验结构**

```bash
ls dist/server-linux-x64 dist/client-linux-x64
tar tzf dist/onelife-server-linux-x64.tar.gz | grep -E 'start-server.sh|README.md|bin/OneLifeServer|data/objects/' | head
tar tzf dist/onelife-client-linux-x64.tar.gz | grep -E 'start-client.sh|README.md|bin/OneLife$|bin/objects/' | head
```

期望匹配行均存在。

- [ ] **Step 3: 校验 gitignore 行为**

```bash
git status --short
```

期望无 `dist/client-linux-x64/`、`dist/server-linux-x64/`、`dist/*.tar.gz` 出现。

- [ ] **Step 4: 若验证通过，无需再提交（代码改动均在上个步骤提交）**

---

## 自检

- 规格第 3 节 `.gitignore` 调整 → Task 1
- 规格第 4 节目录职责 → Task 2 / Task 4
- 规格第 5 节 `build/deploy/README.md` → Task 3
- 规格第 6 节构建入口与参数 → Task 5
- 规格第 6 节编译阶段 → Task 6
- 规格第 7 节组包阶段 → Task 7 / Task 8
- 规格第 10 节 README 更新 → Task 9
- 规格验证要求 → Task 10
