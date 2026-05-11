cmake_minimum_required(VERSION 3.18)
project(OneLifeAndroid LANGUAGES C CXX)

# Phase 0：仅 JNI 入口 + EGL 清屏，不接入 minorGems/gameSource
# Phase 1+ 会扩展到 minorGems + gameSource

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 11)

# native_app_glue（NDK 自带）
add_library(native_app_glue STATIC
    ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)

target_include_directories(native_app_glue PUBLIC
    ${ANDROID_NDK}/sources/android/native_app_glue)

# 主 .so（lib_name=onelife，Manifest 中 lib_name 必须匹配）
add_library(onelife SHARED
    jni/android_main.cpp)

target_include_directories(onelife PRIVATE
    ${ANDROID_NDK}/sources/android/native_app_glue)

target_link_libraries(onelife
    native_app_glue
    android log EGL GLESv1_CM)

# NativeActivity 要求 .so 的所有未引用符号也需保留（ANativeActivity_onCreate）
target_link_options(onelife PRIVATE "-u" "ANativeActivity_onCreate")
