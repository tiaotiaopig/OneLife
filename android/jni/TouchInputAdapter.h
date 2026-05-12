// TouchInputAdapter.h
// 把 NativeActivity 的 AInputEvent（触摸/按键）翻译为 gameSource 期望的
// 鼠标/键盘事件。对外只暴露两个入口：
//   - handle(event)        由 android_main 的 onInputEvent 调用
//   - 状态 accessor（extern "C"）供 game_stubs.cpp 中的 isShiftKeyDown /
//     isLastMouseButtonRight / getLastMouseScreenPos 等 stub 读取

#ifndef ONELIFE_TOUCH_INPUT_ADAPTER_H
#define ONELIFE_TOUCH_INPUT_ADAPTER_H

#include <android/input.h>

namespace onelife {
namespace TouchInputAdapter {

// 处理一个 NativeActivity 输入事件。
// 返回 1 表示已消费（不再由系统处理），0 表示未处理。
int handle(AInputEvent* event);

}  // namespace TouchInputAdapter
}  // namespace onelife

// 供 C 链接的 stub 查询触摸状态
extern "C" {
    int  onelifeAndroidIsShiftDown();        // 双指按下视为 Shift
    int  onelifeAndroidIsRightButtonDown();  // 长按后进入的"右键"模式
    int  onelifeAndroidLastMouseX();         // 最近一次事件的物理像素 X
    int  onelifeAndroidLastMouseY();         //                         Y
}

#endif  // ONELIFE_TOUCH_INPUT_ADAPTER_H
