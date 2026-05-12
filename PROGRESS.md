# OneLife Android 客户端开发进度

> 最后更新：2025-05-12 19:30

## 当前状态

**Phase 2 已完成** ✅ - gameSource 完整集成，APK 可启动并加载所有资源

### 关键指标

- **APK 大小**: 76MB（代码 7MB + 资源 69MB）
- **构建时间**: 35 秒（冷启动完整构建，本地硬盘加速）
- **游戏启动**: "OneLife client v437 (binV=436, dataV=437) starting up"
- **资源加载**: 0 个 Failed/ERROR/CRITICAL 错误
- **渲染**: 60 FPS 稳定，EGL + GLES 1.x 正常工作

### 最近完成的工作

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
c5d4ff65 build(android): 支持本地硬盘构建目录，加速 17x
9ff247d4 fix(android): 实现 readTGAFile 手动解析器 + graphics 资源
9c9dfa6f build(android): 接入 AndroidLog 到 CMakeLists
```

---

## 下一步：Phase 3 - 触摸输入与交互

**目标**: 实现完整可玩的游戏操作，包括移动、拾取、放下、聊天

### Task 3.1 - TouchInput（推荐优先）⭐

**为什么先做这个**:
- 这是从"能跑"到"能玩"的关键一步
- 解锁后续所有功能测试（登录、音频、网络都需要触摸交互）
- 相对独立，风险可控
- 快速看到成果，提升开发体验

**实现计划**:
1. 在 `android_main.cpp` 中接入 `AMotionEvent`
   - `android_app->onInputEvent` 回调
   - 解析触摸坐标（x, y）
   - 转换到游戏坐标系
2. 调用 gameSource 的触摸回调
   - `platformTouchDown(x, y)`
   - `platformTouchMove(x, y)`
   - `platformTouchUp(x, y)`
3. 验证
   - logcat 看到触摸事件
   - 点击 UI 按钮有响应

**预计时间**: 1-2 小时

### Task 3.2 - 软键盘（输入账号密码）

### Task 3.3 - DPI 适配（640×320 太小）

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
│       └── game_stubs.cpp - 手动 TGA 解析器 ✅
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
- [ ] 触摸屏幕有响应（Task 3.1）
- [ ] 能进入登录界面（Task 3.2-3.4）
- [ ] 能连接服务器（Phase 4）

---

## 已知问题与限制

### 当前限制
- **无触摸输入**: 游戏启动但无法交互（Phase 3 待实现）
- **屏幕太小**: 640×320 固定分辨率，UI 元素过小（Task 3.3 DPI 适配）
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
