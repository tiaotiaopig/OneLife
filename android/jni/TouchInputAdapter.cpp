// TouchInputAdapter.cpp
// 触摸 → 鼠标事件映射（Phase 3 Task 3.1）
//
// 入口：handle(event) 由 android_main 的 onInputEvent 回调调用。
//
// 语义：
//   单指 tap                      → 左键 pointerDown/Up
//   单指拖拽                      → pointerDrag（down/drag/up）
//   单指长按 >= 500ms 且未怎么动   → pointerUp 时置右键状态
//   双指按下                      → Shift 修饰（pointer 仍按左键处理）
//
// 坐标：AMotionEvent 返回物理像素，走 screenToWorld（当前 stub 为 identity）
// 转为 gameSource 期望的"世界坐标"。

#ifdef __ANDROID__

#include "TouchInputAdapter.h"

#include <android/input.h>
#include <android/log.h>
#include <math.h>
#include <time.h>

#define LOG_TAG "OneLifeTouch"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

// gameSource 回调（C++ 链接，由 gameSource/game.cpp 提供）
void pointerDown(float inX, float inY);
void pointerMove(float inX, float inY);
void pointerDrag(float inX, float inY);
void pointerUp(float inX, float inY);
void keyDown(unsigned char inASCII);
void keyUp(unsigned char inASCII);

// screenToWorld：物理像素 → 世界坐标
// 当前 Android 下由 game_stubs.cpp 提供 identity 实现（入参=出参），
// 但仍走这个调用，未来替换为真实实现时此处无需改动
void screenToWorld(int inX, int inY, float* outX, float* outY);

namespace {

// ---- 配置 ----------------------------------------------------------------
constexpr long kLongPressMs = 500;   // 长按阈值
constexpr float kTapSlopPx   = 24.0f; // 长按时允许的轻微抖动（像素）

// ---- 状态 ----------------------------------------------------------------
bool  gPointerActive = false;   // 是否有手指按下
bool  gDidDrag       = false;   // 本次按下期间是否判定为拖拽
long  gDownTimeMs    = 0;       // 按下时刻（单调时钟，毫秒）
float gDownX         = 0;       // 按下时的物理像素
float gDownY         = 0;
int   gLastMouseX    = 0;       // 最近一次事件的物理像素（供 getLastMouseScreenPos）
int   gLastMouseY    = 0;
int   gActiveFingers = 0;       // 当前触摸手指数

bool  gShiftDown        = false; // 双指 → Shift 修饰
bool  gLastButtonRight  = false; // 最近一次松开是否为长按 → 右键

long nowMs() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000L + ts.tv_nsec / 1000000L;
}

// 把物理像素（AMotionEvent 原始坐标）转为 gameSource 使用的世界坐标
void toWorld(float inX, float inY, float* outX, float* outY) {
    int ix = (int)inX;
    int iy = (int)inY;
    screenToWorld(ix, iy, outX, outY);
    gLastMouseX = ix;
    gLastMouseY = iy;
}

// 把 AKEYCODE 映射到 ASCII（简化版，仅支持常用字符）
// 返回 0 表示无法映射
unsigned char mapKeyCodeToASCII(int32_t keyCode, int32_t metaState) {
    // Shift 修饰
    bool shift = (metaState & AMETA_SHIFT_ON) != 0;

    // 字母 A-Z
    if (keyCode >= AKEYCODE_A && keyCode <= AKEYCODE_Z) {
        char base = 'a' + (keyCode - AKEYCODE_A);
        return shift ? (base - 32) : base;  // Shift → 大写
    }

    // 数字 0-9
    if (keyCode >= AKEYCODE_0 && keyCode <= AKEYCODE_9) {
        if (shift) {
            // Shift + 数字 → 符号（美式键盘布局）
            const char symbols[] = ")!@#$%^&*(";
            return symbols[keyCode - AKEYCODE_0];
        }
        return '0' + (keyCode - AKEYCODE_0);
    }

    // 特殊键
    switch (keyCode) {
    case AKEYCODE_SPACE:      return ' ';
    case AKEYCODE_ENTER:      return 13;  // '\r'
    case AKEYCODE_DEL:        return 8;   // Backspace
    case AKEYCODE_TAB:        return 9;
    case AKEYCODE_PERIOD:     return shift ? '>' : '.';
    case AKEYCODE_COMMA:      return shift ? '<' : ',';
    case AKEYCODE_SLASH:      return shift ? '?' : '/';
    case AKEYCODE_SEMICOLON:  return shift ? ':' : ';';
    case AKEYCODE_APOSTROPHE: return shift ? '"' : '\'';
    case AKEYCODE_LEFT_BRACKET:  return shift ? '{' : '[';
    case AKEYCODE_RIGHT_BRACKET: return shift ? '}' : ']';
    case AKEYCODE_BACKSLASH:     return shift ? '|' : '\\';
    case AKEYCODE_MINUS:      return shift ? '_' : '-';
    case AKEYCODE_EQUALS:     return shift ? '+' : '=';
    case AKEYCODE_GRAVE:      return shift ? '~' : '`';
    default:                  return 0;
    }
}

// 处理键盘事件（软键盘输入）
int32_t handleKey(AInputEvent* event) {
    int32_t action   = AKeyEvent_getAction(event);
    int32_t keyCode  = AKeyEvent_getKeyCode(event);
    int32_t metaState = AKeyEvent_getMetaState(event);

    unsigned char ascii = mapKeyCodeToASCII(keyCode, metaState);
    if (ascii == 0) {
        return 0;  // 无法映射，不消费
    }

    if (action == AKEY_EVENT_ACTION_DOWN) {
        keyDown(ascii);
        return 1;
    } else if (action == AKEY_EVENT_ACTION_UP) {
        keyUp(ascii);
        return 1;
    }

    return 0;
}

// 只处理第 0 号手指（gameSource 是单指鼠标模型）
int32_t handleMotion(AInputEvent* event) {
    int32_t actionRaw = AMotionEvent_getAction(event);
    int32_t action    = actionRaw & AMOTION_EVENT_ACTION_MASK;
    int     fingers   = (int)AMotionEvent_getPointerCount(event);

    float rx = AMotionEvent_getX(event, 0);
    float ry = AMotionEvent_getY(event, 0);

    float wx = 0, wy = 0;
    toWorld(rx, ry, &wx, &wy);

    switch (action) {
    case AMOTION_EVENT_ACTION_DOWN: {
        gPointerActive = true;
        gDidDrag       = false;
        gDownTimeMs    = nowMs();
        gDownX         = rx;
        gDownY         = ry;
        gActiveFingers = 1;
        gShiftDown     = false;
        gLastButtonRight = false;  // 新一次按下，清除上一次的右键状态
        pointerDown(wx, wy);
        return 1;
    }

    case AMOTION_EVENT_ACTION_POINTER_DOWN: {
        gActiveFingers = fingers;
        if (fingers >= 2) {
            // 双指：进入 Shift 修饰，左键语义保持
            gShiftDown = true;
        }
        return 1;
    }

    case AMOTION_EVENT_ACTION_MOVE: {
        if (!gPointerActive) return 1;
        float dx = fabsf(rx - gDownX);
        float dy = fabsf(ry - gDownY);
        if (dx > kTapSlopPx || dy > kTapSlopPx) {
            gDidDrag = true;
        }
        if (gDidDrag) {
            pointerDrag(wx, wy);
        } else {
            pointerMove(wx, wy);
        }
        return 1;
    }

    case AMOTION_EVENT_ACTION_POINTER_UP: {
        // 多指中有一根抬起
        gActiveFingers = fingers - 1;
        // 最后一根副手指也抬起 → 清除 Shift
        if (gActiveFingers <= 1) {
            gShiftDown = false;
        }
        return 1;
    }

    case AMOTION_EVENT_ACTION_UP:
    case AMOTION_EVENT_ACTION_CANCEL: {
        if (gPointerActive) {
            // 判定长按（未怎么移动且持续 >= 500ms）
            long dur = nowMs() - gDownTimeMs;
            float dx = fabsf(rx - gDownX);
            float dy = fabsf(ry - gDownY);
            bool longPress = (!gDidDrag) &&
                             (dur >= kLongPressMs) &&
                             (dx <= kTapSlopPx) && (dy <= kTapSlopPx);
            gLastButtonRight = longPress;

            pointerUp(wx, wy);

            LOGI("up (%.0f,%.0f) dur=%ldms drag=%d right=%d shift=%d",
                 rx, ry, dur, (int)gDidDrag,
                 (int)gLastButtonRight, (int)gShiftDown);
        }
        gPointerActive = false;
        gDidDrag       = false;
        gActiveFingers = 0;
        gShiftDown     = false;
        return 1;
    }

    default:
        return 0;
    }
}

}  // anonymous namespace

namespace onelife {
namespace TouchInputAdapter {

int handle(AInputEvent* event) {
    if (!event) return 0;
    int32_t type = AInputEvent_getType(event);
    if (type == AINPUT_EVENT_TYPE_MOTION) {
        return handleMotion(event);
    } else if (type == AINPUT_EVENT_TYPE_KEY) {
        return handleKey(event);
    }
    return 0;
}

}  // namespace TouchInputAdapter
}  // namespace onelife

// ---- 供 game_stubs.cpp 查询 ---------------------------------------------
extern "C" {

int onelifeAndroidIsShiftDown()       { return gShiftDown ? 1 : 0; }
int onelifeAndroidIsRightButtonDown() { return gLastButtonRight ? 1 : 0; }
int onelifeAndroidLastMouseX()        { return gLastMouseX; }
int onelifeAndroidLastMouseY()        { return gLastMouseY; }

}

#endif  // __ANDROID__
