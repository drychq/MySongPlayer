# MySongPlayer 

![Qt Version](https://img.shields.io/badge/Qt-6.8%20LTS-green.svg)
![C++ Standard](https://img.shields.io/badge/C++-23-blue.svg)
![Platform](https://img.shields.io/badge/Platform-Linux%20|%20Windows%20|%20macOS-lightgray.svg)
![Build System](https://img.shields.io/badge/Build-CMake-orange.svg)

一个基于 Qt 6.8 LTS 的现代音乐播放器，采用 C++23 标准和 MVC + Coordinator 架构模式开发。MySongPlayer 提供了完整的音频播放功能、简单播放列表管理、播放列表搜索、多种导入方式和歌词同步显示等特性。

##  技术栈

| 技术领域 | 技术选择 | 版本要求 |
|---------|---------|---------|
| **界面框架** | Qt Quick/QML | 6.8 LTS |
| **编程语言** | C++ | C++23 |
| **音频处理** | Qt Multimedia | 6.8+ |
| **数据存储** | Qt SQL (SQLite) | 6.8+ |
| **元数据解析** | TagLib | Latest |
| **构建系统** | CMake | 3.28+ |

##  核心功能

###  音频播放
- 支持多种音频格式 (MP3, WAV, OGG, FLAC, M4A)
- 音量控制和静音功能
- 播放进度控制和实时显示
- 多种播放模式：顺序播放、循环播放、随机播放、单曲循环

###  播放列表管理
- 去重检测
- 播放列表持久化存储

### 搜索功能
- 模糊搜索
- 支持标题、艺术家、专辑等多字段搜索
- 播放列表内搜索

###  音频导入
- 本地文件导入 (支持多选)
- 网络 URL 流媒体导入
- 自动元数据提取和封面获取
- 重复检测

###  歌词显示
- LRC 格式歌词文件支持
- 实时同步滚动显示
- 歌词与音频的精确时间匹配

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
sudo pacman -S qt6-base qt6-multimedia qt6-declarative
sudo pacman -S cmake base-devel
sudo pacman -S taglib

# 安装 pkg-config (用于 TagLib 检测)
sudo pacman -S pkgconf
```

#### Windows
```powershell
# 使用 vcpkg 安装依赖
vcpkg install qt6[core,multimedia,quick] taglib
```

#### macOS
```bash
# 使用 Homebrew 安装依赖
brew install qt@6 cmake taglib
export PATH="/opt/homebrew/opt/qt@6/bin:$PATH"
```

### 构建和运行

```bash
# 克隆项目
git clone https://github.com/your-username/SongPlayer1.git
cd SongPlayer1

# 创建构建目录
mkdir build && cd build

# 配置 CMake
cmake .. -DCMAKE_BUILD_TYPE=Release

# 编译项目
cmake --build . --parallel

# 运行应用程序
./appSongPlayer
```

##  项目结构

```
SongPlayer1/
├── assets/                    # 资源文件
│   └── icons/                # 应用图标
├── qml/                      # QML 用户界面
│   ├── components/           # UI 组件
│   │   ├── base/            # 基础组件
│   │   └── complex/         # 复杂组件
│   ├── layouts/             # 布局组件
│   ├── panels/              # 面板组件
│   ├── styles/              # 样式和主题
│   └── utils/               # 工具函数
├── src/                      # C++ 源代码
│   ├── controllers/         # 控制器层
│   ├── coordinators/        # 协调器
│   ├── interfaces/          # 接口定义
│   ├── models/              # 数据模型
│   ├── services/            # 业务服务
│   └── storage/             # 数据存储
├── uml/                      # UML 设计图
│   ├── architecture/        # 架构图
│   ├── class/               # 类图
│   ├── sequence/            # 时序图
│   └── use-case/            # 用例图
└── CMakeLists.txt           # CMake 构建文件
```

## 开发文档

### UML 设计图
- **[架构图](uml/architecture/)**: 整体系统架构设计
- **[类图](uml/class/)**: 详细的类关系图
- **[时序图](uml/sequence/)**: 关键流程的时序图
- **[用例图](uml/use-case/)**: 用户交互用例

## 自动构建与发行（CI/CD）

本项目已集成GitHub Actions自动化构建与发行，支持Linux（AppImage）、macOS（DMG）、Windows（EXE）三平台。

- 触发条件：推送tag（如v1.0.0）或手动触发
- 构建产物自动上传至GitHub Releases
- 产物包含全部运行所需资源
- Windows仅发布exe，无需zip

### 获取方式

1. 前往 [Releases 页面](https://github.com/你的仓库名/MySongPlayer/releases) 下载对应平台产物。
2. Linux用户下载AppImage，macOS用户下载DMG，Windows用户下载EXE。

### 本地构建

如需本地构建，可参考`scripts/`目录下的各平台脚本。



