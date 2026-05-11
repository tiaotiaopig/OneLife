#ifndef ONELIFE_ANDROID_PLATFORM_H
#define ONELIFE_ANDROID_PLATFORM_H

#include <android/asset_manager.h>

namespace onelife {

class AndroidPlatform {
public:
    // 在 NativeActivity onCreate 时调用，设置 AAssetManager 和内部存储路径
    static void setAssetManager(AAssetManager* mgr);
    static void setInternalDataPath(const char* path);

    // 在 surface ready 后调用，初始化 minorGems + 音频
    static void init(int width, int height);

    // 每帧调用
    static void tick();

    // 在 destroy 时调用
    static void shutdown();
};

}  // namespace onelife

#endif
