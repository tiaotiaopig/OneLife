# dist 部署包构建体系设计

## 背景

当前仓库中的 `dist/` 同时承担两种职责：

1. 保存部署包产物与展开目录
2. 保存启动脚本与部署说明

与此同时，`.gitignore` 直接忽略整个 `dist/` 目录，导致部署模板文件无法通过 git 进行版本管理；`dist/README.md` 与包内脚本之间也容易出现漂移。现有官方 `build/` 目录主要面向正式发布流程与跨平台分发，而当前 `dist/` 更偏向 Linux 私服部署与快速验证，两者目标不同，因此需要为 `dist/` 补齐一条可重复执行、可维护、可跟踪的构建通道。

## 目标

建立一套面向 `dist/` 的 Linux 部署包构建体系，满足以下要求：

- 将部署模板与构建产物彻底分离
- 启动脚本和部署说明纳入 git 管理
- `scripts/build-dist.sh` 负责一键编译客户端与服务端二进制，并组装部署包
- `dist/` 保持为纯产物目录，可随时删除并重建
- 更新 `dist/README.md`，保证其与实际部署结构和启动脚本一致

## 非目标

以下内容不在本次设计范围内：

- 不替换官方 `build/source/makeLinuxBuild` 流程
- 不接入 Steam、增量更新、上传服务器等官方发布能力
- 不新增跨平台部署逻辑，仅面向 Linux x64
- 不调整客户端或服务端运行逻辑本身，只整理部署模板和构建入口

## 现状问题

### 1. 模板与产物混放

当前 `dist/` 下同时存在：

- `start-client.sh`
- `start-server.sh`
- `stop-server.sh`
- 顶层和包内 README
- 已展开的 `bin/`、`data/`、`settings/`
- 最终 `tar.gz`

这使得目录语义混乱，不易区分哪些是手写维护文件，哪些是构建产物。

### 2. git 无法跟踪模板文件

当前 `.gitignore` 直接忽略 `dist/`，导致部署脚本和文档不会被纳入版本管理。

### 3. 文档容易与真实行为漂移

服务端启动脚本已经将运行时产物隔离到 `runtime/` 下，但文档中若仍描述产物生成在 `bin/` 下，就会产生误导。

### 4. 缺少自定义部署包构建入口

官方 `build/` 目录存在发布构建脚本，但它们的定位是正式发布，不是当前这套 `dist/` 结构化 Linux 部署包的生成入口。仓库中缺少一条针对 `dist/` 的一键编译与组包脚本。

## 设计概述

采用“模板进入 `build/deploy/`、产物留在 `dist/`、统一由 `scripts/build-dist.sh` 生成”的结构。

### 目录职责

目标目录结构如下：

```text
build/
  deploy/
    README.md
    client-linux-x64/
      start-client.sh
      README.md
    server-linux-x64/
      start-server.sh
      stop-server.sh
      README.md
dist/
  README.md
  client-linux-x64/
  server-linux-x64/
  onelife-client-linux-x64.tar.gz
  onelife-server-linux-x64.tar.gz
scripts/
  build-dist.sh
```

职责划分如下：

- `build/deploy/`：只保存手写模板文件
- `dist/`：只保存构建输出和对外使用说明
- `scripts/build-dist.sh`：统一完成编译与组包

### git 管理策略

纳入版本管理：

- `build/deploy/**`
- `scripts/build-dist.sh`
- `dist/README.md`

继续忽略：

- `dist/client-linux-x64/`
- `dist/server-linux-x64/`
- `dist/*.tar.gz`
- 包内运行时生成内容

`.gitignore` 将从整目录忽略改为精细忽略，确保 `dist/README.md` 仍被跟踪。

## 构建入口设计

新增统一脚本：`scripts/build-dist.sh`

该脚本负责三段流程：

1. 编译服务端
2. 编译客户端
3. 组装并打包客户端/服务端部署包

其定位是“源码 -> 二进制 -> 部署包”的完整链路，不依赖用户提前手工编译。

### 编译阶段

脚本依次执行：

#### 服务端

- 进入 `server/`
- 运行 `./configure 1`
- 运行 `make`

#### 客户端

- 进入项目根目录
- 运行 `./configure 1`
- 进入 `gameSource/`
- 运行 `make`

脚本采用失败即退出策略，不做隐式跳过。

## 组包阶段设计

### 客户端部署包

生成目录：`dist/client-linux-x64/`

内容包括：

- `start-client.sh`（来自 `build/deploy/client-linux-x64/`）
- `README.md`（来自 `build/deploy/client-linux-x64/`）
- `bin/OneLife`
- `bin/graphics/`
- `bin/languages/`
- `bin/settings/`
- 客户端运行所需数据目录与文件

客户端包沿用现有布局：所有运行所需内容置于 `bin/` 下，由 `start-client.sh` 切换到 `bin/` 后启动。

### 服务端部署包

生成目录：`dist/server-linux-x64/`

内容包括：

- `start-server.sh`（来自 `build/deploy/server-linux-x64/`）
- `stop-server.sh`（来自 `build/deploy/server-linux-x64/`）
- `README.md`（来自 `build/deploy/server-linux-x64/`）
- `bin/OneLifeServer`
- `bin/*.txt` 文本资源
- `data/` 下的服务端必要数据
- `settings/` 下的服务端配置

服务端包保持当前运行时隔离设计：

- `bin/`：不可变二进制和文本资源
- `data/`：不可变游戏数据
- `settings/`：配置
- `runtime/`：启动后自动创建，落地数据库和日志

## 输入依赖

`build-dist.sh` 在开始前做最小校验，缺失立即失败：

- `../minorGems` 存在
- `../OneLifeData7` 存在
- `server/configure` 可执行
- 项目根 `configure` 可执行
- `make`、`tar` 可用

不负责自动安装依赖。

## 参数设计

为避免过度设计，脚本初版仅支持以下选项：

```text
scripts/build-dist.sh [选项]

选项：
  --skip-build        跳过编译，直接使用现有二进制组包
  --only server       只构建服务端部署包
  --only client       只构建客户端部署包
  -h, --help          显示帮助
```

默认行为为全量执行：编译客户端与服务端，并生成两个部署包。

### 选项语义

- `--skip-build`：仅用于本地反复调整模板或文档时加速，不改变默认推荐流程
- `--only server`：仅编译并组装服务端部署包
- `--only client`：仅编译并组装客户端部署包

不支持组合出更复杂的模式，保持脚本接口简单清晰。

## 幂等性与清理策略

构建脚本每次执行都遵循同一规则：

1. 删除旧的 staging 目录
   - `dist/client-linux-x64/`
   - `dist/server-linux-x64/`
2. 重新组装目录
3. 重新覆盖生成 `tar.gz`

具体要求：

- `dist/README.md` 作为跟踪文件必须保留
- 构建脚本不接触服务端 `runtime/` 目录
- 旧产物不残留，保证构建结果可复现

脚本将以 `set -euo pipefail` 运行。

## README 更新策略

### dist/README.md

`dist/README.md` 保留为“产物使用说明”，主要内容包括：

- 部署包列表
- 快速部署指南
- 目录结构
- 配置说明
- 系统要求
- 验证方式
- 版本信息
- 注意事项

需要同步修正以下内容：

- 明确 `dist/` 是构建产物目录，可通过 `scripts/build-dist.sh` 重建
- 明确服务端运行时产物位于 `runtime/`，而不是 `bin/`
- 确保目录结构示例与实际生成结果一致

### build/deploy/README.md

新增 `build/deploy/README.md`，用于说明：

- `build/deploy/` 是部署模板目录
- 模板文件与 `dist/` 产物的关系
- 修改模板后如何执行 `scripts/build-dist.sh` 重新生成部署包

### 包内 README 模板

- `build/deploy/client-linux-x64/README.md`
- `build/deploy/server-linux-x64/README.md`

这两个文件是包内 README 的真实来源，构建时直接复制到 `dist/` 展开目录中。

## 与官方 build/ 的边界

官方 `build/` 体系继续保留，且职责不变：

- `build/source/makeLinuxBuild`：官方 Linux 发布构建
- `makeDistributionMacOSX` / `makeDistributionWindows`：正式多平台发行
- Steam、增量更新、上传：仍属于官方发布体系

新增的 `scripts/build-dist.sh` 只解决当前仓库内 `dist/` 的可维护性和可重复构建问题，不尝试替代官方发行系统。

## 验证要求

实现完成后至少验证以下内容：

1. 运行 `scripts/build-dist.sh` 可成功生成：
   - `dist/onelife-client-linux-x64.tar.gz`
   - `dist/onelife-server-linux-x64.tar.gz`
2. `dist/client-linux-x64/` 和 `dist/server-linux-x64/` 的目录结构与 README 一致
3. 服务端包启动后，运行时产物确实落在 `runtime/`
4. 客户端包可通过 `start-client.sh` 正常进入 `bin/` 并启动
5. `--skip-build`、`--only server`、`--only client` 至少各执行一次
6. `.gitignore` 调整后，模板文件可跟踪，生成产物仍保持忽略

## 实施影响

本设计会带来以下仓库变化：

- 新增 `build/deploy/` 模板目录
- 新增 `scripts/build-dist.sh`
- 调整 `.gitignore`
- 更新 `dist/README.md`
- 从 `dist/` 中移出应被版本管理的模板文件

## 风险与控制

### 风险 1：构建脚本与现有手工目录内容不一致

控制方式：

- 以现有 `dist/client-linux-x64/` 和 `dist/server-linux-x64/` 为基准逐项比对
- 用 README 中声明的目录结构反向校验构建结果

### 风险 2：README 再次与脚本漂移

控制方式：

- 将包内 README 变成模板源文件
- 由构建脚本直接复制模板，避免手工维护多份内容

### 风险 3：误把产物重新纳入版本管理

控制方式：

- 采用精细 `.gitignore` 规则
- 明确 `dist/` 是产物目录，不在其中维护模板

## 成功标准

当以下条件全部满足时，视为本次设计达成目标：

- `build/deploy/` 成为部署模板唯一来源
- `scripts/build-dist.sh` 可一键编译并生成两个 Linux 部署包
- `dist/` 可删除并完全重建
- `dist/README.md` 与真实部署结构一致
- git 仅跟踪模板、构建脚本与产物说明，不跟踪大体积生成内容
