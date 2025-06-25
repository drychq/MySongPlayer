#!/bin/bash
set -e

# MySongPlayer macOS 构建脚本
# 使用方法: ./scripts/build-macos.sh [构建目录]

# 获取脚本目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="${1:-$PROJECT_DIR/build-macos}"
APP_NAME="MySongPlayer"
OUTPUT_DIR="$PROJECT_DIR/dist"

# 改进版本解析 - 使用更兼容的方法
VERSION=$(grep -E "VERSION\s+[0-9]+\.[0-9]+\.[0-9]+" "$PROJECT_DIR/CMakeLists.txt" | sed -E 's/.*VERSION\s+([0-9]+\.[0-9]+\.[0-9]+).*/\1/' | head -1)
if [ -z "$VERSION" ]; then
    echo "警告: 无法从 CMakeLists.txt 解析版本，使用默认版本 1.0.0"
    VERSION="1.0.0"
fi

echo "========== 构建开始 =========="
echo "项目目录: $PROJECT_DIR"
echo "构建目录: $BUILD_DIR"
echo "应用版本: $VERSION"

# 依赖检查
echo "=== 检查依赖 ==="
MISSING_DEPS=()
command -v cmake >/dev/null 2>&1 || MISSING_DEPS+=("cmake")
command -v make >/dev/null 2>&1 || MISSING_DEPS+=("make")
command -v ninja >/dev/null 2>&1 || MISSING_DEPS+=("ninja")
command -v pkg-config >/dev/null 2>&1 || MISSING_DEPS+=("pkg-config")
command -v hdiutil >/dev/null 2>&1 || MISSING_DEPS+=("hdiutil")

if [ ${#MISSING_DEPS[@]} -ne 0 ]; then
    echo "缺少以下依赖: ${MISSING_DEPS[*]}，尝试自动安装..."
    if command -v brew >/dev/null 2>&1; then
        brew update
        brew install ${MISSING_DEPS[*]}
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

# 运行 CMake 构建 - 指定 Ninja 生成器
echo "=== 运行 CMake ==="
cmake -G "Ninja" \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX="$BUILD_DIR/install" \
      "$PROJECT_DIR"

# 构建项目
echo "=== 运行 Ninja ==="
ninja -j$(sysctl -n hw.ncpu)

# 安装到临时目录
echo "=== 安装应用程序包 ==="
ninja install

# 找到构建的 .app 包
APP_BUNDLE=$(find "$BUILD_DIR" -name "*.app" -type d | head -n 1)

if [ -z "$APP_BUNDLE" ]; then
    echo "错误: 找不到生成的 .app 包"
    echo "在以下位置查找："
    find "$BUILD_DIR" -name "*.app" -type d || echo "未找到任何 .app 包"
    exit 1
fi

echo "=== 找到应用程序包: $APP_BUNDLE ==="

# 运行 macdeployqt
echo "=== 运行 macdeployqt ==="

# 简化 macdeployqt 查找
MACDEPLOY_CMD=""

# 优先检查环境变量
if [ -n "$CMAKE_PREFIX_PATH" ] && [ -f "$CMAKE_PREFIX_PATH/bin/macdeployqt" ]; then
    MACDEPLOY_CMD="$CMAKE_PREFIX_PATH/bin/macdeployqt"
elif [ -n "$Qt6_DIR" ] && [ -f "${Qt6_DIR}/../../../bin/macdeployqt" ]; then
    MACDEPLOY_CMD="${Qt6_DIR}/../../../bin/macdeployqt"
# 然后检查 PATH 中的工具
elif command -v macdeployqt >/dev/null 2>&1; then
    MACDEPLOY_CMD="macdeployqt"
elif command -v macdeployqt6 >/dev/null 2>&1; then
    MACDEPLOY_CMD="macdeployqt6"
else
    # 最后尝试通过 qmake 路径推断
    QMAKE_PATH=$(command -v qmake6 2>/dev/null || command -v qmake 2>/dev/null)
    if [ -n "$QMAKE_PATH" ]; then
        QT_BIN_DIR=$(dirname "$QMAKE_PATH")
        if [ -f "$QT_BIN_DIR/macdeployqt" ]; then
            MACDEPLOY_CMD="$QT_BIN_DIR/macdeployqt"
        fi
    fi
fi

if [ -z "$MACDEPLOY_CMD" ]; then
    echo "错误: 找不到 macdeployqt 工具"
    echo "请确保 Qt 安装正确并且 macdeployqt 在 PATH 中"
    exit 1
fi

echo "使用 macdeployqt: $MACDEPLOY_CMD"

# 运行 macdeployqt 创建自包含的 .app 包
"$MACDEPLOY_CMD" "$APP_BUNDLE" -verbose=2

# 创建 DMG 文件
echo "=== 创建 DMG 文件 ==="
DMG_NAME="${APP_NAME}-${VERSION}.dmg"
DMG_PATH="$OUTPUT_DIR/$DMG_NAME"
TMP_DMG_PATH="$BUILD_DIR/tmp.dmg"

# 清理可能存在的旧文件
rm -f "$TMP_DMG_PATH" "$DMG_PATH"

# 创建临时DMG
hdiutil create -volname "$APP_NAME" \
               -srcfolder "$APP_BUNDLE" \
               -ov -format UDRW \
               "$TMP_DMG_PATH"

# 转换为压缩的只读DMG
hdiutil convert "$TMP_DMG_PATH" \
                -format UDZO \
                -o "$DMG_PATH"

# 清理临时DMG
rm -f "$TMP_DMG_PATH"

echo "========== 构建完成 =========="
echo "DMG 文件位于: $DMG_PATH"