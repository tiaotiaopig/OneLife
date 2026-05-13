// SoftKeyboard.h
// Android 软键盘控制（Phase 3 Task 3.2）
// 通过 ANativeActivity_showSoftInput/hideSoftInput 控制系统输入法。
// 由 TextField 焦点变化时调用。

#ifndef ONELIFE_SOFT_KEYBOARD_H
#define ONELIFE_SOFT_KEYBOARD_H

namespace onelife {
namespace SoftKeyboard {

// 显示软键盘（强制模式，即使用户之前手动关闭也会再次弹出）
void show();

// 隐藏软键盘
void hide();

}  // namespace SoftKeyboard
}  // namespace onelife

#endif  // ONELIFE_SOFT_KEYBOARD_H
