#ifndef ONELIFE_ANDROID_EGL_CONTEXT_H
#define ONELIFE_ANDROID_EGL_CONTEXT_H

#include <EGL/egl.h>
#include <android/native_window.h>

namespace onelife {

class EGLContext {
public:
    EGLContext();
    ~EGLContext();

    // 初始化 EGL，绑定到 ANativeWindow
    // 返回 0 成功，-1 失败
    int init(ANativeWindow* window);

    // 清理 EGL 资源
    void term();

    // 交换缓冲区
    void swapBuffers();

    // 获取 surface 尺寸
    int getWidth() const { return mWidth; }
    int getHeight() const { return mHeight; }

    bool isReady() const { return mDisplay != EGL_NO_DISPLAY; }

private:
    EGLDisplay mDisplay;
    EGLContext mContext;
    EGLSurface mSurface;
    int mWidth;
    int mHeight;
};

}  // namespace onelife

#endif
