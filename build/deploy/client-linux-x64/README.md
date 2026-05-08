# OneLife 客户端部署包 (Linux x64)

## 目录结构

```
client-linux-x64/
├── bin/
│   ├── OneLife           # 客户端二进制
│   ├── graphics/         # UI 图形资源
│   ├── languages/        # 语言文件
│   ├── settings/         # 客户端配置
│   ├── objects/          # 游戏对象数据
│   ├── transitions/      # 合成配方
│   ├── categories/       # 分类数据
│   ├── animations/       # 动画数据
│   ├── sprites/          # 精灵图
│   ├── sounds/           # 音效
│   ├── music/            # 音乐
│   ├── ground/           # 地面纹理
│   ├── faces/            # 面部资源
│   ├── scenes/           # 场景
│   └── overlays/         # 覆盖层
├── start-client.sh       # 启动脚本
└── README.md
```

## 系统要求

- Linux x86_64
- glibc 2.31+
- SDL 1.2（`apt install libsdl1.2debian`）
- OpenGL 支持（显卡驱动）
- PulseAudio 或 ALSA（音频）

安装运行时依赖（Ubuntu/Debian）：

```bash
sudo apt install libsdl1.2debian libgl1 libglu1-mesa libasound2 libpulse0
```

## 快速启动

```bash
# 1. 解压
tar xzf onelife-client-linux-x64.tar.gz
cd client-linux-x64

# 2. 修改服务器地址（默认 127.0.0.1:8005）
echo "服务器IP" > bin/settings/customServerAddress.ini
echo "端口"     > bin/settings/customServerPort.ini
echo "密码"     > bin/settings/serverPassword.ini

# 3. 启动
./start-client.sh
```

## 配置说明

配置位于 `bin/settings/`，每个 `.ini` 文件存储一个值。

| 文件 | 默认值 | 说明 |
|------|--------|------|
| useCustomServer.ini | 1 | 使用自定义服务器 |
| customServerAddress.ini | 127.0.0.1 | 服务器地址 |
| customServerPort.ini | 8005 | 服务器端口 |
| serverPassword.ini | testPassword | 连接密码（需与服务端 clientPassword.ini 一致） |
| tutorialDone.ini | 1 | 跳过教程（设 0 可体验教程） |
| fullscreen.ini | 0 | 全屏模式 |
| musicOff.ini | 0 | 关闭音乐 |
