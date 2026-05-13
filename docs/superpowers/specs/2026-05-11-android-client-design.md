# OneLife Android 客户端设计

## 背景

OneLife（One Hour One Life）目前提供 Linux/macOS/Windows 桌面客户端，通过 `gameSource/` 中的 SDL 1.2 + OpenGL 实现，并依赖同级仓库 `minorGems` 提供的跨平台基础设施。游戏运行依赖独立的 `OneLifeServer`（端口 8005，ASCII 文本协议），客户端与服务端在协议层解耦良好。

在用户的当前需求下，需要把客户端移植到 Android 平台。考虑两种主要路线：

- **方案 A：Android 客户端 + Linux 服务端**——只移植客户端，服务端仍由 Linux 主机提供。
- **方案 B：完全离线版**——把服务端 33K 行代码裁剪并嵌入 APK，去除网络层，本地直连。

经过可行性评估：

- 方案 A 仅涉及客户端图形/输入/资源加载等模块改造，工作量约 4-5 周，风险低，保留多人游戏完整功能，服务端零改动。
- 方案 B 需要重构 server.cpp 网络层，重写消息回环，处理地图数据库大小、内存压力、调试复杂度等问题，工作量约 11-16 周，且会牺牲多人功能。

用户已确认选择方案 A。本设计聚焦在客户端的 Android 移植，并为未来"嵌入式服务端"离线模式预留扩展点。

## 目标

- 提供一个独立的 Android APK 构建通道，能在 Android 10+（API 29）设备上运行
- 客户端通过 TCP 8005 端口连接局域网/远端 Linux `OneLifeServer`，复用现有 `protocol.txt` 协议
- 复用 `gameSource/` 和 `minorGems/` 中绝大多数桌面代码，桌面端构建不受影响
- 资源（sprites/sounds/objects 等）打包进 APK，安装即可玩
- 触摸事件透明映射到现有"鼠标事件"模型，复用 `LivingLifePage.cpp` 既有交互逻辑
- 为未来嵌入式服务端方案预留 `GameBackend` 抽象，当前仅实现 `NetworkBackend`

## 非目标

以下内容不在本次设计范围内：

- 不实现嵌入式服务端（EmbeddedBackend），仅留接口
- 不修改服务端代码（`server/` 目录、协议、数据库等一律不动）
- 不引入 SDL2 或其他大型跨平台库
- 不支持 iOS 移植
- 不支持低端设备（<4GB RAM）的特殊优化
- 不设计虚拟摇杆/动作按钮等触屏专属 UI，仅做鼠标事件模拟
- 不接入内购、广告、统计、Google Play 发布流程

## 核心约束

| 项目 | 决定 |
|---|---|
| 最低 Android 版本 | API 29（Android 10+） |
| 目标 ABI | arm64-v8a + armeabi-v7a + x86_64（模拟器调试用） |
| 开发环境 | 纯命令行 + Android NDK + CMake，不依赖 Android Studio |
| 屏幕方向 | 横屏锁定（`sensorLandscape`） |
| 平台抽象层 | 原生 NDK API（EGL/OpenGL ES 1.x/AAssetManager/OpenSL ES），无 SDL2 |
| 入口形式 | NativeActivity（`android_native_app_glue`），无需 Java/Kotlin 代码 |
| 触摸交互 | 单指=左键，长按=标记右键状态，双指=标记 Shift 状态，拖拽=pointerDrag |
| 资源打包 | sprites/sounds/objects 等全部打入 APK assets |
| `minorGems` 修改 | 独立 `android-port` 分支（主仓库内，不用 worktree） |
| 桌面端 | 通过 `#ifdef __ANDROID__` 隔离 Android 代码，桌面端构建不受影响 |

## 总体架构

```
┌──────────────────────────────────────────────┐
│           Android APK                        │
│  ┌────────────────────────────────────────┐  │
│  │ NativeActivity                         │  │
│  │   android_native_app_glue → C++ 入口   │  │
│  └────────────────┬───────────────────────┘  │
│  ┌────────────────▼───────────────────────┐  │
│  │ Native 层 (libonelife.so)              │  │
│  │   ├ minorGems Android 平台层（新增）   │  │
│  │   │    EGL / OpenGL ES / Asset / SL   │  │
│  │   ├ gameSource（条件编译复用）         │  │
│  │   │    LivingLifePage / spriteBank…   │  │
│  │   └ minorGems 基础设施（复用 Unix）    │  │
│  │        pthread / POSIX socket / 文件  │  │
│  └────────────────────────────────────────┘  │
│  assets/  (sprites / sounds / objects)       │
└────────────────────┬─────────────────────────┘
                     │ TCP/IP 8005
                     │ protocol.txt
┌────────────────────▼─────────────────────────┐
│   Linux OneLifeServer（不改动）              │
└──────────────────────────────────────────────┘
```

## 模块拆解

### 1. Android 工程骨架（`OneLife/android/`）

新增独立目录，包含 Android 专属构建文件，与桌面端 `gameSource/` 解耦。

```
android/
├── AndroidManifest.xml          # API 29 + 横屏 + INTERNET + debuggable + NativeActivity
├── CMakeLists.txt               # 主构建脚本（gameSource + minorGems + JNI 胶水）
├── build.sh                     # cmake → aapt → zipalign → apksigner 一键脚本
├── config.sh                    # NDK/SDK 路径 + ABI 列表 + 本地硬盘加速
├── test_settings.sh             # 开发者辅助：配置测试服务端地址（避免改 default_settings）
├── CLAUDE.md                    # 构建/运行/架构指南
├── jni/
│   ├── android_main.cpp         # NativeActivity 入口 + EGL 初始化 + 主循环
│   ├── AndroidPlatform.cpp/.h   # 全局平台句柄（AAssetManager、内部路径）
│   ├── EGLContext.cpp/.h        # 备用 EGL 包装类（当前主循环未使用，保留给未来重构）
│   ├── AssetFileBridge.cpp/.h   # AAssetManager 注入 + 首次启动复制 default_settings
│   ├── TouchInputAdapter.cpp/.h # AInputEvent → pointerDown/Move/Drag/Up + KEY → keyDown/Up
│   ├── SoftKeyboard.cpp/.h      # ANativeActivity_showSoftInput/hide 包装
│   └── game_stubs.cpp           # GL 投影矩阵 (redoDrawMatrix) + screenToWorld
│                                  + socket API 包装 + gameSDL.cpp 中未被其他模块覆盖的函数
├── res/                          # 图标等 Android 资源
└── assets/                       # 软链接到 ../../OneLifeData7 子目录 + default_settings 目录
```

注：`OpenSLAudio` 不在 `android/jni/` 下。音频后端 `OpenSLAudioBackend.cpp` 属于 minorGems 平台层职责，位于 `minorGems/sound/android/`。

`EGLContext.cpp/.h` 提供 `EGLContextWrapper` 类（Phase 1 早期实现），但 `android_main.cpp` 中另有独立的 `initEGL/termEGL` 静态函数，主循环实际使用后者。两套代码并存；未来重构时可合并为单一实现。

### 2. minorGems Android 平台分支

在 `../minorGems` 仓库的 `android-port` 分支添加 Android 实现（独立分支，非 worktree）：

| 路径 | 内容 |
|---|---|
| `minorGems/graphics/openGL/glInclude.h` | 增加 `__ANDROID__` 分支，与 `RASPBIAN` 共用同一路径（包含 `<GLES/gl.h>`，不包含 `<GL/glu.h>`——NDK 不提供） |
| `minorGems/graphics/openGL/glu_android.h` | GLU 函数最简替代（gluPerspective 等，基于 glFrustumf） |
| `minorGems/io/file/android/FileAndroid.cpp` | AAssetManager 管理 + `minorGemsAndroid_readAsset` 回退读取 + 对无斜杠文件名自动尝试 `graphics/` 前缀 |
| `minorGems/io/file/File.h/.cpp` | `exists()` / `readFileContents()` 在 stat/fopen 失败时回退 AAsset |
| `minorGems/sound/android/OpenSLAudioBackend.cpp` | OpenSL ES 实现 minorGems 音频接口（已接入未测试） |
| `minorGems/game/platforms/Android/gameAndroid.cpp` | 替代 `gameSDL.cpp`：platformInit/Tick/Shutdown + 4 个 socket API（与 `game_stubs.cpp` 的 C++ 包装并存，后者实际生效） |
| `minorGems/util/log/AndroidLog.cpp` | AppLog → Android logcat 转发（Tag: OneLifeGame） |

POSIX socket、pthread、Time、Path 等直接复用 `unix/` 和 `linux/` 子目录下的实现，Android NDK 完全兼容。

EGL 初始化**不在 minorGems 中**，而在 `OneLife/android/jni/android_main.cpp`（NativeActivity 直接持有 ANativeWindow*）。

### 3. gameSource 兼容性改造

`gameSource/` 中只做条件编译式改造，桌面端代码完全不受影响：

| 文件 | 改造内容 |
|---|---|
| `gameSource/TextField.cpp` | `#ifdef __ANDROID__` 在 focus/unfocus 时调用 SoftKeyboard::show/hide |
| `gameSource/spriteBank.cpp` / `soundBank.cpp` | 资源路径通过 minorGems File 抽象层透明替换（File::readFileContents 有 AAsset 回退） |
| `gameSource/LivingLifePage.cpp` | 输入事件层无需改造（触摸已在 JNI 层翻译为 pointerDown/Move/Drag/Up） |

**未实现的设计项**：
- `GameBackend.h` / `NetworkBackend.cpp` 抽象层未实现。实际 socket 调用通过 `game_stubs.cpp` 中的 4 个函数（openSocketConnection/sendToSocket/readFromSocket/closeSocket）直接包装 minorGems SocketClient，无需额外抽象。
- `gameSource/game.cpp` 未做 Android 改造。`android_main()` 在 `android/jni/android_main.cpp` 中独立实现，直接调用 minorGems 的 platformInit/Tick/Shutdown。

桌面端功能完全保持不变（无 GameBackend 重构）。

### 4. 触摸输入策略

JNI 层 `TouchInputAdapter` 将 `AInputEvent` 翻译为 minorGems 期望的鼠标/键盘事件：

| 触摸动作 | 游戏事件 | 实现细节 |
|---|---|---|
| 单指 down | `pointerDown(x, y)` | 记录起始位置和时间戳 |
| 单指 move <24px | `pointerMove(x, y)` | 未超过 `kTapSlopPx` 阈值 |
| 单指 move ≥24px | `pointerDrag(x, y)` | 超过阈值后持续触发 drag |
| 单指 up | `pointerUp(x, y)` | 检查长按状态，设置 `gLastButtonRight` |
| 长按 ≥500ms 原地 up | `pointerUp` 时 `isLastMouseButtonRight()` 返回 true | 不发送独立右键事件，通过状态查询接口暴露 |
| 双指按下 | `isShiftKeyDown()` 返回 true | 第二指按下时设置 `gShiftDown=true`，任一指抬起时清除 |
| 软键盘输入 | `keyDown(ascii)` / `keyUp(ascii)` | AKEYCODE → ASCII 映射（A-Z/0-9/符号 + Shift） |

修饰键状态通过 `isShiftKeyDown()` / `isLastMouseButtonRight()` 查询接口暴露给 gameSource，不是事件注入。

聊天虚拟键盘使用 NDK 的 `ANativeActivity_showSoftInput()`（无需 JNI 或 Java 代码），由 `TextField::focus/unfocus` 通过 `#ifdef __ANDROID__` 调用。

### 5. 资源与配置

- 编译期：`android/assets/` 软链接到 `../../OneLifeData7` 子目录（sprites/sounds/objects/transitions/categories/animations/tutorialMaps/contentSettings 等），打包时 `aapt` 把它们打入 APK
- 运行期：通过 AAssetManager 读取
- 设置文件：首次启动从 `assets/default_settings/` 拷贝到内部存储 `/data/data/com.fengli.onelife/files/settings/`，供 SettingsManager 读写
- 关键设置：`customServerAddress.ini`、`customServerPort.ini`、`serverPassword.ini`

### 6. 网络与协议

- 复用 `minorGems/network/linux/` 下的 POSIX TCP 实现，Android NDK 完全兼容
- `AndroidManifest.xml` 声明 `<uses-permission android:name="android.permission.INTERNET" />`
- 协议格式（`protocol.txt`）和客户端协议层无任何改动
- 通过 GameBackend 接口接入，方便未来切换实现

## 实施阶段总览

| 阶段 | 目标 | 关键产物 | 估时 |
|---|---|---|---|
| P0 | 环境与骨架 | NDK 验证、最简 NativeActivity APK 跑通 | 3 天 |
| P1 | minorGems Android 平台层 | 图形/音频/文件 I/O 完成，加载并渲染一张 sprite | 1 周 |
| P2 | 客户端集成 | gameSource 在 Android 编译并启动到主菜单 | 5 天 |
| P3 | 触摸输入 | 完整可玩流程，连接/移动/拾取/聊天 | 1 周 |
| P4 | 联调 | 与 Linux 服务端联调通过，多人同步验证 | 3 天 |
| P5 | 打磨 | 性能/电池/屏幕适配/错误处理 | 5 天 |
| 合计 | — | — | 约 4-5 周 |

详细任务分解在配套实施计划 `docs/superpowers/plans/2026-05-11-android-client-plan.md` 中给出。

## 验证策略

### 阶段性验证

- **P0**：`adb install` APK 后看到横屏纯色窗口，logcat 无致命错误
- **P1**：测试程序加载 sprite TGA 并显示（验证 EGL + OpenGL ES + AAsset）
- **P2**：APK 启动到设置/主菜单页（验证 gameSource 完整编译）
- **P3**：手动完成出生→移动→拾取→死亡的完整生命周期
- **P4**：两台 Android + 桌面客户端在同一服务器互相可见

### 回归验证

- 桌面端 server 构建：`cd server && ./configure 1 && make`
- 桌面端 client 构建：`cd gameSource && cd .. && ./configure 1 && cd gameSource && make`
- 桌面端 GameBackend 抽象重构后行为一致（连接、游戏、断线重连）

### 端到端验证示例

```bash
# 服务端
cd /jfs/fengli16/Projects/CProjects/OneLife/server
./OneLifeServer   # 监听 8005

# Android 客户端
adb install android/build/OneLife-debug.apk
# 启动后填入 settings/customServerAddress.ini 等
# 验证完整游戏流程
```

## 风险与缓解

| 风险 | 缓解措施 | 状态 |
|---|---|---|
| OpenGL 固定管线（glBegin/glEnd）在 GLES 不支持 | **已通过 GLES 1.x + 复用 Raspbian 的 `#ifdef GLES` 分支化解**：`SpriteGL`/`PrimitiveGL` 等都已有 vertex array 实现 | ✅ 已验证 |
| ANR：渲染线程阻塞主循环 | `ALooper_pollAll` 动态超时（非暂停时 0，暂停时 -1），确保事件响应 | ✅ 已实现 |
| 内存：sprites 加载 OOM | 验证 4GB RAM 设备，必要时改为按需/分块加载 | ⏳ 待真机验证 |
| AAssetManager 不支持 mmap | 对大文件用流式读取，避免一次性全量读入 | ✅ 已实现（File::readFileContents） |
| OpenSL ES 在 API 30+ 已 deprecated | API 29 仍支持；后续可平滑迁移到 AAudio | ⏳ 已接入未测试 |
| `minorGems` 同级仓库被污染 | 使用独立 `android-port` 分支（主仓库内，`git checkout android-port`） | ✅ 已实现 |

## 关键决策记录

1. **选 A 不选 B**：方案 A 节省 7-11 周，保留多人功能，服务端零改动
2. **NDK 原生 API 而非 SDL2**：避免 SDL 1.2 → 2.0 大规模迁移，APK 更小
3. **NativeActivity 而非 Java 入口**：符合"纯命令行 + NDK"偏好
4. **资源全部打包进 APK**：用户体验最佳，无首次下载等待
5. **不实现 GameBackend 抽象**：gameSource 的 socket 调用通过 game.h 声明的 4 个函数统一入口（openSocketConnection/sendToSocket/readFromSocket/closeSocket），在 game_stubs.cpp 中直接包装 SocketClient 即可工作，无需额外抽象层。未来实现离线模式时可重新评估是否需要抽象。
6. **minorGems 用分支切换而非 worktree**：worktree 在多机同步时路径不稳定（`../minorGems-android-port` 可能不存在），改为在主仓库 `../minorGems` 内切换到 `android-port` 分支，构建前执行 `git checkout android-port` 即可。

## 后续阶段（不在本期）

- AAudio 替换 OpenSL ES（API 30+ 性能更佳）
- 嵌入式服务端（真正的离线模式）
- 触屏专属 UI（虚拟摇杆、动作按钮）
- iOS 移植
- Google Play 发布流程

---

## 实施结果（2026-05-13 补充）

P0-P4 已完成，P5 部分完成（后台暂停 + 非阻塞连接）。以下记录实际实现与原设计的差异。

### 设计偏差

| 原设计 | 实际实现 | 原因 |
|--------|----------|------|
| GameBackend 抽象 + NetworkBackend 重构 LivingLifePage 30 处调用 | 未实现，直接在 game_stubs.cpp 中提供 socket API（openSocketConnection/sendToSocket/readFromSocket/closeSocket） | gameSource 的 socket 调用通过 game.h 声明的 4 个函数统一入口，无需额外抽象层即可工作 |
| minorGems 用 worktree 隔离 | 改为普通分支切换（`git checkout android-port`） | worktree 在多机同步时路径不稳定，分支切换更简单 |
| 触摸长按 → 转换为右键 down 事件 | 长按 → pointerUp 时置 isLastMouseButtonRight=true | gameSource 在 pointerUp 时检查 isLastMouseButtonRight，不需要额外的 right-down 事件 |
| OpenSLAudio 在 OneLife/android/jni/ | OpenSLAudioBackend 在 minorGems/sound/android/ | 音频后端属于 minorGems 平台层职责 |
| DPI 适配通过 initFrameDrawer 传逻辑分辨率 | 由 game_stubs.cpp 的 redoDrawMatrix() 设置 GL 正交投影处理 | gameSource 自身通过 setViewSize/setLetterbox 控制投影，直接实现这些函数即可 |

### 额外发现的问题及修复

| 问题 | 根因 | 修复 |
|------|------|------|
| UI 全黑 | setViewSize/setLetterbox/setViewCenterPosition 是空 stub，GL 投影矩阵从未设置 | 实现 redoDrawMatrix()（参考 gameSDL.cpp） |
| 登录按钮不显示 | ExistingAccountPage 的 FPS 测量在 targetFrameRate 未设置时永远失败 | default_settings 加 skipFPSMeasure=1 + targetFrameRate=60 |
| Socket 连接失败 | readFromSocket 在非阻塞连接未完成时返回 -1（错误）而非 0（暂无数据） | 未连接时返回 0 |
| screenToWorld 坐标偏移 | viewSize 被 gameSource 调整为 1440（屏幕比 16:9 更宽时 viewWidth 会拉宽） | 正确实现 screenToWorld 的 mouseWorldCoordinates=true 分支 |

### 最终文件结构（与原设计对比）

原设计中的 `AndroidPlatform.cpp/.h` 简化为仅保存全局 `android_app*` 指针；`EGLContext.cpp/.h` 的功能内联到 `android_main.cpp`（initEGL/termEGL 静态函数）。新增了原设计未预见的 `game_stubs.cpp`（~30 个 gameSDL.cpp 中由 ScreenGL 提供的函数在 Android 上需要独立实现）。

