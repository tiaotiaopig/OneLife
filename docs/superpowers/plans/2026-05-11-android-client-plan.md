# OneLife Android 客户端实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 为 OneLife 桌面客户端补一条 Android 构建通道，能在 Android 10+ 设备上运行并连接 Linux 服务端，桌面端构建不受影响。

**Architecture:** 复用 `gameSource/` 与 `minorGems/`（特别是 Raspbian 的 GLES 1.x 适配分支），新增 `OneLife/android/` 作为 Android 工程目录与 JNI 胶水层；`minorGems` 改动通过 worktree 隔离到 `android-port` 分支；触摸事件在 JNI 层翻译为鼠标事件，业务逻辑无需感知；引入 `GameBackend` 抽象为未来嵌入式服务端铺路。

**Tech Stack:** Android NDK r25+、CMake 3.18+、NativeActivity（android_native_app_glue）、EGL、OpenGL ES 1.x、OpenSL ES、AAssetManager、aapt、apksigner、bash、git worktree

**注：** OneLife 项目以"脚本运行结果"和"端到端手测"为验证基准，没有现成的单元测试 Runner。下面 Task 中的"测试"指可独立运行/可观察的验证步骤，不一定是 xUnit 风格的单元测试。

---

## 前置说明

- 所有 OneLife 仓库内路径相对 `/jfs/fengli16/Projects/CProjects/OneLife`
- minorGems 仓库路径：`/jfs/fengli16/Projects/CProjects/minorGems`（worktree 后另增 `/jfs/fengli16/Projects/CProjects/minorGems-android-port`）
- 数据仓库路径：`/jfs/fengli16/Projects/CProjects/OneLifeData7`
- 参考规格：`docs/superpowers/specs/2026-05-11-android-client-design.md`
- 关键复用：
  - `minorGems/graphics/openGL/glInclude.h` —— 已有 RASPBIAN 的 GLES 1.x 分支
  - `minorGems/game/platforms/openGL/SpriteGL.cpp` —— 已有 `#ifdef GLES` 的 vertex array 实现
  - `minorGems/game/platforms/SDL/Makefile.Raspbian` —— GLES 编译参考
  - `minorGems/network/linux/SocketClientLinux.cpp` —— POSIX socket，Android 直接复用
  - `minorGems/system/unix/`、`minorGems/io/file/unix/`、`minorGems/system/linux/` —— pthread/文件 I/O，Android 直接复用

---

## 文件结构（最终态）

### `OneLife/android/` 目录

```
android/
├── AndroidManifest.xml              # NativeActivity 声明、API 29、横屏、INTERNET 权限
├── CMakeLists.txt                    # 主构建脚本，组合 minorGems + gameSource + JNI
├── build.sh                          # 编译 + 打包 + 签名一键脚本
├── config.sh                         # 路径变量（NDK_ROOT、ANDROID_API、ABI）
├── jni/
│   ├── android_main.cpp              # NativeActivity 入口（android_main 函数）
│   ├── AndroidPlatform.h/.cpp        # 全局平台句柄（AAssetManager、ANativeWindow、AAssetManager 路径）
│   ├── EGLContext.h/.cpp             # EGL Display/Context/Surface 管理
│   ├── AssetFileBridge.h/.cpp        # AAssetManager → minorGems File 桥接
│   ├── TouchInputAdapter.h/.cpp      # AInputEvent → minorGems pointerDown/Move/Up
│   └── OpenSLAudio.h/.cpp            # OpenSL ES 音频后端
├── res/
│   ├── values/strings.xml            # 应用名
│   └── mipmap-*/ic_launcher.png      # 图标（暂用占位）
└── assets/                            # 软链接到 OneLifeData7 子目录（编译期）
```

### `OneLife/gameSource/` 改动文件

```
gameSource/
├── GameBackend.h                     # 新增：抽象后端接口
├── NetworkBackend.h/.cpp             # 新增：当前唯一实现，包装 SocketClient
├── game.cpp                          # Modify: __ANDROID__ 分支提供 android_main
├── spriteBank.cpp                    # Modify: 资源加载通过 File 抽象（无破坏性）
├── soundBank.cpp                     # Modify: 同上
└── LivingLifePage.cpp                # Modify: SocketClient 调用替换为 GameBackend
```

### `minorGems-android-port/` 新增文件（在 worktree 内）

```
minorGems/
├── graphics/openGL/glInclude.h       # Modify: 增加 __ANDROID__ 分支
├── io/file/android/
│   ├── FileAndroid.cpp               # 新增：AAsset 读 + 内部存储写
│   └── DirectoryAndroid.cpp          # 新增：内部存储目录操作
├── sound/android/
│   └── OpenSLAudioBackend.cpp        # 新增：OpenSL ES 实现
└── game/platforms/Android/
    ├── gameAndroid.cpp               # 新增：替代 gameSDL.cpp 的主循环
    └── README.md                     # 说明该平台分支
```

---

## 执行顺序

阶段顺序：**Phase 0** → **Phase 1** → **Phase 2** → **Phase 3** → **Phase 4** → **Phase 5**。每个阶段结束都应有可验证产物，并提交一次或多次到 git。

---

# Phase 0：环境准备

**目标**：本机 NDK 工具链就绪，最简 NativeActivity APK 能装到设备并显示纯色窗口。

---

### Task 0.1: 确认/安装 Android NDK

**Files:**（无源码改动，仅环境准备）

- [ ] **Step 1: 检查现有 NDK**

```bash
which ndk-build
ls $ANDROID_NDK_ROOT 2>/dev/null
ls /opt/android-ndk* 2>/dev/null
ls ~/Android 2>/dev/null
```

如果都没有，继续 Step 2 安装；如已存在 NDK r25 及以上版本，直接跳到 Step 4。

- [ ] **Step 2: 下载并安装 NDK r25c**

```bash
cd ~
mkdir -p android-tools && cd android-tools
# 通过本机代理下载（用户 CLAUDE.md 规定）
export https_proxy=http://127.0.0.1:7890 http_proxy=http://127.0.0.1:7890
wget https://dl.google.com/android/repository/android-ndk-r25c-linux.zip
unzip -q android-ndk-r25c-linux.zip
unset https_proxy http_proxy
```

- [ ] **Step 3: 安装 build-tools（aapt/apksigner）**

```bash
cd ~/android-tools
export https_proxy=http://127.0.0.1:7890 http_proxy=http://127.0.0.1:7890
wget https://dl.google.com/android/repository/commandlinetools-linux-9477386_latest.zip
unzip -q commandlinetools-linux-9477386_latest.zip
mkdir -p android-sdk/cmdline-tools/latest
mv cmdline-tools/* android-sdk/cmdline-tools/latest/
yes | android-sdk/cmdline-tools/latest/bin/sdkmanager --licenses
android-sdk/cmdline-tools/latest/bin/sdkmanager "platforms;android-29" "build-tools;33.0.2"
unset https_proxy http_proxy
```

- [ ] **Step 4: 把工具链路径写入 `android/config.sh`**

创建文件 `/jfs/fengli16/Projects/CProjects/OneLife/android/config.sh`：

```bash
#!/bin/bash
# OneLife Android 构建配置——按本机实际路径调整
export ANDROID_NDK_ROOT="${ANDROID_NDK_ROOT:-$HOME/android-tools/android-ndk-r25c}"
export ANDROID_SDK_ROOT="${ANDROID_SDK_ROOT:-$HOME/android-tools/android-sdk}"
export ANDROID_API_LEVEL=29
export ANDROID_ABIS="arm64-v8a armeabi-v7a"
export ANDROID_BUILD_TOOLS_VERSION=33.0.2

# 派生路径
export AAPT="$ANDROID_SDK_ROOT/build-tools/$ANDROID_BUILD_TOOLS_VERSION/aapt"
export APKSIGNER="$ANDROID_SDK_ROOT/build-tools/$ANDROID_BUILD_TOOLS_VERSION/apksigner"
export ZIPALIGN="$ANDROID_SDK_ROOT/build-tools/$ANDROID_BUILD_TOOLS_VERSION/zipalign"
export ANDROID_JAR="$ANDROID_SDK_ROOT/platforms/android-$ANDROID_API_LEVEL/android.jar"
```

- [ ] **Step 5: 验证工具链**

```bash
cd /jfs/fengli16/Projects/CProjects/OneLife/android
mkdir -p .  # 占位
source config.sh
$ANDROID_NDK_ROOT/ndk-build --version
$AAPT version
$APKSIGNER --version
ls $ANDROID_JAR
```

期望：四条命令都能成功输出版本/路径，无 "command not found"。

- [ ] **Step 6: 提交 config.sh**

```bash
cd /jfs/fengli16/Projects/CProjects/OneLife
mkdir -p android
git add android/config.sh
git commit -m "build(android): add NDK toolchain config

定义 ANDROID_NDK_ROOT/SDK_ROOT/API_LEVEL/ABIS 等环境变量，
供后续 build.sh 和 CMake 引用。路径基于本机默认安装位置，
其他开发者可通过环境变量覆盖。"
```

---

### Task 0.2: 为 minorGems 创建 android-port worktree

**Files:**（不动 OneLife 仓库，只对同级 minorGems 仓库做 worktree）

- [ ] **Step 1: 检查 minorGems 仓库状态**

```bash
cd /jfs/fengli16/Projects/CProjects/minorGems
git status
git branch -a
```

期望：在 master 分支，工作树干净。如不干净请先与用户确认。

- [ ] **Step 2: 创建 worktree**

```bash
cd /jfs/fengli16/Projects/CProjects/minorGems
git worktree add -b android-port ../minorGems-android-port
ls /jfs/fengli16/Projects/CProjects/minorGems-android-port
```

期望：worktree 目录创建成功，包含与 master 一致的源码。

- [ ] **Step 3: 验证 worktree 列表**

```bash
git worktree list
```

期望输出包含两行：master 主目录和 android-port 子目录。

- [ ] **Step 4: 在 worktree 内初始化空目录占位**

```bash
cd /jfs/fengli16/Projects/CProjects/minorGems-android-port
mkdir -p io/file/android sound/android game/platforms/Android
echo "# minorGems Android 平台分支" > game/platforms/Android/README.md
git add game/platforms/Android/README.md
git commit -m "android: bootstrap android-port branch"
```

期望：android-port 分支多了一次提交，与 master 区分开。

---

### Task 0.3: 创建 AndroidManifest.xml

**Files:**
- Create: `android/AndroidManifest.xml`

- [ ] **Step 1: 写 Manifest**

创建 `/jfs/fengli16/Projects/CProjects/OneLife/android/AndroidManifest.xml`：

```xml
<?xml version="1.0" encoding="utf-8"?>
<manifest xmlns:android="http://schemas.android.com/apk/res/android"
    package="com.fengli.onelife"
    android:versionCode="1"
    android:versionName="0.1">

    <uses-sdk android:minSdkVersion="29" android:targetSdkVersion="29" />

    <uses-feature android:glEsVersion="0x00010001" android:required="true" />
    <uses-permission android:name="android.permission.INTERNET" />

    <application
        android:label="OneLife"
        android:icon="@mipmap/ic_launcher"
        android:hasCode="false"
        android:allowBackup="false">

        <activity
            android:name="android.app.NativeActivity"
            android:label="OneLife"
            android:screenOrientation="sensorLandscape"
            android:configChanges="orientation|keyboardHidden|screenSize|keyboard"
            android:exported="true">

            <meta-data android:name="android.app.lib_name" android:value="onelife" />

            <intent-filter>
                <action android:name="android.intent.action.MAIN" />
                <category android:name="android.intent.category.LAUNCHER" />
            </intent-filter>
        </activity>
    </application>
</manifest>
```

- [ ] **Step 2: 校验 XML**

```bash
xmllint --noout /jfs/fengli16/Projects/CProjects/OneLife/android/AndroidManifest.xml && echo OK
```

期望：输出 `OK`，无错误。

- [ ] **Step 3: 提交**

```bash
cd /jfs/fengli16/Projects/CProjects/OneLife
git add android/AndroidManifest.xml
git commit -m "build(android): add AndroidManifest.xml

NativeActivity（lib_name=onelife）+ 横屏锁定 + INTERNET 权限 +
android:hasCode=false（无 Java 代码）+ minSdk/targetSdk = API 29。"
```

---

### Task 0.4: 创建占位图标和字符串资源

**Files:**
- Create: `android/res/values/strings.xml`
- Create: `android/res/mipmap-mdpi/ic_launcher.png`（占位）

- [ ] **Step 1: 创建 strings.xml**

```bash
cd /jfs/fengli16/Projects/CProjects/OneLife
mkdir -p android/res/values android/res/mipmap-mdpi
cat > android/res/values/strings.xml <<'EOF'
<?xml version="1.0" encoding="utf-8"?>
<resources>
    <string name="app_name">OneLife</string>
</resources>
EOF
```

- [ ] **Step 2: 生成占位图标**

```bash
cd /jfs/fengli16/Projects/CProjects/OneLife
# 用 ImageMagick 生成纯色 48x48 PNG（CLAUDE.md 提到 imagemagick 可用）
convert -size 48x48 xc:#3a7d3a android/res/mipmap-mdpi/ic_launcher.png || \
  python3 -c "
import struct, zlib
def make_png(w, h, color):
    raw = b''.join(b'\x00' + (struct.pack('BBB', *color) * w) for _ in range(h))
    def chunk(t, d):
        return struct.pack('>I', len(d)) + t + d + struct.pack('>I', zlib.crc32(t+d) & 0xffffffff)
    sig = b'\x89PNG\r\n\x1a\n'
    ihdr = struct.pack('>IIBBBBB', w, h, 8, 2, 0, 0, 0)
    idat = zlib.compress(raw)
    return sig + chunk(b'IHDR', ihdr) + chunk(b'IDAT', idat) + chunk(b'IEND', b'')
open('android/res/mipmap-mdpi/ic_launcher.png','wb').write(make_png(48, 48, (58, 125, 58)))
"
file android/res/mipmap-mdpi/ic_launcher.png
```

期望输出：`PNG image data, 48 x 48`。

- [ ] **Step 3: 提交**

```bash
git add android/res/
git commit -m "build(android): add placeholder app icon and strings"
```

---

### Task 0.5: 创建最简 android_main.cpp（不依赖 gameSource）

**Files:**
- Create: `android/jni/android_main.cpp`

- [ ] **Step 1: 写最简入口**

创建 `/jfs/fengli16/Projects/CProjects/OneLife/android/jni/android_main.cpp`：

```cpp
// OneLife Android 入口（Phase 0 占位版本）
// 仅初始化 EGL 并清屏为绿色，验证 NativeActivity 工具链是否打通。
// 真正的游戏入口在 Phase 1 之后接入。

#include <android_native_app_glue.h>
#include <EGL/egl.h>
#include <GLES/gl.h>
#include <android/log.h>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  "OneLife", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "OneLife", __VA_ARGS__)

struct AppState {
    EGLDisplay display = EGL_NO_DISPLAY;
    EGLContext context = EGL_NO_CONTEXT;
    EGLSurface surface = EGL_NO_SURFACE;
    int width = 0;
    int height = 0;
};

static int initEGL(struct android_app* app, AppState* s) {
    const EGLint attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES_BIT,
        EGL_BLUE_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_RED_SIZE, 8,
        EGL_DEPTH_SIZE, 16,
        EGL_NONE
    };
    s->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (!eglInitialize(s->display, nullptr, nullptr)) { LOGE("eglInitialize failed"); return -1; }

    EGLConfig config; EGLint numConfigs;
    eglChooseConfig(s->display, attribs, &config, 1, &numConfigs);
    if (numConfigs == 0) { LOGE("eglChooseConfig: no configs"); return -1; }

    EGLint format;
    eglGetConfigAttrib(s->display, config, EGL_NATIVE_VISUAL_ID, &format);
    ANativeWindow_setBuffersGeometry(app->window, 0, 0, format);

    s->surface = eglCreateWindowSurface(s->display, config, app->window, nullptr);
    s->context = eglCreateContext(s->display, config, EGL_NO_CONTEXT, nullptr);
    if (!eglMakeCurrent(s->display, s->surface, s->surface, s->context)) {
        LOGE("eglMakeCurrent failed"); return -1;
    }
    eglQuerySurface(s->display, s->surface, EGL_WIDTH,  &s->width);
    eglQuerySurface(s->display, s->surface, EGL_HEIGHT, &s->height);
    LOGI("EGL ready: %dx%d", s->width, s->height);
    return 0;
}

static void termEGL(AppState* s) {
    if (s->display != EGL_NO_DISPLAY) {
        eglMakeCurrent(s->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (s->context != EGL_NO_CONTEXT) eglDestroyContext(s->display, s->context);
        if (s->surface != EGL_NO_SURFACE) eglDestroySurface(s->display, s->surface);
        eglTerminate(s->display);
    }
    *s = AppState{};
}

static void drawFrame(AppState* s) {
    if (s->display == EGL_NO_DISPLAY) return;
    glViewport(0, 0, s->width, s->height);
    glClearColor(0.23f, 0.49f, 0.23f, 1.0f);  // OneLife 绿
    glClear(GL_COLOR_BUFFER_BIT);
    eglSwapBuffers(s->display, s->surface);
}

static void onAppCmd(struct android_app* app, int32_t cmd) {
    AppState* s = (AppState*)app->userData;
    switch (cmd) {
        case APP_CMD_INIT_WINDOW:    if (app->window) initEGL(app, s); break;
        case APP_CMD_TERM_WINDOW:    termEGL(s); break;
        case APP_CMD_DESTROY:        termEGL(s); break;
        default: break;
    }
}

extern "C" void android_main(struct android_app* app) {
    AppState state{};
    app->userData     = &state;
    app->onAppCmd     = onAppCmd;

    LOGI("OneLife Android starting...");

    while (true) {
        int events;
        struct android_poll_source* source;
        while (ALooper_pollAll(state.display == EGL_NO_DISPLAY ? -1 : 0,
                               nullptr, &events, (void**)&source) >= 0) {
            if (source) source->process(app, source);
            if (app->destroyRequested) {
                termEGL(&state);
                LOGI("OneLife Android exiting");
                return;
            }
        }
        drawFrame(&state);
    }
}
```

- [ ] **Step 2: 提交**

```bash
git add android/jni/android_main.cpp
git commit -m "build(android): add Phase 0 minimal NativeActivity entry

仅初始化 EGL 并清屏为绿色，验证 NDK 工具链。
真正的游戏入口将在 Phase 1 接入 minorGems 平台层。"
```

---

### Task 0.6: 创建 CMakeLists.txt（Phase 0 版本）

**Files:**
- Create: `android/CMakeLists.txt`

- [ ] **Step 1: 写 CMakeLists**

创建 `/jfs/fengli16/Projects/CProjects/OneLife/android/CMakeLists.txt`：

```cmake
cmake_minimum_required(VERSION 3.18)
project(OneLifeAndroid LANGUAGES C CXX)

# Phase 0：仅 JNI 入口 + EGL 清屏，不接入 minorGems/gameSource
# Phase 1+ 会扩展到 minorGems + gameSource

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 11)

# native_app_glue（NDK 自带）
add_library(native_app_glue STATIC
    ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)

target_include_directories(native_app_glue PUBLIC
    ${ANDROID_NDK}/sources/android/native_app_glue)

# 主 .so（lib_name=onelife，Manifest 中 lib_name 必须匹配）
add_library(onelife SHARED
    jni/android_main.cpp)

target_include_directories(onelife PRIVATE
    ${ANDROID_NDK}/sources/android/native_app_glue)

target_link_libraries(onelife
    native_app_glue
    android log EGL GLESv1_CM)

# NativeActivity 要求 .so 的所有未引用符号也需保留（ANativeActivity_onCreate）
target_link_options(onelife PRIVATE "-u" "ANativeActivity_onCreate")
```

- [ ] **Step 2: 提交**

```bash
git add android/CMakeLists.txt
git commit -m "build(android): add Phase 0 CMakeLists

链接 EGL + GLESv1_CM（GLES 1.x，与 minorGems Raspbian 分支一致），
保留 ANativeActivity_onCreate 符号供 NativeActivity 反射调用。"
```

---

### Task 0.7: 创建 build.sh 一键构建脚本

**Files:**
- Create: `android/build.sh`

- [ ] **Step 1: 写脚本**

创建 `/jfs/fengli16/Projects/CProjects/OneLife/android/build.sh`：

```bash
#!/bin/bash
# OneLife Android 一键构建脚本
# 用法：./build.sh [debug|release]
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

source ./config.sh

BUILD_TYPE="${1:-debug}"
BUILD_DIR="$SCRIPT_DIR/build/$BUILD_TYPE"
OUT_APK="$SCRIPT_DIR/build/OneLife-$BUILD_TYPE.apk"

mkdir -p "$BUILD_DIR"

# 1) 用 CMake + NDK 编译每个 ABI 的 .so
for ABI in $ANDROID_ABIS; do
    ABI_BUILD="$BUILD_DIR/$ABI"
    mkdir -p "$ABI_BUILD"
    cmake -B "$ABI_BUILD" -S "$SCRIPT_DIR" \
        -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK_ROOT/build/cmake/android.toolchain.cmake" \
        -DANDROID_ABI="$ABI" \
        -DANDROID_PLATFORM="android-$ANDROID_API_LEVEL" \
        -DCMAKE_BUILD_TYPE=$( [ "$BUILD_TYPE" = "debug" ] && echo Debug || echo Release ) \
        -DANDROID_STL=c++_static
    cmake --build "$ABI_BUILD" -j
done

# 2) 收集每个 ABI 的 .so 到 lib/
LIB_DIR="$BUILD_DIR/apk-staging/lib"
rm -rf "$LIB_DIR"
mkdir -p "$LIB_DIR"
for ABI in $ANDROID_ABIS; do
    mkdir -p "$LIB_DIR/$ABI"
    cp "$BUILD_DIR/$ABI/libonelife.so" "$LIB_DIR/$ABI/"
done

# 3) aapt 打包：Manifest + res + lib + assets（assets 在 Phase 0 为空目录）
APK_UNSIGNED="$BUILD_DIR/OneLife-unsigned.apk"
mkdir -p "$SCRIPT_DIR/assets"
$AAPT package -f \
    -M "$SCRIPT_DIR/AndroidManifest.xml" \
    -S "$SCRIPT_DIR/res" \
    -A "$SCRIPT_DIR/assets" \
    -I "$ANDROID_JAR" \
    -F "$APK_UNSIGNED"

# 4) 把 lib/ 加进 APK
cd "$BUILD_DIR/apk-staging"
$AAPT add "$APK_UNSIGNED" $(find lib -type f)
cd "$SCRIPT_DIR"

# 5) zipalign + 签名
APK_ALIGNED="$BUILD_DIR/OneLife-aligned.apk"
$ZIPALIGN -f 4 "$APK_UNSIGNED" "$APK_ALIGNED"

KEYSTORE="$SCRIPT_DIR/build/debug.keystore"
if [ ! -f "$KEYSTORE" ]; then
    keytool -genkeypair -v -keystore "$KEYSTORE" -storepass android -keypass android \
        -alias androiddebugkey -dname "CN=Android Debug,O=Android,C=US" \
        -keyalg RSA -keysize 2048 -validity 10000
fi

$APKSIGNER sign --ks "$KEYSTORE" --ks-pass pass:android --key-pass pass:android \
    --out "$OUT_APK" "$APK_ALIGNED"

echo "==> Built: $OUT_APK"
ls -lh "$OUT_APK"
```

- [ ] **Step 2: 赋予可执行权限**

```bash
chmod +x /jfs/fengli16/Projects/CProjects/OneLife/android/build.sh
```

- [ ] **Step 3: 提交**

```bash
git add android/build.sh
git commit -m "build(android): add build.sh one-shot pipeline

CMake 多 ABI 编译 → aapt 打包 → zipalign → apksigner 签名。
Debug keystore 自动生成。"
```

---

### Task 0.8: 配置 .gitignore，忽略 Android 构建产物

**Files:**
- Modify: `.gitignore`

- [ ] **Step 1: 追加规则**

```bash
cd /jfs/fengli16/Projects/CProjects/OneLife
cat >> .gitignore <<'EOF'

# Android build artifacts
android/build/
android/.cxx/
android/assets/sprites
android/assets/sounds
android/assets/objects
android/assets/transitions
android/assets/categories
android/assets/animations
android/assets/tutorialMaps
android/assets/contentSettings
android/assets/dataVersionNumber.txt
android/assets/isAHAP.txt
android/assets/music
android/assets/ground
android/assets/faces
android/assets/scenes
android/assets/overlays
EOF
```

- [ ] **Step 2: 验证忽略**

```bash
git check-ignore -v android/build/test.apk && echo OK
git check-ignore -v android/AndroidManifest.xml || echo "Manifest 仍被跟踪"
```

期望：第一条命令证明 `android/build/` 被忽略，第二条命令显示 Manifest 不被忽略。

- [ ] **Step 3: 提交**

```bash
git add .gitignore
git commit -m "build(android): ignore build artifacts and asset symlinks"
```

---

### Task 0.9: 跑通 Phase 0 APK

**Files:**（无源码改动）

- [ ] **Step 1: 编译**

```bash
cd /jfs/fengli16/Projects/CProjects/OneLife/android
./build.sh debug 2>&1 | tee build/build.log | tail -30
```

期望：最后一行输出 `==> Built: .../OneLife-debug.apk`，文件大小数百 KB。

- [ ] **Step 2: 安装到设备/模拟器**

```bash
# 需要 adb 连接到设备或运行中的模拟器
adb devices
adb install -r build/OneLife-debug.apk
```

期望：`Success`。如果没有设备：跳到 Step 4 用模拟器或人工测试。

- [ ] **Step 3: 启动并查看 logcat**

```bash
adb shell am start -n com.fengli.onelife/android.app.NativeActivity
adb logcat -s OneLife:* AndroidRuntime:E -t 50
```

期望：屏幕显示绿色，logcat 看到 `OneLife Android starting...` 和 `EGL ready: WxH`。

- [ ] **Step 4: 卸载（清理）**

```bash
adb uninstall com.fengli.onelife
```

- [ ] **Step 5: 提交里程碑标记**

```bash
cd /jfs/fengli16/Projects/CProjects/OneLife
echo "P0 done at $(date '+%Y-%m-%d %H:%M')" >> android/MILESTONES.md
git add android/MILESTONES.md
git commit -m "build(android): Phase 0 done — minimal NativeActivity APK runs

绿色清屏窗口可在 Android 10+ 设备启动，工具链全链路打通。"
```

---

# Phase 1：minorGems Android 平台层

**目标**：在 `minorGems-android-port` 分支补齐 Android 平台实现，能加载并显示一张 sprite 图片。所有改动在 worktree 内完成。

工作目录：`/jfs/fengli16/Projects/CProjects/minorGems-android-port`（除非另作说明）

---

### Task 1.1: 扩展 glInclude.h，增加 __ANDROID__ 分支

**Files:**
- Modify: `minorGems-android-port/graphics/openGL/glInclude.h`

- [ ] **Step 1: 编辑 glInclude.h**

把开头部分改为以下内容（保留原有其他平台分支）：

```c
#ifdef __mac__

#include <OpenGL/gl.h>
#include <OpenGL/glu.h>


#elif defined(RASPBIAN) || defined(__ANDROID__)

// GL ES 1.x (与 Raspbian 共用代码路径)
#include <GLES/gl.h>

// 一些在 GLES 中名字不同或缺失的常量/函数
#define GLdouble     GLfloat
#define GL_CLAMP     GL_CLAMP_TO_EDGE
#define glClearDepth glClearDepthf
#define glOrtho      glOrthof
#define glFrustum    glFrustumf
#define glGetDoublev glGetFloatv
#define GL_SOURCE0_RGB GL_SRC0_RGB
#define GL_SOURCE0_ALPHA GL_SRC0_ALPHA

#ifndef __ANDROID__
// Raspbian 还能用 mesa 提供的 glu
#include <GL/glu.h>
#endif


#else
```

注意：`__ANDROID__` 分支不包含 `<GL/glu.h>`，因为 Android NDK 没有 glu 库。这要求确认 minorGems/gameSource 中没有 `gluXxx` 调用。

- [ ] **Step 2: 验证没有 glu* 调用未被处理**

```bash
cd /jfs/fengli16/Projects/CProjects/minorGems-android-port
grep -rn "gluPerspective\|gluLookAt\|gluOrtho2D\|gluProject\|gluUnProject\|gluBuild2DMipmaps\|gluErrorString" --include="*.cpp" --include="*.h" | grep -v "#ifdef RASPBIAN\|defined(RASPBIAN)" || echo "无未处理 glu 调用"
```

期望：输出 `无未处理 glu 调用`，或调用都已在 `RASPBIAN` 分支内。如果输出有结果，记录下来在 Task 1.6 中替代实现。

实际经验：minorGems 的 `gluBuild2DMipmaps` 在 RASPBIAN 已有替换（用 `glGenerateMipmap` 或简单缩放）。

- [ ] **Step 3: 提交（在 worktree）**

```bash
cd /jfs/fengli16/Projects/CProjects/minorGems-android-port
git add graphics/openGL/glInclude.h
git commit -m "android: glInclude.h supports __ANDROID__ via GLES 1.x

复用 Raspbian 的 GLES 1.x 路径，仅排除 GL/glu.h（NDK 不提供）。
依赖 minorGems/gameSource 中无未处理的 glu* 调用。"
```

---

### Task 1.2: 实现 FileAndroid（AAsset 读 + 内部存储写）

**Files:**
- Create: `minorGems-android-port/io/file/android/FileAndroid.cpp`
- Create: `minorGems-android-port/io/file/android/AndroidAssetGlobal.h`

**关键点**：minorGems 的 `File` 类提供平台特定实现（`unix/` 子目录已有完整 POSIX 实现）。Android 的特殊性在于 APK 内的资源走 AAssetManager（无 fopen），而内部存储走普通 POSIX。最干净的做法是：保留 `unix/` 实现作为基础，**仅在 `readFileContents()` 路径无法 fopen 时**回退到 AAsset 读取。

- [ ] **Step 1: 创建全局 AAssetManager 头文件**

创建 `/jfs/fengli16/Projects/CProjects/minorGems-android-port/io/file/android/AndroidAssetGlobal.h`：

```cpp
#ifndef MINORGEMS_ANDROID_ASSET_GLOBAL_H
#define MINORGEMS_ANDROID_ASSET_GLOBAL_H

// 由 OneLife/android/jni/AndroidPlatform.cpp 在 NativeActivity 启动时设置
// FileAndroid 通过此入口找到 AAssetManager 与内部存储路径

#ifdef __ANDROID__
#include <android/asset_manager.h>

namespace minorGemsAndroid {
    void setAssetManager(AAssetManager* mgr);
    AAssetManager* getAssetManager();

    void setInternalDataPath(const char* path);
    const char* getInternalDataPath();
}
#endif

#endif
```

- [ ] **Step 2: 创建 FileAndroid.cpp**

创建 `/jfs/fengli16/Projects/CProjects/minorGems-android-port/io/file/android/FileAndroid.cpp`：

```cpp
// FileAndroid.cpp
// 透明扩展：File::readFileContents() 在普通 fopen 失败时尝试从 AAsset 读取。
// 内部存储路径在 OneLife/android 入口处通过 chdir() 切换为 internalDataPath，
// 因此普通的相对路径写入文件能直接落到 /data/data/.../files/。

#ifdef __ANDROID__

#include "AndroidAssetGlobal.h"
#include <android/asset_manager.h>
#include <android/log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace {
    AAssetManager* gMgr = nullptr;
    char* gInternalPath = nullptr;
}

namespace minorGemsAndroid {
    void setAssetManager(AAssetManager* mgr) { gMgr = mgr; }
    AAssetManager* getAssetManager() { return gMgr; }

    void setInternalDataPath(const char* path) {
        if (gInternalPath) free(gInternalPath);
        gInternalPath = path ? strdup(path) : nullptr;
    }
    const char* getInternalDataPath() { return gInternalPath ? gInternalPath : "."; }
}

// 提供 C 接口，供 minorGems File 类回退使用
extern "C" {

// 尝试从 AAsset 读取文件。成功返回 malloc 出来的缓冲区与字节数，失败返回 nullptr。
unsigned char* minorGemsAndroid_readAsset(const char* relativePath, int* outBytes) {
    if (!gMgr || !relativePath) return nullptr;
    // 去掉开头的 "./" 或 "/"，AAsset 路径相对 assets/
    const char* p = relativePath;
    while (*p == '.' || *p == '/') p++;

    AAsset* a = AAssetManager_open(gMgr, p, AASSET_MODE_BUFFER);
    if (!a) return nullptr;

    off_t len = AAsset_getLength(a);
    unsigned char* buf = (unsigned char*) malloc(len);
    int read = AAsset_read(a, buf, len);
    AAsset_close(a);
    if (read != (int)len) { free(buf); return nullptr; }
    if (outBytes) *outBytes = (int)len;
    return buf;
}

}  // extern "C"

#endif // __ANDROID__
```

- [ ] **Step 3: 在 minorGems File.cpp 中接入回退（仅 __ANDROID__ 下生效）**

在 worktree 中编辑 `/jfs/fengli16/Projects/CProjects/minorGems-android-port/io/file/File.cpp`，定位 `File::readFileContents` 实现（如果是按平台分发，则定位 `unix/FileUnix.cpp`）。先确认实现位置：

```bash
cd /jfs/fengli16/Projects/CProjects/minorGems-android-port
grep -rn "::readFileContents" io/ | head -5
```

记下文件名和行号（假设是 `io/file/File.cpp`，找不到的话定位 `io/file/linux/FileLinux.cpp` 等），然后在 `readFileContents` 函数失败返回 NULL 之前插入：

```cpp
#ifdef __ANDROID__
{
    extern "C" unsigned char* minorGemsAndroid_readAsset(const char*, int*);
    char* full = getFullFileName();
    if (full) {
        int bytes = 0;
        unsigned char* assetBuf = minorGemsAndroid_readAsset(full, &bytes);
        delete[] full;
        if (assetBuf) {
            if (outNumDataBytes) *outNumDataBytes = bytes;
            // 转成 minorGems 期望的 new[] 内存（caller 用 delete[]）
            unsigned char* result = new unsigned char[bytes];
            memcpy(result, assetBuf, bytes);
            free(assetBuf);
            return result;
        }
    }
}
#endif
```

注意：实际接入位置取决于 `readFileContents` 当前签名和返回类型，要先 Read 这个函数再插入。

- [ ] **Step 4: 提交**

```bash
cd /jfs/fengli16/Projects/CProjects/minorGems-android-port
git add io/file/android/ io/file/File.cpp 2>/dev/null || git add io/file/android/ io/file/linux/
git commit -m "android: AAsset fallback for File::readFileContents

设置全局 AAssetManager 与内部存储路径，
File::readFileContents 在 fopen 失败时回退到 AAsset 读取。
write 路径仍走 fopen + chdir(internalDataPath)。"
```

---

### Task 1.3: 实现 OpenSL ES 音频后端

**Files:**
- Create: `minorGems-android-port/sound/android/OpenSLAudioBackend.cpp`

**注**：minorGems 的音频接口是 `getSoundSamples()` 由游戏侧实现（callback 拉取 PCM），平台侧只负责把 PCM 喂给系统。OpenSL ES 的 `SLAndroidSimpleBufferQueueItf` 正合适。

- [ ] **Step 1: 创建实现文件**

创建 `/jfs/fengli16/Projects/CProjects/minorGems-android-port/sound/android/OpenSLAudioBackend.cpp`：

```cpp
#ifdef __ANDROID__

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <android/log.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define LOG_TAG "OneLifeAudio"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// minorGems 接口（在 game/gameAPI 内声明）
extern "C" void getSoundSamples(int16_t* inBuffer, unsigned int inNumSamples);
extern "C" int  getSampleRate();

namespace {
    SLObjectItf engineObj = nullptr;
    SLEngineItf engineItf = nullptr;
    SLObjectItf mixObj = nullptr;
    SLObjectItf playerObj = nullptr;
    SLPlayItf playItf = nullptr;
    SLAndroidSimpleBufferQueueItf bqItf = nullptr;

    constexpr int kBufferSampleCount = 2048;     // 立体声样本对数
    int16_t buffers[2][kBufferSampleCount * 2];  // 双缓冲，立体声
    int currentBuffer = 0;

    void fillAndEnqueue() {
        int16_t* buf = buffers[currentBuffer];
        getSoundSamples(buf, kBufferSampleCount);  // 游戏侧填充立体声 PCM
        (*bqItf)->Enqueue(bqItf, buf, kBufferSampleCount * 2 * sizeof(int16_t));
        currentBuffer ^= 1;
    }

    void bqCallback(SLAndroidSimpleBufferQueueItf, void*) {
        fillAndEnqueue();
    }
}

extern "C" int minorGemsAndroid_audioStart() {
    SLresult r = slCreateEngine(&engineObj, 0, nullptr, 0, nullptr, nullptr);
    if (r != SL_RESULT_SUCCESS) { LOGE("slCreateEngine failed"); return -1; }
    (*engineObj)->Realize(engineObj, SL_BOOLEAN_FALSE);
    (*engineObj)->GetInterface(engineObj, SL_IID_ENGINE, &engineItf);

    (*engineItf)->CreateOutputMix(engineItf, &mixObj, 0, nullptr, nullptr);
    (*mixObj)->Realize(mixObj, SL_BOOLEAN_FALSE);

    int rate = getSampleRate();
    SLDataLocator_AndroidSimpleBufferQueue locBQ = { SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2 };
    SLDataFormat_PCM fmt = {
        SL_DATAFORMAT_PCM, 2, (SLuint32)(rate * 1000),
        SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16,
        SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT, SL_BYTEORDER_LITTLEENDIAN
    };
    SLDataSource src = { &locBQ, &fmt };

    SLDataLocator_OutputMix locMix = { SL_DATALOCATOR_OUTPUTMIX, mixObj };
    SLDataSink sink = { &locMix, nullptr };

    const SLInterfaceID ids[] = { SL_IID_BUFFERQUEUE };
    const SLboolean req[] = { SL_BOOLEAN_TRUE };
    (*engineItf)->CreateAudioPlayer(engineItf, &playerObj, &src, &sink, 1, ids, req);
    (*playerObj)->Realize(playerObj, SL_BOOLEAN_FALSE);
    (*playerObj)->GetInterface(playerObj, SL_IID_PLAY, &playItf);
    (*playerObj)->GetInterface(playerObj, SL_IID_BUFFERQUEUE, &bqItf);
    (*bqItf)->RegisterCallback(bqItf, bqCallback, nullptr);
    (*playItf)->SetPlayState(playItf, SL_PLAYSTATE_PLAYING);

    fillAndEnqueue();
    fillAndEnqueue();
    LOGI("OpenSL ES started: %d Hz stereo", rate);
    return 0;
}

extern "C" void minorGemsAndroid_audioStop() {
    if (playerObj) { (*playerObj)->Destroy(playerObj); playerObj = nullptr; }
    if (mixObj)    { (*mixObj)->Destroy(mixObj); mixObj = nullptr; }
    if (engineObj) { (*engineObj)->Destroy(engineObj); engineObj = nullptr; }
}

#endif // __ANDROID__
```

- [ ] **Step 2: 提交**

```bash
cd /jfs/fengli16/Projects/CProjects/minorGems-android-port
git add sound/android/OpenSLAudioBackend.cpp
git commit -m "android: OpenSL ES audio backend

双缓冲 BufferQueue 调用 getSoundSamples 拉取立体声 PCM。
入口 minorGemsAndroid_audioStart()/Stop() 由 OneLife 平台层调用。"
```

---

### Task 1.4: 实现 gameAndroid 平台主循环

**Files:**
- Create: `minorGems-android-port/game/platforms/Android/gameAndroid.cpp`

**说明**：minorGems 的 game 框架要求平台层提供 `main()` 入口，调用 `initGame() / drawFrame() / freeGame()` 等钩子。Android 没有传统 main，由 NativeActivity 调用 `android_main`。这里提供"逻辑主循环"，由 OneLife `android_main` 在合适时机调用。

- [ ] **Step 1: 创建主循环文件**

创建 `/jfs/fengli16/Projects/CProjects/minorGems-android-port/game/platforms/Android/gameAndroid.cpp`：

```cpp
#ifdef __ANDROID__

// gameAndroid.cpp
// 提供 minorGems 游戏框架在 Android 上的"主循环 tick"实现。
// 与桌面 SDL 不同，Android 由 NativeActivity 驱动事件循环，
// 这里只对外暴露 init/tick/term 三个钩子，由 OneLife/android/jni/android_main.cpp 调用。

#include <android/log.h>

// minorGems 游戏侧接口（每个游戏项目都会实现）
extern "C" {
    int   getScreenSizeX();
    int   getScreenSizeY();
    char  isMouseDragging();
    void  pointerDown(float, float);
    void  pointerMove(float, float);
    void  pointerUp(float, float);
    void  initFrameDrawer(int width, int height, int targetFrameRate,
                           const char* customRecordedGameData,
                           char inPlayingBack);
    void  initDrawString(int targetFrameRate);
    void  freeFrameDrawer();
    void  drawFrame(char inUpdate);
    void  initGame();
    void  freeGame();
    void  initFrameDrawer();
    void  newGame();
}

namespace minorGemsAndroid {

void platformInit(int width, int height, int targetFrameRate) {
    __android_log_print(ANDROID_LOG_INFO, "OneLife",
        "platformInit %dx%d @%dfps", width, height, targetFrameRate);
    initFrameDrawer(width, height, targetFrameRate, "", false);
    initDrawString(targetFrameRate);
    initGame();
}

void platformTick() {
    drawFrame(true);
}

void platformShutdown() {
    freeGame();
    freeFrameDrawer();
}

}  // namespace

#endif // __ANDROID__
```

注：上面的 `extern "C"` 声明可能与 minorGems 实际签名略有差异。Task 2.x 集成 gameSource 时如出现 link error，按实际签名修正。

- [ ] **Step 2: 提交**

```bash
cd /jfs/fengli16/Projects/CProjects/minorGems-android-port
git add game/platforms/Android/gameAndroid.cpp
git commit -m "android: gameAndroid platform glue

提供 platformInit/Tick/Shutdown 三个钩子。
NativeActivity 在 surface ready 后调用 init，
事件循环每帧调用 tick，destroy 时调用 shutdown。"
```

---

### Task 1.5: 在 OneLife 端创建 EGLContext + AndroidPlatform 模块

**Files:**
- Create: `OneLife/android/jni/AndroidPlatform.h/.cpp`
- Create: `OneLife/android/jni/EGLContext.h/.cpp`
- Create: `OneLife/android/jni/AssetFileBridge.h/.cpp`

工作目录回到 OneLife 仓库。

- [ ] **Step 1: 创建 AndroidPlatform.h**

```cpp
// android/jni/AndroidPlatform.h
#ifndef ONELIFE_ANDROID_PLATFORM_H
#define ONELIFE_ANDROID_PLATFORM_H

#include <android_native_app_glue.h>

namespace OneLifeAndroid {
    void  setApp(struct android_app* app);
    struct android_app* getApp();

    int  width();
    int  height();
    void setSurfaceSize(int w, int h);
}

#endif
```

- [ ] **Step 2: 创建 AndroidPlatform.cpp**

```cpp
#include "AndroidPlatform.h"

namespace {
    struct android_app* gApp = nullptr;
    int gW = 0, gH = 0;
}

namespace OneLifeAndroid {
    void setApp(struct android_app* a) { gApp = a; }
    struct android_app* getApp() { return gApp; }
    int  width()  { return gW; }
    int  height() { return gH; }
    void setSurfaceSize(int w, int h) { gW = w; gH = h; }
}
```

- [ ] **Step 3: 创建 EGLContext.h**

```cpp
// android/jni/EGLContext.h
#ifndef ONELIFE_EGL_CONTEXT_H
#define ONELIFE_EGL_CONTEXT_H

#include <EGL/egl.h>

namespace OneLifeEGL {
    bool init(struct android_app* app);
    void term();
    void swapBuffers();
    bool ready();
    int  width();
    int  height();
}

#endif
```

- [ ] **Step 4: 创建 EGLContext.cpp**

```cpp
#include "EGLContext.h"
#include <android_native_app_glue.h>
#include <GLES/gl.h>
#include <android/log.h>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "OneLife", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "OneLife", __VA_ARGS__)

namespace {
    EGLDisplay display = EGL_NO_DISPLAY;
    EGLContext context = EGL_NO_CONTEXT;
    EGLSurface surface = EGL_NO_SURFACE;
    int w = 0, h = 0;
    bool gReady = false;
}

namespace OneLifeEGL {

bool init(struct android_app* app) {
    if (!app->window) return false;

    const EGLint attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES_BIT,
        EGL_BLUE_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_RED_SIZE, 8,
        EGL_DEPTH_SIZE, 16,
        EGL_NONE
    };

    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (!eglInitialize(display, nullptr, nullptr)) { LOGE("eglInitialize fail"); return false; }

    EGLConfig config; EGLint num;
    eglChooseConfig(display, attribs, &config, 1, &num);
    if (!num) { LOGE("no config"); return false; }

    EGLint format;
    eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);
    ANativeWindow_setBuffersGeometry(app->window, 0, 0, format);

    surface = eglCreateWindowSurface(display, config, app->window, nullptr);
    context = eglCreateContext(display, config, EGL_NO_CONTEXT, nullptr);
    if (!eglMakeCurrent(display, surface, surface, context)) { LOGE("makeCurrent fail"); return false; }

    eglQuerySurface(display, surface, EGL_WIDTH,  &w);
    eglQuerySurface(display, surface, EGL_HEIGHT, &h);
    LOGI("EGL ready: %dx%d", w, h);
    gReady = true;
    return true;
}

void term() {
    if (display != EGL_NO_DISPLAY) {
        eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (context != EGL_NO_CONTEXT) eglDestroyContext(display, context);
        if (surface != EGL_NO_SURFACE) eglDestroySurface(display, surface);
        eglTerminate(display);
    }
    display = EGL_NO_DISPLAY;
    context = EGL_NO_CONTEXT;
    surface = EGL_NO_SURFACE;
    gReady = false;
    w = h = 0;
}

void swapBuffers() {
    if (gReady) eglSwapBuffers(display, surface);
}

bool ready() { return gReady; }
int  width()  { return w; }
int  height() { return h; }

}
```

- [ ] **Step 5: 创建 AssetFileBridge.h/.cpp**

```cpp
// android/jni/AssetFileBridge.h
#ifndef ONELIFE_ASSET_BRIDGE_H
#define ONELIFE_ASSET_BRIDGE_H

#include <android_native_app_glue.h>

namespace OneLifeAssetBridge {
    // 在 NativeActivity 启动时调用：把 AAssetManager 注入 minorGems，
    // 并把当前工作目录切到内部存储路径
    void install(struct android_app* app);
}

#endif
```

```cpp
// android/jni/AssetFileBridge.cpp
#include "AssetFileBridge.h"
#include "minorGems/io/file/android/AndroidAssetGlobal.h"
#include <unistd.h>
#include <sys/stat.h>
#include <android/log.h>

namespace OneLifeAssetBridge {

void install(struct android_app* app) {
    if (app && app->activity) {
        minorGemsAndroid::setAssetManager(app->activity->assetManager);
        const char* dataDir = app->activity->internalDataPath;
        minorGemsAndroid::setInternalDataPath(dataDir);
        if (dataDir) {
            mkdir(dataDir, 0770);
            chdir(dataDir);  // 让相对路径写入落到内部存储
            __android_log_print(ANDROID_LOG_INFO, "OneLife",
                                "Internal data dir: %s", dataDir);
        }
    }
}

}
```

- [ ] **Step 6: 提交**

```bash
cd /jfs/fengli16/Projects/CProjects/OneLife
git add android/jni/AndroidPlatform.h android/jni/AndroidPlatform.cpp \
        android/jni/EGLContext.h android/jni/EGLContext.cpp \
        android/jni/AssetFileBridge.h android/jni/AssetFileBridge.cpp
git commit -m "android: AndroidPlatform/EGL/AssetBridge modules

把 EGL 初始化、AAssetManager 注入、内部存储 chdir 拆成独立模块，
便于 Phase 2 接入 gameSource 后简化 android_main 主循环。"
```

---

### Task 1.6: P1 验证程序——加载并显示一张 sprite

由于在没有 gameSource 的情况下完整渲染管线难以验证，本任务采用一个**最小验证程序**：从 assets/ 读取一张 TGA，用 GLES 1.x 显示在屏幕上。

**Files:**
- Create: `android/jni/p1_test.cpp`（临时，Phase 2 将删除）

- [ ] **Step 1: 准备一张测试 TGA**

```bash
cd /jfs/fengli16/Projects/CProjects/OneLife/android
mkdir -p assets
# 从 OneLifeData7 拷一张小 sprite（任选）
ls /jfs/fengli16/Projects/CProjects/OneLifeData7/sprites/*.tga | head -1
cp $(ls /jfs/fengli16/Projects/CProjects/OneLifeData7/sprites/*.tga | head -1) assets/test.tga
```

- [ ] **Step 2: 写最小渲染验证（暂用 raw GLES，不接入 minorGems Image）**

修改 `android/jni/android_main.cpp`，在 `drawFrame` 之前加载并绑定一张纯色纹理（验证 EGL+GLES1+AAsset 三件套）。先读取现有代码再决定补丁位置。

具体补丁：在 `AppState` 中加 `GLuint texId;`，在 EGL init 成功后立即调用：

```cpp
// 加载 assets/test.tga（前 18 字节是 TGA 头，跳过；假设是 32 位 BGRA）
AAssetManager* mgr = app->activity->assetManager;
AAsset* a = AAssetManager_open(mgr, "test.tga", AASSET_MODE_BUFFER);
if (a) {
    int len = AAsset_getLength(a);
    unsigned char* data = (unsigned char*)malloc(len);
    AAsset_read(a, data, len);
    AAsset_close(a);
    // TGA 头解析（最简：只支持非压缩 32 位）
    int w = data[12] | (data[13] << 8);
    int h = data[14] | (data[15] << 8);
    LOGI("test.tga: %dx%d, %d bytes", w, h, len);
    glEnable(GL_TEXTURE_2D);
    glGenTextures(1, &s->texId);
    glBindTexture(GL_TEXTURE_2D, s->texId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // BGRA → RGBA 简单转换
    int channels = data[16] / 8;
    if (channels == 4) {
        for (int i = 0; i < w*h; i++) {
            unsigned char b = data[18 + i*4];
            data[18 + i*4] = data[18 + i*4 + 2];
            data[18 + i*4 + 2] = b;
        }
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data + 18);
    } else {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data + 18);
    }
    free(data);
}
```

并修改 `drawFrame`，绑定纹理画一个全屏四边形（GLES 1 vertex array）：

```cpp
static void drawFrame(AppState* s) {
    if (s->display == EGL_NO_DISPLAY) return;
    glViewport(0, 0, s->width, s->height);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (s->texId) {
        const GLfloat verts[] = { -1,-1, 1,-1, -1,1, 1,1 };
        const GLfloat tex[]   = { 0,1,  1,1,  0,0,  1,0 };
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, s->texId);
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glVertexPointer(2, GL_FLOAT, 0, verts);
        glTexCoordPointer(2, GL_FLOAT, 0, tex);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

    eglSwapBuffers(s->display, s->surface);
}
```

- [ ] **Step 3: 编译并安装**

```bash
cd /jfs/fengli16/Projects/CProjects/OneLife/android
./build.sh debug 2>&1 | tail -10
adb install -r build/OneLife-debug.apk
adb shell am start -n com.fengli.onelife/android.app.NativeActivity
adb logcat -s OneLife:* -t 50
```

期望：屏幕显示 test.tga 的内容（拉伸到全屏），logcat 看到 `test.tga: WxH, N bytes`。

- [ ] **Step 4: 提交里程碑**

```bash
echo "P1 done at $(date '+%Y-%m-%d %H:%M')" >> android/MILESTONES.md
git add android/MILESTONES.md android/jni/android_main.cpp
git commit -m "build(android): Phase 1 done — texture from AAsset displayed

EGL + GLES 1.x + AAssetManager 三件套验证通过。
下一步在 Phase 2 接入 gameSource 完整渲染管线。"
```

---

# Phase 2：gameSource 集成

**目标**：让 `gameSource/` 完整在 Android 上编译并启动到主菜单/设置页面。

---

### Task 2.1: 写 GameBackend 抽象

**Files:**
- Create: `gameSource/GameBackend.h`
- Create: `gameSource/NetworkBackend.h`
- Create: `gameSource/NetworkBackend.cpp`

工作目录：`/jfs/fengli16/Projects/CProjects/OneLife`

- [ ] **Step 1: 创建 GameBackend.h**

```cpp
// gameSource/GameBackend.h
#ifndef ONELIFE_GAME_BACKEND_H
#define ONELIFE_GAME_BACKEND_H

class GameBackend {
public:
    virtual ~GameBackend() {}
    virtual bool connect(const char* address, int port, const char* password) = 0;
    virtual bool sendMessage(const char* msg) = 0;
    virtual char* receiveMessage() = 0;  // 返回 NULL 表示无新消息；caller 用 delete[]
    virtual bool isConnected() = 0;
    virtual int  bytesAvailable() = 0;
    virtual void disconnect() = 0;
};

GameBackend* createNetworkBackend();
// 未来：GameBackend* createEmbeddedBackend();

#endif
```

- [ ] **Step 2: 创建 NetworkBackend.h/.cpp（包装现有 SocketClient）**

先查看现有 SocketClient 用法：

```bash
cd /jfs/fengli16/Projects/CProjects/OneLife
grep -rn "SocketClient\|openConnection\|sendCharsToServer\|getMessage" gameSource/*.cpp | head -20
```

记下 SocketClient 提供的方法签名，然后基于此实现。`NetworkBackend.cpp` 大致结构：

```cpp
#include "NetworkBackend.h"
#include "GameBackend.h"
#include "minorGems/network/SocketClient.h"
// ... 其他既有头文件

class NetworkBackend : public GameBackend {
public:
    NetworkBackend() : mSocket(nullptr) {}
    ~NetworkBackend() override { disconnect(); }

    bool connect(const char* address, int port, const char* password) override {
        // 调用 SocketClient::connectToServer 等既有 API
        // ...
        return mSocket != nullptr;
    }
    bool sendMessage(const char* msg) override { /* 调用 SocketClient::send */ }
    char* receiveMessage() override { /* 调用 SocketClient::getMessage */ }
    bool isConnected() override { return mSocket && mSocket->isConnected(); }
    int  bytesAvailable() override { /* SocketClient::bytesAvailable */ }
    void disconnect() override { /* delete mSocket */ }

private:
    Socket* mSocket;
};

GameBackend* createNetworkBackend() { return new NetworkBackend(); }
```

具体方法签名待 Step 1 的 grep 结果填入。

- [ ] **Step 3: 提交**

```bash
git add gameSource/GameBackend.h gameSource/NetworkBackend.h gameSource/NetworkBackend.cpp
git commit -m "feat(client): introduce GameBackend abstraction

将 SocketClient 调用收口到 GameBackend 接口，
当前唯一实现 NetworkBackend 行为与原有逻辑一致。
为未来 EmbeddedBackend（嵌入式服务端）预留扩展点。"
```

---

### Task 2.2: 在 LivingLifePage.cpp 替换 SocketClient 直调

**Files:**
- Modify: `gameSource/LivingLifePage.cpp`（约 30 处）
- Modify: `gameSource/game.cpp`（创建/销毁 backend）

- [ ] **Step 1: 列出所有调用点**

```bash
cd /jfs/fengli16/Projects/CProjects/OneLife
grep -n "openConnection\|sendCharsToServer\|getMessage\|closeSocket" gameSource/LivingLifePage.cpp > /tmp/socketclient_calls.txt
wc -l /tmp/socketclient_calls.txt
```

记下行号。

- [ ] **Step 2: 在 LivingLifePage 中持有 backend 指针**

参考 `LivingLifePage.h` 中现有 socket 成员的位置，插入：

```cpp
// 在私有成员区
GameBackend* mBackend;  // 由 game.cpp 注入，不持有所有权
```

并提供 setter：

```cpp
void LivingLifePage::setBackend(GameBackend* b) { mBackend = b; }
```

- [ ] **Step 3: 逐处替换调用**

例如：

```cpp
// 原代码
sendCharsToServer( mServerSocket, "MOVE 100 200#" );

// 新代码
mBackend->sendMessage( "MOVE 100 200#" );
```

替换所有 30 处。每替换 5-10 处编译一次确保没有遗漏。

- [ ] **Step 4: 桌面端构建回归**

```bash
cd /jfs/fengli16/Projects/CProjects/OneLife/server
./configure 1 && make 2>&1 | tail -5
cd ../gameSource
cd .. && ./configure 1 && cd gameSource
make 2>&1 | tail -10
```

期望：两个二进制都构建成功。

- [ ] **Step 5: 桌面端运行回归**

启动服务端 + 客户端，验证连接、登录、移动、聊天等功能。这一步必须人工测试，确保 GameBackend 重构不破坏桌面端。

- [ ] **Step 6: 提交**

```bash
git add gameSource/LivingLifePage.cpp gameSource/LivingLifePage.h gameSource/game.cpp
git commit -m "refactor(client): route SocketClient through GameBackend

LivingLifePage 不再直接调用 SocketClient，
全部通过 mBackend 间接操作。桌面端功能回归通过。"
```

---

### Task 2.3: 扩展 CMakeLists 接入 gameSource + minorGems

**Files:**
- Modify: `android/CMakeLists.txt`

- [ ] **Step 1: 重写 CMakeLists**

```cmake
cmake_minimum_required(VERSION 3.18)
project(OneLifeAndroid LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 11)

# 路径变量
set(MINORGEMS_DIR ${CMAKE_SOURCE_DIR}/../../minorGems-android-port
    CACHE PATH "minorGems android-port worktree path")
set(GAMESOURCE_DIR ${CMAKE_SOURCE_DIR}/../gameSource
    CACHE PATH "OneLife gameSource path")

# native_app_glue
add_library(native_app_glue STATIC
    ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)
target_include_directories(native_app_glue PUBLIC
    ${ANDROID_NDK}/sources/android/native_app_glue)

# minorGems 源码（按平台筛选）
file(GLOB_RECURSE MINORGEMS_CORE
    ${MINORGEMS_DIR}/util/*.cpp
    ${MINORGEMS_DIR}/io/*.cpp
    ${MINORGEMS_DIR}/network/*.cpp
    ${MINORGEMS_DIR}/system/*.cpp
    ${MINORGEMS_DIR}/graphics/*.cpp
    ${MINORGEMS_DIR}/sound/*.cpp
    ${MINORGEMS_DIR}/crypto/*.cpp
    ${MINORGEMS_DIR}/game/*.cpp
)

# 排除非 Android 平台分支
list(FILTER MINORGEMS_CORE EXCLUDE REGEX "/(win32|mac|portaudio)/")
# 排除 SDL 桌面入口
list(FILTER MINORGEMS_CORE EXCLUDE REGEX "/platforms/SDL/")
# 排除测试程序
list(FILTER MINORGEMS_CORE EXCLUDE REGEX "(test|Test)\\.cpp$")
list(FILTER MINORGEMS_CORE EXCLUDE REGEX "testRaspbian\\.cpp$")
list(FILTER MINORGEMS_CORE EXCLUDE REGEX "/sound/openal/")

# gameSource（排除 editor）
file(GLOB GAMESOURCE_SRC ${GAMESOURCE_DIR}/*.cpp)
list(FILTER GAMESOURCE_SRC EXCLUDE REGEX "(Editor.*|editor)\\.cpp$")

# 主 .so
add_library(onelife SHARED
    jni/android_main.cpp
    jni/AndroidPlatform.cpp
    jni/EGLContext.cpp
    jni/AssetFileBridge.cpp
    jni/TouchInputAdapter.cpp
    ${MINORGEMS_CORE}
    ${GAMESOURCE_SRC}
)

target_include_directories(onelife PRIVATE
    ${MINORGEMS_DIR}
    ${MINORGEMS_DIR}/..   # 让 #include "minorGems/xxx" 工作
    ${GAMESOURCE_DIR}
    ${ANDROID_NDK}/sources/android/native_app_glue
)

target_compile_definitions(onelife PRIVATE
    GLES=1
    LINUX=1                   # minorGems 期望此宏区分 POSIX
    DONT_USE_OPENAL=1
)

target_compile_options(onelife PRIVATE
    -Wno-unused-parameter
    -Wno-unused-variable
    -Wno-deprecated-declarations
    -Wno-write-strings
    -Wno-unused-but-set-variable
)

target_link_libraries(onelife
    native_app_glue
    android log EGL GLESv1_CM OpenSLES
)

target_link_options(onelife PRIVATE "-u" "ANativeActivity_onCreate")
```

- [ ] **Step 2: 编译，记录错误**

```bash
cd /jfs/fengli16/Projects/CProjects/OneLife/android
./build.sh debug 2>&1 | tee build/p2-build.log | tail -50
```

预期会有编译错误（缺少头文件、未定义的符号、平台分支冲突等）。把错误分类，逐个解决：
- 头文件路径：调整 `target_include_directories`
- 平台分支冲突：扩展 `list(FILTER ... EXCLUDE)`
- 未定义符号：可能是 minorGems 引用了 unix 子目录但被排除了

不要试图一次解决所有问题，先解决前 5 个。

- [ ] **Step 3: 反复迭代修复编译错误**

每解决一类错误就重新编译，直到编译通过。常见修复：
- 排除 `sound/portaudio/`、`graphics/3D/` 等 Android 不需要的子模块
- 在 CMake 中增加 `${MINORGEMS_DIR}/io/file/unix/*.cpp` 等显式包含
- 解决符号冲突：minorGems 的 `gameSDL` 入口冲突 → 排除整个 `platforms/SDL/`

- [ ] **Step 4: 链接成功后提交**

```bash
git add android/CMakeLists.txt
git commit -m "build(android): integrate minorGems + gameSource into CMake

按平台过滤 minorGems 源码，复用 unix/linux 子目录的 POSIX 实现，
排除 SDL/portaudio/win32/mac 等不适用 Android 的部分。
gameSource 排除 editor 相关文件。"
```

---

### Task 2.4: 让 android_main 调用 minorGems 主循环

**Files:**
- Modify: `android/jni/android_main.cpp`

- [ ] **Step 1: 重写 android_main**

把 Phase 1 的 p1 测试代码替换为调用 minorGems 框架：

```cpp
#include "AndroidPlatform.h"
#include "EGLContext.h"
#include "AssetFileBridge.h"
#include "TouchInputAdapter.h"
#include <android_native_app_glue.h>
#include <android/log.h>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "OneLife", __VA_ARGS__)

// 由 minorGems gameAndroid.cpp 提供
namespace minorGemsAndroid {
    void platformInit(int width, int height, int targetFrameRate);
    void platformTick();
    void platformShutdown();
}

// OpenSL 启停
extern "C" int  minorGemsAndroid_audioStart();
extern "C" void minorGemsAndroid_audioStop();

static bool gInited = false;

static void onAppCmd(struct android_app* app, int32_t cmd) {
    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            if (!gInited && app->window) {
                if (!OneLifeEGL::init(app)) break;
                OneLifeAndroid::setSurfaceSize(OneLifeEGL::width(), OneLifeEGL::height());
                minorGemsAndroid::platformInit(OneLifeEGL::width(),
                                               OneLifeEGL::height(), 60);
                minorGemsAndroid_audioStart();
                gInited = true;
            }
            break;
        case APP_CMD_TERM_WINDOW:
            if (gInited) {
                minorGemsAndroid_audioStop();
                minorGemsAndroid::platformShutdown();
                OneLifeEGL::term();
                gInited = false;
            }
            break;
        case APP_CMD_DESTROY:
            if (gInited) {
                minorGemsAndroid_audioStop();
                minorGemsAndroid::platformShutdown();
                OneLifeEGL::term();
                gInited = false;
            }
            break;
    }
}

static int32_t onInputEvent(struct android_app* app, AInputEvent* event) {
    return OneLifeTouchInput::handle(event) ? 1 : 0;
}

extern "C" void android_main(struct android_app* app) {
    OneLifeAndroid::setApp(app);
    OneLifeAssetBridge::install(app);

    app->onAppCmd     = onAppCmd;
    app->onInputEvent = onInputEvent;

    LOGI("OneLife Android main loop");

    while (true) {
        int events;
        struct android_poll_source* source;
        while (ALooper_pollAll(gInited ? 0 : -1, nullptr, &events, (void**)&source) >= 0) {
            if (source) source->process(app, source);
            if (app->destroyRequested) {
                LOGI("destroyRequested");
                return;
            }
        }
        if (gInited && OneLifeEGL::ready()) {
            minorGemsAndroid::platformTick();
            OneLifeEGL::swapBuffers();
        }
    }
}
```

- [ ] **Step 2: 编译并安装**

```bash
cd /jfs/fengli16/Projects/CProjects/OneLife/android
./build.sh debug 2>&1 | tail -10
adb install -r build/OneLife-debug.apk
adb shell am start -n com.fengli.onelife/android.app.NativeActivity
adb logcat -s OneLife:* AndroidRuntime:E -t 100
```

期望：APK 启动后能看到 OneLife 主菜单或设置页面。可能因 assets 还未打包出现资源加载错误，到 Task 2.5 解决。

- [ ] **Step 3: 提交**

```bash
git add android/jni/android_main.cpp
git commit -m "build(android): wire android_main into minorGems game loop"
```

---

### Task 2.5: 把 OneLifeData7 资源软链接到 assets

**Files:**
- Create: `android/assets/sprites` (软链接)
- 等等

- [ ] **Step 1: 创建软链接**

```bash
cd /jfs/fengli16/Projects/CProjects/OneLife/android/assets
for d in sprites sounds objects transitions categories animations \
         tutorialMaps contentSettings music ground faces scenes overlays; do
    ln -sf ../../../OneLifeData7/$d $d
done
ln -sf ../../../OneLifeData7/dataVersionNumber.txt dataVersionNumber.txt
ln -sf ../../../OneLifeData7/isAHAP.txt isAHAP.txt
ls -la
```

- [ ] **Step 2: 把 gameSource/settings 也作为默认设置打包**

```bash
mkdir -p assets/default_settings
cp /jfs/fengli16/Projects/CProjects/OneLife/gameSource/settings/*.ini assets/default_settings/ 2>/dev/null || true
ls assets/default_settings | head -5
```

- [ ] **Step 3: 在 AssetFileBridge 中加首次启动复制设置**

修改 `android/jni/AssetFileBridge.cpp`，在 `install()` 末尾加一段：

```cpp
// 首次启动从 assets/default_settings 拷贝 .ini 到 internalDataPath/settings/
namespace {
void firstRunInitSettings(AAssetManager* mgr, const char* dataDir) {
    char settingsDir[512];
    snprintf(settingsDir, sizeof(settingsDir), "%s/settings", dataDir);
    struct stat st;
    if (stat(settingsDir, &st) == 0) return;  // 已存在不重复
    mkdir(settingsDir, 0770);

    AAssetDir* d = AAssetManager_openDir(mgr, "default_settings");
    const char* fn;
    while ((fn = AAssetDir_getNextFileName(d)) != nullptr) {
        char src[512]; snprintf(src, sizeof(src), "default_settings/%s", fn);
        AAsset* a = AAssetManager_open(mgr, src, AASSET_MODE_BUFFER);
        if (!a) continue;
        char dst[512]; snprintf(dst, sizeof(dst), "%s/%s", settingsDir, fn);
        FILE* f = fopen(dst, "wb");
        if (f) {
            const void* buf = AAsset_getBuffer(a);
            fwrite(buf, AAsset_getLength(a), 1, f);
            fclose(f);
        }
        AAsset_close(a);
    }
    AAssetDir_close(d);
}
}

// 在 install() 末尾调用
firstRunInitSettings(app->activity->assetManager, app->activity->internalDataPath);
```

- [ ] **Step 4: 编译并安装**

```bash
cd /jfs/fengli16/Projects/CProjects/OneLife/android
./build.sh debug 2>&1 | tail -10
ls -lh build/OneLife-debug.apk    # 应当显著大于 Phase 1（assets 已打包）
adb install -r build/OneLife-debug.apk
adb shell am start -n com.fengli.onelife/android.app.NativeActivity
adb logcat -s OneLife:* -t 100
```

期望：APK 大小约 50-100MB；应用启动后加载游戏资源，至少看到 OneLife 主菜单。

- [ ] **Step 5: 提交**

```bash
git add android/jni/AssetFileBridge.cpp
git commit -m "build(android): bundle game data and seed settings on first run

assets/ 软链接 OneLifeData7 子目录 + 拷贝 gameSource/settings 为默认值。
首次启动把 default_settings 复制到 internalDataPath/settings/。"
echo "P2 done at $(date '+%Y-%m-%d %H:%M')" >> android/MILESTONES.md
git add android/MILESTONES.md
git commit -m "build(android): Phase 2 done — gameSource boots on Android"
```

---

# Phase 3：触摸输入适配

**目标**：实现完整可玩的游戏操作，包括移动、拾取、放下、聊天。

---

### Task 3.1: 实现 TouchInputAdapter

**Files:**
- Create: `android/jni/TouchInputAdapter.h`
- Create: `android/jni/TouchInputAdapter.cpp`

- [ ] **Step 1: 创建头文件**

```cpp
// android/jni/TouchInputAdapter.h
#ifndef ONELIFE_TOUCH_INPUT_H
#define ONELIFE_TOUCH_INPUT_H

#include <android_native_app_glue.h>

namespace OneLifeTouchInput {
    bool handle(AInputEvent* event);  // 返回 true 表示已消费
}

#endif
```

- [ ] **Step 2: 创建实现文件**

```cpp
#include "TouchInputAdapter.h"
#include "EGLContext.h"
#include <android/input.h>
#include <android/log.h>
#include <math.h>
#include <time.h>

// minorGems 游戏侧接口
extern "C" {
    void pointerDown(float x, float y);
    void pointerMove(float x, float y);
    void pointerUp(float x, float y);
}

namespace {
    constexpr long kLongPressMs = 500;
    constexpr float kMoveThresholdPx = 12.0f;

    bool gShiftDown = false;
    bool gRightButton = false;
    bool gPointerActive = false;
    long gDownTimeMs = 0;
    float gDownX = 0, gDownY = 0;
    int  gActiveFingers = 0;

    long nowMs() {
        struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
        return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
    }

    // 把 NativeActivity 绝对坐标转换为 minorGems 期望的归一化坐标
    // OneLife 桌面端用屏幕像素坐标，这里按 EGL surface 大小直接传
    void emitDown(float x, float y) { pointerDown(x, y); }
    void emitMove(float x, float y) { pointerMove(x, y); }
    void emitUp(float x, float y)   { pointerUp(x, y); }
}

extern "C" bool isShiftDown() { return gShiftDown; }
extern "C" bool isRightButtonDown() { return gRightButton; }

namespace OneLifeTouchInput {

bool handle(AInputEvent* event) {
    int32_t type = AInputEvent_getType(event);
    if (type != AINPUT_EVENT_TYPE_MOTION) return false;

    int32_t action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
    int fingers = AMotionEvent_getPointerCount(event);
    float x = AMotionEvent_getX(event, 0);
    float y = AMotionEvent_getY(event, 0);

    switch (action) {
    case AMOTION_EVENT_ACTION_DOWN:
        gActiveFingers = 1;
        gDownTimeMs = nowMs();
        gDownX = x; gDownY = y;
        gShiftDown = false;
        gRightButton = false;
        gPointerActive = true;
        emitDown(x, y);
        return true;

    case AMOTION_EVENT_ACTION_POINTER_DOWN:
        gActiveFingers = fingers;
        if (fingers == 2 && gPointerActive) {
            // 双指 → Shift+Click
            gShiftDown = true;
            // 如果还没触发长按，重新发射 down 表示这是一次 shift 修饰
            // 这里保守做法：保持原 down 不变，让游戏侧通过 isShiftDown() 自查
        }
        return true;

    case AMOTION_EVENT_ACTION_MOVE:
        if (!gPointerActive) return true;
        // 长按检测：未移动且超过阈值
        if (!gRightButton &&
            (nowMs() - gDownTimeMs) >= kLongPressMs &&
            fabsf(x - gDownX) < kMoveThresholdPx &&
            fabsf(y - gDownY) < kMoveThresholdPx) {
            // 触发右键：先 up 再以 right=true 重新 down
            emitUp(x, y);
            gRightButton = true;
            emitDown(x, y);
        } else {
            emitMove(x, y);
        }
        return true;

    case AMOTION_EVENT_ACTION_UP:
    case AMOTION_EVENT_ACTION_CANCEL:
        emitUp(x, y);
        gPointerActive = false;
        gActiveFingers = 0;
        gShiftDown = false;
        gRightButton = false;
        return true;

    case AMOTION_EVENT_ACTION_POINTER_UP:
        gActiveFingers = fingers - 1;
        return true;
    }
    return false;
}

}
```

- [ ] **Step 3: 在 minorGems 侧暴露修饰键查询**

minorGems 现有 `isCommandKeyDown()`、`isShiftKeyDown()` 等接口。在 worktree 内 grep 这些函数的实现，给 Android 分支增加：

```bash
cd /jfs/fengli16/Projects/CProjects/minorGems-android-port
grep -rn "isShiftKeyDown\|isCommandKeyDown" --include="*.cpp" --include="*.h" | head -10
```

如果发现 minorGems 在 Android 下需要这些函数的实现，在 `game/platforms/Android/gameAndroid.cpp` 末尾加：

```cpp
extern "C" bool isShiftDown();   // 由 OneLife/android/jni/TouchInputAdapter.cpp 提供
extern "C" bool isRightButtonDown();

extern "C" {
    char isShiftKeyDown() { return isShiftDown() ? 1 : 0; }
    char isCommandKeyDown() { return 0; }
    // 其他可能被引用的函数桩
}
```

具体函数列表以编译错误为准。

- [ ] **Step 4: 编译并测试**

```bash
cd /jfs/fengli16/Projects/CProjects/OneLife/android
./build.sh debug 2>&1 | tail -10
adb install -r build/OneLife-debug.apk
adb shell am start -n com.fengli.onelife/android.app.NativeActivity
```

人工测试：
- 单击屏幕：游戏角色应响应（如选择菜单项）
- 长按 1 秒：应触发右键行为
- 双指点击：应触发 shift 修饰行为

- [ ] **Step 5: 提交**

```bash
cd /jfs/fengli16/Projects/CProjects/OneLife
git add android/jni/TouchInputAdapter.h android/jni/TouchInputAdapter.cpp
git commit -m "feat(android): touch-to-mouse adapter

单指=左键，长按>500ms=右键，双指=Shift+左键。
游戏侧通过 isShiftKeyDown() 查询修饰键状态。"
```

---

### Task 3.2: 接入软键盘（聊天）

**Files:**
- Modify: `android/jni/android_main.cpp` 或新增 `SoftKeyboard.cpp`

- [ ] **Step 1: 实现 NDK 软键盘控制**

NDK 没有直接的 `showSoftInput()` 入口，需通过 `ANativeActivity_showSoftInput()`：

```cpp
// android/jni/SoftKeyboard.cpp
#include "AndroidPlatform.h"
#include <android/native_activity.h>

namespace OneLifeKeyboard {

void show() {
    auto app = OneLifeAndroid::getApp();
    if (app && app->activity) {
        ANativeActivity_showSoftInput(app->activity,
            ANATIVEACTIVITY_SHOW_SOFT_INPUT_FORCED);
    }
}
void hide() {
    auto app = OneLifeAndroid::getApp();
    if (app && app->activity) {
        ANativeActivity_hideSoftInput(app->activity,
            ANATIVEACTIVITY_HIDE_SOFT_INPUT_NOT_ALWAYS);
    }
}

}
```

- [ ] **Step 2: 在 TextField 焦点变化时调用**

`gameSource/TextField` 的"获得焦点"和"失去焦点"位置加 `#ifdef __ANDROID__` 调用 show/hide。具体修改位置以 grep 结果为准：

```bash
cd /jfs/fengli16/Projects/CProjects/OneLife
grep -n "setFocus\|gainFocus\|loseFocus" gameSource/TextField.cpp | head -5
```

- [ ] **Step 3: 在 onInputEvent 处理 KEY 事件**

修改 `TouchInputAdapter::handle()`，加 `AINPUT_EVENT_TYPE_KEY` 分支：

```cpp
if (type == AINPUT_EVENT_TYPE_KEY) {
    int32_t action = AKeyEvent_getAction(event);
    int32_t code = AKeyEvent_getKeyCode(event);
    int32_t unicode = AKeyEvent_getMetaState(event);  // 简化
    if (action == AKEY_EVENT_ACTION_DOWN) {
        extern "C" void keyDown(unsigned char inKey);
        if (code >= AKEYCODE_A && code <= AKEYCODE_Z) keyDown('a' + (code - AKEYCODE_A));
        else if (code >= AKEYCODE_0 && code <= AKEYCODE_9) keyDown('0' + (code - AKEYCODE_0));
        else if (code == AKEYCODE_SPACE) keyDown(' ');
        else if (code == AKEYCODE_DEL) keyDown(8);
        else if (code == AKEYCODE_ENTER) keyDown(13);
    }
    return true;
}
```

- [ ] **Step 4: 提交**

```bash
git add android/jni/SoftKeyboard.cpp android/jni/TouchInputAdapter.cpp \
        gameSource/TextField.cpp android/CMakeLists.txt
git commit -m "feat(android): soft keyboard for TextField focus + key event routing"
```

---

### Task 3.3: 屏幕缩放与 DPI 适配

**Files:**
- Modify: `android/jni/EGLContext.cpp` 或 `gameSource/game.cpp`

- [ ] **Step 1: 检测 DPI 并设置渲染分辨率**

minorGems 期望固定渲染分辨率（OneLife 默认 1280x720），物理屏幕分辨率不同时通过 GL 视口缩放。

EGL 已经返回了 surface 的物理像素尺寸。在 `platformInit` 时把 minorGems 的"游戏分辨率"和"屏幕分辨率"分开：

修改 `minorGems-android-port/game/platforms/Android/gameAndroid.cpp`：

```cpp
void platformInit(int width, int height, int targetFrameRate) {
    // OneLife 期望 1280x720 的逻辑分辨率，物理屏幕由 GL viewport 处理
    int logicalW = 1280, logicalH = 720;
    initFrameDrawer(logicalW, logicalH, targetFrameRate, "", false);
    initDrawString(targetFrameRate);
    initGame();
    // 在 drawFrame 之前 setViewport(width, height)
}
```

具体实现取决于 minorGems `ScreenGL` 的 viewport 接口。可能需要查 `ScreenGL::setViewport()` 或 `setupOrtho()`。

- [ ] **Step 2: 编译验证**

```bash
./build.sh debug && adb install -r build/OneLife-debug.apk
```

人工验证：界面不被拉伸或截断，按钮可正常点击。

- [ ] **Step 3: 提交**

```bash
git add ... # 实际改动文件
git commit -m "feat(android): DPI-aware viewport setup"
```

---

### Task 3.4: P3 完整流程验证

**Files:**（无源码改动）

- [ ] **Step 1: 启动局域网服务端**

按 CLAUDE.md 指引：

```bash
cd /jfs/fengli16/Projects/CProjects/OneLife/server
# 设置：requireTicketServerCheck=0, requireClientPassword=1, clientPassword=test123
./OneLifeServer
```

- [ ] **Step 2: 配置 Android 客户端**

```bash
adb shell run-as com.fengli.onelife sh -c 'echo "192.168.x.x" > files/settings/customServerAddress.ini'
adb shell run-as com.fengli.onelife sh -c 'echo "8005" > files/settings/customServerPort.ini'
adb shell run-as com.fengli.onelife sh -c 'echo "test123" > files/settings/serverPassword.ini'
adb shell run-as com.fengli.onelife sh -c 'echo "1" > files/settings/useCustomServer.ini'
adb shell run-as com.fengli.onelife sh -c 'echo "1" > files/settings/requireClientPassword.ini'
```

- [ ] **Step 3: 启动客户端，完整流程测试**

启动后人工测试：
- 出生为婴儿
- 点击移动到附近物品
- 长按物品交互
- 拾取/放下
- 进入聊天，软键盘弹出，输入文字发送

- [ ] **Step 4: 提交里程碑**

```bash
echo "P3 done at $(date '+%Y-%m-%d %H:%M')" >> android/MILESTONES.md
git add android/MILESTONES.md
git commit -m "build(android): Phase 3 done — full gameplay loop on touchscreen"
```

---

# Phase 4：联调

**目标**：与 Linux 服务端做端到端联调，验证多人同步与协议兼容性。

---

### Task 4.1: 多人同步验证

- [ ] **Step 1: 启动 Linux 桌面客户端 + Android 客户端**

两台客户端连接同一服务器，相同密码不同账号。

- [ ] **Step 2: 验证场景**

- 出生在同一区域是否能看到对方
- 走动同步是否流畅
- 物品互动是否一致（A 拾起，B 看到消失）
- 聊天能否互通

- [ ] **Step 3: 记录 bug 列表**

在 `android/PHASE4-BUGS.md` 中记录每个发现的问题（描述、复现步骤、严重程度）。

- [ ] **Step 4: 修复 bug**

逐个修复，每修一个 bug 提交一次：

```bash
git add ...
git commit -m "fix(android): <bug 简述>"
```

- [ ] **Step 5: 完成验证**

```bash
echo "P4 done at $(date '+%Y-%m-%d %H:%M')" >> android/MILESTONES.md
git add android/MILESTONES.md android/PHASE4-BUGS.md
git commit -m "build(android): Phase 4 done — end-to-end LAN play verified"
```

---

# Phase 5：打磨

**目标**：性能、电池、屏幕适配、错误处理。每项独立任务。

---

### Task 5.1: 帧率监控与性能优化

- [ ] **Step 1: 加 FPS 显示**

在 `android_main` 主循环里统计帧时间，每秒打印一次到 logcat。如果帧率持续低于 30 FPS，分析瓶颈：

```bash
adb logcat -s OneLife:* | grep FPS
```

- [ ] **Step 2: 排查热点**

如果是渲染瓶颈：
- 检查纹理分辨率是否过高
- 减少 overdraw

如果是逻辑瓶颈：
- 检查每帧 sprite 加载是否未做缓存

- [ ] **Step 3: 提交**

```bash
git add ...
git commit -m "perf(android): <优化点>"
```

---

### Task 5.2: 后台暂停（电池优化）

- [ ] **Step 1: 在 onAppCmd 处理 PAUSE/RESUME**

```cpp
case APP_CMD_PAUSE:
    // 停止渲染循环（主循环用 ALooper_pollAll 阻塞）
    // 停止音频
    minorGemsAndroid_audioStop();
    break;
case APP_CMD_RESUME:
    minorGemsAndroid_audioStart();
    break;
```

- [ ] **Step 2: 提交**

```bash
git add android/jni/android_main.cpp
git commit -m "feat(android): suspend render+audio when app backgrounded"
```

---

### Task 5.3: 异形屏与刘海屏

- [ ] **Step 1: 处理 safe area**

在 `AndroidManifest.xml` 内 activity 加：

```xml
android:resizeableActivity="false"
```

并且在 themes（如有）中：

```xml
<item name="android:windowLayoutInDisplayCutoutMode">shortEdges</item>
```

由于无 Java，可在 `android_main` 启动时通过 `ANativeWindow_getWidth/Height` + 检查 `AInputEvent` 边距判断；通常 NativeActivity 默认避让，不需要额外处理。

- [ ] **Step 2: 提交**

```bash
git add android/AndroidManifest.xml
git commit -m "build(android): handle display cutouts"
```

---

### Task 5.4: 错误处理与诊断

- [ ] **Step 1: 网络断线提示**

在 NetworkBackend 中检测断线，调用 `setStatusMessage("disconnected")`（minorGems 现有 API）。

- [ ] **Step 2: 提交**

```bash
git add gameSource/NetworkBackend.cpp
git commit -m "feat(client): graceful disconnect notification"
```

---

### Task 5.5: 文档与发布准备

**Files:**
- Create: `android/README.md`

- [ ] **Step 1: 写 README**

```markdown
# OneLife Android Client

Android 客户端，连接 Linux OneLifeServer。

## 构建

1. 安装 Android NDK r25+ 和 build-tools 33+
2. 编辑 `android/config.sh` 设置 `ANDROID_NDK_ROOT`/`ANDROID_SDK_ROOT`
3. 创建 minorGems android-port worktree：
   ```
   cd ../minorGems
   git worktree add -b android-port ../minorGems-android-port
   ```
4. 拉取该分支提交（如已发布）
5. 链接资源：
   ```
   cd android/assets
   for d in sprites sounds objects transitions categories animations \
            tutorialMaps contentSettings music ground faces scenes overlays; do
       ln -sf ../../../OneLifeData7/$d $d
   done
   ```
6. 构建：
   ```
   cd android && ./build.sh debug
   ```

## 运行

```
adb install -r build/OneLife-debug.apk
adb shell am start -n com.fengli.onelife/android.app.NativeActivity
```

首次运行后需配置 `customServerAddress.ini` 等。

## 已知限制

- 仅支持 API 29+（Android 10+）
- 横屏锁定
- 暂不支持完整离线模式（GameBackend 抽象已就位，待 EmbeddedBackend 实现）
```

- [ ] **Step 2: 提交里程碑**

```bash
git add android/README.md
echo "P5 done at $(date '+%Y-%m-%d %H:%M')" >> android/MILESTONES.md
git add android/MILESTONES.md
git commit -m "docs(android): user-facing README and Phase 5 done"
```

---

## 全局完成标准

满足以下全部条件即视为本计划完成：

- [ ] `cd android && ./build.sh debug` 一键构建生成有效 APK
- [ ] APK 在 Android 10+ 设备上能启动到 OneLife 主菜单
- [ ] 配置局域网服务地址后能连接 Linux OneLifeServer
- [ ] 完成完整生命周期（出生→交互→死亡）
- [ ] 与 Linux 桌面客户端在同一服务器互动行为一致
- [ ] 桌面端 server/client 构建+运行回归通过（GameBackend 重构未破坏桌面端）
- [ ] `minorGems-android-port` 分支与 master 干净分离，未污染主分支

---

## 自审记录

### Spec 覆盖检查

- ✅ 方案 A 实施 → 全部 6 个 Phase
- ✅ NDK 原生 API（不引入 SDL2）→ Phase 0+1 直接用 EGL/GLES
- ✅ NativeActivity → Task 0.5 + Task 2.4
- ✅ 横屏锁定 → Task 0.3 Manifest
- ✅ API 29 → Task 0.3 Manifest + Task 0.1 config.sh
- ✅ 触摸→鼠标 → Task 3.1
- ✅ 资源打包 → Task 2.5
- ✅ GameBackend 抽象 → Task 2.1-2.2
- ✅ minorGems worktree → Task 0.2
- ✅ GLES 1.x 复用 Raspbian → Task 1.1
- ✅ 桌面端构建不受影响 → Task 2.2 Step 4 回归

### 类型一致性检查

- `GameBackend::receiveMessage()` 返回 `char*`，caller `delete[]` —— 在 Task 2.1 与 NetworkBackend 实现里一致
- `pointerDown/Move/Up(float, float)` —— 在 TouchInputAdapter 与 minorGems 接口一致
- `minorGemsAndroid::setAssetManager` 与 OneLifeAssetBridge::install 一致

### 风险确认

- OpenGL 固定管线：Raspbian 已有 `#ifdef GLES` 替代实现，复用即可
- glu* 调用：Task 1.1 Step 2 增加了显式检查
- 桌面端被破坏：Task 2.2 Step 4 强制回归构建+运行
