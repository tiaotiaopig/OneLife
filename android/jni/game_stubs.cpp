// 临时桩函数：用于 Phase 1 验证 CMakeLists.txt 链接通路
//
// 这些是 minorGems 框架（gameAndroid.cpp / OpenSLAudioBackend.cpp）
// 需要的游戏侧回调，正常情况下由 gameSource/game.cpp 提供。
//
// TODO(Task 2.4): 接入 gameSource 后删除此文件

#include <stdint.h>
#include <android/log.h>

typedef uint8_t Uint8;

#define LOG_TAG "OneLifeStub"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

// gameSource 批 1 引用的全局变量（正常由 game.cpp 定义）
// 接入完整 game.cpp 后这些定义会从 game.cpp 提供，此处的定义需移除
int dataVersionNumber = 0;
char *accountKey = nullptr;
int serverSequenceNumber = 0;
int accountHmacVersionNumber = 0;

extern "C" {

// 音频采样率（CD 质量立体声）
int getSampleRate() {
    return 44100;
}

// 音频回调：填充静音（gameSource 接入后会输出实际音频）
void getSoundSamples(Uint8* inBuffer, int inLengthToFillInBytes) {
    if (inBuffer && inLengthToFillInBytes > 0) {
        for (int i = 0; i < inLengthToFillInBytes; ++i) {
            inBuffer[i] = 0;
        }
    }
}

// 字符串绘制初始化
void initDrawString(int inWidth, int inHeight) {
    LOGI("stub initDrawString %dx%d", inWidth, inHeight);
}

// 字符串绘制清理
void freeDrawString() {
    LOGI("stub freeDrawString");
}

// 帧绘制初始化
void initFrameDrawer(int inWidth, int inHeight, int inTargetFrameRate,
                     const char* inCustomRecordedGameData,
                     char inPlayingBack) {
    LOGI("stub initFrameDrawer %dx%d @%dfps", inWidth, inHeight, inTargetFrameRate);
}

// 帧绘制清理
void freeFrameDrawer() {
    LOGI("stub freeFrameDrawer");
}

// 每帧绘制
void drawFrame(char inUpdate) {
    // 桩：什么都不做，由 android_main 中的 EGL 清屏代替
    (void)inUpdate;
}

}  // extern "C"
