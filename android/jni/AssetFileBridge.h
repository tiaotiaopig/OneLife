#ifndef ONELIFE_ANDROID_ASSET_FILE_BRIDGE_H
#define ONELIFE_ANDROID_ASSET_FILE_BRIDGE_H

// Phase 2 会在这里实现 OneLifeData7 资源的软链接逻辑
// Phase 1 暂时占位

namespace onelife {

class AssetFileBridge {
public:
    // 在首次启动时，把 assets/ 中的 default_settings 等文件复制到内部存储
    static void copyDefaultSettings();
};

}  // namespace onelife

#endif
