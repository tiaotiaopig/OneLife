cmake_minimum_required(VERSION 3.18)
project(OneLifeAndroid LANGUAGES C CXX)

# Phase 2.3：接入 minorGems-android-port + JNI 模块

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 11)

# minorGems 路径（android-port worktree）
set(MINORGEMS_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../minorGems-android-port)

# minorGems 核心源文件（Phase 1 创建的模块）
set(MINORGEMS_SOURCES
    ${MINORGEMS_ROOT}/graphics/openGL/glInclude.h
    ${MINORGEMS_ROOT}/io/file/android/FileAndroid.cpp
    ${MINORGEMS_ROOT}/sound/android/OpenSLAudioBackend.cpp
    ${MINORGEMS_ROOT}/game/platforms/Android/gameAndroid.cpp
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
)

target_include_directories(onelife PRIVATE
    ${ANDROID_NDK}/sources/android/native_app_glue
    ${CMAKE_CURRENT_SOURCE_DIR}/jni
    ${MINORGEMS_ROOT}
)

target_link_libraries(onelife
    native_app_glue
    android log EGL GLESv1_CM OpenSLES
)

# NativeActivity 要求 .so 的所有未引用符号也需保留（ANativeActivity_onCreate）
target_link_options(onelife PRIVATE "-u" "ANativeActivity_onCreate")
