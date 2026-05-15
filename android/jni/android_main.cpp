// OneLife Android 入口（Task 2.4：接入 gameSource 主循环）
// EGL 就绪后调用 minorGemsAndroid::platformInit 启动 gameSource，
// 每帧调用 platformTick（内部 drawFrame），销毁时 platformShutdown。

#include <android_native_app_glue.h>
#include <EGL/egl.h>
#include <GLES/gl.h>
#include <android/log.h>
#include <unistd.h>

#include "AssetFileBridge.h"
#include "TouchInputAdapter.h"
#include "DeviceInfo.h"

// OpenSL ES 音频后端（minorGems/sound/android/OpenSLAudioBackend.cpp）
extern "C" int  minorGemsAndroid_audioStart();
extern "C" void minorGemsAndroid_audioStop();

// minorGems Android 平台层接口（FileAndroid.cpp / gameAndroid.cpp 中定义）
namespace minorGemsAndroid {
    void setAssetManager(AAssetManager* mgr);
    void setInternalDataPath(const char* path);
    void platformInit(int width, int height, int targetFrameRate);
    void platformTick();
    void platformShutdown();
}

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  "OneLife", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "OneLife", __VA_ARGS__)

// 全局 android_app 指针（供 SoftKeyboard 等模块访问）
static struct android_app* gAndroidApp = nullptr;

extern "C" struct android_app* onelifeAndroidGetApp() {
    return gAndroidApp;
}

struct AppState {
    EGLDisplay display = EGL_NO_DISPLAY;
    EGLContext context = EGL_NO_CONTEXT;
    EGLSurface surface = EGL_NO_SURFACE;
    int  width  = 0;
    int  height = 0;
    bool platformInitialized = false;  // gameSource 是否已启动
    bool audioStarted = false;          // OpenSL ES 是否已启动
    bool paused = false;                // 后台暂停（停止渲染，节省电池）
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

    // 启用 alpha 混合（与桌面端 ScreenGL_SDL.cpp 一致）
    // 字体渲染依赖此设置:字符纹理是 RGB=255 + A=字符强度,
    // 没有 GL_BLEND 会显示为不透明白色方块
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    onelife::DeviceInfo::logGLInfo();
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

// 每帧调用：把控制权交给 gameSource 的 drawFrame，再 swapBuffers。
static void tickGame(AppState* s) {
    if (s->display == EGL_NO_DISPLAY) return;
    static int frameCount = 0;
    frameCount++;
    if (frameCount <= 3 || frameCount % 300 == 0) {
        LOGI("tickGame #%d (%dx%d)", frameCount, s->width, s->height);
    }
    glViewport(0, 0, s->width, s->height);
    minorGemsAndroid::platformTick();
    EGLBoolean ok = eglSwapBuffers(s->display, s->surface);
    if (!ok) {
        EGLint err = eglGetError();
        LOGE("eglSwapBuffers failed, err=0x%x", err);
    }
}

// EGL 就绪但 gameSource 尚未启动时的占位渲染（理论上不会触发，仅兜底）
static void drawFrameFallback(AppState* s) {
    if (s->display == EGL_NO_DISPLAY) return;
    glViewport(0, 0, s->width, s->height);
    glClearColor(0.23f, 0.49f, 0.23f, 1.0f);  // OneLife 绿
    glClear(GL_COLOR_BUFFER_BIT);
    eglSwapBuffers(s->display, s->surface);
}

static void onAppCmd(struct android_app* app, int32_t cmd) {
    AppState* s = (AppState*)app->userData;
    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            if (app->window) {
                // 注入 AAssetManager + 内部存储路径（供 File API 和 AssetFileBridge 使用）
                if (app->activity) {
                    minorGemsAndroid::setAssetManager(app->activity->assetManager);
                    minorGemsAndroid::setInternalDataPath(app->activity->internalDataPath);
                    // 切换工作目录到内部存储，让 fopen 相对路径写入能落到正确位置
                    if (app->activity->internalDataPath) {
                        chdir(app->activity->internalDataPath);
                    }
                    // 首次启动复制默认设置（settings/*.ini）
                    onelife::AssetFileBridge::copyDefaultSettings();
                }
                if (initEGL(app, s) == 0 && !s->platformInitialized) {
                    LOGI("Starting gameSource via platformInit(%dx%d @60fps)",
                         s->width, s->height);
                    minorGemsAndroid::platformInit(s->width, s->height, 60);
                    s->platformInitialized = true;
                    LOGI("gameSource platformInit returned");

                    // 启动 OpenSL ES 音频（必须在 platformInit 之后，
                    // 因为 getSampleRate() 依赖 gameSource 已初始化）
                    if (!s->audioStarted) {
                        if (minorGemsAndroid_audioStart() == 0) {
                            s->audioStarted = true;
                        } else {
                            LOGE("Audio disabled (OpenSL ES init failed)");
                        }
                    }
                }
            }
            break;
        case APP_CMD_TERM_WINDOW:
        case APP_CMD_DESTROY:
            if (s->audioStarted) {
                minorGemsAndroid_audioStop();
                s->audioStarted = false;
            }
            if (s->platformInitialized) {
                LOGI("Shutting down gameSource");
                minorGemsAndroid::platformShutdown();
                s->platformInitialized = false;
            }
            termEGL(s);
            break;
        case APP_CMD_PAUSE:
            LOGI("App paused — stopping render loop");
            s->paused = true;
            if (s->audioStarted) {
                minorGemsAndroid_audioStop();
                s->audioStarted = false;
            }
            break;
        case APP_CMD_RESUME:
            LOGI("App resumed — restarting render loop");
            s->paused = false;
            if (s->platformInitialized && !s->audioStarted) {
                if (minorGemsAndroid_audioStart() == 0) {
                    s->audioStarted = true;
                }
            }
            break;
        default:
            break;
    }
}

extern "C" void android_main(struct android_app* app) {
    gAndroidApp = app;  // 保存全局指针供 SoftKeyboard 等模块使用

    AppState state{};
    app->userData = &state;
    app->onAppCmd = onAppCmd;
    app->onInputEvent = [](struct android_app*, AInputEvent* e) -> int32_t {
        return onelife::TouchInputAdapter::handle(e);
    };

    LOGI("OneLife Android starting...");
    onelife::DeviceInfo::logStartupInfo(app);

    while (true) {
        int events;
        struct android_poll_source* source;
        // 暂停或无 display 时阻塞等待事件（-1），否则非阻塞（0）
        int timeout = (state.paused || state.display == EGL_NO_DISPLAY) ? -1 : 0;
        while (ALooper_pollAll(timeout, nullptr, &events, (void**)&source) >= 0) {
            if (source) source->process(app, source);
            if (app->destroyRequested) {
                if (state.platformInitialized) {
                    minorGemsAndroid::platformShutdown();
                    state.platformInitialized = false;
                }
                termEGL(&state);
                LOGI("OneLife Android exiting");
                return;
            }
            // 内层循环也用同样的 timeout（暂停时持续阻塞）
            timeout = (state.paused || state.display == EGL_NO_DISPLAY) ? -1 : 0;
        }
        if (state.platformInitialized && !state.paused) {
            tickGame(&state);
            onelife::DeviceInfo::tickFPS();
        } else if (state.display != EGL_NO_DISPLAY && !state.paused) {
            drawFrameFallback(&state);
        }
    }
}
