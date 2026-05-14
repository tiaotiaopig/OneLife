# macmini 本地真机调试方案

> 把构建/调试环境搬到 macmini 本地后，可以用 Claude Code 全自动驱动真机调试循环。
> 本文档记录搬迁前后需要做的所有准备和操作。

---

## 为什么搬到 macmini

当前在 Docker 容器（`172.17.0.6`）跑构建和服务端，存在两个问题：

1. **网络隔离**：手机连同 WiFi 也访问不到容器 IP，必须做端口转发或内网穿透
2. **调试链路长**：手机 → 物理电脑 → 转发 → 容器，问题难定位

搬到 macmini 后：

- macmini 直接在路由器分配的局域网（`192.168.x.x`）
- 手机连同 WiFi 直接访问
- USB 接 macmini 后 adb 直连真机，Claude Code 可以直接 `adb logcat / install / shell`
- 服务端和客户端 logcat 都在同一台机器，便于关联分析

---

## Claude Code 在 macmini 本地能做什么

### ✅ 可以全自动

1. **完整调试循环**：改代码 → 构建 → `adb install` → 启动 → 抓 logcat → 分析错误 → 改代码（无人值守）
2. **自动化测试脚本**：构建 → 安装 → 启动 → 30 秒抓启动日志 → 自动判断成功/失败
3. **截图对比**：`adb exec-out screencap -p > screen.png`，我可以读图
4. **UI 自动化**：`adb shell input tap/swipe/text/keyevent`
5. **服务端 + 客户端联合诊断**：同时盯两边日志
6. **代码迭代**：直接改 git 仓库、提交、推送（如果你授权）

### ⚠️ 必须你手动操作的

| 事项 | 原因 | 解决 |
|------|------|------|
| 首次插 USB 后点"允许此电脑调试" | Android 安全机制 | 点一次，勾选"始终允许"后免触发 |
| 解锁手机屏幕 | 锁屏时 adb 部分功能受限 | 设置 → 开发者选项 → "充电时不锁屏"打开 |
| 物理操作（拔插线、转向） | 没有手 | 大部分用 adb 软件模拟 |
| 看屏幕实物 | 没有摄像头 | 让我用 `screencap` 截图 |
| 系统级敏感弹窗（安装未知来源） | 安全限制 | 点一次即可 |

---

## 搬迁前准备（在当前 Docker 容器执行）

### 1. 推送两个仓库的所有分支

保证 macmini 能 `git clone` 到完整代码：

```bash
cd /data1/fengli16/Projects/CProjects/OneLife
git push origin android-client

cd /data1/fengli16/Projects/CProjects/minorGems
git push origin android-port
```

确认两个分支都已经在远程：

```bash
git ls-remote origin | grep -E "android-client|android-port"
```

### 2. 确认 OneLifeData7 同步方案

OneLifeData7 体积大（数十 GB sprite/sound/object），不适合 git。两种选择：

**A. rsync 直接拷过去**（推荐）
```bash
rsync -avh --progress \
  /data1/fengli16/Projects/CProjects/OneLifeData7/ \
  user@macmini.local:~/Projects/CProjects/OneLifeData7/
```

**B. 让 macmini 从原始仓库拉**
```bash
# 在 macmini 上：
git clone https://github.com/jasonrohrer/OneLifeData7 ~/Projects/CProjects/OneLifeData7
# 注意 OneLifeData7 是真正的游戏数据，可能在私有仓库或者需要从 OneLife wiki 拿
```

---

## macmini 上的环境准备

### 1. 命令行工具

```bash
# 如未装过 Xcode
xcode-select --install

# Homebrew（如未装）
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

### 2. Android 工具链

```bash
# adb / aapt 等
brew install --cask android-platform-tools

# NDK 不在 brew 里，去官方下
# https://developer.android.com/ndk/downloads → 选 macOS .dmg 或 .zip
# 推荐 r25c（与当前 Linux 配置一致）
# 解压后放到 ~/android-ndk-r25c

# build-tools / platform-tools / android.jar
# 简单方式：装 Android Studio（图形化），或：
brew install --cask android-commandlinetools
yes | sdkmanager "platforms;android-29" "build-tools;33.0.2"
```

### 3. 环境变量（写到 `~/.zshrc`）

```bash
export ANDROID_NDK_ROOT="$HOME/android-ndk-r25c"
export ANDROID_SDK_ROOT="$HOME/Library/Android/sdk"   # 或你实际的路径
export PATH="$ANDROID_SDK_ROOT/platform-tools:$PATH"
```

```bash
source ~/.zshrc
adb --version            # 验证
ls $ANDROID_NDK_ROOT     # 验证
ls $ANDROID_SDK_ROOT/platforms/android-29/android.jar
```

### 4. clone 代码

```bash
mkdir -p ~/Projects/CProjects && cd ~/Projects/CProjects

git clone <你的-OneLife-fork>.git OneLife
git clone <你的-minorGems-fork>.git minorGems
# OneLifeData7 见上面 rsync

# 切到 Android 分支
cd OneLife && git checkout android-client
cd ../minorGems && git checkout android-port
```

### 5. 服务端编译（macOS）

```bash
cd ~/Projects/CProjects/OneLife/server
./configure 1     # 选 macOS 平台
make
```

minorGems 在 macOS 上一般能直接编（POSIX 兼容），如果有错告诉我。

### 6. 真机准备

- USB 数据线（注意：要支持数据传输的，不是纯充电线）
- 手机：设置 → 关于手机 → 连按 7 次「版本号」开发者选项
- 开发者选项 → **USB 调试** 打开
- 开发者选项 → **充电时不锁屏** 打开（避免反复输密码）
- 插 USB → 手机弹"允许此电脑调试" → 勾选"始终允许" → 确定

---

## 启动调试（macmini 本地 Claude Code 会话）

打开 Claude Code，给我说一句：

> macmini 本地，仓库在 `~/Projects/CProjects/OneLife`（android-client 分支）和 `~/Projects/CProjects/minorGems`（android-port 分支），手机 USB 已连接，开始 Android 16 黑屏调试。

我会按以下顺序执行：

### Phase 1: 环境验证（约 5 分钟）

```bash
adb devices                    # 看到手机
adb shell getprop ro.build.version.release  # Android 版本
adb shell getprop ro.build.version.sdk      # API level
adb shell getprop ro.product.cpu.abi        # arm64-v8a
adb shell getprop ro.build.version.opporom  # ColorOS 版本
```

如果环境变量没配好，我会引导你修。

### Phase 2: 适配 macOS 构建（约 10 分钟）

当前 `android/config.sh` 是为 Linux 写的，可能需要调整：

- NDK / SDK 路径
- `ANDROID_BUILD_DIR` 默认走 `/data1/`（macOS 没有），需要改
- `keytool` 路径（macOS 自带 JDK 应该 ok）

我会改完后跑一次 `./build.sh debug` 验证能出 APK。

### Phase 3: 抓黑屏 logcat（约 5 分钟）

```bash
adb install -r build/OneLife-debug.apk
adb logcat -c
adb shell am start -n com.fengli.onelife/android.app.NativeActivity
sleep 15
adb logcat -d > debug-android16.log
```

### Phase 4: 定位并修复（迭代）

根据 logcat 关键字定位：

- `eglChooseConfig: no configs` → GLES 1.x 不支持，改 EGL 配置
- `eglCreateContext failed` → 同上
- `unable to load library` → .so 加载问题，改 `extractNativeLibs`
- `SecurityException` → 权限问题
- `Fatal signal 11` → native crash，看 stacktrace
- 其它 → 实际看到再分析

每修一处：构建 → 装 → 启动 → 抓日志 → 验证。

### Phase 5: 真机端到端跑通

- 进登录界面
- 配 `customServerAddress` = macmini 局域网 IP
- 启动本地服务端（如未启）
- 点登录 → 进入游戏世界
- 验证移动 / 拾取 / 音频
- 截图记录效果

---

## Android 16 黑屏的预案

OnePlus Ace2 Pro 上 "Android 16" 一般是 **ColorOS 16**（基于 Android 15，API 35）。需要确认下面命令的输出：

```bash
adb shell getprop ro.build.version.release  # 期望 14 或 15
adb shell getprop ro.build.version.sdk      # 期望 34 或 35
```

按概率排序的可能原因 + 对应修复：

### 1. GLES 1.x 不被新系统接受（最高嫌疑）

**症状**：`eglChooseConfig` 返回 0 个 config，或 `eglCreateContext` 失败

**修复**：
- 修改 `android_main.cpp` 的 `EGL_RENDERABLE_TYPE`，从 `EGL_OPENGL_ES_BIT` 改为 `EGL_OPENGL_ES_BIT | EGL_OPENGL_ES2_BIT`
- 系统会优先给 ES2 context，但 GLES 1.x API 在大多数厂商驱动上仍可工作（向下兼容）
- 极端情况要重写部分渲染调用

### 2. .so 解压问题（中等嫌疑）

**症状**：`couldn't find "libonelife.so"` 或 `cannot locate symbol`

**修复**：`AndroidManifest.xml` 加 `android:extractNativeLibs="true"`

### 3. targetSdk=29 在 Android 14+ 受限（低嫌疑）

**症状**：启动后立即闪退，logcat 有 `INSTALL_FAILED_DEPRECATED_SDK_VERSION` 或运行时被限制

**修复**：升级 `targetSdk` 到 34，对应需要补一些权限声明

### 4. NativeActivity 在 ColorOS 16 上有特殊限制（很低嫌疑）

**症状**：`Activity ComponentInfo{...} did not call finish()` 或者直接被系统杀

**修复**：写一个最小 Java 包装 Activity 启动 NativeActivity（绕过限制）

---

## 备份方案：手机抓不到 logcat 时

如果手机连不上 adb（罕见），有两个备用：

### A. 写日志到 sdcard

改 `android_main.cpp` 把 LOGI 同时写文件：

```cpp
FILE* fp = fopen("/sdcard/onelife.log", "a");
if (fp) { fprintf(fp, ...); fclose(fp); }
```

需要在 `AndroidManifest.xml` 加 `WRITE_EXTERNAL_STORAGE` 权限。然后用文件管理器导出 `/sdcard/onelife.log` 给我。

### B. 屏幕显示日志

在 `drawFrameFallback` 上画 `glClearColor` 用不同颜色编码错误：
- 红色 = EGL 失败
- 黄色 = 资源加载失败
- 蓝色 = 进入主循环但 platformInit 没返回
- 绿色 = 一切正常但 gameSource 没绘制

这样光看屏幕颜色就能定位大致问题。

---

## 完成调试后的归档

问题修好后，我会：

1. 把所有修复 commit 到 `android-client` 分支
2. 更新 `IMPLEMENTATION_STATUS.md` 标记 Android 14+ / ColorOS 兼容性已验证
3. 在 `REAL_DEVICE_GUIDE.md` 补充真机实测信息（性能、UI 实际尺寸、音频效果）
4. 推送到远程分支

你只需要 `git pull` 就能在其他设备上获得验证过的代码。

---

## 检查清单

搬迁前（当前 Docker）：

- [ ] `git push origin android-client`（OneLife）
- [ ] `git push origin android-port`（minorGems）
- [ ] OneLifeData7 同步方案确定（rsync 还是重新 clone）
- [ ] 把当前 APK（`build/OneLife-release.apk`）拷到 macmini 一份作为基线

macmini 准备：

- [ ] Xcode CLT（`xcode-select --install`）
- [ ] Homebrew + adb
- [ ] NDK r25c 解压到 `~/android-ndk-r25c`
- [ ] SDK platforms;android-29 + build-tools;33.0.2
- [ ] `~/.zshrc` 配 `ANDROID_NDK_ROOT` / `ANDROID_SDK_ROOT`
- [ ] clone OneLife（android-client 分支）
- [ ] clone minorGems（android-port 分支）
- [ ] OneLifeData7 同步完成
- [ ] 服务端编译：`cd server && ./configure 1 && make`

真机准备：

- [ ] USB 数据线
- [ ] 开发者选项 + USB 调试 已开
- [ ] "充电时不锁屏" 已开
- [ ] USB 连接后已点"允许此电脑调试"+"始终允许"
- [ ] `adb devices` 在 macmini 上能看到手机

开始调试：

- [ ] 给 Claude Code 说："macmini 本地，开始 Android 16 黑屏调试"
