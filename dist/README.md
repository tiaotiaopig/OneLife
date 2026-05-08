# OneLife 部署包说明

本目录包含 OneLife 游戏的服务端和客户端部署包（Linux x64）。

本目录下的展开目录与 `*.tar.gz` 均为构建产物，不纳入版本管理。可在项目根目录执行：

```bash
scripts/build-dist.sh
```

重新生成全部部署包。模板文件（启动脚本、包内 README）位于 `build/deploy/`，详见 `build/deploy/README.md`。

## 部署包列表

- **onelife-server-linux-x64.tar.gz** (4.3M) — 服务端部署包
- **onelife-client-linux-x64.tar.gz** (66M) — 客户端部署包

## 快速部署指南

### 1. 部署服务端

```bash
# 解压
tar xzf onelife-server-linux-x64.tar.gz
cd server-linux-x64

# 修改连接密码（必须）
echo "你的密码" > settings/clientPassword.ini

# 启动服务端
./start-server.sh

# 验证（应返回 SN 消息和玩家数）
echo "" | nc 127.0.0.1 8005
```

服务端启动后会监听 8005 端口，首次启动需要加载约 4400 个游戏对象，耗时约 1-2 分钟。

### 2. 部署客户端

```bash
# 解压
tar xzf onelife-client-linux-x64.tar.gz
cd client-linux-x64

# 安装运行时依赖（Ubuntu/Debian）
sudo apt install libsdl1.2debian libgl1 libglu1-mesa libasound2 libpulse0

# 配置服务器地址
echo "服务器IP" > bin/settings/customServerAddress.ini
echo "8005"      > bin/settings/customServerPort.ini
echo "你的密码"  > bin/settings/serverPassword.ini

# 启动客户端
./start-client.sh
```

**注意**：客户端的 `serverPassword.ini` 必须与服务端的 `clientPassword.ini` 一致。

## 目录结构

### 服务端 (server-linux-x64/)
```
├── bin/              # 二进制 + 文本资源
├── data/             # 游戏数据（objects/transitions/categories/animations/tutorialMaps）
├── settings/         # 配置文件（.ini）
├── start-server.sh   # 启动脚本
├── stop-server.sh    # 停止脚本
└── README.md
```

### 客户端 (client-linux-x64/)
```
├── bin/
│   ├── OneLife       # 客户端二进制
│   ├── graphics/     # UI 资源
│   ├── settings/     # 配置
│   └── [游戏数据]    # objects/sprites/sounds/music 等
├── start-client.sh
└── README.md
```

## 配置说明

### 服务端关键配置 (settings/)

| 文件 | 默认值 | 说明 |
|------|--------|------|
| port.ini | 8005 | 监听端口 |
| clientPassword.ini | testPassword | 连接密码 |
| requireClientPassword.ini | 1 | 启用密码认证 |
| requireTicketServerCheck.ini | 0 | 关闭官方 ticket 验证（私服必须） |
| maxPlayers.ini | 160 | 最大玩家数 |

### 客户端关键配置 (bin/settings/)

| 文件 | 默认值 | 说明 |
|------|--------|------|
| useCustomServer.ini | 1 | 使用自定义服务器 |
| customServerAddress.ini | 127.0.0.1 | 服务器地址 |
| customServerPort.ini | 8005 | 服务器端口 |
| serverPassword.ini | testPassword | 连接密码 |
| tutorialDone.ini | 1 | 跳过教程 |

## 系统要求

### 服务端
- Linux x86_64
- glibc 2.31+（Ubuntu 20.04+ / Debian 11+）
- 无额外依赖（纯 C++ 标准库）

### 客户端
- Linux x86_64
- glibc 2.31+
- SDL 1.2、OpenGL、音频库（PulseAudio/ALSA）

## 验证部署

服务端启动后，使用 nc 验证：
```bash
echo "" | nc 127.0.0.1 8005
```

应返回类似：
```
SN
0/160
A57A3A135DA5D1EB201390553692BED1C49C98471
435
#
```

- `SN` — 服务器名称消息
- `0/160` — 当前玩家数/最大玩家数
- `435` — 数据版本号

## 版本信息

- 服务端代码版本：409
- 数据版本：435
- 游戏对象数：4431 个（含自动生成的变体约 10800 个）

## 注意事项

1. **密码安全**：部署前务必修改默认密码 `testPassword`
2. **防火墙**：确保服务端端口（默认 8005）对客户端可访问
3. **首次启动**：服务端首次启动会生成地图种子，耗时 1-2 分钟
4. **运行时产物**：服务端运行后会在 `runtime/` 下生成 `*.db`、`*Log/`、`log.txt` 等文件；备份世界只需 `tar czf backup.tar.gz runtime/`，重置世界 `rm -rf runtime/` 即可
5. **教程模式**：客户端默认跳过教程，如需体验教程可将 `tutorialDone.ini` 设为 0

## 停止服务

```bash
# 服务端
cd server-linux-x64
./stop-server.sh

# 客户端
直接关闭窗口或按 Ctrl+C
```
