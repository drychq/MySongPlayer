#!/bin/bash


################################################################################
# 用户配置区域 - 请确认这些变量
################################################################################
# 1. 您的 CMake 项目生成的可执行文件的名称
readonly PROJECT_NAME="appMySongPlayer"

# 2. 您的应用程序的显示名称
readonly APP_NAME="MySongPlayer"

# 3. 位于项目根目录的图标文件名
readonly ICON_FILE_RELATIVE="assets/icons/app_icon.png"
readonly ICON_ICO_FILE_RELATIVE="assets/icons/app_icon.ico"

# 4. 存放QML源文件(.qml)的目录，相对于项目根目录
readonly QML_SOURCES_DIR_RELATIVE="qml"

################################################################################
# 脚本核心逻辑 - 通常无需修改
################################################################################

# 获取脚本所在目录的绝对路径
SCRIPT_DIR=$(dirname "$(readlink -f "$0")")

# 定义项目根目录 (假设脚本位于 project_root/scripts/)
readonly PROJECT_ROOT=$(dirname "$SCRIPT_DIR")

echo "=== 开始构建和打包 AppImage ==="
echo "项目名称: ${PROJECT_NAME}"
echo "应用版本: ${APP_VERSION}"
echo "项目根目录: ${PROJECT_ROOT}"

# 定义工具和目录路径 (相对于项目根目录的绝对路径)
readonly TOOLS_DIR="${PROJECT_ROOT}/tools"
readonly LINUXDEPLOY_PATH="${TOOLS_DIR}/linuxdeploy-x86_64.AppImage"
readonly QT_PLUGIN_PATH="${TOOLS_DIR}/linuxdeploy-plugin-qt-x86_64.AppImage"
readonly APPIMAGETOOL_PATH="${TOOLS_DIR}/appimagetool-x86_64.AppImage"
readonly BUILD_DIR="${PROJECT_ROOT}/build"
readonly DIST_DIR="${PROJECT_ROOT}/dist"
readonly APPDIR_PATH="${DIST_DIR}/${PROJECT_NAME}.AppDir"
readonly ICON_FILE="${PROJECT_ROOT}/${ICON_FILE_RELATIVE}"
readonly ICON_ICO_FILE="${PROJECT_ROOT}/${ICON_ICO_FILE_RELATIVE}"
readonly QML_SOURCES_DIR="${PROJECT_ROOT}/${QML_SOURCES_DIR_RELATIVE}"


# 检查QML目录是否存在
if [ ! -d "$QML_SOURCES_DIR" ]; then
    echo "错误: QML源文件目录 '${QML_SOURCES_DIR}' 不存在！"
    echo "请修改脚本中的 'QML_SOURCES_DIR_RELATIVE' 变量为您存放QML文件的目录。"
    exit 1
fi

# --- 1. 准备工具 ---
echo ""
echo "--- 步骤 1: 检查并准备打包工具 ---"
mkdir -p "$TOOLS_DIR"


# 下载 linuxdeploy
if [ ! -f "$LINUXDEPLOY_PATH" ]; then
    echo "下载 linuxdeploy..."
    wget -c -q "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage" -O "$LINUXDEPLOY_PATH" || { echo "!! linuxdeploy 下载失败"; exit 1; }
else
    echo "linuxdeploy 已存在，跳过下载。"
fi

# 下载 linuxdeploy-plugin-qt
if [ ! -f "$QT_PLUGIN_PATH" ]; then
    echo "下载 linuxdeploy-plugin-qt..."
    wget -c -q "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage" -O "$QT_PLUGIN_PATH" || { echo "!! linuxdeploy-plugin-qt 下载失败"; exit 1; }
else
    echo "linuxdeploy-plugin-qt 已存在，跳过下载。"
fi

# 下载 appimagetool
if [ ! -f "$APPIMAGETOOL_PATH" ]; then
    echo "下载 appimagetool..."
    wget -c -q "https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage" -O "$APPIMAGETOOL_PATH" || { echo "!! appimagetool 下载失败"; exit 1; }
else
    echo "appimagetool 已存在，跳过下载。"
fi

chmod +x "${LINUXDEPLOY_PATH}" "${QT_PLUGIN_PATH}" "${APPIMAGETOOL_PATH}"
echo "工具准备就绪。"


# --- 2. 构建项目 ---
echo ""
echo "--- 步骤 2: 执行 Release 构建 ---"
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
(
    cd "$BUILD_DIR"
    # 添加了错误检查
    cmake -G Ninja -DCMAKE_BUILD_TYPE=Release "${PROJECT_ROOT}" && ninja
) || { echo "!! CMake 或 Ninja 构建失败，请检查编译错误。"; exit 1; }
echo "构建完成。"

# --- 3. 创建 AppDir ---
echo ""
echo "--- 步骤 3: 创建并填充 AppDir ---"
rm -rf "$DIST_DIR"
mkdir -p "$APPDIR_PATH/usr/bin"
mkdir -p "$APPDIR_PATH/usr/share/applications"
mkdir -p "$APPDIR_PATH/usr/share/icons/hicolor/256x256/apps"

cp "${BUILD_DIR}/${PROJECT_NAME}" "${APPDIR_PATH}/usr/bin/"

cat <<EOF > "${APPDIR_PATH}/${PROJECT_NAME}.desktop"
[Desktop Entry]
Name=${APP_NAME}
Exec=${PROJECT_NAME}
Icon=${PROJECT_NAME}
Type=Application
Categories=Utility;
EOF

cp "${APPDIR_PATH}/${PROJECT_NAME}.desktop" "${APPDIR_PATH}/usr/share/applications/"
cp "${ICON_FILE}" "${APPDIR_PATH}/usr/share/icons/hicolor/256x256/apps/${PROJECT_NAME}.png"
ln -s "usr/share/icons/hicolor/256x256/apps/${PROJECT_NAME}.png" "${APPDIR_PATH}/${PROJECT_NAME}.png"
mkdir -p "${APPDIR_PATH}/assets/icons/"
cp "${ICON_ICO_FILE}" "${APPDIR_PATH}/assets/icons/"

echo "AppDir 准备就绪。"


# --- 4. 运行 linuxdeploy 填充依赖并处理潜在错误 ---
echo ""
echo "--- 步骤 4: 运行 linuxdeploy 填充依赖 ---"

# 创建一个临时文件来存储linuxdeploy的错误输出
readonly LINUXDEPLOY_LOG=$(mktemp)

# 临时禁用 "出错立即退出" 以便我们捕获错误
set +e
(
    export LINUXDEPLOY_PLUGINS_PATH="${TOOLS_DIR}"
    export NO_STRIP=1
    export QMAKE=$(which qmake6)
    export QML_SOURCES_PATHS="${QML_SOURCES_DIR}"

    # 执行linuxdeploy，并将标准错误(2)重定向到日志文件
    "$LINUXDEPLOY_PATH" --appdir "${APPDIR_PATH}" --plugin qt 2> "$LINUXDEPLOY_LOG"
)
# 获取linuxdeploy的退出码
readonly LINUXDEPLOY_EXIT_CODE=$?
# 恢复默认行为（如果需要）
set -e

# 检查退出码，如果不为0，则表示打包出错
if [ ${LINUXDEPLOY_EXIT_CODE} -ne 0 ]; then
    echo ""
    echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
    echo "!!! 错误: linuxdeploy 执行失败 (退出码: ${LINUXDEPLOY_EXIT_CODE})"
    echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
    echo ""
    echo "--- linuxdeploy 原始错误输出 ---"
    cat "${LINUXDEPLOY_LOG}"
    echo "------------------------------------"
    echo ""
    echo "--- 自动诊断和修复建议 ---"
    echo "打包失败通常是因为您的构建环境中缺少某些Qt插件的底层依赖库。"
    echo "请根据下面的提示安装对应的开发包(dev/devel)，然后重新运行此脚本。"
    echo ""

    # 使用 grep 在错误日志中搜索关键词并提供建议
    if grep -q -i "jxr" "${LINUXDEPLOY_LOG}"; then
        echo "=> 检测到与 JPEG XR (.jxr) 格式相关的错误。"
        echo "   这通常是因为缺少 'libjxr' 开发库。"
        echo "   在 Debian/Ubuntu 上，请尝试安装: sudo apt-get install libjxr-dev"
        echo "   在 Fedora/CentOS 上，请尝试搜索: sudo dnf search jxrlib"
        echo ""
    fi

    if grep -q -i "firebird" "${LINUXDEPLOY_LOG}" || grep -q "libfbclient" "${LINUXDEPLOY_LOG}"; then
        echo "=> 检测到与 Firebird SQL 数据库驱动相关的错误。"
        echo "   这通常是因为缺少 Firebird 客户端开发库。"
        echo "   在 Debian/Ubuntu 上，请尝试安装: sudo apt-get install firebird-dev"
        echo "   在 Fedora/CentOS 上，请尝试安装: sudo dnf install firebird-devel"
        echo ""
    fi

    if grep -q -i "avif" "${LINUXDEPLOY_LOG}"; then
        echo "=> 检测到与 AVIF (.avif) 图像格式相关的错误。"
        echo "   这通常是因为缺少 'libavif' 开发库。"
        echo "   在 Debian/Ubuntu 上，请尝试安装: sudo apt-get install libavif-dev"
        echo "   在 Fedora/CentOS 上，请尝试安装: sudo dnf install libavif-devel"
        echo ""
    fi

    if grep -q -i "heif" "${LINUXDEPLOY_LOG}"; then
        echo "=> 检测到与 HEIF (.heif) 图像格式相关的错误。"
        echo "   这通常是因为缺少 'libheif' 开发库。"
        echo "   在 Debian/Ubuntu 上，请尝试安装: sudo apt-get install libheif-dev"
        echo "   在 Fedora/CentOS 上，请尝试安装: sudo dnf install libheif-devel"
        echo ""
    fi

    echo "如果以上建议没有解决问题，请仔细阅读上面的“原始错误输出”，"
    echo "错误信息通常会明确指出缺少哪个 '.so' 文件。"
    echo "您可以使用 'apt-file search <文件名>' (Ubuntu) 或 'dnf provides */<文件名>' (Fedora) 来找到包含该文件的软件包。"
    echo ""

    # 清理临时文件并退出
    rm -f "${LINUXDEPLOY_LOG}"
    exit 1
fi

# 如果执行成功
echo "依赖填充完成。"
rm -f "${LINUXDEPLOY_LOG}" # 清理临时文件


# --- 5. 手动清理不需要的插件 ---
echo ""
echo "--- 步骤 5: 清理不需要的插件 ---"
# 即使linuxdeploy成功，也执行清理，以防万一
rm -f ${APPDIR_PATH}/usr/plugins/imageformats/kimg_jxr.so
rm -f ${APPDIR_PATH}/usr/plugins/imageformats/kimg_avif.so
rm -f ${APPDIR_PATH}/usr/plugins/imageformats/kimg_heif.so
rm -f ${APPDIR_PATH}/usr/plugins/imageformats/kimg_kra.so
rm -f ${APPDIR_PATH}/usr/plugins/sqldrivers/libqsqlfb.so
echo "清理完成。"

# --- 5.5. 创建自定义AppRun启动脚本 ---
echo ""
echo "--- 步骤 5.5: 创建自定义AppRun以设置环境变量 ---"
cat <<EOF > "${APPDIR_PATH}/AppRun"
#!/bin/bash
HERE="\$(dirname "\$(readlink -f "\${0}")")"
export QT_QPA_PLATFORMTHEME=xdgdesktopportal
exec "\${HERE}/usr/bin/${PROJECT_NAME}" "\$@"
EOF
chmod +x "${APPDIR_PATH}/AppRun"
echo "自定义AppRun创建成功。"


# --- 6. 使用 appimagetool 创建最终文件 ---
echo ""
echo "--- 步骤 6: 创建最终的 AppImage 文件 ---"
(
    cd "${DIST_DIR}"
    export VERSION="${APP_VERSION}"
    # 添加错误检查
    "$APPIMAGETOOL_PATH" "${APPDIR_PATH}"
) || { echo "!! appimagetool 执行失败。"; exit 1; }

# --- 7. 完成 ---
echo ""
echo "=========================================="
echo " AppImage 打包成功! "
echo "=========================================="
echo "文件位于: ${DIST_DIR}"
ls -lh "${DIST_DIR}"/*.AppImage
echo ""
