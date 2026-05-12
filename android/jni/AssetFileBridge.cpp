#include "AssetFileBridge.h"
#include <android/asset_manager.h>
#include <android/log.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  "OneLife", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "OneLife", __VA_ARGS__)

// FileAndroid.cpp 中定义的全局访问器
namespace minorGemsAndroid {
    AAssetManager* getAssetManager();
    const char*    getInternalDataPath();
}

namespace onelife {

void AssetFileBridge::copyDefaultSettings() {
    AAssetManager* mgr     = minorGemsAndroid::getAssetManager();
    const char*    dataDir = minorGemsAndroid::getInternalDataPath();

    if (!mgr || !dataDir) {
        LOGE("copyDefaultSettings: missing asset manager or data dir");
        return;
    }

    // 如果 internal/settings 已存在就跳过（已初始化过）
    char settingsDir[1024];
    snprintf(settingsDir, sizeof(settingsDir), "%s/settings", dataDir);

    struct stat st;
    if (stat(settingsDir, &st) == 0 && S_ISDIR(st.st_mode)) {
        LOGI("copyDefaultSettings: settings/ already exists, skip");
        return;
    }

    // 创建目录
    if (mkdir(settingsDir, 0770) != 0 && errno != EEXIST) {
        LOGE("copyDefaultSettings: mkdir %s failed: %s", settingsDir, strerror(errno));
        return;
    }

    // 遍历 assets/default_settings/ 并逐个拷贝
    AAssetDir* d = AAssetManager_openDir(mgr, "default_settings");
    if (!d) {
        LOGE("copyDefaultSettings: cannot open assets/default_settings/");
        return;
    }

    int count = 0;
    const char* fn;
    while ((fn = AAssetDir_getNextFileName(d)) != nullptr) {
        char srcPath[1024];
        snprintf(srcPath, sizeof(srcPath), "default_settings/%s", fn);

        AAsset* a = AAssetManager_open(mgr, srcPath, AASSET_MODE_BUFFER);
        if (!a) continue;

        off_t       len = AAsset_getLength(a);
        const void* buf = AAsset_getBuffer(a);
        if (!buf) { AAsset_close(a); continue; }

        char dstPath[1024];
        snprintf(dstPath, sizeof(dstPath), "%s/%s", settingsDir, fn);
        FILE* f = fopen(dstPath, "wb");
        if (f) {
            fwrite(buf, 1, (size_t)len, f);
            fclose(f);
            count++;
        } else {
            LOGE("copyDefaultSettings: fopen %s failed: %s", dstPath, strerror(errno));
        }
        AAsset_close(a);
    }
    AAssetDir_close(d);

    LOGI("copyDefaultSettings: copied %d files to %s", count, settingsDir);
}

}  // namespace onelife
