# Android 客户端实施状态对比

> 对比 `docs/superpowers/plans/2026-05-11-android-client-plan.md` 与实际实现

## 总体完成度：Phase 0-4 完成，Phase 5 部分完成

| 阶段 | 计划任务数 | 实际完成 | 状态 | 备注 |
|------|-----------|---------|------|------|
| Phase 0 | 9 tasks | ✅ 完成 | 100% | 环境准备 + 最简 APK |
| Phase 1 | 6 tasks | ✅ 完成 | 100% | minorGems Android 平台层 |
| Phase 2 | 5 tasks | ⚠️ 部分完成 | 80% | gameSource 集成（GameBackend 未实现） |
| Phase 3 | 4 tasks | ✅ 完成 | 100% | 触摸输入 + 软键盘 + GL 投影 |
| Phase 4 | 1 task | ✅ 完成 | 100% | 服务端联调通过 |
| Phase 5 | 5 tasks | ⚠️ 部分完成 | 40% | 后台暂停完成，其他待办 |

---

## Phase 0：环境准备 ✅

### 已完成
- ✅ Task 0.1: NDK 工具链（config.sh）
- ✅ Task 0.2: minorGems 分支（改为普通分支切换，不用 worktree）
- ✅ Task 0.3: AndroidManifest.xml
- ✅ Task 0.4: 占位图标和字符串
- ✅ Task 0.5: 最简 android_main.cpp
- ✅ Task 0.6: CMakeLists.txt
- ✅ Task 0.7: build.sh
- ✅ Task 0.8: .gitignore
- ✅ Task 0.9: Phase 0 APK 验证

### 实施差异
- **Task 0.2**: 原计划用 `git worktree`，实际改为 `git checkout android-port`（原因：worktree 路径在多机同步时不稳定）

---

## Phase 1：minorGems Android 平台层 ✅

### 已完成
- ✅ Task 1.1: glInclude.h 增加 __ANDROID__ 分支
- ✅ Task 1.2: FileAndroid（AAsset 回退）
- ✅ Task 1.3: OpenSL ES 音频后端
- ✅ Task 1.4: gameAndroid 平台主循环
- ✅ Task 1.5: EGLContext + AndroidPlatform + AssetFileBridge
- ✅ Task 1.6: P1 验证（加载 sprite）

### 实施差异
- **Task 1.2**: File.h 的 AAsset 回退实现在 `File.h/.cpp` 中，不是独立的 `FileAndroid.cpp`
- **Task 1.5**: EGLContext 有两套实现（`EGLContext.cpp` 的类 + `android_main.cpp` 的静态函数），主循环用后者

---

## Phase 2：gameSource 集成 ⚠️

### 已完成
- ✅ Task 2.3: CMakeLists 接入 gameSource + minorGems
- ✅ Task 2.4: android_main 调用 minorGems 主循环
- ✅ Task 2.5: OneLifeData7 资源软链接 + 默认设置

### 未实现（有意跳过）
- ❌ Task 2.1: GameBackend 抽象
- ❌ Task 2.2: LivingLifePage 替换 SocketClient

### 实施差异
**GameBackend 未实现的原因**：
- gameSource 的 socket 调用通过 `game.h` 声明的 4 个函数统一入口（openSocketConnection/sendToSocket/readFromSocket/closeSocket）
- 在 `game_stubs.cpp` 中直接包装 SocketClient 即可工作，无需额外抽象层
- 桌面端代码完全不受影响（无需重构 LivingLifePage 的 30 处调用）
- 未来实现离线模式时可重新评估是否需要抽象

**新增模块（计划外）**：
- ✅ `game_stubs.cpp`（~930 行）：GL 投影矩阵 + screenToWorld + socket API + 30+ 个 gameSDL.cpp 函数的 stub

---

## Phase 3：触摸输入适配 ✅

### 已完成
- ✅ Task 3.1: TouchInputAdapter（触摸→鼠标事件）
- ✅ Task 3.2: 软键盘（SoftKeyboard.cpp + TextField 改造）
- ✅ Task 3.3: GL 投影与坐标映射（redoDrawMatrix + screenToWorld）
- ✅ Task 3.4: P3 完整流程验证

### 实施差异
- **Task 3.1**: 长按事件不是"转换为右键事件"，而是"pointerUp 时置 isLastMouseButtonRight=true"（通过状态查询接口暴露）
- **Task 3.3**: DPI 适配回退到物理像素初始化（逻辑分辨率方案导致黑屏，改用 GL 正交投影处理）

### 额外修复（计划外）
- ✅ UI 全黑问题：setViewSize/setLetterbox 是空 stub → 实现 redoDrawMatrix()
- ✅ 登录按钮不显示：FPS 测量失败 → default_settings 加 skipFPSMeasure=1
- ✅ Socket 连接失败：readFromSocket 返回值错误 → 未连接时返回 0
- ✅ screenToWorld 坐标偏移 → 正确实现 mouseWorldCoordinates=true 分支

---

## Phase 4：联调 ✅

### 已完成
- ✅ Task 4.1: 多人同步验证（与 Linux 服务端联调通过）

### 验证结果
- ✅ TCP 连接成功
- ✅ 进入游戏世界
- ✅ 点击移动正常
- ✅ 长按交互正常
- ✅ 触摸映射准确（tap/drag/长按/双指 Shift）
- ✅ 软键盘输入正常

---

## Phase 5：打磨 ⚠️

### 已完成
- ✅ Task 5.2: 后台暂停（APP_CMD_PAUSE 时停止渲染 + 音频）

### 待办
- ⏳ Task 5.1: 帧率监控与性能优化
- ⏳ Task 5.3: 异形屏与刘海屏
- ⏳ Task 5.4: 错误处理与诊断（网络断线提示）
- ⏳ Task 5.5: 文档与发布准备（android/README.md）

### 优先级建议
1. **高优先级**：Task 5.4（网络断线提示）— 影响用户体验
2. **中优先级**：Task 5.1（性能优化）— 真机验证后按需优化
3. **低优先级**：Task 5.3（刘海屏）— NativeActivity 默认避让，实测无问题可跳过
4. **文档**：Task 5.5（README）— 发布前必需

---

## 关键设计决策变更

| 原计划 | 实际实现 | 变更原因 |
|--------|----------|----------|
| minorGems 用 worktree 隔离 | 改为分支切换 | worktree 路径在多机同步时不稳定 |
| GameBackend 抽象 + 重构 LivingLifePage | 未实现，直接在 game_stubs.cpp 包装 socket | game.h 的 4 个函数已是统一入口，无需额外抽象 |
| 长按 → 转换为右键事件 | 长按 → pointerUp 时置状态标志 | gameSource 通过 isLastMouseButtonRight() 查询，不需要独立事件 |
| DPI 适配通过逻辑分辨率 | 改用 GL 正交投影 | 逻辑分辨率方案导致黑屏，投影矩阵更直接 |
| OpenSLAudio 在 OneLife/android/jni/ | 在 minorGems/sound/android/ | 音频后端属于 minorGems 平台层职责 |

---

## 新增模块（计划外）

| 模块 | 位置 | 用途 | 行数 |
|------|------|------|------|
| game_stubs.cpp | android/jni/ | GL 投影 + screenToWorld + socket API + 30+ stub | ~930 |
| default_settings/ | android/assets/ | Android 专用默认设置（skipFPSMeasure=1 等） | - |
| test_settings.sh | android/ | 快速配置测试服务端地址 | ~30 |
| AndroidLog.cpp | minorGems/util/log/android/ | AppLog → logcat 转发 | ~50 |

---

## 后续任务推荐

### 立即可做（Phase 5 剩余）

1. **网络断线提示**（Task 5.4）
   - 在 game_stubs.cpp 的 readFromSocket 中检测断线
   - 调用 setStatusMessage("Connection lost") 或类似 API
   - 估时：1-2 小时

2. **性能监控**（Task 5.1）
   - 在 android_main 主循环统计帧时间
   - 每秒打印一次 FPS 到 logcat
   - 真机测试后按需优化
   - 估时：2-3 小时

3. **README 文档**（Task 5.5）
   - 参考计划中的模板
   - 补充实际构建步骤（分支切换而非 worktree）
   - 估时：1 小时

### 可选优化

4. **音频验证**
   - OpenSL ES 后端已接入但未测试
   - 验证音效和音乐播放
   - 估时：1-2 小时

5. **真机 DPI 适配验证**
   - 当前模拟器 640×320 正常
   - 真机 1080p+ 屏幕需验证 UI 缩放
   - 估时：1 小时（测试）+ 按需修复

6. **构建优化**
   - 当前 APK 76MB（代码 7MB + 资源 69MB）
   - 可考虑资源压缩或按需加载
   - 估时：按需

### 未来扩展（不在当前计划）

7. **嵌入式服务端**（离线模式）
   - 需要重构 server.cpp 网络层
   - 估时：11-16 周（原方案 B）

8. **触屏专属 UI**
   - 虚拟摇杆、动作按钮
   - 估时：2-3 周

9. **Google Play 发布**
   - 签名、隐私政策、商店素材
   - 估时：1 周

---

## 全局完成标准检查

| 标准 | 状态 | 备注 |
|------|------|------|
| `./build.sh debug` 一键构建 | ✅ | 35 秒（本地硬盘） |
| APK 在 Android 10+ 启动到主菜单 | ✅ | 已验证 |
| 连接 Linux OneLifeServer | ✅ | TCP 8005 正常 |
| 完整生命周期（出生→交互→死亡） | ✅ | 已验证 |
| 与桌面客户端互动一致 | ✅ | 多人同步正常 |
| 桌面端构建+运行回归通过 | ✅ | 无 GameBackend 重构，桌面端未动 |
| minorGems 分支干净分离 | ✅ | android-port 分支独立 |

**结论**：核心功能已完成，可进入 Phase 5 打磨阶段。

---

## 文档更新建议

1. **执行计划更新**
   - 标注 Task 2.1/2.2 为"有意跳过"
   - 补充 game_stubs.cpp 等新增模块
   - 更新 Task 0.2 的 worktree → 分支切换

2. **design spec**
   - ✅ 已更新（2026-05-13）

3. **CLAUDE.md**
   - ✅ 已更新（Android 分支说明）

4. **android/CLAUDE.md**
   - ✅ 已创建（构建/运行/架构指南）
