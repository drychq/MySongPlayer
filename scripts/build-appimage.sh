#!/usr/bin/env bash

# Build a self-contained Linux x86_64 AppImage. This script deliberately owns
# only build/package-linux and its own AppDir; it never removes development
# builds or other release artifacts.
set -Eeuo pipefail
IFS=$'\n\t'

readonly PROJECT_NAME="appMySongPlayer"
readonly APP_NAME="MySongPlayer"
readonly PACKAGE_PRESET="package-linux"
SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
readonly SCRIPT_DIR
PROJECT_ROOT="$(cd -- "${SCRIPT_DIR}/.." && pwd -P)"
readonly PROJECT_ROOT
readonly BUILD_DIR="${PROJECT_ROOT}/build/package-linux"
readonly RELEASE_DIR="${PROJECT_ROOT}/dist/release"
readonly APPDIR_PATH="${RELEASE_DIR}/${APP_NAME}.AppDir"
readonly FILTERED_QT_PLUGIN_DIR="${BUILD_DIR}/packaging/qt-plugins"
readonly QMAKE_WRAPPER_PATH="${BUILD_DIR}/packaging/qmake-filtered"
readonly ICON_FILE="${PROJECT_ROOT}/assets/icons/app_icon.png"
readonly QML_SOURCES_DIR="${PROJECT_ROOT}/qml"
readonly TOOLS_DIR="${PROJECT_ROOT}/tools"

readonly LINUXDEPLOY_PATH="${TOOLS_DIR}/linuxdeploy-x86_64.AppImage"
readonly QT_PLUGIN_PATH="${TOOLS_DIR}/linuxdeploy-plugin-qt-x86_64.AppImage"
readonly APPIMAGE_PLUGIN_PATH="${TOOLS_DIR}/linuxdeploy-plugin-appimage-x86_64.AppImage"
readonly APPIMAGE_RUNTIME_PATH="${TOOLS_DIR}/appimage-runtime-x86_64"
readonly LINUXDEPLOY_URL="https://github.com/linuxdeploy/linuxdeploy/releases/download/1-alpha-20251107-1/linuxdeploy-x86_64.AppImage"
readonly QT_PLUGIN_SNAPSHOT_URL="https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage"
readonly APPIMAGE_PLUGIN_SNAPSHOT_URL="https://github.com/linuxdeploy/linuxdeploy-plugin-appimage/releases/download/continuous/linuxdeploy-plugin-appimage-x86_64.AppImage"
readonly APPIMAGE_RUNTIME_URL="https://github.com/AppImage/type2-runtime/releases/download/20251108/runtime-x86_64"
readonly LINUXDEPLOY_SHA256="c20cd71e3a4e3b80c3483cef793cda3f4e990aca14014d23c544ca3ce1270b4d"
readonly QT_PLUGIN_SHA256="cfc1055b2b9dbc08412b579f20990b7b41a17b61beaa5847dc9477c96c9e9617"
readonly APPIMAGE_PLUGIN_SHA256="a45d3e227bc7f397e9cf6bfa4c9507494efa2293357b6e86690a3de2ca992e79"
readonly APPIMAGE_RUNTIME_SHA256="2fca8b443c92510f1483a883f60061ad09b46b978b2631c807cd873a47ec260d"

die() {
    echo "error: $*" >&2
    exit 1
}

require_command() {
    command -v "$1" >/dev/null 2>&1 || die "required command not found: $1"
}

safe_remove() {
    local path="$1"
    case "${path}" in
        "${BUILD_DIR}"|"${APPDIR_PATH}")
            rm -rf -- "${path}"
            ;;
        *)
            die "refusing to remove unexpected path: ${path}"
            ;;
    esac
}

verify_sha256() {
    local path="$1"
    local expected="$2"
    printf '%s  %s\n' "${expected}" "${path}" | sha256sum --check --status
}

download_tool() {
    local url="$1"
    local destination="$2"
    local expected_sha256="$3"
    local temporary

    if [[ -f "${destination}" ]] && verify_sha256 "${destination}" "${expected_sha256}"; then
        chmod 0755 "${destination}"
        return
    fi

    temporary="$(mktemp "${TOOLS_DIR}/.$(basename "${destination}").XXXXXX")"
    trap 'rm -f -- "${temporary}"' RETURN
    curl --fail --location --retry 3 --silent --show-error --output "${temporary}" "${url}"
    verify_sha256 "${temporary}" "${expected_sha256}" \
        || die "SHA-256 mismatch for ${url}"
    chmod 0755 "${temporary}"
    mv -f -- "${temporary}" "${destination}"
    trap - RETURN
}

find_single_file() {
    local description="$1"
    local pattern="$2"
    local result
    result="$(find "${QT_PLUGIN_DIR}" -type f -name "${pattern}" -print -quit)"
    [[ -n "${result}" ]] || die "required ${description} was not found under ${QT_PLUGIN_DIR}"
    printf '%s\n' "${result}"
}

require_packaged_path() {
    local description="$1"
    local pattern="$2"
    find "${APPDIR_PATH}" -type f -path "${pattern}" -print -quit | grep -q . \
        || die "AppDir is missing ${description} (${pattern})"
}

require_qml_import() {
    local module_path="$1"
    find "${APPDIR_PATH}" -type f -path "*/qml/${module_path}/qmldir" -print -quit | grep -q . \
        || die "AppDir is missing QML import ${module_path}"
}

verify_elf_dependencies() {
    local elf_path="$1"
    local dependencies
    dependencies="$(ldd "${elf_path}")"
    if grep -q 'not found' <<< "${dependencies}"; then
        printf '%s\n' "${dependencies}" >&2
        die "unresolved shared-library dependency in ${elf_path}"
    fi
}

require_command cmake
require_command curl
require_command file
require_command find
require_command grep
require_command ldd
require_command readelf
require_command sha256sum
require_command uname

[[ "$(uname -s)" == "Linux" ]] || die "AppImage packaging is supported only on Linux"
[[ "$(uname -m)" == "x86_64" ]] || die "AppImage packaging requires an x86_64 host"

[[ -d "${QML_SOURCES_DIR}" ]] || die "QML source directory not found: ${QML_SOURCES_DIR}"
[[ -f "${ICON_FILE}" ]] || die "application icon not found: ${ICON_FILE}"

mkdir -p "${TOOLS_DIR}" "${RELEASE_DIR}"
download_tool "${LINUXDEPLOY_URL}" "${LINUXDEPLOY_PATH}" "${LINUXDEPLOY_SHA256}"
download_tool "${QT_PLUGIN_SNAPSHOT_URL}" "${QT_PLUGIN_PATH}" "${QT_PLUGIN_SHA256}"
download_tool "${APPIMAGE_PLUGIN_SNAPSHOT_URL}" "${APPIMAGE_PLUGIN_PATH}" "${APPIMAGE_PLUGIN_SHA256}"
download_tool "${APPIMAGE_RUNTIME_URL}" "${APPIMAGE_RUNTIME_PATH}" "${APPIMAGE_RUNTIME_SHA256}"

safe_remove "${BUILD_DIR}"
cmake --preset "${PACKAGE_PRESET}"
cmake --build --preset "${PACKAGE_PRESET}" --parallel

readonly CACHE_FILE="${BUILD_DIR}/CMakeCache.txt"
[[ -f "${CACHE_FILE}" ]] || die "CMake cache was not generated: ${CACHE_FILE}"
CMAKE_PROJECT_VERSION="$(awk -F= '$1 ~ /^CMAKE_PROJECT_VERSION(:[^=]+)?$/ { print $2; exit }' "${CACHE_FILE}")"
[[ -n "${CMAKE_PROJECT_VERSION}" ]] || die "CMAKE_PROJECT_VERSION is absent from ${CACHE_FILE}"
[[ "${CMAKE_PROJECT_VERSION}" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]] \
    || die "CMAKE_PROJECT_VERSION must use X.Y.Z format: ${CMAKE_PROJECT_VERSION}"
if [[ -n "${APP_VERSION:-}" && "${APP_VERSION}" != "${CMAKE_PROJECT_VERSION}" ]]; then
    die "APP_VERSION (${APP_VERSION}) must exactly match CMAKE_PROJECT_VERSION (${CMAKE_PROJECT_VERSION})"
fi
readonly APP_VERSION="${CMAKE_PROJECT_VERSION}"
readonly BINARY_PATH="${BUILD_DIR}/${PROJECT_NAME}"
readonly OUTPUT_FILE_NAME="${APP_NAME}-${APP_VERSION}-linux-x86_64.AppImage"
readonly OUTPUT_PATH="${RELEASE_DIR}/${OUTPUT_FILE_NAME}"
readonly CHECKSUM_PATH="${OUTPUT_PATH}.sha256"

[[ -f "${BINARY_PATH}" ]] || die "release binary not found: ${BINARY_PATH}"
file --brief "${BINARY_PATH}" | grep -q 'ELF' || die "release binary is not an ELF executable: ${BINARY_PATH}"
readelf --file-header "${BINARY_PATH}" | grep -q 'ELF Header' \
    || die "release binary failed ELF header validation: ${BINARY_PATH}"

if command -v qmake6 >/dev/null 2>&1; then
    QMAKE_PATH="$(command -v qmake6)"
elif command -v qmake >/dev/null 2>&1; then
    QMAKE_PATH="$(command -v qmake)"
else
    die "required command not found: qmake6 or qmake"
fi
readonly QMAKE_PATH
QT_PLUGIN_DIR="$("${QMAKE_PATH}" -query QT_INSTALL_PLUGINS)"
readonly QT_PLUGIN_DIR
[[ -d "${QT_PLUGIN_DIR}" ]] || die "Qt plugin directory not found: ${QT_PLUGIN_DIR}"

QXCB_PLUGIN="$(find_single_file 'XCB platform plugin' 'libqxcb.so')"
readonly QXCB_PLUGIN
QSVG_PLUGIN="$(find_single_file 'SVG image plugin' 'libqsvg.so')"
readonly QSVG_PLUGIN
QSQLITE_PLUGIN="$(find_single_file 'SQLite SQL plugin' 'libqsqlite.so')"
readonly QSQLITE_PLUGIN
FFMPEG_PLUGIN="$(find_single_file 'FFmpeg multimedia plugin' 'libffmpegmediaplugin.so*')"
readonly FFMPEG_PLUGIN
mapfile -t WAYLAND_PLUGINS < <(find "${QT_PLUGIN_DIR}/platforms" -maxdepth 1 -type f -name 'libqwayland*.so' -print | sort)
(( ${#WAYLAND_PLUGINS[@]} > 0 )) || die "required Wayland platform plugins were not found"
WAYLAND_PLUGIN_NAMES=""
for wayland_plugin in "${WAYLAND_PLUGINS[@]}"; do
    WAYLAND_PLUGIN_NAMES+="$(basename "${wayland_plugin}");"
done
readonly WAYLAND_PLUGIN_NAMES

# linuxdeploy-plugin-qt deploys every driver in QT_INSTALL_PLUGINS/sqldrivers.
# Present it with a read-only view that keeps all other plugin categories but
# exposes only the SQLite driver this application uses. This avoids pulling in
# MySQL/PostgreSQL client libraries for unused Qt SQL backends.
[[ ! -e "${FILTERED_QT_PLUGIN_DIR}" ]] \
    || die "filtered Qt plugin directory already exists: ${FILTERED_QT_PLUGIN_DIR}"
mkdir -p "${FILTERED_QT_PLUGIN_DIR}/sqldrivers"
while IFS= read -r qt_plugin_category; do
    ln -s -- "${qt_plugin_category}" \
        "${FILTERED_QT_PLUGIN_DIR}/$(basename "${qt_plugin_category}")"
done < <(
    find "${QT_PLUGIN_DIR}" -mindepth 1 -maxdepth 1 -type d \
        ! -name sqldrivers -print | sort
)
cp -- "${QSQLITE_PLUGIN}" \
    "${FILTERED_QT_PLUGIN_DIR}/sqldrivers/$(basename "${QSQLITE_PLUGIN}")"

cat > "${QMAKE_WRAPPER_PATH}" <<'EOF'
#!/usr/bin/env bash
set -Eeuo pipefail

if [[ "$#" -eq 1 && "$1" == "-query" ]]; then
    while IFS= read -r query_line; do
        case "${query_line}" in
            QT_INSTALL_PLUGINS:*)
                printf 'QT_INSTALL_PLUGINS:%s\n' "${FILTERED_QT_PLUGIN_DIR}"
                ;;
            *)
                printf '%s\n' "${query_line}"
                ;;
        esac
    done < <("${REAL_QMAKE}" -query)
else
    exec "${REAL_QMAKE}" "$@"
fi
EOF
chmod 0755 "${QMAKE_WRAPPER_PATH}"

safe_remove "${APPDIR_PATH}"
rm -f -- "${OUTPUT_PATH}" "${CHECKSUM_PATH}"
mkdir -p "${APPDIR_PATH}"
cp "${ICON_FILE}" "${APPDIR_PATH}/${APP_NAME}.png"
cat > "${APPDIR_PATH}/${APP_NAME}.desktop" <<EOF
[Desktop Entry]
Type=Application
Name=${APP_NAME}
Exec=${PROJECT_NAME}
Icon=${APP_NAME}
Categories=AudioVideo;Audio;
Terminal=false
EOF

library_arguments=(
    --library "${QXCB_PLUGIN}"
    --library "${QSVG_PLUGIN}"
    --library "${QSQLITE_PLUGIN}"
    --library "${FFMPEG_PLUGIN}"
)
for wayland_plugin in "${WAYLAND_PLUGINS[@]}"; do
    library_arguments+=(--library "${wayland_plugin}")
done

(
    cd -- "${RELEASE_DIR}"
    export APPIMAGE_EXTRACT_AND_RUN=1
    export NO_STRIP=1
    export LINUXDEPLOY_PLUGINS_PATH="${TOOLS_DIR}"
    export REAL_QMAKE="${QMAKE_PATH}"
    export FILTERED_QT_PLUGIN_DIR
    export QMAKE="${QMAKE_WRAPPER_PATH}"
    export QML_SOURCES_PATHS="${QML_SOURCES_DIR}"
    export EXTRA_QT_MODULES="svg;"
    export EXTRA_PLATFORM_PLUGINS="${WAYLAND_PLUGIN_NAMES}"
    export LINUXDEPLOY_OUTPUT_VERSION="${APP_VERSION}"
    export LDAI_OUTPUT="${OUTPUT_FILE_NAME}"
    export LDAI_RUNTIME_FILE="${APPIMAGE_RUNTIME_PATH}"
    "${LINUXDEPLOY_PATH}" \
        --appdir "${APPDIR_PATH}" \
        --executable "${BINARY_PATH}" \
        --desktop-file "${APPDIR_PATH}/${APP_NAME}.desktop" \
        --icon-file "${APPDIR_PATH}/${APP_NAME}.png" \
        "${library_arguments[@]}" \
        --plugin qt \
        --output appimage
)

[[ -f "${OUTPUT_PATH}" ]] || die "linuxdeploy did not create the expected artifact: ${OUTPUT_PATH}"
[[ -x "${OUTPUT_PATH}" ]] || die "generated AppImage is not executable: ${OUTPUT_PATH}"
[[ -x "${APPDIR_PATH}/AppRun" ]] || die "AppDir AppRun is missing or not executable"
readonly PACKAGED_BINARY="${APPDIR_PATH}/usr/bin/${PROJECT_NAME}"
[[ -x "${PACKAGED_BINARY}" ]] || die "AppDir executable is missing: ${PACKAGED_BINARY}"
file --brief "${PACKAGED_BINARY}" | grep -q 'ELF' \
    || die "AppDir executable was replaced or is not ELF: ${PACKAGED_BINARY}"
require_packaged_path 'XCB platform plugin' '*/plugins/platforms/libqxcb.so'
require_packaged_path 'Wayland platform plugin' '*/plugins/platforms/libqwayland*.so'
require_packaged_path 'SQLite SQL plugin' '*/plugins/sqldrivers/libqsqlite.so'
require_packaged_path 'FFmpeg multimedia plugin' '*/plugins/multimedia/libffmpegmediaplugin.so*'
require_packaged_path 'SVG image plugin' '*/plugins/imageformats/libqsvg.so'
for qml_import in QtQuick QtQuick/Controls QtQuick/Dialogs QtQuick/Layouts QtCore QtMultimedia; do
    require_qml_import "${qml_import}"
done
verify_elf_dependencies "${PACKAGED_BINARY}"
critical_plugin_patterns=(
    '*/plugins/platforms/libqxcb.so'
    '*/plugins/platforms/libqwayland*.so'
    '*/plugins/sqldrivers/libqsqlite.so'
    '*/plugins/multimedia/libffmpegmediaplugin.so*'
    '*/plugins/imageformats/libqsvg.so'
)
for plugin_pattern in "${critical_plugin_patterns[@]}"; do
    while IFS= read -r plugin_path; do
        verify_elf_dependencies "${plugin_path}"
    done < <(find "${APPDIR_PATH}" -type f -path "${plugin_pattern}" -print)
done

(
    cd -- "${RELEASE_DIR}"
    sha256sum "${OUTPUT_FILE_NAME}" > "$(basename "${CHECKSUM_PATH}")"
    sha256sum --check --strict "$(basename "${CHECKSUM_PATH}")"
)

echo "AppImage created: ${OUTPUT_PATH}"
echo "SHA-256 manifest: ${CHECKSUM_PATH}"
