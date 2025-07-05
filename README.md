# MySongPlayer

![Qt Version](https://img.shields.io/badge/Qt-6.8%20LTS-green.svg)
![C++ Standard](https://img.shields.io/badge/C++-23-blue.svg)
![Platform](https://img.shields.io/badge/Platform-Linux%20|%20Windows%20|%20macOS-lightgray.svg)
![Build System](https://img.shields.io/badge/Build-CMake-orange.svg)

一个基于 Qt 6.8 LTS 的现代音乐播放器，采用 C++23 标准和 MVC + Coordinator 架构模式开发。MySongPlayer 提供了完整的音频播放功能、播放列表管理、歌词同步显示等特性，并支持从本地文件或通过 Jamendo API 导入网络歌曲。

##  技术栈

| 技术领域 | 技术选择 | 版本要求 |
|---|---|---|
| **界面框架** | Qt Quick/QML | 6.8 LTS |
| **编程语言** | C++ | C++23 |
| **音频处理** | Qt Multimedia | 6.8+ |
| **数据存储** | Qt SQL (SQLite) | 6.8+ |
| **元数据解析** | TagLib | 最新版本 |
| **构建系统** | CMake | 3.28+ |

##  核心功能

- **全功能音频播放**：支持多种格式、音量与进度控制、多种播放模式（顺序、循环、随机等）。
- **播放列表管理**：支持向当前播放列表添加、移除歌曲，并自动持久化保存。
- **多方式音频导入**：支持从本地文件、文件夹及网络 URL 导入音频。
- **模糊与精确搜索**：支持在全局曲库和当前歌单中快速搜索歌曲。
- **实时歌词同步**：自动加载并同步显示 LRC 格式的歌词。

##  快速开始

### 系统要求

- **操作系统**: Linux, Windows 10+, macOS 12+
- **编译器**: GCC 13+, Clang 16+, 或 MSVC 2022+
- **Qt 版本**: Qt 6.8 LTS 或更高版本
- **CMake**: 3.28 或更高版本
- **TagLib**: 最新版本

### 依赖安装

#### Manjaro/Arch Linux
```bash
# 安装 Qt 6.8 LTS 和开发工具
sudo pacman -S qt6-base qt6-multimedia qt6-declarative qt6-sql
sudo pacman -S cmake base-devel
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
brew install qt@6 cmake taglib
export PATH="/opt/homebrew/opt/qt@6/bin:$PATH"
```

### 构建和运行

#### 标准构建流程

```bash
# 克隆项目
git clone https://github.com/drychq/MySongPlayer.git
cd MySongPlayer

# 创建构建目录
mkdir build && cd build

# 配置 CMake
cmake .. -DCMAKE_BUILD_TYPE=Release

# 编译项目
cmake --build . --parallel

# 运行应用程序
./appMySongPlayer
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