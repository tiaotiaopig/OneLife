# OneLife Android 客户端开发进度

> 最后更新：2026-05-13 13:42

## 当前状态

**Phase 4 完成** ✅ - 成功连接 Linux 服务端并进入游戏世界，交互正常

### 关键指标

- **APK 大小**: 76MB（代码 7MB + 资源 69MB）
- **构建时间**: 35 秒（冷启动完整构建，本地硬盘加速）
- **游戏启动**: "OneLife client v437 (binV=436, dataV=437) starting up"
- **资源加载**: 0 个 Failed/ERROR/CRITICAL 错误
- **渲染**: 60 FPS 稳定，EGL + GLES 1.x 正常工作
- **UI**: 登录界面 + 游戏世界正常显示
- **触摸**: 点击移动、长按交互均正常
- **软键盘**: TextField 焦点变化自动 show/hide，能输入邮箱/密码
- **网络**: TCP 连接成功，进入游戏世界，能看到地面纹理和游戏元素
- **GL 投影**: setViewSize/setLetterbox/screenToWorld 完整实现
- **触摸**: tap / drag / 长按 / 双指 Shift 均正确识别，screenToWorld 映射到世界坐标
- **软键盘**: TextField 焦点变化自动 show/hide，键盘输入映射到 keyDown/keyUp

### 最近完成的工作

#### Phase 4 - 服务端联调（2026-05-13）
- **问题 1**: 登录按钮不显示 —— `skipFPSMeasure=0` + `targetFrameRate` 未设置导致
  FPS 测量永远失败
- **问题 2**: Socket 连接失败 —— `readFromSocket` 在非阻塞连接未完成时返回 -1（错误）
  而非 0（暂无数据）；`openSocketConnection` 的 timeout=0 非阻塞模式不可靠
- **问题 3**: `useCustomServer=0`（默认设置未配置服务端地址）
- **修复**:
  - `readFromSocket` 未连接时返回 0
  - `openSocketConnection` 改用 5 秒阻塞连接
  - `default_settings` 加入 skipFPSMeasure=1, targetFrameRate=60, useCustomServer=1 等
- **效果**: TCP 连接成功，进入游戏世界，点击移动和长按交互正常

#### Phase 3.4 - GL 投影修复（2026-05-13）
- **问题**: UI 全黑 —— game_stubs.cpp 中 setViewSize/setLetterbox/setViewCenterPosition
  都是空 stub，GL 投影矩阵从未被设置（默认 identity -1~1），gameSource 绘制的
  所有内容（坐标在 -640~640 范围）都在屏幕外
- **方案**:
  - 实现 `redoDrawMatrix()`：参考 gameSDL.cpp，设置 glOrthof 正交投影 + glViewport
  - `setViewSize/setLetterbox/setViewCenterPosition` 保存状态并调用 redoDrawMatrix
  - `screenToWorld` 实现 mouseWorldCoordinates=true 分支（物理像素→世界坐标）
  - 回退 Task 3.3 的逻辑分辨率方案（改由正交投影处理）
  - AndroidManifest 加 `android:debuggable="true"`
- **效果**: UI 正常显示，登录界面可见，按钮可点击，文字可输入，网络连通

#### Phase 3.1-3.2 - TouchInputAdapter + 软键盘（2026-05-13）
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
63953152 fix(android): 实现 GL 正交投影矩阵 + screenToWorld 世界坐标
e451075d feat(android): 软键盘支持 + KEY 事件映射
2c6fc219 feat(android): TouchInputAdapter 触摸→鼠标事件适配
```

---

## 下一步：Phase 5 - 打磨

**目标**: 性能优化、电池、屏幕适配、错误处理

### 优先事项
1. **后台暂停**：APP_CMD_PAUSE 时停止渲染和音频，节省电池
2. **DPI 适配**：真机上 1080p+ 屏幕的 UI 缩放（当前 640×320 模拟器上 OK，真机需验证）
3. **音频验证**：OpenSL ES 后端已接入但未测试
4. **错误处理**：网络断线提示、重连逻辑

### 已知问题
- `openSocketConnection` 使用 5 秒阻塞连接（会冻结 UI），需改为后台线程
- `game_stubs.cpp` 中仍有 ~20 个空 stub（大部分是渲染相关，不影响核心游戏流程）
- 模拟器分辨率 640×320 较低，真机上需验证 UI 缩放效果

---

## 阶段总览

| 阶段 | 状态 | 关键产物 |
|---|---|---|
| **Phase 0** | ✅ 完成 | NDK 工具链、最简 NativeActivity APK |
| **Phase 1** | ✅ 完成 | minorGems Android 平台层（EGL/GLES/AAsset/OpenSL） |
| **Phase 2** | ✅ 完成 | gameSource 集成、资源加载、构建优化 |
| **Phase 3** | ✅ 完成 | 触摸输入、软键盘、GL 投影、坐标映射 |
| **Phase 4** | ✅ 完成 | 与 Linux 服务端联调，进入游戏世界 |
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
│   ├── minorGems (android-port 分支)
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

## 分支管理

### 策略：独立分支 + 定期 merge 上游

OneLife 是开源项目（原作者 Jason Rohrer 持续更新），Android 适配作为独立分支维护，
定期 merge 上游 master 获取新功能和 bug 修复。

| 仓库 | 上游分支 | Android 分支 | 冲突面 |
|------|----------|-------------|--------|
| OneLife | `master` | `android-client` | 极小（95% 改动在 `android/` 新目录） |
| minorGems | `master` | `android-port` | 小（新增文件为主，仅 glInclude.h + File.h 有改动） |

### 同步上游更新

```bash
# OneLife
git checkout master
git pull origin master          # 拉取上游更新
git checkout android-client
git merge master                # 合入上游更新

# minorGems
cd ../minorGems
git checkout android-port
git fetch origin master
git merge origin/master         # 合入上游更新
```

### 冲突风险点

- `gameSource/TextField.cpp`：5 行 `#ifdef __ANDROID__` 改动
- `minorGems/graphics/openGL/glInclude.h`：增加 `__ANDROID__` 分支
- `minorGems/io/file/File.h`：AAsset 回退逻辑

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
  - minorGems: `/data1/fengli16/Projects/CProjects/minorGems`（切到 `android-port` 分支）
  - OneLifeData7: `/data1/fengli16/Projects/CProjects/OneLifeData7`
- **构建输出**: `/data1/build/OneLife-android/debug/`
- **NDK**: `$ANDROID_NDK_ROOT` (android-ndk-r25c)
- **SDK**: `$ANDROID_SDK_ROOT` (API 29, build-tools 33.0.2)
