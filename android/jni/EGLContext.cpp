#include "EGLContext.h"
#include <GLES/gl.h>
#include <android/log.h>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "OneLife", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "OneLife", __VA_ARGS__)

namespace onelife {

EGLContextWrapper::EGLContextWrapper()
    : mDisplay(EGL_NO_DISPLAY)
    , mContext(EGL_NO_CONTEXT)
    , mSurface(EGL_NO_SURFACE)
    , mWidth(0)
    , mHeight(0)
{}

EGLContextWrapper::~EGLContextWrapper() {
    term();
}

int EGLContextWrapper::init(ANativeWindow* window) {
    if (!window) {
        LOGE("EGLContextWrapper::init: null window");
        return -1;
    }

    const EGLint attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES_BIT,
        EGL_BLUE_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_RED_SIZE, 8,
        EGL_DEPTH_SIZE, 16,
        EGL_NONE
    };

    mDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (mDisplay == EGL_NO_DISPLAY) {
        LOGE("eglGetDisplay failed");
        return -1;
    }

    if (!eglInitialize(mDisplay, nullptr, nullptr)) {
        LOGE("eglInitialize failed");
        return -1;
    }

    EGLConfig config;
    EGLint numConfigs;
    if (!eglChooseConfig(mDisplay, attribs, &config, 1, &numConfigs) || numConfigs == 0) {
        LOGE("eglChooseConfig failed");
        term();
        return -1;
    }

    EGLint format;
    eglGetConfigAttrib(mDisplay, config, EGL_NATIVE_VISUAL_ID, &format);
    ANativeWindow_setBuffersGeometry(window, 0, 0, format);

    mSurface = eglCreateWindowSurface(mDisplay, config, window, nullptr);
    if (mSurface == EGL_NO_SURFACE) {
        LOGE("eglCreateWindowSurface failed");
        term();
        return -1;
    }

    mContext = eglCreateContext(mDisplay, config, EGL_NO_CONTEXT, nullptr);
    if (mContext == EGL_NO_CONTEXT) {
        LOGE("eglCreateContext failed");
        term();
        return -1;
    }

    if (!eglMakeCurrent(mDisplay, mSurface, mSurface, mContext)) {
        LOGE("eglMakeCurrent failed");
        term();
        return -1;
    }

    eglQuerySurface(mDisplay, mSurface, EGL_WIDTH, &mWidth);
    eglQuerySurface(mDisplay, mSurface, EGL_HEIGHT, &mHeight);

    LOGI("EGL initialized: %dx%d", mWidth, mHeight);
    return 0;
}

void EGLContextWrapper::term() {
    if (mDisplay != EGL_NO_DISPLAY) {
        eglMakeCurrent(mDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (mContext != EGL_NO_CONTEXT) {
            eglDestroyContext(mDisplay, mContext);
            mContext = EGL_NO_CONTEXT;
        }
        if (mSurface != EGL_NO_SURFACE) {
            eglDestroySurface(mDisplay, mSurface);
            mSurface = EGL_NO_SURFACE;
        }
        eglTerminate(mDisplay);
        mDisplay = EGL_NO_DISPLAY;
    }
    mWidth = mHeight = 0;
}

void EGLContextWrapper::swapBuffers() {
    if (mDisplay != EGL_NO_DISPLAY && mSurface != EGL_NO_SURFACE) {
        eglSwapBuffers(mDisplay, mSurface);
    }
}

}  // namespace onelife
