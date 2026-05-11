#include "AndroidPlatform.h"
#include <android/log.h>
#include <unistd.h>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "OneLife", __VA_ARGS__)

// minorGems 接口（在 minorGems-android-port 中定义）
namespace minorGemsAndroid {
    void setAssetManager(AAssetManager* mgr);
    void setInternalDataPath(const char* path);
    void platformInit(int width, int height, int targetFrameRate);
    void platformTick();
    void platformShutdown();
}

// OpenSL 音频接口（在 minorGems-android-port/sound/android/ 中定义）
extern "C" {
    int minorGemsAndroid_audioStart();
    void minorGemsAndroid_audioStop();
}

namespace onelife {

void AndroidPlatform::setAssetManager(AAssetManager* mgr) {
    minorGemsAndroid::setAssetManager(mgr);
}

void AndroidPlatform::setInternalDataPath(const char* path) {
    minorGemsAndroid::setInternalDataPath(path);
    // 切换工作目录到内部存储，让 fopen 相对路径写入能落到正确位置
    if (path && chdir(path) == 0) {
        LOGI("chdir to internal storage: %s", path);
    }
}

void AndroidPlatform::init(int width, int height) {
    LOGI("AndroidPlatform::init %dx%d", width, height);
    minorGemsAndroid::platformInit(width, height, 60);  // 60 fps target
    minorGemsAndroid_audioStart();
}

void AndroidPlatform::tick() {
    minorGemsAndroid::platformTick();
}

void AndroidPlatform::shutdown() {
    LOGI("AndroidPlatform::shutdown");
    minorGemsAndroid_audioStop();
    minorGemsAndroid::platformShutdown();
}

}  // namespace onelife
