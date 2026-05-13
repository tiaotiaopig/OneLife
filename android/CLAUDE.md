# Android 客户端

本目录包含 OneLife Android 客户端的构建系统和 JNI 胶水层。

## 前置条件

- Android NDK r25c
- Android SDK（API 29 + build-tools 33.0.2）
- minorGems 仓库切到 `android-port` 分支：`cd ../minorGems && git checkout android-port`
- OneLifeData7 位于 `../../OneLifeData7`

## 构建

```bash
cd android
./build.sh debug    # ~35 秒（本地硬盘），产出 build/OneLife-debug.apk (76MB)
```

`config.sh` 自动检测 `/data1/` 可写时将构建目录设到本地硬盘（避免 JuiceFS 小文件 I/O 瓶颈）。

## 安装与运行

```bash
source config.sh
ADB="$ANDROID_SDK_ROOT/platform-tools/adb"
$ADB install -r build/OneLife-debug.apk
$ADB shell am start -n com.fengli.onelife/android.app.NativeActivity
$ADB logcat -s 'OneLife:V' 'OneLifeGame:V'
```

## 配置测试服务端

首次安装后运行 `test_settings.sh` 配置连接到本地服务端：

```bash
./test_settings.sh 10.0.2.2 8005   # 模拟器用 10.0.2.2 访问宿主机
```

或手动（需先启动一次 APP 让 settings 目录创建）：

```bash
$ADB shell "run-as com.fengli.onelife sh -c 'echo 1 > /data/data/com.fengli.onelife/files/settings/useCustomServer.ini'"
$ADB shell "run-as com.fengli.onelife sh -c 'echo 10.0.2.2 > /data/data/com.fengli.onelife/files/settings/customServerAddress.ini'"
```

## 架构

```
android/
├── AndroidManifest.xml       # NativeActivity + 横屏 + INTERNET + debuggable
├── CMakeLists.txt            # 组合 minorGems + gameSource + JNI 模块
├── build.sh / config.sh      # 一键构建 + 环境配置
├── jni/
│   ├── android_main.cpp      # NativeActivity 入口，EGL 初始化，主循环
│   ├── TouchInputAdapter.*   # AInputEvent → pointerDown/Move/Drag/Up + keyDown/Up
│   ├── SoftKeyboard.*        # ANativeActivity_showSoftInput/hide
│   ├── EGLContext.*          # EGL Display/Context/Surface 管理
│   ├── AssetFileBridge.*     # AAssetManager 注入 + 首次启动复制默认设置
│   ├── AndroidPlatform.*     # 全局平台句柄
│   └── game_stubs.cpp        # GL 投影矩阵 + screenToWorld + socket API + 其他 stub
├── assets/
│   ├── default_settings/     # Android 专用默认设置（skipFPSMeasure=1 等）
│   ├── sprites → OneLifeData7/sprites  (软链接)
│   └── ...
├── res/                      # 占位图标
└── test_settings.sh          # 快速配置测试服务端
```

## 触摸映射

| 手势 | 游戏事件 |
|------|----------|
| 单指 tap | pointerDown + pointerUp（左键） |
| 单指拖动 >24px | pointerDrag |
| 长按 ≥500ms 原地 | pointerUp 时 isLastMouseButtonRight=true |
| 双指按下 | isShiftKeyDown=true |

## 关键实现细节

- GL 投影：`game_stubs.cpp` 中的 `redoDrawMatrix()` 参考 gameSDL.cpp 实现 glOrthof + glViewport
- 坐标映射：`screenToWorld` 实现 mouseWorldCoordinates=true 分支（物理像素→世界坐标）
- Socket：`openSocketConnection` 用 timeout=0 非阻塞连接；`readFromSocket` 在连接未完成时返回 0（非 -1）
- 后台暂停：APP_CMD_PAUSE 时主循环用 ALooper_pollAll(-1) 阻塞，不消耗 CPU
- 软键盘：TextField::focus/unfocus 通过 `#ifdef __ANDROID__` 调用 show/hide

## 已知限制

- `game_stubs.cpp` 中仍有 ~20 个空 stub（setDrawColor/drawSprite 等渲染函数由 gameSource 自身实现，stub 是历史遗留的未使用函数）
- 音频（OpenSL ES）已接入但未验证
- 真机 DPI 适配未验证（模拟器 640×320 正常）
- `default_settings` 中 `skipFPSMeasure=1` 是 Android 必需的（否则登录按钮不显示）
