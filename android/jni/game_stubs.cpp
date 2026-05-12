// 临时桩函数：用于 Phase 1 验证 CMakeLists.txt 链接通路
//
// 这些是 minorGems 框架（gameAndroid.cpp / OpenSLAudioBackend.cpp）
// 需要的游戏侧回调，正常情况下由 gameSource/game.cpp 提供。
//
// TODO(Task 2.4): 接入 gameSource 后删除此文件

#include <stdint.h>
#include <android/log.h>

// Font 类声明（mainFont / smallFont 全局变量需要）
#include "minorGems/game/Font.h"

typedef uint8_t Uint8;

#define LOG_TAG "OneLifeStub"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

// gameSource 批 1 引用的全局变量（正常由 game.cpp 定义）
// 接入完整 game.cpp 后这些定义会从 game.cpp 提供，此处的定义需移除
int dataVersionNumber = 0;
char *accountKey = nullptr;
int serverSequenceNumber = 0;
int accountHmacVersionNumber = 0;

extern "C" {

// 字符串绘制初始化
void initDrawString(int inWidth, int inHeight) {
    LOGI("stub initDrawString %dx%d", inWidth, inHeight);
}

// 字符串绘制清理
void freeDrawString() {
    LOGI("stub freeDrawString");
}

// 帧绘制初始化
void initFrameDrawer(int inWidth, int inHeight, int inTargetFrameRate,
                     const char* inCustomRecordedGameData,
                     char inPlayingBack) {
    LOGI("stub initFrameDrawer %dx%d @%dfps", inWidth, inHeight, inTargetFrameRate);
}

// 帧绘制清理
void freeFrameDrawer() {
    LOGI("stub freeFrameDrawer");
}

// 每帧绘制
void drawFrame(char inUpdate) {
    // 桩：什么都不做，由 android_main 中的 EGL 清屏代替
    (void)inUpdate;
}

}  // extern "C"

// ----------------------------------------------------------------------------
// game.h 中的 C++ 链接函数（不能放在 extern "C" 块内）
// ----------------------------------------------------------------------------

// 音频采样率（CD 质量立体声）
// 注：getSoundSamples 由 gameSource/musicPlayer.cpp 提供，无需在此定义
int getSampleRate() {
    return 44100;
}

// ============================================================================
// 批 2 stubs：sprite/sound/graphics API（gameGraphics.h / game.h）
// ============================================================================

#include "minorGems/game/doublePair.h"
#include "minorGems/graphics/Image.h"
#include "minorGems/graphics/RawRGBAImage.h"

typedef void* SpriteHandle;
typedef void* SoundSpriteHandle;

// gameGraphics.h - 颜色/混合模式
void setDrawColor(float inR, float inG, float inB, float inA) {
    // stub: 设置绘制颜色（OpenGL ES 实现）
}

void setDrawFade(float inA) {
    // stub: 设置透明度
}

float getTotalGlobalFade() {
    return 1.0f;
}

void toggleAdditiveBlend(char inAdditive) {
    // stub: 切换加法混合模式
}

void toggleMultiplicativeBlend(char inMultiplicative) {
    // stub: 切换乘法混合模式
}

void toggleAdditiveTextureColoring(char inAdditive) {
    // stub: 切换纹理颜色加法模式
}

// gameGraphics.h - sprite 加载/管理
SpriteHandle fillSprite(unsigned char *inRGBA, unsigned int inWidth, unsigned int inHeight) {
    // stub: 从 RGBA 数据创建 sprite（返回假句柄）
    return (void*)0x1;
}

void freeSprite(SpriteHandle inSprite) {
    // stub: 释放 sprite
}

void setSpriteCenterOffset(SpriteHandle inSprite, doublePair inOffset) {
    // stub: 设置 sprite 中心偏移
}

void drawSprite(SpriteHandle inSprite, doublePair inCenter, double inZoom, double inRotation, char inFlipH) {
    // stub: 绘制 sprite
}

RawRGBAImage* readTGAFileRawFromBuffer(unsigned char *inBuffer, int inLength) {
    // stub: 从 TGA 缓冲区读取图像（返回空）
    return nullptr;
}

SpriteHandle loadSpriteBase(const char *inTGAFileName, char inTransparentLowerLeftCorner) {
    // stub: 从主目录加载 sprite
    return (void*)0x1;
}

// game.h - sound sprite API
void setMaxTotalSoundSpriteVolume(double inMaxTotal, double inCompressionFraction) {
    // stub: 设置音效总音量上限
}

void setMaxSimultaneousSoundSprites(int inMaxCount) {
    // stub: 设置同时播放音效数量上限
}

SoundSpriteHandle setSoundSprite(int16_t *inSamples, int inNumSamples) {
    // stub: 从单声道采样创建音效（返回假句柄）
    return (void*)0x1;
}

SoundSpriteHandle setSoundSprite(int16_t *inSamplesL, int16_t *inSamplesR, int inNumSamples) {
    // stub: 从立体声采样创建音效（返回假句柄）
    return (void*)0x1;
}

void freeSoundSprite(SoundSpriteHandle inHandle) {
    // stub: 释放音效
}

void playSoundSprite(SoundSpriteHandle inHandle, double inVolumeTweak, double inStereoPosition) {
    // stub: 播放音效
}

void playSoundSprite(int inNumSprites, SoundSpriteHandle *inHandles, double *inVolumeTweaks, double *inStereoPositions) {
    // stub: 同时播放多个音效
}

// game.h - 时间/录音 API
double game_getCurrentTime() {
    // stub: 获取当前游戏时间（秒）
    return 0.0;
}

void startRecording16BitMonoSound(int inSampleRate) {
    // stub: 开始录音
}

int16_t* stopRecording16BitMonoSound(int *outNumSamples) {
    // stub: 停止录音（返回空）
    if (outNumSamples) *outNumSamples = 0;
    return nullptr;
}

// musicPlayer.h - 音乐抑制 API
void addMusicSuppression(const char *inTag) {
    // stub: 添加音乐抑制标签
}

void removeMusicSuppression(const char *inTag) {
    // stub: 移除音乐抑制标签
}

// gameGraphics.h - 更多绘制函数
void toggleInvertedBlend(char inInverted) {
    // stub: 切换反色混合模式
}

void startAddingToStencil(char inDrawColorToo, char inAdd, float inMinAlpha) {
    // stub: 开始添加到模板缓冲区
}

void startDrawingThroughStencil(char inInvertStencil) {
    // stub: 开始通过模板缓冲区绘制
}

void stopStencil() {
    // stub: 停止模板缓冲区
}

void startOutputAllFrames() {
    // stub: 开始输出所有帧
}

void stopOutputAllFrames() {
    // stub: 停止输出所有帧
}

// game.h - 音频锁
void lockAudio() {
    // stub: 锁定音频线程
}

void unlockAudio() {
    // stub: 解锁音频线程
}

// musicPlayer.cpp 需要的全局变量
double musicHeadroom = 1.0;

// gameGraphics.h - setDrawColor 重载（FloatColor 已在 gameGraphics.h 中定义）
void setDrawColor(FloatColor inColor) {
    setDrawColor(inColor.r, inColor.g, inColor.b, inColor.a);
}

// gameGraphics.h - 更多 sprite 函数
Image* readTGAFileBase(const char *inTGAFileName) {
    // stub: 从主目录读取 TGA 文件
    return nullptr;
}

SpriteHandle fillSprite(Image *inImage, char inTransparentLowerLeftCorner) {
    // stub: 从 Image 创建 sprite
    return (void*)0x1;
}

// 异步文件读取 API
void* startAsyncFileRead(const char *inPath) {
    // stub: 开始异步文件读取（返回假句柄）
    return (void*)0x1;
}

// ============================================================================
// 批 3 stubs：UI 框架（gameSource 批 3 需要）
// ============================================================================

// game.cpp 中定义的全局变量（批 3 UI 文件通过 extern 引用）
double frameRateFactor = 1.0;
Font *mainFont = nullptr;
Font *smallFont = nullptr;

// gameGraphics.h - 绘制四边形/三角形
void drawQuads(int inNumQuads, double inVertices[]) {
    // stub: 绘制四边形（顶点数组）
}

void drawQuads(int inNumQuads, double inVertices[], float inVertexColors[]) {
    // stub: 绘制带颜色的四边形
}

void drawTriangles(int inNumTriangles, double inVertices[],
                   char inFill, char inSmooth) {
    // stub: 绘制三角形
}

void drawTrianglesColor(int inNumTriangles, double inVertices[],
                        float inVertexColors[], char inFill, char inSmooth) {
    // stub: 绘制带颜色的三角形
}

// gameGraphics.h - 从文件加载 sprite
SpriteHandle loadSprite(const char *inTGAFileName,
                        char inTransparentLowerLeftCorner) {
    // stub: 从当前目录加载 sprite（返回假句柄）
    return (void*)0x1;
}

// gameGraphics.h - 从文件读取 TGA
Image* readTGAFile(const char *inTGAFileName) {
    // stub: 从当前目录读取 TGA 文件
    return nullptr;
}

// gameGraphics.h - 获取当前绘制透明度
float getDrawFade() {
    // stub: 返回完全不透明
    return 1.0f;
}

// game.h - 视图中心位置
void setViewCenterPosition(float inX, float inY) {
    // stub: 设置视图中心（无操作）
}

doublePair getViewCenterPosition() {
    // stub: 返回原点
    doublePair p = { 0.0, 0.0 };
    return p;
}

// game.h - 键盘修饰键状态
char isShiftKeyDown() {
    // stub: Shift 键未按下
    return 0;
}

char isCommandKeyDown() {
    // stub: Command/Ctrl 键未按下
    return 0;
}

// game.h - 鼠标按键状态
char isLastMouseButtonRight() {
    // stub: 最后按下的不是右键
    return 0;
}

// game.h - 剪贴板 API
char isClipboardSupported() {
    // stub: Android 暂不支持剪贴板
    return 0;
}

char* getClipboardText() {
    // stub: 返回空字符串
    return nullptr;
}
