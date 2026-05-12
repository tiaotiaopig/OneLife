// OneLife Android 入口（Phase 0 占位版本）
// 仅初始化 EGL 并清屏为绿色，验证 NativeActivity 工具链是否打通。
// 真正的游戏入口在 Phase 1 之后接入。

#include <android_native_app_glue.h>
#include <EGL/egl.h>
#include <GLES/gl.h>
#include <android/log.h>
#include <unistd.h>

#include "AssetFileBridge.h"

// FileAndroid.cpp 中定义的全局注入函数
namespace minorGemsAndroid {
    void setAssetManager(AAssetManager* mgr);
    void setInternalDataPath(const char* path);
}

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
    static int frameCount = 0;
    frameCount++;
    // 仅在前几帧和每 5 秒（300 帧 @ 60fps）打日志，确认渲染循环存活
    if (frameCount <= 3 || frameCount % 300 == 0) {
        LOGI("drawFrame #%d (%dx%d)", frameCount, s->width, s->height);
    }
    glViewport(0, 0, s->width, s->height);
    glClearColor(0.23f, 0.49f, 0.23f, 1.0f);  // OneLife 绿
    glClear(GL_COLOR_BUFFER_BIT);
    EGLBoolean ok = eglSwapBuffers(s->display, s->surface);
    if (!ok) {
        EGLint err = eglGetError();
        LOGE("eglSwapBuffers failed, err=0x%x", err);
    }
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
                    // 首次启动复制默认设置
                    onelife::AssetFileBridge::copyDefaultSettings();
                }
                initEGL(app, s);
            }
            break;
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
