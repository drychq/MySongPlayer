#!/bin/bash
set -e

# 构建和打包 MySongPlayer 为 AppImage
# 使用方法: ./scripts/build-linux.sh [构建目录]

# 获取脚本目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="${1:-$PROJECT_DIR/build-linux}"
INSTALL_DIR="$BUILD_DIR/AppDir"
APP_NAME="MySongPlayer"

# 改进版本解析 - 使用更兼容的方法
VERSION=$(grep -E "VERSION\s+[0-9]+\.[0-9]+\.[0-9]+" "$PROJECT_DIR/CMakeLists.txt" | sed -E 's/.*VERSION\s+([0-9]+\.[0-9]+\.[0-9]+).*/\1/' | head -1)
if [ -z "$VERSION" ]; then
    echo "警告: 无法从 CMakeLists.txt 解析版本，使用默认版本 1.0.0"
    VERSION="1.0.0"
fi

OUTPUT_DIR="$PROJECT_DIR/dist"

echo "========== 构建开始 =========="
echo "项目目录: $PROJECT_DIR"
echo "构建目录: $BUILD_DIR"
echo "安装目录: $INSTALL_DIR"
echo "应用版本: $VERSION"

# 依赖检查
echo "=== 检查依赖 ==="
MISSING_DEPS=()
command -v cmake >/dev/null 2>&1 || MISSING_DEPS+=("cmake")
command -v ninja >/dev/null 2>&1 || MISSING_DEPS+=("ninja-build")
command -v pkg-config >/dev/null 2>&1 || MISSING_DEPS+=("pkg-config")
command -v wget >/dev/null 2>&1 || MISSING_DEPS+=("wget")

if [ ${#MISSING_DEPS[@]} -ne 0 ]; then
    echo "缺少以下依赖: ${MISSING_DEPS[*]}，尝试自动安装..."
    if command -v apt-get >/dev/null 2>&1; then
        sudo apt-get update
        sudo apt-get install -y "${MISSING_DEPS[@]}"
    else
        echo "请手动安装缺失依赖: ${MISSING_DEPS[*]}"
        exit 1
    fi
fi

# 确保输出目录存在
mkdir -p "$OUTPUT_DIR"

# 创建构建目录
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# 运行 CMake 构建
echo "=== 运行 CMake ==="
cmake -G "Ninja" \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX=/usr \
      "$PROJECT_DIR"

# 构建项目
echo "=== 运行 Ninja ==="
ninja

# 准备 AppImage 目录
echo "=== 准备 AppImage 目录 ==="
DESTDIR="$INSTALL_DIR" ninja install

# 创建 desktop 文件
echo "=== 创建桌面入口文件 ==="
mkdir -p "$INSTALL_DIR/usr/share/applications"
cat > "$INSTALL_DIR/usr/share/applications/$APP_NAME.desktop" << EOF
[Desktop Entry]
Type=Application
Name=$APP_NAME
Exec=$APP_NAME
Icon=$APP_NAME
Categories=Audio;AudioVideo;
EOF

# 复制图标
echo "=== 复制图标 ==="
mkdir -p "$INSTALL_DIR/usr/share/icons/hicolor/256x256/apps"
cp "$PROJECT_DIR/assets/icons/app_icon.png" "$INSTALL_DIR/usr/share/icons/hicolor/256x256/apps/$APP_NAME.png"

# 下载并运行 linuxdeployqt
echo "=== 下载 linuxdeployqt ==="
if [ ! -f "$BUILD_DIR/linuxdeployqt-continuous-x86_64.AppImage" ]; then
    wget -c "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage" -O "$BUILD_DIR/linuxdeployqt-continuous-x86_64.AppImage"
    chmod +x "$BUILD_DIR/linuxdeployqt-continuous-x86_64.AppImage"
fi

echo "=== 运行 linuxdeployqt ==="
# 查找 qmake 工具
QMAKE_CMD=""
if command -v qmake6 >/dev/null 2>&1; then
    QMAKE_CMD="qmake6"
elif command -v qmake >/dev/null 2>&1; then
    QMAKE_CMD="qmake"
else
    echo "错误: 找不到 qmake 工具"
    exit 1
fi

export QMAKE="$QMAKE_CMD"
export VERSION="$VERSION"
"$BUILD_DIR/linuxdeployqt-continuous-x86_64.AppImage" "$INSTALL_DIR/usr/share/applications/$APP_NAME.desktop" \
    -appimage \
    -bundle-non-qt-libs \
    -extra-plugins=iconengines,imageformats,platformthemes \
    -verbose=2

# 移动 AppImage 到输出目录 - 修复查找路径
echo "=== 完成打包 ==="
# 修复: 在构建目录中查找生成的 AppImage 文件
APP_IMAGE_FILE=$(find "$BUILD_DIR" -name "*-x86_64.AppImage" -type f | head -n 1)
if [ -z "$APP_IMAGE_FILE" ]; then
    echo "错误：找不到生成的 AppImage 文件"
    echo "在以下位置查找："
    find "$BUILD_DIR" -name "*.AppImage" -type f || echo "未找到任何 AppImage 文件"
    exit 1
fi
echo "找到 AppImage 文件: $APP_IMAGE_FILE"
cp "$APP_IMAGE_FILE" "$OUTPUT_DIR/$APP_NAME-$VERSION-x86_64.AppImage"

echo "========== 构建完成 =========="
echo "AppImage 文件位于: $OUTPUT_DIR/$APP_NAME-$VERSION-x86_64.AppImage" 