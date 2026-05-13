# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 项目概述

OneLife（One Hour One Life）是 Jason Rohrer 开发的持久化在线多人游戏。玩家以婴儿身份出生，在约一小时的真实时间内经历完整的生命周期（每分钟 = 一年），通过世代传承建设文明。项目包含 C++ 客户端、C++ 服务器，以及若干 PHP/Python 辅助服务。代码置于公有领域（见 no_copyright.txt）。

## 构建

### 外部依赖

项目依赖 `minorGems` 库，必须位于 `../minorGems`（与 OneLife 同级目录）。minorGems 提供 SDL 图形、网络、加密等基础设施。客户端还需要 SDL 开发库，编辑器额外需要 `libpng-dev`。

### 构建服务器

```bash
cd server
./configure 1    # 参数 1=Linux, 2=macOS, 3=MinGW, 4=Raspbian
make
```

产出二进制：`OneLifeServer`

### 构建客户端（游戏）

```bash
cd gameSource
cd .. && ./configure 1 && cd gameSource
make
```

产出二进制：`OneLife`

### 构建编辑器

```bash
cd gameSource
./makeEditor.sh   # 临时替换 makeFileList 为 makeFileListEditor，然后 make
```

产出二进制：`EditOneLife`

### 发布构建

```bash
cd build/source
./makeLinuxBuild <release_name>   # 打包源码 + 编译 + 生成 tar.gz
```

## 运行

服务器运行前需要链接数据目录（objects、transitions、categories、animations 等），通常来自 `OneLifeData7`：

```bash
cd server
ln -s ../../OneLifeData7/objects .
ln -s ../../OneLifeData7/transitions .
ln -s ../../OneLifeData7/categories .
ln -s ../../OneLifeData7/animations .
ln -s ../../OneLifeData7/tutorialMaps .
ln -s ../../OneLifeData7/contentSettings .
ln -s ../../OneLifeData7/dataVersionNumber.txt .
ln -s ../../OneLifeData7/isAHAP.txt .
./OneLifeServer
```

### 服务端数据依赖说明

服务端实际加载的数据目录（均来自 OneLifeData7）：

| 目录/文件 | 用途 | 必需 |
|-----------|------|------|
| objects/ | 游戏对象定义（4431 个 + 自动生成变体） | 是 |
| transitions/ | 合成/转化配方 | 是 |
| categories/ | 对象分类 | 是 |
| animations/ | 动画元数据（服务端用于对象状态计算） | 是 |
| tutorialMaps/ | 教程地图数据 | 启用教程时需要 |
| contentSettings/ | 内容相关配置（婴儿骨骼等） | 是 |
| dataVersionNumber.txt | 数据版本号 | 是 |
| isAHAP.txt | AHAP 变体标识 | 是 |

服务端**不需要**：sprites/、sounds/、music/、ground/、faces/、scenes/、overlays/（这些仅客户端渲染使用）。

### 部署包构建

预构建的部署包位于 `dist/` 目录：

```bash
dist/
├── onelife-server-linux-x64.tar.gz  # 服务端（~4.3M）
├── onelife-client-linux-x64.tar.gz  # 客户端（~66M）
└── README.md                        # 部署说明
```

服务端部署包结构采用运行时隔离设计：
- `bin/` — 二进制 + 文本资源（不可变）
- `data/` — 游戏数据（不可变）
- `settings/` — 配置文件
- `runtime/` — 启动后自动创建，所有 .db 和日志落在此处

备份世界：`tar czf backup.tar.gz runtime/`；重置世界：`rm -rf runtime/`

### 客户端连接远程服务端

客户端和服务端支持分离部署（如 macOS 客户端 + Linux 服务端）。修改 `gameSource/settings/` 下的配置：

- `useCustomServer.ini` 设为 `1`
- `customServerAddress.ini` 设为服务端 IP 地址
- `customServerPort.ini` 设为服务端端口（默认 8005）
- 若服务端启用了密码认证，还需设置 `requireClientPassword.ini` 为 `1`，`clientPassword.ini` 为对应密码

### 各平台构建客户端

macOS 需要 Xcode Command Line Tools（`xcode-select --install`）和 SDL 1.2（`brew install sdl12-compat`），configure 选择平台 2：

```bash
cd .. && ./configure 2 && cd gameSource
make
```

Windows 使用 MinGW，configure 选择平台 3。

## 代码架构

### 客户端 (`gameSource/`)

- `game.cpp` — 游戏入口，初始化和页面路由
- `LivingLifePage.cpp`（~28K 行）— 核心游戏循环，处理玩家交互、渲染、网络消息
- `objectBank.cpp` / `categoryBank.cpp` / `transitionBank.cpp` — 游戏对象、分类、合成配方的数据加载与管理
- `spriteBank.cpp` / `animationBank.cpp` / `soundBank.cpp` — 精灵、动画、音效资源管理
- `editor.cpp` + `Editor*Page.cpp` — 游戏内容编辑器（独立二进制 EditOneLife）
- UI 组件：`PageComponent.cpp`、`Button.cpp`、`TextField.cpp`

客户端基于 minorGems 的 `game.h` 框架，采用页面（Page）模式组织 UI 流程。

### 服务器 (`server/`)

- `server.cpp`（~33K 行）— 主服务器逻辑，处理所有客户端连接、游戏状态、玩家生命周期
- `map.cpp`（~11K 行）— 世界地图生成与管理，包含生物群系、资源放置
- `lineardb*.cpp` / `kissdb.cpp` / `stackdb.cpp` — 自研轻量级键值数据库
- `curseDB.cpp` — 玩家诅咒/封禁系统
- `lifeLog.cpp` / `lineageLog.cpp` — 生命记录与家族谱系
- `triggers.cpp` — 事件触发器系统
- `names.cpp` — 玩家命名系统

### 共享代码 (`commonSource/`)

客户端和服务器共用的代码，如分形噪声生成、发言限制等。

### 网络协议

ASCII 文本协议，以 `#` 字符终止消息。详见 `server/protocol.txt`。对象 ID 可以是单个整数或逗号分隔的整数列表（基础对象 + 容器内对象）。

### 辅助服务

- `ahapGate/` — AHAP 变体认证网关（PHP）
- `curseServer/` / `fitnessServer/` / `lineageServer/` / `photoServer/` — 各类辅助 PHP 服务
- `reflector/` — 服务器反射/代理
- `discord/` — Discord 集成（Python）

## 版本与发布

- 客户端版本标签格式：`OneLife_vN`
- 服务器活跃分支：`OneLife_liveServer`
- 版本号文件：`serverCodeVersionNumber.txt`、`dataVersionNumber.txt`
- 服务器向客户端报告代码版本和数据版本中较大的那个

## 编码注意事项

- C++ 代码风格偏传统 C，大量使用全局状态和超长函数
- 核心文件（server.cpp、LivingLifePage.cpp）体量巨大，修改时注意定位准确
- 构建系统基于 shell 脚本 + minorGems 的 configure/make，无 CMake
- 设置通过 `settings/` 目录下的 `.ini` 文件管理，使用 minorGems 的 `SettingsManager` 读取
- 无正式测试框架，`server/` 下有少量独立测试程序（`kissTest.cpp`、`stackdbTest.cpp`、`stressTestClient.cpp`）

## 构建和运行注意事项

- configure 是交互式脚本，必须传平台编号参数（1=Linux），否则会阻塞
- 客户端 make 的图形资源转换步骤依赖 ImageMagick 的 `convert` 命令（`apt install imagemagick`），缺失不影响二进制编译
- 本地测试服务端需修改 `server/settings/`：`requireTicketServerCheck.ini` 设为 0，`requireClientPassword.ini` 设为 1，`clientPassword.ini` 设置密码
- 服务端运行后会在 server/ 下生成 `*.db`、`*Log/`、`log.txt` 等运行时文件，清理时注意不要误删数据链接和编译产物
- 服务端默认监听端口 8005（见 `server/settings/port.ini`）
- 可用 `echo "" | nc 127.0.0.1 8005` 快速验证服务端是否正常响应（应返回 SN 消息）
- 客户端首次登录会进入教程模式（tutorial=1），教程地图加载需 2-3 分钟。跳过教程：在客户端 `settings/` 下设置 `tutorialDone.ini` 为 1

### macOS 客户端特殊配置

- **SDL 依赖**：Homebrew 安装的 `sdl12-compat` 不是框架形式，需修改 `gameSource/Makefile`：
  - `PLATFORM_COMPILE_FLAGS` 添加 `-I/opt/homebrew/include -D_THREAD_SAFE`
  - `PLATFORM_LINK_FLAGS` 将 `-framework SDL` 替换为 `$(shell sdl-config --libs)`
- **工作目录**：macOS 客户端启动时会将工作目录强制设为项目根目录（`OneLife/`），需在根目录创建符号链接：
  ```bash
  cd OneLife/
  ln -s gameSource/settings settings
  ln -s gameSource/graphics graphics
  for dir in objects transitions categories animations sprites sounds ground overlays music faces scenes tutorialMaps contentSettings; do ln -s ../OneLifeData7/$dir .; done
  ln -s ../OneLifeData7/dataVersionNumber.txt .
  ln -s ../OneLifeData7/isAHAP.txt .
  ```
- **密码配置命名**：客户端读取 `serverPassword.ini`（不是 `clientPassword.ini`），需与服务端 `clientPassword.ini` 的值保持一致

## 服务端硬件需求

服务端采用单线程事件驱动架构，资源占用特点：

- **CPU**：空闲时几乎为 0，玩家活动时按需计算。单核性能最关键，教程地图加载、地图生成、transition 计算是 CPU 密集型操作
- **内存**：空载约 100MB（加载 4431 个对象 + 自动生成的 ~6400 个变体对象），随玩家数和地图扩展增长
- **磁盘**：使用自研 lineardb3 键值数据库存储地图，频繁读写。SSD 性能优于 HDD
- **网络**：ASCII 文本协议，带宽需求低，但延迟敏感
- **并发能力**：默认最大 160 人（`server/settings/maxPlayers.ini`），官方服务器支持 230 人

少量玩家的私服，1 核 1G 云服务器即可满足需求。

## Android 客户端（`android-client` 分支）

Android 适配在独立分支 `android-client` 上维护，不合入 master（上游 jasonrohrer/OneLife 持续更新，独立分支便于定期 merge）。

### 分支结构

| 仓库 | 上游分支 | Android 分支 | 冲突面 |
|------|----------|-------------|--------|
| OneLife | `master` | `android-client` | 极小（95% 改动在 `android/` 新目录） |
| minorGems | `master` | `android-port` | 小（新增文件为主，仅 glInclude.h + File.h 有改动） |

### 同步上游

```bash
# OneLife
git checkout master && git pull origin master
git checkout android-client && git merge master

# minorGems
cd ../minorGems
git checkout master && git pull
git checkout android-port && git merge master
```

### 构建 Android APK

```bash
cd ../minorGems && git checkout android-port
cd ../OneLife && git checkout android-client
cd android && ./build.sh debug
```

详见 `android/CLAUDE.md`。

