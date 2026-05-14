# 真机联调指南

> 把 OneLife Android APK 装到真实手机上，连到本机服务端进行联调。
> 适用对象：开发/测试。最终发布流程见 `Phase 5.5: Google Play`（待办）。

## 总体步骤

```
┌─────────┐         ┌─────────┐         ┌──────────────┐
│ 真机    │  WiFi   │ 路由器  │  局域网 │ 服务端宿主机 │
│ APK     │ ──────► │         │ ──────► │ OneLifeServer│
└─────────┘         └─────────┘         └──────────────┘
     ▲                                          │
     │ adb install                              │
     └────────────  USB / WiFi adb  ────────────┘
                  （开发电脑）
```

本文档假设你有**三台设备**或角色：

1. **构建机**（当前 Linux 容器，`172.17.0.6`）—— 编译 APK、跑服务端
2. **真机**（Android 10+，arm64-v8a，安装 APK 玩游戏）
3. **物理电脑**（可选，用于 USB adb 连接真机）—— 如果构建机直接能连真机也可省略

---

## Step 1. 选择网络拓扑

手机要能访问服务端的 8005 端口。根据你的环境选一个方案：

### 方案 A. 构建机有真实局域网 IP（最简单）

如果 `ip -4 addr` 看到 `192.168.x.x` 或 `10.x.x.x`（路由器分配的内网地址）：

- 手机连同一 WiFi
- 客户端 `customServerAddress` 填这个 IP
- 跳到 Step 2

### 方案 B. 构建机在 Docker / 虚拟机内（本项目当前情况）

构建机 IP 是 `172.17.0.6`，手机无法直接访问。需要把服务端**通过宿主物理机暴露到局域网**。

选一种：

**B.1 端口转发（推荐，需要 root 物理机）**

在 Docker 宿主机上：
```bash
# 把宿主机 8005 端口转发到容器 8005
iptables -t nat -A PREROUTING -p tcp --dport 8005 -j DNAT \
    --to-destination 172.17.0.6:8005
iptables -A FORWARD -p tcp -d 172.17.0.6 --dport 8005 -j ACCEPT
```
或重启容器时加 `-p 8005:8005`。

手机连接到**宿主物理机的局域网 IP**，端口 8005。

**B.2 SSH 反向隧道（手机不通过路由器）**

如果手机端能通过 USB 连开发电脑，直接用 adb reverse 把手机的 `localhost:8005` 映射到构建机的 `8005`：

```bash
# 在能 ssh 到构建机的电脑上：
ssh -L 8005:172.17.0.6:8005 你的构建机
# 然后在该电脑上：
adb reverse tcp:8005 tcp:8005
```

手机客户端 `customServerAddress = 127.0.0.1`。

**B.3 frp / ngrok 内网穿透**

如果上面都不通且手机用蜂窝网络：
```bash
# frp 客户端（构建机）
./frpc -t tcp -P 8005 --proxy_name onelife --remote_port 18005
```
手机客户端填 frp 公网入口的 IP + 端口。

---

## Step 2. 服务端准备

### 启动服务端（如未启动）

```bash
cd /jfs/fengli16/Projects/CProjects/OneLife/dist/server-linux-x64
./bin/OneLifeServer
```

确认监听：
```bash
ss -tlnp | grep 8005
# 应看到 LISTEN ... 0.0.0.0:8005
```

### 服务端配置（关键）

确认 `settings/` 下：
```bash
cd /jfs/fengli16/Projects/CProjects/OneLife/dist/server-linux-x64/settings
cat port.ini                       # 8005
cat requireTicketServerCheck.ini   # 0（不验证 ticket，本地测试用）
cat requireClientPassword.ini      # 0 或 1
cat clientPassword.ini             # 若上面为 1，这里是密码
```

> **注意**：默认 `requireTicketServerCheck=0`，意味着任何客户端都能连入。仅供局域网测试用，**不要暴露到公网**。

### 验证服务端能响应

```bash
echo '' | nc -w 1 127.0.0.1 8005
# 期望输出：SN ... 数字 ... 一串十六进制
```

看到 `SN` 开头的握手消息说明服务端正常。

---

## Step 3. 构建 arm64-v8a 单架构 APK

```bash
cd /data1/fengli16/Projects/CProjects/OneLife/android
ANDROID_ABIS="arm64-v8a" ./build.sh release
# 输出：build/OneLife-release.apk (72MB)
```

> **为什么单架构？**
> - 默认三架构 APK 76MB（arm64 + armv7 + x86_64），其中 x86_64 仅模拟器需要
> - 单 arm64-v8a 72MB，下载/安装快 5%
> - 绝大多数 2018 年后的真机都是 arm64-v8a

如果你的手机是老机型或 32 位 ARM：
```bash
ANDROID_ABIS="arm64-v8a armeabi-v7a" ./build.sh release
```

### 验证 APK

```bash
source config.sh
"$AAPT" dump badging build/OneLife-release.apk | grep -E "package|native-code"
# package: name='com.fengli.onelife' ...
# native-code: 'arm64-v8a'
```

---

## Step 4. 把 APK 传到真机

选一种：

### 方法 4.A：USB + adb（推荐）

手机开**开发者选项 → USB 调试**，连 USB，开发电脑上：

```bash
adb devices
# 应看到你的手机序列号

adb install -r OneLife-release.apk
# 期望：Performing Streamed Install → Success
```

如果构建机不能直接连 USB，先把 APK 拷到开发电脑：
```bash
# 从构建机拷到本地
scp 构建机:/data1/fengli16/Projects/CProjects/OneLife/android/build/OneLife-release.apk .
```

### 方法 4.B：HTTP 分发

构建机上：
```bash
cd /data1/fengli16/Projects/CProjects/OneLife/android/build
python3 -m http.server 8000
```

手机浏览器访问 `http://构建机IP:8000/OneLife-release.apk` 下载（需 Step 1 方案 A 网络可达）。

下载后从手机文件管理器点击安装（系统会提示"未知来源"，需手动允许）。

### 方法 4.C：云盘 / 邮件

把 APK 上传网盘或发邮件给自己，手机下载安装。

---

## Step 5. 首次启动 + 配置服务端

### 5.1 首次启动

手机桌面找到 **OneLife** 图标（绿色方块占位图），点击。

第一次启动需要 5-10 秒（解压资源、加载 sprite bank），看到登录界面（黄色 LOGIN 按钮）即成功。

首次启动会自动生成 `/data/data/com.fengli.onelife/files/settings/` 目录，复制 77 个默认 `.ini` 文件。

### 5.2 配置服务端地址

**问题**：默认 `customServerAddress.ini` 是 `10.0.2.2`（Android 模拟器专用），真机需要改成实际服务端 IP。

**方案 A：USB adb 改配置**

```bash
# SERVER_IP 替换为实际服务端地址（见 Step 1）
SERVER_IP="192.168.1.10"

adb shell "run-as com.fengli.onelife sh -c 'echo $SERVER_IP > /data/data/com.fengli.onelife/files/settings/customServerAddress.ini'"
adb shell "run-as com.fengli.onelife sh -c 'echo 8005 > /data/data/com.fengli.onelife/files/settings/customServerPort.ini'"
adb shell "run-as com.fengli.onelife sh -c 'echo 1    > /data/data/com.fengli.onelife/files/settings/useCustomServer.ini'"

# 重启 app 应用配置
adb shell am force-stop com.fengli.onelife
adb shell am start -n com.fengli.onelife/android.app.NativeActivity
```

**方案 B：游戏内手动改（无 USB 时）**

登录界面 →（暂无服务器输入框，需要先支持 Settings 页面，目前不可行）→ 退而求其次用方案 A。

### 5.3 试登录

登录界面：
- **EMAIL** 字段：随便填如 `test@test.com`（local 服务端不验证）
- **KEY** 字段：随便填如 `ABCDEFGHIJKLMNO`（15 位）
- 点 **LOGIN** 按钮

成功后看到游戏世界（地面 + 角色）即联调通过。

---

## Step 6. 调试与日志

### 实时查看客户端日志

USB adb 连接时：
```bash
adb logcat -s 'OneLife:*' 'OneLifeGame:*' 'OneLifeAudio:*' 'OneLifeTouch:*' 'AndroidRuntime:E'
```

关键日志标签：
- `OneLife:I` — Android 平台层（启动诊断、socket、FPS）
- `OneLifeGame:*` — gameSource（来自 AppLog）
- `OneLifeAudio:*` — OpenSL ES 音频
- `OneLifeTouch:*` — 触摸事件
- `AndroidRuntime:E` — Java 层崩溃

### 启动诊断（应看到）

```
OneLife : =========================================
OneLife : OneLife Android 启动诊断
OneLife :   内核: Linux 4.x (aarch64)         ← 真机应是 aarch64
OneLife :   内存: 总 XXXMB, 可用 XXXMB
OneLife :   屏幕: WxHdp, density=XXX (xhdpi/xxhdpi/xxxhdpi)
OneLife :   方向: landscape
OneLife : =========================================
OneLife : EGL ready: 1920x1080  （或真机分辨率）
OneLife : OpenGL ES:
OneLife :   Vendor:   Qualcomm / ARM / ...
OneLife :   Renderer: Adreno / Mali / ...
OneLife : platformInit 1920x1080 @60fps
OneLifeGame: OneLife client v437 (binV=436, dataV=437) starting up
OneLifeAudio: ✓ OpenSL ES started: 44100 Hz stereo  ← 音频
OneLife : FPS: 59.8 (min 14.5ms, max 22.1ms)  ← 5 秒后
```

### Socket 连接（点击 LOGIN 后）

```
OneLife : openSocketConnection: 192.168.1.10:8005
```

服务端日志应该同时出现：
```
New connection from xxx, x slot used / N
```

### 触摸坐标（点击屏幕时）

```
OneLifeTouch: up (X,Y) dur=Nms drag=0 right=0 shift=0
```

---

## 故障排查

### "应用未响应" / 白屏 / 黑屏 5 秒以上

```bash
adb logcat -d -s 'OneLife:E' 'AndroidRuntime:E' -t 100
```

常见原因：
- **资源加载失败**：看 `OneLifeGame` 日志找 `Failed to open` 关键字
- **GL 初始化失败**：找 `eglInitialize failed` 或 `eglMakeCurrent failed`
- **崩溃**：找 `FATAL EXCEPTION` 或 `signal 11`，附 stacktrace

### 看到登录界面但点 LOGIN 无反应

检查触摸映射：
```bash
adb logcat -d -s 'OneLifeTouch:*'
```

应看到 `up (X,Y) ...`。如果 X/Y 坐标看起来不对（比如总是 0 或负数），可能是 GL viewport 没设好（参考 design spec 中关于 `redoDrawMatrix` 的说明）。

### 连接失败 / 服务端无新连接

```bash
# 1. 客户端日志看是否真的调用了 socket
adb logcat -d -s 'OneLife:*' | grep openSocketConnection
# 应有：openSocketConnection: <IP>:8005

# 2. 验证手机能 ping 通服务端
adb shell ping -c 3 <SERVER_IP>

# 3. 验证 8005 端口可达
adb shell "nc -w 2 <SERVER_IP> 8005 < /dev/null && echo OK"

# 4. 服务端是否在监听
ss -tlnp | grep 8005
```

### 音频静音

```bash
adb logcat -d -s 'OneLifeAudio:*'
```

- 看到 `✓ OpenSL ES started`：音频引擎正常，可能是游戏侧暂时无 BGM
- 看到 `audio disabled`：OpenSL ES 启动失败，游戏会静音运行但不影响玩
- 看到 `slCreateEngine failed (result=X)`：设备 OpenSL 实现有问题，记录 result 码上报

### 卡顿 / 帧率低

```bash
adb logcat -d -s 'OneLife:*' | grep FPS
```

- FPS < 30：性能问题，看 `max` 字段定位卡顿帧
- FPS = 60 但视觉卡顿：可能是 GL 同步问题

### UI 元素过小（高分屏常见）

启动日志应有警告：
```
OneLife W: 高分屏设备（xxhdpi+）：UI 元素可能偏小，建议后续实现 DPI 缩放
```

这是已知限制（Phase 5 的 DPI 自适应任务），目前不影响玩，但按钮触摸热区较小。临时方案：用手指肚而不是指尖点击。

---

## 关键限制

本次联调能跑通的功能：

- ✅ 启动到登录界面
- ✅ 软键盘输入 email/key
- ✅ 触摸点击进入游戏
- ✅ 移动 / 拾取 / 互动（点击 + 长按）
- ✅ 音频（OpenSL ES）
- ✅ 后台暂停省电

**暂不支持**：

- ❌ 自动重连（断网后需手动重启 app）
- ❌ Settings 页面（改服务器地址需 adb）
- ❌ DPI 自适应（高分屏 UI 偏小）
- ❌ 异形屏/刘海屏特殊适配
- ❌ 横竖屏切换（manifest 锁定横屏）
- ❌ 多人同房间真机互动（需要至少两台真机或一真机一桌面客户端）

详见 `android/IMPLEMENTATION_STATUS.md`。

---

## 联调检查清单

出发前确认：

- [ ] 构建机能跑通 `./build.sh release`（72MB APK）
- [ ] 服务端 8005 端口监听
- [ ] 选定了 Step 1 中的网络方案（A/B.1/B.2/B.3）
- [ ] 手机 Android 版本 ≥ 10
- [ ] 手机 CPU 是 arm64-v8a（设置 → 关于手机 → 处理器，或 `adb shell getprop ro.product.cpu.abi`）
- [ ] 准备好 USB 线（如方案 4.A）
- [ ] 手机已开启开发者选项 + USB 调试

出现问题时可立即查的日志位置：

- 启动诊断：`adb logcat -d -s 'OneLife:I' -t 50 | grep "启动诊断\|EGL ready\|OpenGL"`
- 性能：`adb logcat -d -s 'OneLife:I' | grep FPS | tail -5`
- 网络：`adb logcat -d -s 'OneLife:*' | grep -i socket`
- 崩溃：`adb logcat -d -s 'AndroidRuntime:E' -t 50`
