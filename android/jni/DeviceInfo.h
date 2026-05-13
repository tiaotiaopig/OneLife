// DeviceInfo.h
// 真机诊断：启动时打印设备信息（屏幕、内存、GL），运行时统计 FPS。
//
// 所有信息走 logcat（Tag: OneLife），不影响游戏逻辑。

#ifndef ONELIFE_DEVICE_INFO_H
#define ONELIFE_DEVICE_INFO_H

#include <android_native_app_glue.h>

namespace onelife {
namespace DeviceInfo {

// 启动时调用一次：打印硬件/系统/Android 配置信息
void logStartupInfo(struct android_app* app);

// EGL 就绪后调用：打印 GL 信息（vendor/renderer/version/extensions）
void logGLInfo();

// 每帧调用：累积帧时间，每秒打印一次 FPS + 当前帧时间
void tickFPS();

}  // namespace DeviceInfo
}  // namespace onelife

#endif  // ONELIFE_DEVICE_INFO_H
