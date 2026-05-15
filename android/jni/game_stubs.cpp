// 临时桩函数：用于 Phase 1 验证 CMakeLists.txt 链接通路
//
// 这些是 minorGems 框架（gameAndroid.cpp / OpenSLAudioBackend.cpp）
// 需要的游戏侧回调，正常情况下由 gameSource/game.cpp 提供。
//
// TODO(Task 2.4): 接入 gameSource 后删除此文件

#include <stdint.h>
#include <android/log.h>
#include <GLES/gl.h>

// Phase 3 Task 3.2：软键盘控制（供 TextField.cpp 调用）
#include "SoftKeyboard.h"

// Phase 3 Task 3.3/3.4：GL 投影矩阵 + 坐标转换需要物理屏幕尺寸
namespace minorGemsAndroid {
    int getPhysicalWidth();
    int getPhysicalHeight();
}

extern "C" void onelifeAndroidShowSoftKeyboard() {
    onelife::SoftKeyboard::show();
}

extern "C" void onelifeAndroidHideSoftKeyboard() {
    onelife::SoftKeyboard::hide();
}

// 批 4a：提前包含 LivingLifePage.h（间接包含 game.h），
// 确保所有函数签名与 game.h 声明一致
#include "LivingLifePage.h"

// TGA 加载所需头文件
#include "minorGems/io/file/File.h"
#include "minorGems/io/file/FileInputStream.h"
#include "minorGems/graphics/converters/TGAImageConverter.h"
#include "minorGems/io/InputStream.h"
#include <string.h>
#include "minorGems/io/file/Path.h"
#include "minorGems/util/stringUtils.h"

typedef uint8_t Uint8;

#define LOG_TAG "OneLifeStub"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

// 批 1 全局变量已由 game.cpp 提供（批 4c 接入后移除）

// game.h - 音频采样率（CD 质量立体声）
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
// 注：setDrawColor/setDrawFade/getTotalGlobalFade/toggleAdditiveBlend/
//     toggleMultiplicativeBlend/toggleAdditiveTextureColoring 由
//     minorGems/game/platforms/openGL/gameGraphicsGL.cpp 提供。

// gameGraphics.h - sprite 加载/管理
// 注：drawSprite / freeSprite / setSpriteCenterOffset / fillSprite(unsigned char*) /
//     fillSprite(Image*) 由 gameGraphicsGL.cpp 提供。

RawRGBAImage* readTGAFileRawFromBuffer(unsigned char *inBuffer, int inLength) {
    // stub: 从 TGA 缓冲区读取图像（返回空）
    return nullptr;
}

SpriteHandle loadSpriteBase(const char *inTGAFileName, char inTransparentLowerLeftCorner) {
    // 实现真正的 sprite 加载(从主目录读取,不带 graphics/ 前缀)
    Image *image = readTGAFileBase(inTGAFileName);
    if (image == nullptr) {
        return nullptr;
    }
    SpriteHandle handle = fillSprite(image, inTransparentLowerLeftCorner);
    delete image;
    return handle;
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

// game.h 声明 startRecording16BitMonoSound 返回 char
char startRecording16BitMonoSound(int inSampleRate) {
    // stub: 开始录音（Android 暂不支持）
    return 0;
}

int16_t* stopRecording16BitMonoSound(int *outNumSamples) {
    // stub: 停止录音（返回空）
    if (outNumSamples) *outNumSamples = 0;
    return nullptr;
}

// musicPlayer.h - 音乐抑制 API
// 注：addMusicSuppression / removeMusicSuppression 已在 musicPlayer2.cpp 中实现，
//     切换到 musicPlayer2.cpp 后此处不再需要桩定义

// gameGraphics.h - 更多绘制函数
// 注：toggleInvertedBlend / startAddingToStencil / startDrawingThroughStencil /
//     stopStencil 均由 gameGraphicsGL.cpp 提供。

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

// gameGraphics.h - setDrawColor(FloatColor) 重载由 gameGraphicsGL.cpp 提供

// gameGraphics.h - 更多 sprite 函数
// 简易内存 InputStream，给 TGAImageConverter 用
class MemInputStream : public InputStream {
public:
    MemInputStream(unsigned char *inBuf, int inLen)
        : mBuf(inBuf), mLen(inLen), mPos(0) {}
    virtual long read(unsigned char *outBuf, long inNumBytes) {
        long avail = mLen - mPos;
        long n = (inNumBytes < avail) ? inNumBytes : avail;
        if (n > 0) {
            memcpy(outBuf, mBuf + mPos, n);
            mPos += n;
        }
        return n;
    }
private:
    unsigned char *mBuf;
    int mLen, mPos;
};

// 通过 minorGems File + 手动 TGA 解析读取（无 graphics/ 前缀）
Image* readTGAFileBase(const char *inTGAFileName) {
    File tgaFile(NULL, (char*)inTGAFileName);
    int length = 0;
    unsigned char *data = tgaFile.readFileContents(&length);
    if (data == nullptr || length <= 0) {
        if (data) delete [] data;
        __android_log_print(ANDROID_LOG_WARN, "OneLife",
            "readTGAFileBase: failed to read %s", inTGAFileName);
        return nullptr;
    }
    MemInputStream stream(data, length);
    TGAImageConverter converter;
    Image *image = converter.deformatImage(&stream);
    delete [] data;
    if (image == nullptr) {
        __android_log_print(ANDROID_LOG_WARN, "OneLife",
            "readTGAFileBase: TGAImageConverter failed on %s", inTGAFileName);
    }
    return image;
}

// 异步文件读取 API
int startAsyncFileRead(const char *inPath) {
    // stub: 开始异步文件读取（返回假句柄）
    return 1;
}

// game.h - 检查异步文件读取是否完成（musicPlayer2 / soundBank 调用）
char checkAsyncFileReadDone( int inHandle ) {
    // stub: 返回完成（无实际异步读取）
    (void)inHandle;
    return 1;
}

// game.h - 获取异步文件数据（musicPlayer2 / soundBank 调用）
unsigned char *getAsyncFileData( int inHandle, int *outDataLength ) {
    // stub: 返回空（无实际数据）
    (void)inHandle;
    if( outDataLength ) *outDataLength = 0;
    return nullptr;
}

// ============================================================================
// 批 3 stubs：UI 框架（gameSource 批 3 需要）
// ============================================================================

// gameGraphics.h - 绘制四边形/三角形
// 注：drawQuads / drawTriangles / drawTrianglesColor 均由 gameGraphicsGL.cpp 提供。

// gameGraphics.h - 从文件加载 sprite
SpriteHandle loadSprite(const char *inTGAFileName,
                        char inTransparentLowerLeftCorner) {
    // 从 graphics/ 目录加载 sprite
    Image *image = readTGAFile(inTGAFileName);
    if (image == nullptr) {
        return nullptr;
    }
    SpriteHandle handle = fillSprite(image, inTransparentLowerLeftCorner);
    delete image;
    return handle;
}

// gameGraphics.h - 从文件读取 TGA（使用 graphics/ 子目录，与桌面端 gameSDL.cpp 行为一致）
// Android 上 FileInputStream 不能直接 fopen APK assets，
// 所以用 File::readFileContents()（有 AAsset 回退）读全部字节，再手动解析 TGA。
// OneLife 只用非压缩 TGA（type 2），简单解析足够。
Image* readTGAFile(const char *inTGAFileName) {
    char *pathSteps[1] = { stringDuplicate("graphics") };
    File tgaFile(new Path(pathSteps, 1, false), (char*)inTGAFileName);
    delete [] pathSteps[0];

    int length = 0;
    unsigned char *data = tgaFile.readFileContents(&length);
    if (data == nullptr || length <= 0) {
        if (data) delete [] data;
        __android_log_print(ANDROID_LOG_WARN, "OneLife",
            "readTGAFile: failed to read graphics/%s", inTGAFileName);
        return nullptr;
    }

    if (length < 18) {
        delete [] data;
        __android_log_print(ANDROID_LOG_WARN, "OneLife",
            "readTGAFile: graphics/%s too short (%d bytes)", inTGAFileName, length);
        return nullptr;
    }

    int w = data[12] | (data[13] << 8);
    int h = data[14] | (data[15] << 8);
    int bpp = data[16];
    int channels = bpp / 8;
    int idLen = data[0];
    int pixelDataOffset = 18 + idLen;

    if (channels < 3 || channels > 4) {
        delete [] data;
        __android_log_print(ANDROID_LOG_WARN, "OneLife",
            "readTGAFile: graphics/%s unsupported bpp=%d", inTGAFileName, bpp);
        return nullptr;
    }

    if (pixelDataOffset + w * h * channels > length) {
        delete [] data;
        __android_log_print(ANDROID_LOG_WARN, "OneLife",
            "readTGAFile: graphics/%s truncated", inTGAFileName);
        return nullptr;
    }

    Image *image = new Image(w, h, channels, false);
    double *r = image->getChannel(0);
    double *g = image->getChannel(1);
    double *b = image->getChannel(2);
    double *a = (channels == 4) ? image->getChannel(3) : nullptr;

    unsigned char *pixels = data + pixelDataOffset;
    char topToBottom = (data[17] & 0x20) != 0;

    // TGA 像素是 BGRA 顺序,Image 通道 0=R, 1=G, 2=B, 3=A
    // 需要做 BGR→RGB 交换(与桌面端 TGAImageConverter 一致)
    for (int y = 0; y < h; y++) {
        int srcY = topToBottom ? y : (h - 1 - y);
        for (int x = 0; x < w; x++) {
            int srcIdx = (srcY * w + x) * channels;
            int dstIdx = y * w + x;

            r[dstIdx] = pixels[srcIdx + 2] / 255.0;  // TGA B位置 → R (BGR→RGB)
            g[dstIdx] = pixels[srcIdx + 1] / 255.0;  // G → G
            b[dstIdx] = pixels[srcIdx + 0] / 255.0;  // TGA R位置 → B (BGR→RGB)
            if (a) a[dstIdx] = pixels[srcIdx + 3] / 255.0;  // A → A
        }
    }

    delete [] data;
    return image;
}

// gameGraphics.h - 获取当前绘制透明度
// 注：getDrawFade 由 gameGraphicsGL.cpp 提供

// ============================================================================
// GL 投影矩阵 + 视图管理（参考 gameSDL.cpp 的 redoDrawMatrix）
// ============================================================================
static float gViewSize = 2.0f;
static float gVisibleWidth = -1.0f;
static float gVisibleHeight = -1.0f;
static float gViewCenterX = 0.0f;
static float gViewCenterY = 0.0f;
static char gMouseWorldCoordinates = false;

static void redoDrawMatrix() {
    float hRadius = gViewSize / 2;
    float wRadius = hRadius;

    if (gVisibleHeight > 0) {
        wRadius = gVisibleWidth / 2;
        hRadius = gVisibleHeight / 2;
    }

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrthof(gViewCenterX - wRadius, gViewCenterX + wRadius,
             gViewCenterY - hRadius, gViewCenterY + hRadius, -1.0f, 1.0f);

    // viewport 占满整个 EGL surface,letterbox 由 GL 投影矩阵处理
    int vpX = 0, vpY = 0;
    int vpW = minorGemsAndroid::getPhysicalWidth();
    int vpH = minorGemsAndroid::getPhysicalHeight();
    glViewport(vpX, vpY, vpW, vpH);

    static int callCount = 0;
    callCount++;
    if (callCount <= 3 || callCount % 600 == 0) {
        __android_log_print(ANDROID_LOG_INFO, "OneLife",
            "redoDrawMatrix #%d: ortho L=%.1f R=%.1f B=%.1f T=%.1f viewport=%d,%d,%d,%d (vis=%.1fx%.1f size=%.1f)",
            callCount,
            gViewCenterX - wRadius, gViewCenterX + wRadius,
            gViewCenterY - hRadius, gViewCenterY + hRadius,
            vpX, vpY, vpW, vpH, gVisibleWidth, gVisibleHeight, gViewSize);
    }

    glMatrixMode(GL_MODELVIEW);
}

// game.h - 视图中心位置
void setViewCenterPosition(float inX, float inY) {
    gViewCenterX = inX;
    gViewCenterY = inY;
    redoDrawMatrix();
}

doublePair getViewCenterPosition() {
    doublePair p = { (double)gViewCenterX, (double)gViewCenterY };
    return p;
}

// game.h - 键盘修饰键状态
// 由 TouchInputAdapter.cpp 维护（双指 → Shift）
extern "C" int onelifeAndroidIsShiftDown();
extern "C" int onelifeAndroidIsRightButtonDown();
extern "C" int onelifeAndroidLastMouseX();
extern "C" int onelifeAndroidLastMouseY();

char isShiftKeyDown() {
    return onelifeAndroidIsShiftDown() ? 1 : 0;
}

char isCommandKeyDown() {
    // Android 无物理 Ctrl/Cmd 键
    return 0;
}

// game.h - 鼠标按键状态
// 长按后松手视为右键（在 LivingLifePage 中用于"交互"语义）
char isLastMouseButtonRight() {
    return onelifeAndroidIsRightButtonDown() ? 1 : 0;
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

// ============================================================================
// 批 4a stubs：web 请求 API（game.h 声明，gameSDL.cpp 实现，Android 暂用桩）
// serialWebRequests / lifeTokens / fitnessScore / photos 等文件调用这些函数
// ============================================================================

int startWebRequest( const char *inMethod, const char *inURL,
                     const char *inBody ) {
    // stub: Android 暂不发起真实 HTTP 请求
    return -1;
}

int stepWebRequest( int inHandle ) {
    // stub: 返回 -1 表示请求失败
    return -1;
}

char *getWebResult( int inHandle ) {
    // stub: 返回空（无结果）
    return nullptr;
}

unsigned char *getWebResult( int inHandle, int *outSize ) {
    // stub: 返回空（无结果）
    if( outSize ) *outSize = 0;
    return nullptr;
}

void clearWebRequest( int inHandle ) {
    // stub: 无操作
}

// ============================================================================
// 批 4a stubs：game.cpp 全局变量（批 4a 文件通过 extern 引用）
// 接入 game.cpp 后这些定义需移除
// ============================================================================

#include "minorGems/system/Time.h"
#include "minorGems/graphics/Image.h"
#include "minorGems/graphics/RawRGBAImage.h"

// game.h 声明的函数（gameSDL.cpp 实现，Android 暂用桩）

// 翻译函数：直接返回 key（无翻译表时原样显示）
const char *translate( const char *inTranslationKey ) {
    return inTranslationKey;
}

// 游戏时间（秒）
timeSec_t game_timeSec() {
    return Time::timeSec();
}

// gameGraphics.h - 从基础目录读取 TGA（Raw 格式）
RawRGBAImage *readTGAFileRawBase( const char *inTGAFileName ) {
    // stub: 返回空（无文件系统访问）
    return nullptr;
}

// gameGraphics.h - 从 RawRGBAImage 创建 sprite
SpriteHandle fillSprite( RawRGBAImage *inRawImage ) {
    // stub: 返回假句柄
    return (void*)0x1;
}

// gameGraphics.h - 写入 TGA 文件
void writeTGAFile( const char *inTGAFileName, Image *inImage ) {
    // stub: 无操作（Android 不写 TGA 缓存）
}

// gameGraphics.h - 截取屏幕区域
Image *getScreenRegion( double inX, double inY,
                        double inWidth, double inHeight ) {
    // stub: 返回空（无 OpenGL 读回）
    return nullptr;
}

// gameGraphics.h - 截取屏幕区域（像素坐标）
Image *getScreenRegionRaw( int inStartX, int inStartY,
                           int inWidth, int inHeight ) {
    // stub: 返回空
    return nullptr;
}

// gameGraphics.h - 从 Raw 格式读取 TGA
RawRGBAImage *readTGAFileRaw( const char *inTGAFileName ) {
    // stub: 返回空
    return nullptr;
}

// ============================================================================
// 批 4a stubs：gameGraphics.h 函数（gameSDL.cpp 实现，Android 暂用桩）
// ============================================================================

// 注：toggleLinearMagFilter / getLinearMagFilterOn 由 gameGraphicsGL.cpp 提供

void getScreenDimensions( int *outWidth, int *outHeight ) {
    // stub: 返回默认分辨率
    if( outWidth ) *outWidth = 1024;
    if( outHeight ) *outHeight = 768;
}

// ============================================================================
// 批 4b stubs：Page 类需要的全局变量和函数（game.cpp 定义，Android 暂用桩）
// ============================================================================

// game.h 声明（gameSDL.cpp 实现，Android 暂用桩）

// 剪贴板写入（ServicesPage / ExistingAccountPage 调用）
void setClipboardText( const char *inText ) {
    // stub: Android 暂不支持剪贴板写入
    (void)inText;
}

// 重启游戏（AutoUpdatePage 调用）
char relaunchGame() {
    // stub: Android 不支持自动重启
    return 0;
}

// ============================================================================
// 批 4b stubs（续）：game.h / musicPlayer.h 函数（gameSDL.cpp 实现，Android 暂用桩）
// ============================================================================

// game.h - 硬退出模式（RebirthChoicePage / GeneticHistoryPage 调用）
char isHardToQuitMode() {
    // stub: 非硬退出模式
    return 0;
}

// game.h - 打开 URL（GeneticHistoryPage / ExistingAccountPage 调用）
void launchURL( char *inURL ) {
    // stub: Android 暂不支持打开 URL
    (void)inURL;
}

// game.h - 是否支持打开 URL
char isURLLaunchSupported() {
    // stub: 暂不支持
    return 0;
}

// game.h - VSync 状态（SettingsPage 调用）
char getCountingOnVsync() {
    // stub: 不使用 VSync 计数
    return 0;
}

// game.h - 最近帧率（SettingsPage 调用）
double getRecentFrameRate() {
    // stub: 返回目标帧率
    return 60.0;
}

// game.h - 光标模式（SettingsPage 调用，Android 无鼠标光标）
void setCursorMode( int inMode ) {
    // stub: 无操作
    (void)inMode;
}

int getCursorMode() {
    // stub: 返回默认模式 0
    return 0;
}

void setEmulatedCursorScale( double inScale ) {
    // stub: 无操作
    (void)inScale;
}

double getEmulatedCursorScale() {
    // stub: 返回默认缩放 1.0
    return 1.0;
}

// game.h - 音效总音量（SettingsPage 调用）
void setSoundLoudness( float inLoudness ) {
    // stub: 无操作
    (void)inLoudness;
}

// game.h - 恢复音效播放（SettingsPage 调用）
void resumePlayingSoundSprites() {
    // stub: 无操作
}

// game.h - 获取 web 请求进度大小（game.h 声明）
int getWebProgressSize( int inHandle ) {
    // stub: 返回 0
    (void)inHandle;
    return 0;
}

// musicPlayer.h - 步进音乐播放器（SettingsPage 调用）
// 注：stepMusicPlayer 已在 musicPlayer.cpp 中实现，此处不重复定义

// game.cpp 全局变量（ExistingAccountPage / RebirthChoicePage 引用）
// instructionsSprite 由 game.cpp 提供（批 4c 接入后移除此注释）

// ============================================================================
// 批 4c stubs：LivingLifePage / game.cpp 引用的 gameGraphics.h / game.h 函数
// gameSDL.cpp 实现，Android 暂用桩
// ============================================================================

// gameGraphics.h - 颜色查询
// 注：getDrawColor / getFloatColor 由 gameGraphicsGL.cpp 提供。

// gameGraphics.h - 灰度绘制 / sprite 查询 / sprite 像素计数 / drawSprite 带角点颜色
// 注：toggleGrayscaleDrawing / getSpriteWidth / getSpriteHeight /
//     startCountingSpritesDrawn / endCountingSpritesDrawn /
//     startCountingSpritePixelsDrawn / endCountingSpritePixelsDrawn /
//     drawSprite(各重载) 均由 gameGraphicsGL.cpp 提供。

// gameGraphics.h - drawSprite 带角点颜色
// gameGraphics.h - 方差切换（sound sprite）
void toggleVariance( SoundSpriteHandle inHandle, char inNoVariance ) {
    (void)inHandle; (void)inNoVariance;
}

// game.h - 截图
void saveScreenShot( const char *inPrefix, Image **outImage ) {
    (void)inPrefix;
    if( outImage ) *outImage = nullptr;
}

// game.h - 退出游戏
void quitGame() {
    // stub: Android 不支持直接退出，忽略
}

// game.h - 坐标转换（屏幕 → 世界）
// 参考 gameSDL.cpp 的 screenToWorld（mouseWorldCoordinates=true 分支）

void screenToWorld( int inX, int inY, float *outX, float *outY ) {
    if (gMouseWorldCoordinates) {
        int screenWidth  = minorGemsAndroid::getPhysicalWidth();
        int screenHeight = minorGemsAndroid::getPhysicalHeight();
        if (screenWidth <= 0) screenWidth = 640;
        if (screenHeight <= 0) screenHeight = 320;

        // 相对于屏幕中心，viewSize 在 screenWidth 上展开
        float x = (float)(inX - (screenWidth / 2)) / (float)screenWidth;
        float y = -(float)(inY - (screenHeight / 2)) / (float)screenWidth;

        *outX = x * gViewSize + gViewCenterX;
        *outY = y * gViewSize + gViewCenterY;
    } else {
        if (outX) *outX = (float)inX;
        if (outY) *outY = (float)inY;
    }
}

// game.h - 音效淡出
void fadeSoundSprites( double inFadeSeconds ) {
    (void)inFadeSeconds;
}

// game.h - 从文件加载音效
SoundSpriteHandle loadSoundSprite( const char *inAIFFFileName ) {
    (void)inAIFFFileName;
    return (void*)0x1;
}

SoundSpriteHandle loadSoundSprite( const char *inFolderName,
                                   const char *inFileNameBase ) {
    (void)inFolderName; (void)inFileNameBase;
    return (void*)0x1;
}

// ============================================================================
// 批 4c stubs：socket 函数（C++ 链接包装 gameAndroid.cpp 的 C 实现）
// gameAndroid.cpp 用 extern "C" 定义，但 game.h 声明为 C++ 链接，
// 此处提供 C++ 包装以解决名称修饰不匹配问题
// ============================================================================

// 前向声明 gameAndroid.cpp 中的 C 链接实现
extern "C" {
    int openSocketConnection_c( const char *inNumericalAddress, int inPort );
    int sendToSocket_c( int inHandle, unsigned char *inData, int inDataLength );
    int readFromSocket_c( int inHandle, unsigned char *inDataBuffer, int inBytesToRead );
    void closeSocket_c( int inHandle );
}

// 实际上 gameAndroid.cpp 的函数名就是 openSocketConnection 等，
// 只是用 extern "C" 导出，所以直接用 minorGems 的 SocketClient 重新实现
#include "minorGems/network/SocketClient.h"
#include "minorGems/network/Socket.h"
#include "minorGems/network/HostAddress.h"
#include "minorGems/util/SimpleVector.h"

namespace {
    struct SockRecord {
        int handle;
        Socket *sock;
    };
    static SimpleVector<SockRecord> sCppSocketRecords;
    static int sCppNextHandle = 1000; // 避免与 gameAndroid.cpp 的 handle 冲突
}

int openSocketConnection( const char *inNumericalAddress, int inPort ) {
    if (!inNumericalAddress || inPort <= 0) {
        __android_log_print(ANDROID_LOG_ERROR, "OneLife",
            "openSocketConnection: invalid params (addr=%p, port=%d)",
            inNumericalAddress, inPort);
        return -1;
    }
    __android_log_print(ANDROID_LOG_INFO, "OneLife",
        "openSocketConnection: %s:%d", inNumericalAddress, inPort);
    SockRecord r;
    r.handle = sCppNextHandle++;
    HostAddress addr( stringDuplicate( inNumericalAddress ), inPort );
    char timedOut = false;
    // timeout=0：非阻塞 connect，与 gameSDL.cpp 保持一致
    // 调用方通过 isConnected() 轮询连接状态
    r.sock = SocketClient::connectToServer( &addr, 0, &timedOut );
    if( r.sock != NULL ) {
        sCppSocketRecords.push_back( r );
        return r.handle;
    }
    __android_log_print(ANDROID_LOG_ERROR, "OneLife",
        "openSocketConnection: FAILED (sock=NULL)");
    return -1;
}

int sendToSocket( int inHandle, unsigned char *inData, int inDataLength ) {
    if (!inData || inDataLength <= 0) return -1;
    for( int i = 0; i < sCppSocketRecords.size(); i++ ) {
        SockRecord *r = sCppSocketRecords.getElement( i );
        if( r->handle == inHandle ) {
            if( r->sock && r->sock->isConnected() ) {
                int n = r->sock->send( inData, inDataLength, false, false );
                return ( n == -2 ) ? 0 : n;
            }
            return -1;
        }
    }
    return -1;
}

int readFromSocket( int inHandle, unsigned char *inDataBuffer, int inBytesToRead ) {
    if (!inDataBuffer || inBytesToRead <= 0) return -1;
    for( int i = 0; i < sCppSocketRecords.size(); i++ ) {
        SockRecord *r = sCppSocketRecords.getElement( i );
        if( r->handle == inHandle ) {
            if( r->sock && r->sock->isConnected() == 1 ) {
                int n = r->sock->receive( inDataBuffer, inBytesToRead, 0 );
                if (n == -1) {
                    __android_log_print(ANDROID_LOG_WARN, "OneLife",
                        "readFromSocket: connection lost (handle=%d)", inHandle);
                }
                return ( n == -2 ) ? 0 : n;
            }
            __android_log_print(ANDROID_LOG_WARN, "OneLife",
                "readFromSocket: not connected (handle=%d)", inHandle);
            return 0;
        }
    }
    return -1;
}

void closeSocket( int inHandle ) {
    for( int i = 0; i < sCppSocketRecords.size(); i++ ) {
        SockRecord *r = sCppSocketRecords.getElement( i );
        if( r->handle == inHandle ) {
            if (r->sock) delete r->sock;
            sCppSocketRecords.deleteElement( i );
            return;
        }
    }
}

// ============================================================================
// 批 4c stubs（续）：game.h / gameGraphics.h 函数（gameSDL.cpp 实现，Android 暂用桩）
// ============================================================================

// gameGraphics.h - MipMap 控制 / Transparent cropping
// 注：toggleMipMapMinFilter / toggleMipMapGeneration / toggleTransparentCropping
//     均由 gameGraphicsGL.cpp 提供。

// game.h - 视图大小 / 信箱
void setViewSize( float inSize ) {
    gViewSize = inSize;
}

void setLetterbox( float inVisibleWidth, float inVisibleHeight ) {
    gVisibleWidth = inVisibleWidth;
    gVisibleHeight = inVisibleHeight;
    redoDrawMatrix();
}

// game.h - 光标 / 输入
void setCursorVisible( char inIsVisible ) {
    (void)inIsVisible;
}

void grabInput( char inGrabOn ) {
    (void)inGrabOn;
}

void setMouseReportingMode( char inWorldCoordinates ) {
    gMouseWorldCoordinates = inWorldCoordinates;
}

void getLastMouseScreenPos( int *outX, int *outY ) {
    if( outX ) *outX = onelifeAndroidLastMouseX();
    if( outY ) *outY = onelifeAndroidLastMouseY();
}

// game.h - 音频控制
void setSoundPlaying( char inPlaying ) {
    (void)inPlaying;
}

void setSoundSpriteVolumeRange( double inMin, double inMax ) {
    (void)inMin; (void)inMax;
}

// game.h - 暂停 / 帧率
void pauseGame() {}

char isPaused() { return 0; }

char isQuittingBlocked() { return 0; }

void wakeUpPauseFrameRate() {}

// game.h - 加载完成回调（gameSDL.cpp 实现，Android 暂用桩）
void loadingComplete() {}

// game.h - Steam 认证（非 Steam 平台返回 false）
char runSteamGateClient() { return 0; }

// ============================================================================
// 批 4c stubs：ObjectPickable::sStack（PickableStatics.cpp 依赖 Editor*.h，不接入）
// LivingLifePage.cpp 通过 OHOL_NON_EDITOR 宏使用 ObjectPickable，
// 但 ObjectPickable::sStack 仍需定义
// ============================================================================

#define OHOL_NON_EDITOR 1
#include "ObjectPickable.h"
SimpleVector<int> ObjectPickable::sStack;
