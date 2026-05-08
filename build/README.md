# OneLife 发布构建系统

本目录包含 OneLife 项目的官方发布构建工具链，用于为多个平台打包和发布游戏。

## 快速参考

### 常用命令

```bash
# macOS 完整发布（从 v37 升级到 v39）
cd build
./pullBuildAndPostMac.sh 37 39

# Windows 完整发布（从 v37 升级到 v39）
cd build
./buildAndPostWindows.sh 37 39

# Linux 二进制发布
cd build/source
./makeLinuxBuild v39

# 单独打包（不含增量更新和上传）
cd build
./makeDistributionMacOSX v39 IntelMacOSX /System/Library/Frameworks/SDL.framework
./makeDistributionWindows v39
```

### 发布包内容速查

| 平台 | 客户端 | 服务端 | 编辑器 | 数据 | 体积 |
|------|--------|--------|--------|------|------|
| Windows | ✅ | ✅ | ✅ | ✅ | ~70MB |
| macOS | ✅ | ❌ | ❌ | ✅ | ~65MB |
| Linux | ✅ | ❌ | ✅ | ✅ | ~65MB |
| Steam | ✅ | ❌ | ❌ | ❌ | ~4MB |

**注意：** Windows 是唯一包含服务端的发布包。

## 目录结构

```
build/
├── source/              # Linux 源码发布脚本
├── macOSX/              # macOS 应用包模板和资源
├── win32/               # Windows DLL 依赖和图标资源
├── steam/               # Steam depot 构建配置
├── makeBaseDistributionFolder    # 基础发布包构建（所有平台共用）
├── makeDistributionMacOSX        # macOS 发布打包
├── makeDistributionWindows       # Windows 发布打包
├── pullBuildAndPostMac.sh        # macOS 完整发布流程（自动化）
└── buildAndPostWindows.sh        # Windows 完整发布流程（自动化）
```

## 支持的平台

- **Linux** — 源码发布 + 二进制发布
- **macOS** — .app 应用包 + tar.gz
- **Windows** — 可执行文件 + DLL + 自解压安装包
- **Steam** — Windows/macOS Steam depot 构建

## 各平台发布包内容对比

不同平台的发布包包含的组件有所不同：

| 平台 | 客户端 | 服务端 | 编辑器 | 游戏数据 | 备注 |
|------|--------|--------|--------|----------|------|
| **Windows** | ✅ OneLife.exe | ✅ OneLifeServer.exe | ✅ EditOneLife.exe | ✅ 完整数据 | 最完整的发布包 |
| **macOS** | ✅ OneLife.app | ❌ | ❌ | ✅ 完整数据 | 仅客户端 |
| **Linux** | ✅ OneLifeApp | ❌ | ✅ EditOneLife | ✅ 完整数据 | 客户端 + 编辑器 |
| **Steam (Windows)** | ✅ | ❌ | ❌ | ❌ | 数据通过单独 depot 分发 |

### 为什么 Windows 包含服务端？

**Windows 是唯一同时包含客户端和服务端的发布包**，设计原因：

1. **私服友好** — Windows 用户可以轻松搭建本地服务器测试
2. **内容创作** — 包含编辑器和服务器，方便内容创作者（Content Leader）修改游戏内容
3. **一站式体验** — 单个下载包即可体验完整功能（玩游戏 + 开服务器 + 编辑内容）

Windows 发布包额外包含：
- `runServer.bat` — 服务器启动脚本
- `contentLeaderSetup.bat` — 内容编辑器环境配置
- `copyChangesToRepo.bat` — 内容变更同步脚本

### macOS/Linux 为什么不包含服务端？

1. **目标用户不同** — macOS 用户主要是玩家，不是服务器管理员
2. **服务器部署习惯** — Linux 服务器通常从源码构建或使用专门的服务端包（如项目根目录 `dist/` 的部署包）
3. **包体积优化** — 减少不必要的文件

### Steam 版本的特殊性

Steam 版本采用**数据分离架构**：

- **Depot 595692** — Windows 客户端二进制和配置（~4MB）
- **Depot 595691** — 游戏数据（sprites、objects、sounds 等，~60MB）

**优势：**
- 代码更新和数据更新可以独立进行
- 减少玩家下载量（代码更新时不需要重新下载数据）
- Steam 自动管理数据完整性

**特殊配置：**
- `reportWildBugToUser.ini = 0` — Steam 用户无法访问文件系统报告 bug
- `useSteamUpdate.ini = 1` — 禁用内置更新器，使用 Steam 自动更新

### 如何在 macOS/Linux 上运行服务端？

如果需要在 macOS 或 Linux 上运行服务端，有两个选择：

**方案 1：使用 dist/ 部署包（推荐）**
```bash
cd dist
tar xzf onelife-server-linux-x64.tar.gz
cd onelife-server
./start.sh
```

**方案 2：手动编译服务端**
```bash
cd server
./configure 1  # 1=Linux, 2=macOS
make
# 然后手动链接数据目录（见项目根目录 CLAUDE.md）
```

Windows 用户最方便，解压发布包后直接运行 `runServer.bat` 即可启动服务器。

## 核心脚本说明

### makeBaseDistributionFolder

所有平台共用的基础发布包构建脚本。

**用途：**
- 创建标准目录结构（graphics、sprites、objects、transitions 等）
- 从 `gameSource/` 复制客户端资源和配置
- 从 `OneLifeData7/` 复制游戏数据（精灵、对象、音效等）
- 从 `server/` 复制服务端配置和文本文件
- 清理编辑器备份文件（`*~`）

**输出：**
```
base/OneLife_<版本>/
├── graphics/          # 客户端图形资源
├── sprites/           # 精灵图
├── objects/           # 游戏对象定义（4431+ 个）
├── transitions/       # 合成配方
├── animations/        # 动画元数据
├── music/sounds/      # 音频资源
├── settings/          # 客户端配置
├── serverSettings/    # 服务端配置
└── mods/              # Mod 支持目录
```

### makeDistributionMacOSX

macOS 平台发布打包脚本。

**用法：**
```bash
./makeDistributionMacOSX <版本名> <平台名> <SDL.framework路径>
# 示例：
./makeDistributionMacOSX v39 IntelMacOSX /System/Library/Frameworks/SDL.framework
```

**流程：**
1. 调用 `makeBaseDistributionFolder` 创建基础包
2. 生成资源缓存文件（`regenerateCaches`）
3. 将 `OneLife` 二进制打包进 `OneLife.app` 应用包
4. 安装 SDL.framework 依赖到 `Contents/Frameworks/`
5. 强制设置窗口模式（`fullscreen.ini = 0`，全屏模式有崩溃问题）
6. 删除 `bin_*cache.fcz`（体积大且每次更新都变化）
7. 打包为 `OneLife_<版本>_<平台>.tar.gz`

**输出：**
- `mac/OneLife_<版本>_<平台>.tar.gz`

### makeDistributionWindows

Windows 平台发布打包脚本。

**用法：**
```bash
./makeDistributionWindows <版本名>
# 示例：
./makeDistributionWindows v39
```

**流程：**
1. 调用 `makeBaseDistributionFolder` 创建基础包
2. 复制可执行文件：
   - `OneLife.exe` — 游戏客户端
   - `EditOneLife.exe` — 内容编辑器
   - `OneLifeServer.exe` — 游戏服务器
3. 复制 DLL 依赖（SDL.dll、libpng14.dll、zlib1.dll、libgcc_s_dw2-1.dll）
4. 复制内容编辑器脚本（`contentLeaderSetup.bat`、`copyChangesToRepo.bat`）
5. 将所有 `.txt` 文件转换为 Windows 换行符（CRLF）
6. 生成自解压安装包脚本（`makeWindowsExtractor_<版本>.bat`）

**输出：**
- `windows/OneLife_<版本>/` — 发布目录
- `windows/makeWindowsExtractor_<版本>.bat` — 自解压安装包生成脚本

### pullBuildAndPostMac.sh

macOS 完整发布流程自动化脚本（包含增量更新和 Steam 构建）。

**用法：**
```bash
./pullBuildAndPostMac.sh <上一版本号> <新版本号>
# 示例：从 v37 升级到 v39
./pullBuildAndPostMac.sh 37 39
```

**前置条件：**
- 旧版本发布包必须存在于 `mac/OneLife_v<上一版本号>/`（用于生成增量补丁）
- `game.cpp` 中的版本号必须与新版本号一致

**流程：**
1. **代码更新**：`git pull` 更新 minorGems 和 OneLife
2. **数据版本锁定**：将 `OneLifeData7` 切换到 `OneLife_v25` 标签
3. **版本校验**：检查 `game.cpp` 中的 `versionNumber` 是否匹配
4. **编译**：`./configure 2 && make` 构建 macOS 客户端
5. **打包**：调用 `makeDistributionMacOSX` 生成发布包
6. **增量更新包**：使用 `minorGems/game/diffBundle` 生成 `<版本>_inc_mac.dbz`
7. **上传**：scp 上传增量补丁到 `onehouronelife.com:diffBundles/`
8. **数据恢复**：将 `OneLifeData7` 切换回 `master` 分支

**输出：**
- `mac/OneLife_v<新版本>_IntelMacOSX.tar.gz` — 完整发布包
- `mac/<新版本>_inc_mac.dbz` — 增量更新包（已上传）

### buildAndPostWindows.sh

Windows 完整发布流程自动化脚本（包含增量更新和 Steam depot 构建）。

**用法：**
```bash
./buildAndPostWindows.sh <上一版本号> <新版本号>
# 示例：从 v37 升级到 v39
./buildAndPostWindows.sh 37 39
```

**前置条件：**
- 旧版本发布包必须存在于 `windows/OneLife_v<上一版本号>/`
- 需要手动 `git pull` 更新 minorGems 和 OneLife
- 需要手动将 `OneLifeData7` 切换到 `OneLife_v20` 标签

**流程：**
1. **版本校验**：检查 `game.cpp` 中的 `versionNumber` 是否匹配
2. **编译**：
   - `./configure 3` — 配置 MinGW 构建环境
   - 构建客户端（`OneLife.exe`）、编辑器（`EditOneLife.exe`）、服务器（`OneLifeServer.exe`）
3. **打包**：调用 `makeDistributionWindows` 生成发布包
4. **增量更新包**：生成 `<版本>_inc_win.dbz` 并上传
5. **Steam 构建**：
   - 创建 `steamLatest/` 目录，复制发布包内容
   - 移除数据文件（Steam 用户从 Steam 下载）
   - 添加 Steam API 集成（`steam_api.dll`、`steamGateClient.exe`）
   - 设置 `reportWildBugToUser.ini = 0`（Steam 用户无法访问文件报告 bug）
   - 设置 `useSteamUpdate.ini = 1`（禁用内置更新器）
   - 打包并上传到服务器
   - 远程执行 Steam depot 构建：`~/checkout/OneLifeWorking/scripts/runWindowsSteamDepotBuild.sh <版本>`

**输出：**
- `windows/OneLife_v<新版本>/` — 完整发布包
- `windows/<新版本>_inc_win.dbz` — 增量更新包（已上传）
- `windows/steamLatest.tar.gz` — Steam 构建包（已上传）

**注意事项：**
- 脚本会在关键步骤暂停，等待用户按 ENTER 确认
- Steam depot 构建需要在远程服务器上执行（需要 Steam SDK）
- 完成后记得在 `OneLifeData7` 中运行 `git checkout master`

## Linux 源码发布

### source/makeLinuxBuild

从源码构建 Linux 二进制发布包。

**用法：**
```bash
cd source
./makeLinuxBuild <版本名>
# 示例：
./makeLinuxBuild v39
```

**流程：**
1. 调用 `makeSourceBundle` 打包源码 tarball
2. 调用 `makeLinuxBuildFromSourceBundle` 从源码构建二进制发布包

**输出：**
- `OneLife_<版本>_UnixSource.tar.gz` — 源码包
- `OneLife_<版本>_Linux.tar.gz` — 二进制发布包

**二进制发布包内容：**
- `OneLifeApp` — 游戏客户端（重命名自 `OneLife`）
- `EditOneLife` — 内容编辑器
- 完整游戏数据（sprites、objects、transitions、animations、sounds、music 等）
- 客户端配置文件（settings/）
- 缓存目录（reverbCache、groundTileCache）

### source/makeSourceBundle

打包 Linux 源码发布包。

**用法：**
```bash
cd source
./makeSourceBundle <版本名>
```

**流程：**
1. 调用 `exportSrc` 克隆 minorGems、OneLife、OneLifeData7 仓库
2. 将游戏数据从 OneLifeData7 移动到源码包根目录
3. 调用 `cleanSrc` 清理不必要的文件
4. 复制 `runToBuild` 构建脚本
5. 打包为 `.tar.gz`

**输出：**
- `OneLife_<版本>_UnixSource.tar.gz` — 包含完整源码和构建脚本

**源码包结构：**
```
OneLife_<版本>_UnixSource/
├── minorGems/         # minorGems 库源码
├── OneLife/           # OneLife 源码
├── animations/        # 游戏数据（从 OneLifeData7 提取）
├── objects/
├── sprites/
├── transitions/
├── sounds/
├── music/
└── runToBuild         # 自动构建脚本
```

### source/runToBuild

源码包自动构建脚本（包含在源码发布包中）。

**用法：**
```bash
cd OneLife_<版本>_UnixSource
./runToBuild <平台编号>
# 1=Linux, 2=macOS, 3=MinGW, 4=Raspbian
```

**流程：**
1. 运行 `./configure <平台编号>` 配置构建环境
2. 编译客户端（`OneLife`）
3. 编译编辑器（`EditOneLife`）
4. 创建必要的目录（graphics、settings、reverbCache 等）
5. 复制资源文件到对应目录

**输出：**
- `OneLifeApp` — 可执行的游戏客户端
- `EditOneLife` — 可执行的内容编辑器

## 工具脚本

### unix2dos.c / unix2dosScript

将 Unix 换行符（LF）转换为 Windows 换行符（CRLF）。

**用途：**
- Windows 发布包中的所有 `.txt` 文件需要使用 CRLF 换行符
- `makeDistributionWindows` 会自动调用此工具

### pullWindowsGit.sh

Windows 构建前的 Git 更新脚本。

## 平台特定资源

### macOSX/

- `OneLife.app/` — macOS 应用包模板（包含 Info.plist、图标等）
- `icon*.png` — 应用图标资源（彩色、蒙版、合并版本）
- `dmgNotes.txt` — DMG 安装说明

### win32/

- **DLL 依赖库：**
  - `SDL.dll` — Simple DirectMedia Layer 图形库
  - `libpng14.dll` — PNG 图像库
  - `zlib1.dll` — 压缩库
  - `libgcc_s_dw2-1.dll` — GCC 运行时库
- **图标资源：**
  - `icon.ico` / `icon.png` — 游戏图标
  - `icon_installer.ico` / `icon_installer.png` — 安装程序图标
  - `icon.rc` — Windows 资源脚本
- **工具：**
  - `MakeSFX.exe` — 自解压安装包生成工具
  - `WindowsDebugTools/` — 调试工具目录

### steam/

- **Steam depot 构建配置：**
  - `app_build_windows_595690.vdf` — Windows 应用构建配置
  - `depot_build_windows_595692.vdf` — Windows depot 构建配置
  - `app_build_content_595690.vdf` — 内容构建配置
  - `depot_build_content_595691.vdf` — 内容 depot 构建配置
- **Steam 集成文件：**
  - `ahap/` — AHAP 变体 Steam 集成
  - `windows/` — Windows Steam 集成（`steam_api.dll`、`steamGateClient.exe`）

## 发布流程最佳实践

### 完整发布流程（以 v39 为例）

**1. 准备阶段**
```bash
# 更新代码
cd ~/Projects/CProjects/minorGems && git pull
cd ~/Projects/CProjects/OneLife && git pull
cd ~/Projects/CProjects/OneLifeData7 && git pull

# 更新 game.cpp 中的版本号
# 编辑 gameSource/game.cpp，修改 versionNumber = 39

# 提交版本号变更
git add gameSource/game.cpp
git commit -m "Bump version to v39"
```

**2. macOS 发布**
```bash
cd build
./pullBuildAndPostMac.sh 37 39
# 脚本会自动完成：编译 → 打包 → 增量补丁 → 上传
```

**3. Windows 发布**
```bash
cd build
# 手动准备：
cd ../../OneLifeData7 && git checkout OneLife_v20 && cd ../OneLife/build

./buildAndPostWindows.sh 37 39
# 脚本会暂停等待确认：
# - 上传增量补丁前
# - 上传 Steam 包前
# - Steam depot 构建完成后

# 完成后恢复数据版本
cd ../../OneLifeData7 && git checkout master
```

**4. Linux 发布**
```bash
cd build/source
./makeLinuxBuild v39
# 输出：OneLife_v39_UnixSource.tar.gz
```

### 增量更新包说明

增量更新包（`.dbz` 文件）由 `minorGems/game/diffBundle` 工具生成，包含：

- **文件差异**：新增、修改、删除的文件列表
- **二进制差异**：对于修改的文件，仅包含差异部分
- **压缩**：使用 bzip2 压缩

**优势：**
- 大幅减少玩家下载量（通常从 60MB+ 减少到 1-5MB）
- 客户端自动检测并应用增量更新
- 失败时自动回退到完整下载

### Steam 构建特殊处理

Steam 版本与标准版本的差异：

| 配置项 | 标准版 | Steam 版 |
|--------|--------|----------|
| 数据文件 | 包含在发布包中 | 从 Steam 下载（单独 depot） |
| 更新机制 | 内置更新器 | Steam 自动更新 |
| Bug 报告 | 显示给用户 | 不显示（用户无法访问文件） |
| Steam API | 无 | 集成 `steam_api.dll` 和 `steamGateClient.exe` |

**Steam depot 结构：**
- **Depot 595692** — Windows 客户端二进制和配置
- **Depot 595691** — 游戏数据（sprites、objects、sounds 等）

## 常见问题

### Q: 为什么 macOS 强制使用窗口模式？

A: 注释说明全屏模式有崩溃问题（`too many problems and crashes with fullscreen`）。

### Q: 为什么删除 bin_*cache.fcz？

A: 这些缓存文件体积大（数十 MB）且每次更新都会变化，包含在发布包中会增加下载量。客户端首次运行时会自动生成。

### Q: 为什么 OneLifeData7 要切换到特定标签？

A: 确保数据版本与代码版本一致。例如 v39 的代码可能需要 v20 的数据格式。构建完成后需切换回 `master`。

### Q: 增量更新包如何工作？

A: 客户端启动时会检查服务器上的增量更新包（`<版本>_inc_<平台>.dbz`），如果存在且适用于当前版本，会下载并应用差异，否则下载完整包。

### Q: 如何测试发布包？

A: 解压发布包到独立目录，运行客户端/服务器，验证：
- 客户端能否正常启动和连接服务器
- 服务器能否正常加载数据和处理玩家连接
- 版本号是否正确显示

### Q: 发布包的体积大约是多少？

A: 各平台发布包的典型体积：
- **Windows 完整包**：~70MB（包含客户端 + 服务端 + 编辑器 + 完整数据）
- **macOS 完整包**：~65MB（包含客户端 + 完整数据）
- **Linux 完整包**：~65MB（包含客户端 + 编辑器 + 完整数据）
- **Steam 客户端**：~4MB（仅二进制和配置）
- **Steam 数据 depot**：~60MB（游戏数据）
- **增量更新包**：1-5MB（取决于变更量）

### Q: 为什么 Linux 源码包要包含 OneLifeData7？

A: 源码包设计为自包含（self-contained），用户解压后即可构建，无需额外下载依赖。`exportSrc` 脚本会克隆三个仓库（minorGems、OneLife、OneLifeData7）并将游戏数据提取到源码包根目录。

### Q: 如何验证发布包的完整性？

A: 检查关键文件是否存在：

**Windows 包：**
```bash
# 必须包含的文件
OneLife.exe
OneLifeServer.exe
EditOneLife.exe
SDL.dll
settings/
serverSettings/
objects/
transitions/
sprites/
```

**macOS 包：**
```bash
# 必须包含的文件
OneLife_<版本>.app/Contents/MacOS/OneLife
OneLife_<版本>.app/Contents/Frameworks/SDL.framework
settings/
objects/
transitions/
sprites/
```

**Linux 包：**
```bash
# 必须包含的文件
OneLifeApp
EditOneLife
settings/
objects/
transitions/
sprites/
```

### Q: 构建失败常见原因？

A: 常见问题和解决方案：

1. **版本号不匹配**
   - 错误：`game.cpp version number mismatch`
   - 解决：检查 `gameSource/game.cpp` 中的 `versionNumber` 是否与命令行参数一致

2. **旧版本发布包不存在**
   - 错误：`Folder 'mac/OneLife_v37' not found`
   - 解决：增量更新需要旧版本包，首次发布跳过增量更新步骤

3. **OneLifeData7 版本错误**
   - 错误：数据加载失败或对象 ID 不匹配
   - 解决：确保 `OneLifeData7` 切换到正确的标签（如 `OneLife_v20`）

4. **SDL 依赖缺失（macOS）**
   - 错误：`SDL.framework not found`
   - 解决：安装 SDL 1.2 或指定正确的 framework 路径

5. **MinGW 环境未配置（Windows）**
   - 错误：`configure: command not found` 或编译失败
   - 解决：安装 MinGW 并配置环境变量

### Q: 如何自定义发布包？

A: 修改 `makeBaseDistributionFolder` 脚本：

```bash
# 添加自定义文件
cp /path/to/custom/file $baseFolder/

# 修改默认配置
echo "1" > $baseFolder/settings/customSetting.ini

# 排除特定文件
rm -f $baseFolder/settings/unwantedFile.ini
```

然后运行对应平台的 `makeDistribution*` 脚本。

## 故障排查

### 构建失败

**问题：版本号不匹配**
```
game.cpp version number mismatch (found '38', expecting 39).
```
**解决：** 编辑 `gameSource/game.cpp`，修改 `versionNumber = 39`

**问题：旧版本发布包不存在**
```
Folder 'mac/OneLife_v37' not found.
```
**解决：** 增量更新需要旧版本包作为基准。首次发布时，手动创建发布包或跳过增量更新步骤。

**问题：OneLifeData7 版本错误**
```
Error loading objects: ID mismatch
```
**解决：** 确保 `OneLifeData7` 切换到正确的标签：
```bash
cd ../../OneLifeData7
git checkout OneLife_v20  # 或其他指定版本
```

**问题：SDL 依赖缺失（macOS）**
```
SDL.framework not found
```
**解决：** 安装 SDL 1.2 或指定正确的 framework 路径：
```bash
brew install sdl12-compat
./makeDistributionMacOSX v39 IntelMacOSX /opt/homebrew/opt/sdl12-compat/lib/SDL.framework
```

**问题：MinGW 环境未配置（Windows）**
```
configure: command not found
```
**解决：** 安装 MinGW 并配置环境变量，或在 MSYS2/Cygwin 环境中运行。

### 运行时问题

**问题：客户端启动后黑屏**
```
Failed to load graphics
```
**解决：** 检查 `graphics/` 目录是否存在且包含 `.tga` 文件。

**问题：服务器无法加载数据**
```
Error: objects directory not found
```
**解决：** 确保数据目录（objects、transitions、categories 等）存在且包含文件。Windows 发布包已包含这些目录。

**问题：客户端无法连接服务器**
```
Connection failed
```
**解决：** 
1. 检查服务器是否正常运行：`echo "" | nc 127.0.0.1 8005`（应返回 SN 消息）
2. 检查客户端配置：`settings/customServerAddress.ini` 和 `settings/customServerPort.ini`
3. 检查防火墙设置

**问题：Steam 版本无法启动**
```
Missing steam_api.dll
```
**解决：** 确保 Steam 集成文件已正确复制：
- `steam_api.dll`
- `steamGateClient.exe`

### 增量更新问题

**问题：增量更新包生成失败**
```
diffBundle: command not found
```
**解决：** 确保 `minorGems/game/diffBundle` 已编译：
```bash
cd ../../minorGems/game/diffBundle
./diffBundleCompile  # macOS/Linux
./diffBundleCompileWindows  # Windows
```

**问题：增量更新包过大**
```
<版本>_inc_<平台>.dbz is 50MB
```
**原因：** 大量文件变更或二进制文件变更。
**解决：** 这是正常现象，客户端会自动判断是否使用增量更新。如果增量包接近完整包大小，客户端会下载完整包。

### 上传问题

**问题：scp 上传失败**
```
Permission denied (publickey)
```
**解决：** 配置 SSH 密钥或使用密码认证：
```bash
ssh-copy-id jcr15@onehouronelife.com
```

**问题：Steam depot 构建失败**
```
ERROR! Failed to get application info for AppID
```
**解决：** 
1. 确保在远程服务器上执行（需要 Steam SDK）
2. 检查 Steam 账号权限
3. 验证 `.vdf` 配置文件中的 AppID 和 DepotID

## 高级用法

### 自定义发布包

修改 `makeBaseDistributionFolder` 脚本以自定义发布包内容：

```bash
# 添加自定义文件
cp /path/to/custom/README.txt $baseFolder/

# 修改默认配置
echo "1" > $baseFolder/settings/customFeature.ini

# 排除特定文件
rm -f $baseFolder/settings/developerMode.ini

# 添加自定义目录
mkdir $baseFolder/customMods
cp -r /path/to/mods/* $baseFolder/customMods/
```

### 批量构建多个平台

创建一个脚本批量构建所有平台：

```bash
#!/bin/bash
VERSION=$1

# macOS
cd build
./makeDistributionMacOSX v$VERSION IntelMacOSX /System/Library/Frameworks/SDL.framework

# Windows
./makeDistributionWindows v$VERSION

# Linux
cd source
./makeLinuxBuild v$VERSION

echo "All platforms built successfully!"
```

### 本地测试发布包

在上传前本地测试发布包：

```bash
# 解压到临时目录
mkdir /tmp/test_release
cd /tmp/test_release
tar xzf ~/Projects/CProjects/OneLife/build/windows/OneLife_v39.tar.gz

# 运行客户端
cd OneLife_v39
./OneLife.exe  # Windows
# 或
./OneLifeApp  # Linux

# 运行服务器（仅 Windows）
./runServer.bat
```

### 生成校验和

为发布包生成 SHA256 校验和：

```bash
cd build/windows
sha256sum OneLife_v39.tar.gz > OneLife_v39.tar.gz.sha256

cd ../mac
sha256sum OneLife_v39_IntelMacOSX.tar.gz > OneLife_v39_IntelMacOSX.tar.gz.sha256
```

### 自动化发布流程

使用 CI/CD 自动化发布流程（示例）：

```yaml
# .github/workflows/release.yml
name: Build Release
on:
  push:
    tags:
      - 'v*'

jobs:
  build-linux:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - name: Build Linux
        run: |
          cd build/source
          ./makeLinuxBuild ${GITHUB_REF#refs/tags/}
      - name: Upload artifact
        uses: actions/upload-artifact@v2
        with:
          name: linux-release
          path: build/source/OneLife_*.tar.gz
```

## 与 dist/ 的关系

项目中还有一个 `dist/` 目录，用于快速部署测试：

| 特性 | dist/ | build/ |
|------|-------|--------|
| 用途 | 快速部署测试 | 官方发布 |
| 平台 | Linux | Linux/macOS/Windows/Steam |
| 增量更新 | ❌ | ✅ |
| Steam 集成 | ❌ | ✅ |
| 自动化程度 | 手动 | 全自动（git pull → 编译 → 打包 → 上传） |
| 目标用户 | 私服管理员 | 官方玩家 |
| 部署包结构 | 运行时隔离（bin/data/runtime/） | 传统结构（所有文件混合） |

**建议：**
- 开发和测试使用 `dist/` 的简化部署包
- 官方发布使用 `build/` 的完整构建系统

## 参考资料

- 项目根目录 `CLAUDE.md` — 项目构建和运行指南
- `server/protocol.txt` — 网络协议文档
- `documentation/Readme.txt` — 官方 README
- `no_copyright.txt` — 公有领域声明
