cmake_minimum_required(VERSION 3.18)
project(OneLifeAndroid LANGUAGES C CXX)

# Phase 2.3：接入 minorGems-android-port + JNI 模块

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 11)

# minorGems 路径（android-port worktree）
set(MINORGEMS_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../minorGems-android-port)

# minorGems 的头文件以 "minorGems/xxx.h" 形式引用。
# 在构建目录下创建 include/minorGems -> MINORGEMS_ROOT 的符号链接，
# 然后把 include/ 加入 include 路径，使 "minorGems/xxx.h" 能被正确解析。
set(MINORGEMS_INCLUDE_STAGING ${CMAKE_CURRENT_BINARY_DIR}/include)
file(MAKE_DIRECTORY ${MINORGEMS_INCLUDE_STAGING})
execute_process(
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${MINORGEMS_ROOT}
        ${MINORGEMS_INCLUDE_STAGING}/minorGems
)

# minorGems 核心源文件（Phase 1 创建的模块）
set(MINORGEMS_SOURCES
    ${MINORGEMS_ROOT}/graphics/openGL/glInclude.h
    ${MINORGEMS_ROOT}/io/file/android/FileAndroid.cpp
    ${MINORGEMS_ROOT}/sound/android/OpenSLAudioBackend.cpp
    ${MINORGEMS_ROOT}/game/platforms/Android/gameAndroid.cpp
)

# minorGems 网络层（socket API 实现）
# 只需要 SocketClient/Socket/HostAddress/NetworkFunctionLocks 这几个核心文件
set(MINORGEMS_NETWORK
    ${MINORGEMS_ROOT}/network/NetworkFunctionLocks.cpp
    ${MINORGEMS_ROOT}/network/linux/SocketClientLinux.cpp
    ${MINORGEMS_ROOT}/network/linux/SocketLinux.cpp
    ${MINORGEMS_ROOT}/network/linux/HostAddressLinux.cpp
)

# minorGems util（stringUtils、AppLog 等）
set(MINORGEMS_UTIL
    ${MINORGEMS_ROOT}/util/stringUtils.cpp
    ${MINORGEMS_ROOT}/util/printUtils.cpp
    ${MINORGEMS_ROOT}/util/log/AppLog.cpp
    ${MINORGEMS_ROOT}/util/log/Log.cpp
    ${MINORGEMS_ROOT}/util/log/PrintLog.cpp
    ${MINORGEMS_ROOT}/util/log/AndroidLog.cpp
)

# minorGems system（MutexLock 等 POSIX 实现，NetworkFunctionLocks 需要）
set(MINORGEMS_SYS
    ${MINORGEMS_ROOT}/system/linux/MutexLockLinux.cpp
    ${MINORGEMS_ROOT}/system/unix/TimeUnix.cpp
)

# minorGems 扩展工具（gameSource 批 1 需要）
set(MINORGEMS_EXT
    ${MINORGEMS_ROOT}/util/StringTree.cpp
    ${MINORGEMS_ROOT}/util/SettingsManager.cpp
    ${MINORGEMS_ROOT}/formats/encodingUtils.cpp
    ${MINORGEMS_ROOT}/util/crc32.cpp
    ${MINORGEMS_ROOT}/crypto/hashes/sha1.cpp
    ${MINORGEMS_ROOT}/io/file/linux/PathLinux.cpp
    ${MINORGEMS_ROOT}/io/file/unix/DirectoryUnix.cpp
    # 批 4a 新增：URL 编解码（lifeTokens/fitnessScore/photos 需要）
    ${MINORGEMS_ROOT}/network/web/URLUtils.cpp
)

# minorGems 图形/音频工具（gameSource 批 2 需要）
# TGAImageConverter/Image/RGBAImage 都是 header-only
# Font.cpp / drawUtils.cpp 在批 3（UI 框架）接入时加入
set(MINORGEMS_GRAPHICS
    ${MINORGEMS_ROOT}/game/doublePair.cpp
    ${MINORGEMS_ROOT}/game/Font.cpp
    ${MINORGEMS_ROOT}/game/drawUtils.cpp
    ${MINORGEMS_ROOT}/sound/formats/aiff.cpp
    ${MINORGEMS_ROOT}/util/StringBufferOutputStream.cpp
)

# gameSource 批 1：叶子数据结构（Task 2.3a）
# 只包含不依赖其他 gameSource .cpp 的文件（或仅依赖本批内文件）
# 排除：objectBank/transitionBank/spriteBank/soundBank/animationBank（批 2/3 处理）
set(GAMESOURCE_BATCH_1
    ${CMAKE_CURRENT_SOURCE_DIR}/../gameSource/GridPos.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gameSource/folderCache.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gameSource/binFolderCache.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gameSource/settingsToggle.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gameSource/ageControl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gameSource/categoryBank.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gameSource/SoundUsage.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gameSource/accountHmac.cpp
)

# gameSource 批 2：核心 bank 类（Task 2.3b）
# 包含：objectBank / transitionBank / spriteBank / soundBank / animationBank
# 以及它们的依赖：objectMetadata / ogg / musicPlayer / convolution / fft
set(GAMESOURCE_BATCH_2
    ${CMAKE_CURRENT_SOURCE_DIR}/../gameSource/objectMetadata.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gameSource/objectBank.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gameSource/transitionBank.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gameSource/spriteBank.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gameSource/soundBank.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gameSource/animationBank.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gameSource/ogg.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gameSource/musicPlayer2.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gameSource/convolution.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gameSource/fft.cpp
)

# gameSource 批 3：UI 框架基础（Task 2.3c）
# 包含：PageComponent / Button / TextField / TextArea / TextButton /
#       SpriteButton / SpriteToggleButton / CheckboxButton /
#       KeyEquivalentTextButton / RadioButtonSet / ValueSlider /
#       Picker / SoundWidget / buttonStyle / spellCheck
# 排除：GamePage（依赖 message.h/serialWebRequests.h/whiteSprites.h）
#       LoadingPage/ExistingAccountPage 等具体 Page（依赖 message/accountHmac 等）
#       Editor*.cpp（Android 不需要编辑器）
set(GAMESOURCE_BATCH_3
    ${CMAKE_CURRENT_SOURCE_DIR}/../gameSource/PageComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gameSource/Button.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gameSource/TextField.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gameSource/TextArea.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gameSource/TextButton.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gameSource/SpriteButton.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gameSource/SpriteToggleButton.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gameSource/CheckboxButton.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gameSource/KeyEquivalentTextButton.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gameSource/RadioButtonSet.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gameSource/ValueSlider.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gameSource/Picker.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gameSource/SoundWidget.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gameSource/buttonStyle.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gameSource/spellCheck.cpp
)

# gameSource 批 4a：游戏逻辑支持工具（Task 2.3d-a）
# 包含：message / serialWebRequests / whiteSprites / pathFind /
#       liveObjectSet / liveAnimationTriggers / lifeTokens / fitnessScore /
#       emotion / groundSprites / overlayBank /
#       zoomView / keyLegend / photos / photoCache
# 排除：PickableStatics（依赖 Editor*.h，永不接入）
#       rocketAnimation（依赖 LivingLifePage 方法 + 大量 game.cpp 全局变量，移至批 4c）
# 新增 minorGems：URLUtils.cpp（已加入 MINORGEMS_EXT）
# 新增 commonSource：fractalNoise.cpp（groundSprites 需要）
# 新增 stubs：translate / game_timeSec / startWebRequest 等（见 game_stubs.cpp）
set(GAMESOURCE_BATCH_4A
    ${CMAKE_CURRENT_SOURCE_DIR}/../gameSource/message.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gameSource/serialWebRequests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gameSource/whiteSprites.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gameSource/pathFind.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gameSource/liveObjectSet.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gameSource/liveAnimationTriggers.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gameSource/lifeTokens.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gameSource/fitnessScore.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gameSource/emotion.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gameSource/groundSprites.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gameSource/overlayBank.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gameSource/zoomView.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gameSource/keyLegend.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gameSource/photos.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gameSource/photoCache.cpp
    # commonSource 依赖（groundSprites 需要）
    ${CMAKE_CURRENT_SOURCE_DIR}/../commonSource/fractalNoise.cpp
)

# gameSource 批 4b：各种具体 Page 类（Task 2.3d-b）
# 包含：LoadingPage / TestPage / FinalMessagePage / ExtendedMessagePage /
#       GeneticHistoryPage / GamePage / TwinPage / PollPage /
#       RebirthChoicePage / ReviewPage / SettingsPage / ServicesPage /
#       ExistingAccountPage / ServerActionPage / AutoUpdatePage
# 排除：AHAPResultPage / AHAPSettingsPage（AHAP 变体，永不接入）
#       LivingLifePage（体量巨大，单独批次）
# 新增 minorGems：diffBundleClient.cpp（AutoUpdatePage 需要）
# 新增 stubs：musicLoudness / targetFramesPerSecond / userTwinCode / userTwinCount /
#             mainFontReview / userID / gamePlayingBack / loginEditOverride /
#             relaunchGame / setClipboardText（见 game_stubs.cpp）
set(GAMESOURCE_BATCH_4B
    ${CMAKE_CURRENT_SOURCE_DIR}/../gameSource/LoadingPage.cpp
    # TestPage.cpp 排除：TestPage.h 不存在（测试/调试页面，不完整）
    ${CMAKE_CURRENT_SOURCE_DIR}/../gameSource/FinalMessagePage.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gameSource/ExtendedMessagePage.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gameSource/GeneticHistoryPage.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gameSource/GamePage.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gameSource/TwinPage.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gameSource/PollPage.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gameSource/RebirthChoicePage.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gameSource/ReviewPage.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gameSource/SettingsPage.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gameSource/ServicesPage.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gameSource/ExistingAccountPage.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gameSource/ServerActionPage.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gameSource/AutoUpdatePage.cpp
    # minorGems 依赖（AutoUpdatePage 需要）
    ${MINORGEMS_ROOT}/game/diffBundle/client/diffBundleClient.cpp
)

# gameSource 批 4c：核心游戏循环（Task 2.3d-c）
# 包含：LivingLifePage / game.cpp / rocketAnimation
# 排除：inPlaceTipsWorking（代码片段，非独立编译单元）
# 新增依赖：importer / exporter / AHAPResultPage / AHAPSettingsPage（game.cpp 引用）
#           sayLimit（LivingLifePage 引用）
set(GAMESOURCE_BATCH_4C
    ${CMAKE_CURRENT_SOURCE_DIR}/../gameSource/LivingLifePage.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gameSource/game.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gameSource/rocketAnimation.cpp
    # game.cpp 依赖
    ${CMAKE_CURRENT_SOURCE_DIR}/../gameSource/importer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gameSource/exporter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gameSource/AHAPResultPage.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gameSource/AHAPSettingsPage.cpp
    # LivingLifePage 依赖
    ${CMAKE_CURRENT_SOURCE_DIR}/../commonSource/sayLimit.cpp
)

# OneLife Android JNI 模块（Phase 1 创建的模块）
set(JNI_SOURCES
    jni/EGLContext.cpp
    jni/AndroidPlatform.cpp
    jni/AssetFileBridge.cpp
    jni/game_stubs.cpp  # TODO(Task 2.4): 接入 gameSource 后删除
)

# native_app_glue（NDK 自带）
add_library(native_app_glue STATIC
    ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)

target_include_directories(native_app_glue PUBLIC
    ${ANDROID_NDK}/sources/android/native_app_glue)

# 主 .so（lib_name=onelife，Manifest 中 lib_name 必须匹配）
add_library(onelife SHARED
    jni/android_main.cpp
    ${JNI_SOURCES}
    ${MINORGEMS_SOURCES}
    ${MINORGEMS_NETWORK}
    ${MINORGEMS_UTIL}
    ${MINORGEMS_SYS}
    ${MINORGEMS_EXT}
    ${MINORGEMS_GRAPHICS}
    ${GAMESOURCE_BATCH_1}
    ${GAMESOURCE_BATCH_2}
    ${GAMESOURCE_BATCH_3}
    ${GAMESOURCE_BATCH_4A}
    ${GAMESOURCE_BATCH_4B}
    ${GAMESOURCE_BATCH_4C}
)

target_include_directories(onelife PRIVATE
    ${ANDROID_NDK}/sources/android/native_app_glue
    ${CMAKE_CURRENT_SOURCE_DIR}/jni
    ${MINORGEMS_ROOT}
    # "minorGems/xxx.h" 形式的 include 通过此 staging 目录解析
    ${MINORGEMS_INCLUDE_STAGING}
    # gameSource 头文件（批 1 文件的 #include "xxx.h" 能找到）
    ${CMAKE_CURRENT_SOURCE_DIR}/../gameSource
)

target_link_libraries(onelife
    native_app_glue
    android log EGL GLESv1_CM OpenSLES
)

# NativeActivity 要求 .so 的所有未引用符号也需保留（ANativeActivity_onCreate）
target_link_options(onelife PRIVATE "-u" "ANativeActivity_onCreate")
