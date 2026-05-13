# OneLife Android 客户端开发进度

> 最后更新：2026-05-13 11:30

## 当前状态

**Phase 3 进行中** 🔄 - Task 3.1/3.2/3.3 已完成

### 关键指标

- **APK 大小**: 76MB（代码 7MB + 资源 69MB）
- **构建时间**: 35 秒（冷启动完整构建，本地硬盘加速）
- **游戏启动**: "OneLife client v437 (binV=436, dataV=437) starting up"
- **资源加载**: 0 个 Failed/ERROR/CRITICAL 错误
- **渲染**: 60 FPS 稳定，EGL + GLES 1.x 正常工作
- **逻辑分辨率**: 物理 640×320 → 逻辑 1440×720（按 OneLife 1280×720 + 物理比例校正）
- **触摸**: tap / drag / 长按 / 双指 Shift 均正确识别，映射到逻辑坐标
- **软键盘**: TextField 焦点变化自动 show/hide，键盘输入映射到 keyDown/keyUp

### 最近完成的工作

#### Phase 3.3 - DPI 适配（2026-05-13）
- **问题**: platformInit 直接把 EGL 物理像素（640×320）传给 initFrameDrawer，
  导致 gameSource 把 UI 当 640×320 绘制；若换成 1080p 真机 UI 又会过小
- **方案**:
  - `gameAndroid.cpp` 的 `platformInit` 调用 `doesOverrideGameImageSize()` 查
    gameSource 期望的逻辑分辨率（OneLife 是 1280×720）
  - 按物理屏幕宽高比调整逻辑尺寸避免拉伸（640×320 → 逻辑 1440×720）
  - 对外暴露 `getPhysicalWidth/Height`、`getLogicalWidth/Height`
  - `screenToWorld` stub 按比例线性映射物理→逻辑
- **效果**: logcat 看到 `platformInit physical=640x320 logical=1440x720`，
  60 FPS 稳定，触摸坐标在 gameSource 侧是逻辑空间

#### Phase 3.2 - 软键盘 + KEY 事件映射（2026-05-13）
- **目标**: TextField 焦点变化自动弹键盘，键盘输入送到 gameSource
- **实现**:
  - 新增 `android/jni/SoftKeyboard.{h,cpp}`：包装 `ANativeActivity_showSoftInput/hide`
  - `android_main.cpp` 保存全局 `android_app*` 并通过 `onelifeAndroidGetApp()` 暴露
  - `TouchInputAdapter::handleKey` 把 AKEYCODE 映射到 ASCII（A-Z/0-9/常用符号 + Shift）
  - `TextField::focus/unfocus` `#ifdef __ANDROID__` 调软键盘接口
- **效果**: APK 76MB 构建成功，部署到模拟器启动正常

#### Phase 3.1 - TouchInputAdapter（2026-05-13）
- **目标**: 让 gameSource 的 pointerDown/Move/Drag/Up 拿到触摸事件
- **实现**:
  - 新增 `android/jni/TouchInputAdapter.{h,cpp}`：AInputEvent → 鼠标事件翻译
  - 单指 tap = 左键 / 单指 >24px 移动 = pointerDrag / 长按 ≥500ms 原地松手 = 右键 / 双指 = Shift
  - `extern "C"` 暴露 4 个状态访问器给 game_stubs.cpp
  - `android_main` 通过 lambda 挂 `onInputEvent`
  - game_stubs.cpp 里 `isShiftKeyDown` / `isLastMouseButtonRight` / `getLastMouseScreenPos` 改为读取真实状态
- **效果**: `adb input tap/swipe` + 长按在 logcat 里都能看到对应的 up (x,y) dur=... drag=0/1 right=0/1 shift=0/1；gameSource 收到 pointer 回调无 crash

#### Phase 2.3 - 构建性能优化（2025-05-12）
- **问题**: JuiceFS 上 aapt 打包 ~20000 个小资源文件 I/O 极慢（冷启动构建需 10 分钟）
- **方案**: 
  - config.sh 自动检测 `/data1/` 可写时设置 ANDROID_BUILD_DIR 到本地硬盘
  - build.sh 支持 ANDROID_BUILD_DIR 环境变量覆盖
  - OneLifeData7 资源同步到 `/data1/fengli16/Projects/CProjects/OneLifeData7`
  - assets 软链接改为绝对路径指向本地硬盘资源
- **效果**: 冷启动完整构建从 10 分钟降到 35 秒（**17× 加速**）

#### Phase 2.2 - graphics 资源加载修复（2025-05-12）
- **问题**: FileInputStream 无法读取 APK assets 中的 TGA 文件（fopen 限制）
- **方案**: 实现 readTGAFile/readTGAFileBase 手动 TGA 解析器
  - 绕过 FileInputStream，直接用 File::readFileContents()（有 AAsset 回退）
  - 手动解析 TGA 头和像素数据（支持 RGB/RGBA，自顶向下/自底向上）
- **效果**: 所有 UI 图标（checkMark、font 等 75 个文件）成功从 APK assets 加载

#### Phase 2.1 - minorGems File.h AAsset 支持（2025-05-12）
- **问题**: minorGems File::exists() 和 File::readFileContents() 无法检测/读取 APK assets
- **方案**:
  - File::exists() 在 stat 失败后尝试 AAsset 检查
  - File::readFileContents() 优先尝试物理文件，失败后回退 AAsset
  - 把 minorGemsAndroid_readAsset extern "C" 声明移到 class File 之前
- **效果**: minorGems File 类可以透明地从 APK assets/ 读取资源文件

#### Phase 2.0 - AndroidLog 实现（2025-05-12）
- 实现 AndroidLog 类，转发 AppLog 到 logcat
- 所有游戏日志（"OneLife client v437 starting up"、"tickGame #1"等）可见

### 最近 3 个提交

```
5d152d71 feat(android): screenToWorld 物理像素→逻辑坐标映射
e451075d feat(android): 软键盘支持 + KEY 事件映射
2c6fc219 feat(android): TouchInputAdapter 触摸→鼠标事件适配
```

---

## 下一步：Phase 3 剩余

**目标**: 实现完整可玩的游戏操作，包括移动、拾取、放下、聊天

### Task 3.4 - P3 完整流程验证 ⭐

- 配置 `customServerAddress.ini` 等指向本地 Linux 服务端
- 启动客户端，点击进入登录界面
- 软键盘输入账号密码
- 验证登录成功、角色出生、行走、拾取
- 如发现 bug 记录在 `android/PHASE4-BUGS.md` 并逐个修复

### Task 3.4 - P3 验证（能进入登录界面并输入）

---

## 阶段总览

| 阶段 | 状态 | 关键产物 |
|---|---|---|
| **Phase 0** | ✅ 完成 | NDK 工具链、最简 NativeActivity APK |
| **Phase 1** | ✅ 完成 | minorGems Android 平台层（EGL/GLES/AAsset/OpenSL） |
| **Phase 2** | ✅ 完成 | gameSource 集成、资源加载、构建优化 |
| **Phase 3** | 🔄 进行中 | 触摸输入、软键盘、DPI 适配 |
| **Phase 4** | ⏳ 待开始 | 与 Linux 服务端联调 |
| **Phase 5** | ⏳ 待开始 | 性能优化、电池、屏幕适配 |

---

## 技术架构

### 已实现的关键模块

```
OneLife Android APK (76MB)
├── libonelife.so (7MB)
│   ├── gameSource (68 个核心文件)
│   │   ├── LivingLifePage.cpp - 游戏主逻辑
│   │   ├── game.cpp - 主循环
│   │   └── spriteBank/soundBank - 资源管理
│   ├── minorGems-android-port
│   │   ├── File.h - AAsset 回退支持 ✅
│   │   ├── AndroidLog - logcat 转发 ✅
│   │   ├── gameAndroid.cpp - 平台主循环 ✅
│   │   └── OpenSLAudioBackend - 音频后端（已接入未测试）
│   └── android/jni
│       ├── android_main.cpp - NativeActivity 入口 ✅
│       ├── EGLContext - EGL 管理 ✅
│       ├── AssetFileBridge - AAsset 注入 ✅
│       ├── TouchInputAdapter - 触摸/键盘事件 ✅
│       ├── SoftKeyboard - 软键盘控制 ✅
│       └── game_stubs.cpp - stub + 坐标映射 ✅
└── assets/ (69MB)
    ├── sprites/ (19,865 个资源文件) ✅
    ├── sounds/
    ├── objects/
    └── graphics/ (75 个 UI 图标) ✅
```

### 待实现的模块

```
android/jni/
├── TouchInputAdapter.cpp - 触摸→鼠标事件映射 ⏳
└── SoftKeyboard.cpp - 软键盘控制 ⏳
```

---

## 构建与运行

### 快速构建（本地硬盘加速）

```bash
cd /data1/fengli16/Projects/CProjects/OneLife/android
./build.sh debug  # 35 秒完成
```

### 安装与测试

```bash
source config.sh
ADB="$ANDROID_SDK_ROOT/platform-tools/adb"
$ADB install -r build/OneLife-debug.apk
$ADB shell am start -n com.fengli.onelife/android.app.NativeActivity
$ADB logcat -s 'OneLifeGame:V' -t 50
```

### 验证清单

- [x] APK 能安装并启动
- [x] 显示绿色 EGL 窗口（640×320）
- [x] logcat 看到 "OneLife client v437 starting up"
- [x] 所有 UI 图标加载成功（0 个 Failed 错误）
- [x] 60 FPS 稳定渲染
- [x] 触摸屏幕有响应（Task 3.1）
- [x] 逻辑分辨率切到 1440×720 + 坐标映射（Task 3.3）
- [x] TextField 焦点触发软键盘（Task 3.2，接口已通，实际流程 Task 3.4 验证）
- [ ] 能进入登录界面并输入账号密码（Task 3.4）
- [ ] 能连接服务器（Phase 4）

---

## 已知问题与限制

### 当前限制
- **UI 精准点击受限**: 640×320 下按钮过小，难以精准命中（Task 3.3 DPI 适配）
- **无软键盘**: 还无法输入账号密码（Task 3.2）
- **音频未测试**: OpenSL 后端已接入但未验证
- **网络未测试**: socket API 已实现但未连接服务器

### 技术债务
- `game_stubs.cpp` 中有 ~30 个 stub 函数待实现
- 部分 minorGems 函数返回硬编码值（如 `getScreenSizeX()` 返回 640）

---

## 相关文档

- **详细计划**: `docs/superpowers/plans/2026-05-11-android-client-plan.md`
- **设计规格**: `docs/superpowers/specs/2026-05-11-android-client-design.md`
- **构建配置**: `android/config.sh`
- **构建脚本**: `android/build.sh`
- **CMake 配置**: `android/CMakeLists.txt`

---

## 开发环境

- **工作目录**: `/data1/fengli16/Projects/CProjects/OneLife`（本地硬盘，编译快）
- **源码同步**: 从 `/jfs/fengli16/Projects/CProjects/OneLife` rsync
- **依赖仓库**:
  - minorGems-android-port: `/jfs/fengli16/Projects/CProjects/minorGems-android-port`
  - OneLifeData7: `/data1/fengli16/Projects/CProjects/OneLifeData7`
- **构建输出**: `/data1/build/OneLife-android/debug/`
- **NDK**: `$ANDROID_NDK_ROOT` (android-ndk-r25c)
- **SDK**: `$ANDROID_SDK_ROOT` (API 29, build-tools 33.0.2)
