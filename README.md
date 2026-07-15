# MySongPlayer

![Qt Version](https://img.shields.io/badge/Qt-6.11-green.svg)
![C++ Standard](https://img.shields.io/badge/C++-23-blue.svg)
![Platform](https://img.shields.io/badge/Platform-Linux%20|%20Windows%20|%20macOS-lightgray.svg)
![Build System](https://img.shields.io/badge/Build-CMake-orange.svg)

一个基于 C++23 与 Qt 6.11 的跨平台音乐播放器。业务核心使用标准库实现，Qt 被限制在 QML 界面、音频、网络、数据库和平台适配边界；这样既保留 Qt 的跨平台能力，也避免业务逻辑与 Qt 类型深度绑定。

##  技术栈

| 技术领域 | 技术选择 | 版本要求 |
|---|---|---|
| **界面框架** | Qt Quick/QML | 6.11+ |
| **编程语言** | C++ | C++23 |
| **音频处理** | Qt Multimedia | 6.11+ |
| **数据存储** | Qt SQL (SQLite) | 6.11+ |
| **元数据解析** | TagLib | 2.0+ |
| **构建系统** | CMake | 3.28+ |

##  核心功能

- **全功能音频播放**：支持多种格式、音量与进度控制、多种播放模式（顺序、循环、随机等）。
- **播放列表管理**：支持向当前播放列表添加、移除歌曲，并自动持久化保存。
- **非阻塞音频导入**：本地多文件元数据与封面在有界后台任务中处理，支持进度、取消和逐文件错误；同时保留网络 URL 导入。
- **模糊与精确搜索**：支持在全局曲库和当前歌单中快速搜索歌曲。
- **实时歌词同步**：自动加载并同步显示 LRC 格式的歌词。

## 架构边界

- `src/core`：可移植业务核心，仅允许 C++ 标准库，不链接 Qt。
- `src/adapters`：Qt 类型与核心类型之间的显式转换边界。
- `src/infrastructure`：TagLib 等第三方能力对核心端口的实现；不依赖 Qt。
- `src/models`、`src/controllers`、`src/coordinators`：面向 QML 的应用与展示层。
- `src/services`、`src/storage`：Qt Multimedia、Qt Network 和 Qt SQL 等平台能力适配。
- `qml`：只负责展示与用户交互，不承载业务规则。

CMake 会扫描全部核心文件并拒绝 Qt、TagLib 和外层模块 include；`MYSONGPLAYER_BUILD_UI=OFF` 可在不查找 Qt/TagLib 的情况下独立构建核心。详细规则见 [架构说明](./doc/ARCHITECTURE.md)。

##  快速开始

### 系统要求

- **操作系统**: Linux, Windows 10+, macOS 12+
- **编译器**: 支持项目所用 C++23 特性的 GCC、Clang 或 MSVC
- **Qt 版本**: Qt 6.11 或更高版本
- **CMake**: 3.28 或更高版本
- **构建器**: Ninja 1.11 或更高版本（项目 Presets 默认使用 Ninja）
- **TagLib**: 2.0 或更高版本

### 依赖安装

#### Manjaro/Arch Linux
```bash
# 安装发行版提供的 Qt 6 开发组件；版本不足 6.11 时请使用 Qt 官方安装器
sudo pacman -S qt6-base qt6-multimedia qt6-declarative qt6-sql
sudo pacman -S cmake ninja base-devel
sudo pacman -S taglib

# 安装 pkg-config (用于 TagLib 检测)
sudo pacman -S pkgconf
```

#### Windows
```powershell
# 使用 vcpkg 安装依赖
vcpkg install qt6[core,multimedia,quick,sql] taglib
```

#### macOS
```bash
# 使用 Homebrew 安装依赖
brew install qt@6 cmake ninja taglib
export PATH="/opt/homebrew/opt/qt@6/bin:$PATH"
```

### 构建和运行

#### 推荐构建流程

```bash
# 克隆项目
git clone https://github.com/drychq/MySongPlayer.git
cd MySongPlayer

# Qt 应用：Debug + C++ warnings-as-errors
cmake --preset dev
cmake --build --preset dev
ctest --preset dev

# 运行应用程序
./build/dev/appMySongPlayer
```

仅验证标准库核心（不会查找 Qt 或 TagLib）：

```bash
cmake --preset core-only
cmake --build --preset core-only
ctest --preset core-only
```

##  项目结构

```
MySongPlayer/
├── assets/                    # 资源文件
├── doc/                       # 开发文档
├── qml/                       # QML 用户界面
├── scripts/                   # 构建和打包脚本
├── src/                       # C++ 源代码
├── uml/                       # UML 设计图
└── CMakeLists.txt             # CMake 构建文件
```

## 打包

项目提供了 `AppImage` 打包脚本，方便在 Linux 系统中分发。

```bash
# 确保已完成标准构建流程

# 切换到 scripts 目录
cd ../scripts

# 运行打包脚本
bash build-appimage.sh
```

打包完成后，生成的 `MySongPlayer-x86_64.AppImage` 文件位于项目根目录下的 `dist` 目录中。

## 开发文档

关于项目的架构设计、模块功能和开发规范的详细信息，请参阅 [开发文档](./doc/DEVELOPMENT.md)。

## 参与贡献

项目采用单主线、短主题分支、Pull Request 和 Squash Merge 的轻量工作流。分支命名、提交规范、测试要求与发布约定见 [贡献指南](./CONTRIBUTING.md)。
