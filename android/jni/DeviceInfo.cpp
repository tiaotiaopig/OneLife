// DeviceInfo.cpp
// 真机诊断实现

#ifdef __ANDROID__

#include "DeviceInfo.h"

#include <android/configuration.h>
#include <android/log.h>
#include <android/native_window.h>
#include <EGL/egl.h>
#include <GLES/gl.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <time.h>
#include <stdio.h>
#include <string.h>

#define LOG_TAG "OneLife"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

namespace onelife {
namespace DeviceInfo {

// ---- 启动信息 ------------------------------------------------------------

void logStartupInfo(struct android_app* app) {
    LOGI("=========================================");
    LOGI("OneLife Android 启动诊断");
    LOGI("=========================================");

    // 内核信息
    struct utsname uts;
    if (uname(&uts) == 0) {
        LOGI("  内核: %s %s (%s)", uts.sysname, uts.release, uts.machine);
    }

    // 内存
    struct sysinfo si;
    if (sysinfo(&si) == 0) {
        long total_mb  = (si.totalram * si.mem_unit) / (1024 * 1024);
        long free_mb   = (si.freeram  * si.mem_unit) / (1024 * 1024);
        LOGI("  内存: 总 %ldMB, 可用 %ldMB", total_mb, free_mb);
    }

    // Android 配置（屏幕密度、尺寸、语言等）
    if (app && app->config) {
        int32_t density = AConfiguration_getDensity(app->config);
        int32_t sw_dp   = AConfiguration_getScreenWidthDp(app->config);
        int32_t sh_dp   = AConfiguration_getScreenHeightDp(app->config);
        int32_t orient  = AConfiguration_getOrientation(app->config);
        char lang[3] = {0, 0, 0};
        AConfiguration_getLanguage(app->config, lang);

        const char* density_class = "unknown";
        if      (density <= 120) density_class = "ldpi";
        else if (density <= 160) density_class = "mdpi";
        else if (density <= 240) density_class = "hdpi";
        else if (density <= 320) density_class = "xhdpi";
        else if (density <= 480) density_class = "xxhdpi";
        else                     density_class = "xxxhdpi";

        LOGI("  屏幕: %ddp x %ddp, density=%d (%s)",
             sw_dp, sh_dp, density, density_class);
        LOGI("  方向: %s",
             orient == ACONFIGURATION_ORIENTATION_PORT ? "portrait" :
             orient == ACONFIGURATION_ORIENTATION_LAND ? "landscape" : "any");
        LOGI("  语言: %s", lang[0] ? lang : "unknown");

        // DPI 自适应建议（真机高分屏 UI 可能过小）
        if (density >= 480) {
            LOGW("  高分屏设备（xxhdpi+）：UI 元素可能偏小，建议后续实现 DPI 缩放");
        }
    }

    // 物理窗口尺寸
    if (app && app->window) {
        int w = ANativeWindow_getWidth(app->window);
        int h = ANativeWindow_getHeight(app->window);
        int fmt = ANativeWindow_getFormat(app->window);
        LOGI("  窗口: %d x %d (format=%d)", w, h, fmt);
    }

    // 内部存储路径
    if (app && app->activity) {
        LOGI("  内部存储: %s", app->activity->internalDataPath);
    }

    LOGI("=========================================");
}

// ---- GL 信息 -------------------------------------------------------------

void logGLInfo() {
    const char* vendor   = (const char*) glGetString(GL_VENDOR);
    const char* renderer = (const char*) glGetString(GL_RENDERER);
    const char* version  = (const char*) glGetString(GL_VERSION);

    LOGI("OpenGL ES:");
    LOGI("  Vendor:   %s", vendor   ? vendor   : "(null)");
    LOGI("  Renderer: %s", renderer ? renderer : "(null)");
    LOGI("  Version:  %s", version  ? version  : "(null)");

    // 关键能力
    GLint maxTextureSize = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
    LOGI("  MaxTexSize: %d", maxTextureSize);
}

// ---- FPS 统计 ------------------------------------------------------------

namespace {

long nowNs() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000L + ts.tv_nsec;
}

long gLastReportNs = 0;
int  gFrameCount = 0;
long gMinFrameNs = 0;
long gMaxFrameNs = 0;
long gLastFrameNs = 0;

}  // namespace

void tickFPS() {
    long now = nowNs();
    if (gLastFrameNs != 0) {
        long delta = now - gLastFrameNs;
        if (gMinFrameNs == 0 || delta < gMinFrameNs) gMinFrameNs = delta;
        if (delta > gMaxFrameNs) gMaxFrameNs = delta;
    }
    gLastFrameNs = now;
    gFrameCount++;

    if (gLastReportNs == 0) {
        gLastReportNs = now;
        return;
    }

    long elapsed = now - gLastReportNs;
    if (elapsed >= 5000000000L) {  // 每 5 秒汇报一次
        double seconds = elapsed / 1e9;
        double fps = gFrameCount / seconds;
        double minMs = gMinFrameNs / 1e6;
        double maxMs = gMaxFrameNs / 1e6;

        LOGI("FPS: %.1f (min %.1fms, max %.1fms, frames=%d/%.1fs)",
             fps, minMs, maxMs, gFrameCount, seconds);

        // 性能警告
        if (fps < 30) {
            LOGW("FPS 低于 30，可能存在性能问题");
        }
        if (maxMs > 100) {
            LOGW("单帧耗时 > 100ms，可能有卡顿");
        }

        gFrameCount = 0;
        gMinFrameNs = 0;
        gMaxFrameNs = 0;
        gLastReportNs = now;
    }
}

}  // namespace DeviceInfo
}  // namespace onelife

#endif  // __ANDROID__
