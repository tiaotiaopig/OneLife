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
- `default_settings` 中 `skipFPSMeasure=1` 是 Android 必需的（否则登录按钮不显示）

## 真机调试经验

### 字体渲染问题（已解决）

**症状**：真机上所有文字显示为白色方块，而不是清晰字母。

**根本原因**：Android 端的 EGL 初始化后没有启用 OpenGL 的 alpha 混合（`GL_BLEND`），导致字体纹理的 alpha 通道被忽略。

OneLife 字体渲染机制（Font.cpp）：
1. 将字体 TGA 的 R 通道作为 alpha：`spriteRGBA[i].comp.a = spriteRGBA[i].comp.r`
2. 将所有像素设为白色：`RGB = 255, 255, 255`
3. 最终字符纹理：字符像素 `RGBA=(255,255,255,255)`，背景像素 `RGBA=(255,255,255,0)`

**关键点**：背景像素的 alpha=0 需要 `GL_BLEND` 才能正确渲染为透明。没有 blend 时，背景也显示为不透明白色，整个字符方块变成白色方块。

**修复**：在 `android_main.cpp` 的 EGL 初始化后添加（与桌面端 ScreenGL_SDL.cpp 一致）：
```cpp
glEnable(GL_BLEND);
glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
```

**排查教训**：
1. 不要凭文件头部字节猜测整个文件内容（TGA 是 bottom-up 存储，头部是底部边界）
2. 诊断要看完整数据，不要看局部（局部可能是边界标记，不是实际内容）
3. 多模态模型应该直接看截图，不要用像素统计推测（统计只能区分"黑屏 vs 有内容"）
4. OpenGL 状态初始化要完整（GL_BLEND 是字体渲染的必需设置）
5. 优先验证桌面端的实际行为（桌面端用 TGAImageConverter 做标准 BGR→RGB 转换）
