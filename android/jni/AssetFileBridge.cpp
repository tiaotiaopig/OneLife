#include "AssetFileBridge.h"
#include <android/log.h>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "OneLife", __VA_ARGS__)

namespace onelife {

void AssetFileBridge::copyDefaultSettings() {
    // Phase 2 实现：检查内部存储是否有 settings/ 目录，
    // 如无则从 assets/settings/ 复制默认配置
    LOGI("AssetFileBridge::copyDefaultSettings (stub)");
}

}  // namespace onelife
