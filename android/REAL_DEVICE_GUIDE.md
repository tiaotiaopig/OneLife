# OneLife Android 真机试玩指南

> 版本：2025-05-13  
> 状态：✅ 真机准备完成（Phase 0-4 + 真机优化）

---

## 前置条件

### 硬件要求
- **Android 版本**：10+（API 29+）
- **架构**：arm64-v8a / armeabi-v7a / x86_64
- **内存**：建议 4GB+
- **存储**：至少 200MB 可用空间（APK 76MB + 内部数据 ~100MB）
- **屏幕**：横屏设备或支持横屏旋转

### 软件要求
- **开发者模式**：已启用
- **USB 调试**：已开启
- **adb**：已安装并能连接设备

---

## 快速开始

### 1. 连接设备

```bash
# 确认设备连接
adb devices
# 应显示：
# List of devices attached
# <设备序列号>    device
```

### 2. 安装 APK

```bash
cd /data1/fengli16/Projects/CProjects/OneLife/android
adb install -r build/OneLife-debug.apk
```

期望输出：`Success`

### 3. 配置服务器地址

**方法 A：使用快捷脚本**（推荐）

```bash
# 编辑 test_settings.sh 中的服务器 IP
vim test_settings.sh
# 修改 SERVER_IP="192.168.1.100" 为你的服务器局域网 IP

# 一键配置
./test_settings.sh
```

**方法 B：手动配置**

```bash
# 替换 192.168.1.100 为你的服务器 IP
SERVER_IP="192.168.1.100"

adb shell run-as com.fengli.onelife sh -c "echo '$SERVER_IP' > files/settings/customServerAddress.ini"
adb shell run-as com.fengli.onelife sh -c 'echo "8005" > files/settings/customServerPort.ini'
adb shell run-as com.fengli.onelife sh -c 'echo "test123" > files/settings/serverPassword.ini'
adb shell run-as com.fengli.onelife sh -c 'echo "1" > files/settings/useCustomServer.ini'
adb shell run-as com.fengli.onelife sh -c 'echo "1" > files/settings/requireClientPassword.ini'
```

### 4. 启动并监控

```bash
# 启动应用
adb shell am start -n com.fengli.onelife/android.app.NativeActivity

# 实时查看日志（新终端）
adb logcat -s OneLife:* OneLifeAudio:* OneLifeGame:* AndroidRuntime:E
```

---

## 诊断信息解读

### 启动诊断（前 10 秒）

```
=========================================
OneLife Android 启动诊断
=========================================
  内核: Linux 4.14.190 (aarch64)
  内存: 总 3840MB, 可用 1520MB
  屏幕: 411dp x 731dp, density=420 (xxhdpi)
  方向: landscape
  语言: zh
  窗口: 2340 x 1080 (format=4)
  内部存储: /data/user/0/com.fengli.onelife/files
=========================================
EGL ready: 2340x1080
OpenGL ES:
  Vendor:   ARM
  Renderer: Mali-G76
  Version:  OpenGL ES 3.2 v1.r26p0-01rel0
  MaxTexSize: 16384
✓ OpenSL ES started: 44100 Hz stereo
```

**关键指标**：
- **density >= 480**：高分屏，UI 可能偏小（会有警告）
- **可用内存 < 1GB**：可能卡顿
- **OpenSL ES 启动失败**：静音运行（不影响游戏）

### 运行时诊断（每 5 秒）

```
FPS: 58.3 (min 15.2ms, max 18.9ms, frames=291/5.0s)
```

**性能标准**：
- **FPS >= 50**：流畅 ✅
- **FPS 30-50**：可玩 ⚠️
- **FPS < 30**：卡顿 ❌（会有警告）
- **max > 100ms**：有明显掉帧（会有警告）

### 网络诊断

```
openSocketConnection: 192.168.1.100:8005
readFromSocket: connection lost (handle=0)
readFromSocket: not connected (handle=0)
```

**常见问题**：
- `connection lost`：网络中断，检查 WiFi 信号
- `not connected`：服务器未启动或地址错误
- `FAILED (sock=NULL)`：DNS 解析失败或端口被占用

---

## 操作指南

### 触摸映射

| 操作 | 桌面端 | Android 触摸 |
|------|--------|-------------|
| 左键点击 | 鼠标左键 | **单指点击** |
| 右键点击 | 鼠标右键 | **长按 500ms** |
| Shift+左键 | Shift+鼠标左键 | **双指点击** |
| 拖拽 | 鼠标拖动 | **单指滑动** |

### 游戏操作

1. **移动**：点击目标位置
2. **拾取物品**：长按物品（500ms）
3. **放下物品**：长按空地
4. **使用物品**：点击物品 A，再点击物品 B
5. **聊天**：点击聊天框，软键盘自动弹出

### 软键盘

- **自动弹出**：点击文本框（邮箱/密码/聊天）
- **手动关闭**：点击键盘外区域或按返回键
- **支持按键**：字母、数字、空格、退格、回车

---

## 常见问题

### Q1: 安装失败 "INSTALL_FAILED_UPDATE_INCOMPATIBLE"

**原因**：已安装旧版本，签名不一致

**解决**：
```bash
adb uninstall com.fengli.onelife
adb install build/OneLife-debug.apk
```

### Q2: 启动后黑屏或闪退

**诊断**：
```bash
adb logcat -s AndroidRuntime:E | grep -A 20 "FATAL EXCEPTION"
```

**常见原因**：
- EGL 初始化失败 → 设备不支持 OpenGL ES 1.x
- 资源加载失败 → APK 损坏，重新构建
- 内存不足 → 关闭其他应用

### Q3: 登录界面按钮不显示

**原因**：FPS 测量失败（default_settings 已修复）

**验证**：
```bash
adb shell run-as com.fengli.onelife cat files/settings/skipFPSMeasure.ini
# 应输出：1
```

### Q4: 点击移动无响应

**诊断**：
```bash
adb logcat -s OneLife:* | grep "screenToWorld\|pointerDown"
```

**常见原因**：
- GL 投影矩阵错误 → 已在 game_stubs.cpp 修复
- 触摸事件未传递 → 检查 TouchInputAdapter 日志

### Q5: 连接服务器失败

**检查清单**：
1. 服务器是否启动？`ps aux | grep OneLifeServer`
2. 端口是否监听？`netstat -an | grep 8005`
3. 防火墙是否放行？`iptables -L | grep 8005`
4. 设备与服务器在同一局域网？`ping <服务器IP>`
5. customServerAddress.ini 是否正确？

### Q6: 音频无声

**诊断**：
```bash
adb logcat -s OneLifeAudio:*
```

**预期行为**：
- 成功：`✓ OpenSL ES started: 44100 Hz stereo`
- 失败：`audio disabled` → 静音运行，不影响游戏

### Q7: UI 元素过小（高分屏）

**临时方案**：
- 使用触控笔提高精度
- 开启系统"放大手势"（设置 → 辅助功能）

**长期方案**：
- 实现 DPI 缩放（Phase 5 待办）

### Q8: 发热严重

**优化建议**：
1. 降低帧率：修改 `targetFrameRate.ini` 为 30
2. 后台暂停：按 Home 键自动停止渲染
3. 关闭音频：删除 OpenSL ES 初始化代码

---

## 性能基准

### 测试设备：Android 模拟器（x86_64）

| 指标 | 数值 |
|------|------|
| 分辨率 | 640×320 |
| FPS | 58-60 |
| 内存占用 | ~150MB |
| APK 大小 | 76MB |
| 首次启动 | ~3 秒 |
| 连接服务器 | ~500ms |

### 真机预期（arm64，1080p）

| 指标 | 低端（4GB RAM） | 中端（6GB RAM） | 高端（8GB+ RAM） |
|------|----------------|----------------|-----------------|
| FPS | 30-45 | 50-60 | 60 |
| 内存占用 | ~200MB | ~180MB | ~150MB |
| 首次启动 | ~5 秒 | ~3 秒 | ~2 秒 |
| 发热 | 明显 | 轻微 | 无 |

---

## 已知限制

### 功能限制
- ❌ 离线模式（需要 EmbeddedBackend，Phase 6+）
- ❌ 触屏专属 UI（虚拟摇杆、动作按钮）
- ❌ 自动重连（网络断线需手动重启）
- ❌ 崩溃上报（无 Crashlytics 集成）
- ⚠️ 音频未测试（OpenSL ES 已接入但未验证）

### 平台限制
- 仅支持横屏（Manifest 锁定）
- 仅支持 API 29+（Android 10+）
- 不支持 x86（32 位）架构
- 不支持分屏模式

### UI 限制
- 高分屏（xxhdpi+）UI 元素偏小
- 无 DPI 自适应缩放
- 无刘海屏/异形屏适配
- 按钮触摸热区未放大

---

## 下一步

### 立即可做（Phase 5 剩余）

1. **真机测试**（1-2 小时）
   - 在真机上完整走一遍本指南
   - 记录实际 FPS、内存、发热
   - 验证音频是否正常

2. **README 文档**（1 小时）
   - 补充构建步骤
   - 补充运行指南
   - 补充已知限制

### 可选优化

3. **音频验证**（1-2 小时）
   - 确认 BGM 和音效播放
   - 测试音频暂停/恢复

4. **DPI 缩放**（2-3 小时）
   - 基于 density 动态缩放 UI
   - 放大按钮触摸热区

5. **自动重连**（2-3 小时）
   - 检测断线后自动重试
   - 显示重连进度

### 未来扩展（不在当前计划）

6. **嵌入式服务端**（11-16 周）
   - 离线单机模式
   - 本地多人联机

7. **触屏专属 UI**（2-3 周）
   - 虚拟摇杆
   - 快捷动作按钮

8. **Google Play 发布**（1 周）
   - 签名密钥
   - 隐私政策
   - 商店素材

---

## 技术细节

### 真机准备改动（2025-05-13）

| Task | 模块 | 改动 | 影响 |
|------|------|------|------|
| 13 | DeviceInfo | 启动诊断 + FPS 统计 | 方便真机调试 |
| 15 | game_stubs | socket 参数验证 + 空指针检查 | 避免网络异常崩溃 |
| 14 | game_stubs | readFromSocket 断线日志 | 快速诊断网络问题 |
| 17 | DeviceInfo | 高分屏警告 | 提示 UI 缩放需求 |
| 16 | OpenSLAudioBackend | 错误检测 + 优雅降级 | 音频失败不影响游戏 |

### 架构概览

```
android/
├── jni/
│   ├── android_main.cpp       # NativeActivity 入口 + 主循环
│   ├── DeviceInfo.cpp         # 设备诊断（启动信息 + FPS）
│   ├── TouchInputAdapter.cpp  # 触摸→鼠标事件映射
│   ├── SoftKeyboard.cpp       # 软键盘控制
│   ├── game_stubs.cpp         # GL 投影 + socket API + 30+ stubs
│   └── AssetFileBridge.cpp    # AAsset 注入 + 默认设置复制
├── assets/                    # 软链接到 OneLifeData7
├── build.sh                   # 一键构建脚本
├── test_settings.sh           # 快速配置测试服务器
└── REAL_DEVICE_GUIDE.md       # 本文档

minorGems/sound/android/
└── OpenSLAudioBackend.cpp     # OpenSL ES 音频后端

minorGems/game/platforms/Android/
└── gameAndroid.cpp            # minorGems 平台层 + socket API
```

---

## 反馈与支持

### 报告问题

1. **收集日志**：
   ```bash
   adb logcat -d > onelife-android.log
   ```

2. **设备信息**：
   ```bash
   adb shell getprop ro.build.version.release  # Android 版本
   adb shell getprop ro.product.model          # 设备型号
   adb shell getprop ro.product.cpu.abi        # CPU 架构
   ```

3. **提交 Issue**：
   - 附上日志文件
   - 描述复现步骤
   - 标注预期行为 vs 实际行为

### 文档更新

- **CLAUDE.md**：项目总览 + Android 分支入口
- **android/CLAUDE.md**：构建/运行/架构实战指南
- **android/IMPLEMENTATION_STATUS.md**：执行计划对比 + 后续任务
- **docs/superpowers/specs/2026-05-11-android-client-design.md**：设计思路

---

**祝试玩愉快！** 🎮
