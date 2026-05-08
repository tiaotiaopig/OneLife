# OneLife 服务端部署包 (Linux x64)

## 目录结构

```
server-linux-x64/
├── bin/              # 二进制 + 名字表等文本资源
│   └── OneLifeServer
├── data/             # 游戏数据（objects/transitions/categories/animations/tutorialMaps/contentSettings）
├── settings/         # 服务端配置（.ini 文件）
├── runtime/          # 运行时产物（首次启动后自动创建，含 .db / log）
├── start-server.sh   # 启动脚本
├── stop-server.sh    # 停止脚本
└── README.md
```

## 快速部署

```bash
# 1. 解压到目标目录
tar xzf onelife-server-linux-x64.tar.gz
cd server-linux-x64

# 2. 修改密码（必须）
echo "你的密码" > settings/clientPassword.ini

# 3. 启动
./start-server.sh

# 4. 验证（应返回 SN 消息）
echo "" | nc 127.0.0.1 8005
```

## 配置说明

所有配置位于 `settings/` 目录，每个 `.ini` 文件存储一个值。

| 文件 | 默认值 | 说明 |
|------|--------|------|
| port.ini | 8005 | 监听端口 |
| clientPassword.ini | testPassword | 客户端连接密码 |
| requireClientPassword.ini | 1 | 是否要求密码 |
| requireTicketServerCheck.ini | 0 | ticket 验证（私服设为 0） |
| maxPlayers.ini | 160 | 最大玩家数 |

## 运行时产物

服务端运行后会在 `runtime/` 下生成：
- `*.db` — 地图、玩家等数据库文件
- `*Log/` — 各类日志目录
- `log.txt` — 主日志

备份世界存档只需：`tar czf backup.tar.gz runtime/`
重置世界：`rm -rf runtime/`

## 停止服务

```bash
./stop-server.sh
```

## 系统要求

- Linux x86_64
- glibc 2.31+（Ubuntu 20.04+ / Debian 11+）
- 无额外依赖
