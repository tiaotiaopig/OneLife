// SoftKeyboard.cpp
// Android 软键盘控制实现（Phase 3 Task 3.2）

#ifdef __ANDROID__

#include "SoftKeyboard.h"

#include <android/native_activity.h>
#include <android_native_app_glue.h>
#include <android/log.h>

#define LOG_TAG "OneLifeKeyboard"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

// 由 android_main.cpp 提供的全局 android_app 访问器
extern "C" struct android_app* onelifeAndroidGetApp();

namespace onelife {
namespace SoftKeyboard {

void show() {
    struct android_app* app = onelifeAndroidGetApp();
    if (!app || !app->activity) {
        LOGI("show: app or activity is null");
        return;
    }

    // ANATIVEACTIVITY_SHOW_SOFT_INPUT_FORCED：即使用户之前手动关闭也强制弹出
    ANativeActivity_showSoftInput(app->activity,
                                  ANATIVEACTIVITY_SHOW_SOFT_INPUT_FORCED);
    LOGI("showSoftInput called");
}

void hide() {
    struct android_app* app = onelifeAndroidGetApp();
    if (!app || !app->activity) {
        return;
    }

    // ANATIVEACTIVITY_HIDE_SOFT_INPUT_NOT_ALWAYS：仅当键盘由本应用显示时才隐藏
    ANativeActivity_hideSoftInput(app->activity,
                                  ANATIVEACTIVITY_HIDE_SOFT_INPUT_NOT_ALWAYS);
    LOGI("hideSoftInput called");
}

}  // namespace SoftKeyboard
}  // namespace onelife

#endif  // __ANDROID__
