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
    ${GAMESOURCE_BATCH_1}
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
