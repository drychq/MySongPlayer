# MySongPlayer Windows 构建脚本
# 使用方法: .\scripts\build-windows.ps1 [构建目录]

param (
    [string]$BuildDir = ""
)

# 设置错误处理
$ErrorActionPreference = "Stop"

# 获取脚本和项目路径
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectDir = (Get-Item $ScriptDir).Parent.FullName

# 如果未提供构建目录，则使用默认值
if ([string]::IsNullOrEmpty($BuildDir)) {
    $BuildDir = Join-Path $ProjectDir "build-windows"
}

$AppName = "MySongPlayer"
$OutputDir = Join-Path $ProjectDir "dist"
$PackageDir = Join-Path $BuildDir "package"

# 改进版本解析
$CmakeVersion = ""
try {
    $CmakeVersion = (Select-String -Path (Join-Path $ProjectDir "CMakeLists.txt") -Pattern "VERSION\s+([0-9\.]+)").Matches.Groups[1].Value
} catch {
    Write-Host "警告: 无法从 CMakeLists.txt 解析版本，使用默认版本 1.0.0" -ForegroundColor Yellow
    $CmakeVersion = "1.0.0"
}

Write-Host "========== 构建开始 ==========" -ForegroundColor Green
Write-Host "项目目录: $ProjectDir"
Write-Host "构建目录: $BuildDir"
Write-Host "打包目录: $PackageDir"
Write-Host "应用版本: $CmakeVersion"

# 依赖检查
Write-Host "=== 检查依赖 ===" -ForegroundColor Cyan
$MissingDeps = @()

# 检查 CMake
if (-Not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    $MissingDeps += "cmake"
}

# 检查 Ninja
if (-Not (Get-Command ninja -ErrorAction SilentlyContinue)) {
    $MissingDeps += "ninja"
}

# 检查 tar
if (-Not (Get-Command tar -ErrorAction SilentlyContinue)) {
    $MissingDeps += "tar"
}

if ($MissingDeps.Count -gt 0) {
    Write-Host "缺少以下依赖: $($MissingDeps -join ', ')，尝试自动安装..." -ForegroundColor Yellow
    if (Get-Command winget -ErrorAction SilentlyContinue) {
        foreach ($dep in $MissingDeps) {
            winget install --id $dep -e --accept-source-agreements --accept-package-agreements
        }
    } elseif (Get-Command choco -ErrorAction SilentlyContinue) {
        choco install $($MissingDeps -join ' ') -y
    } else {
        Write-Host "请手动安装缺失依赖: $($MissingDeps -join ', ')" -ForegroundColor Red
        exit 1
    }
}

# 确保目录存在
New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null
New-Item -ItemType Directory -Force -Path $PackageDir | Out-Null

# 安装 Windows 特定依赖
Write-Host "=== 安装 Windows 依赖 ===" -ForegroundColor Cyan

# 安装 ZLIB 通过 vcpkg
$VcpkgRoot = "C:\vcpkg"
if (-Not (Test-Path $VcpkgRoot)) {
    Write-Host "安装 vcpkg..." -ForegroundColor Cyan
    git clone https://github.com/Microsoft/vcpkg.git $VcpkgRoot
    & "$VcpkgRoot\bootstrap-vcpkg.bat"
}

# 安装 zlib
if (-Not (Test-Path "$VcpkgRoot\installed\x64-windows\lib\zlib.lib")) {
    Write-Host "安装 zlib..." -ForegroundColor Cyan
    & "$VcpkgRoot\vcpkg.exe" install zlib:x64-windows
}

# 下载并编译 TagLib（2.1 版）
$TagLibVersion = "2.1"
$TagLibSourceUrl = "https://github.com/taglib/taglib/releases/download/v$TagLibVersion/taglib-$TagLibVersion.tar.gz"
$TagLibSourceDir = Join-Path $BuildDir "taglib-src"
$TagLibBuildDir = Join-Path $BuildDir "taglib-build"
$TagLibInstallDir = Join-Path $BuildDir "taglib-install"

if (-Not (Test-Path $TagLibInstallDir)) {
    Write-Host "=== 下载 TagLib $TagLibVersion ===" -ForegroundColor Cyan
    $TagLibTar = Join-Path $BuildDir "taglib-$TagLibVersion.tar.gz"
    
    # 添加下载重试机制
    $MaxRetries = 3
    $RetryCount = 0
    do {
        try {
            Invoke-WebRequest -Uri $TagLibSourceUrl -OutFile $TagLibTar
            break
        } catch {
            $RetryCount++
            if ($RetryCount -ge $MaxRetries) {
                throw "下载 TagLib 失败: $_"
            }
            Write-Host "下载失败，重试 $RetryCount/$MaxRetries" -ForegroundColor Yellow
            Start-Sleep -Seconds 2
        }
    } while ($RetryCount -lt $MaxRetries)
    
    Write-Host "=== 解压 TagLib ===" -ForegroundColor Cyan
    tar -xf $TagLibTar -C $BuildDir
    Rename-Item -Path (Join-Path $BuildDir "taglib-$TagLibVersion") -NewName "taglib-src"
    
    New-Item -ItemType Directory -Force -Path $TagLibBuildDir | Out-Null
    Push-Location $TagLibBuildDir
    Write-Host "=== 配置 TagLib ===" -ForegroundColor Cyan
    cmake -G "Ninja" `
          -DCMAKE_BUILD_TYPE=Release `
          -DCMAKE_INSTALL_PREFIX="$TagLibInstallDir" `
          -DCMAKE_TOOLCHAIN_FILE="$VcpkgRoot/scripts/buildsystems/vcpkg.cmake" `
          -DUTFCPP_INCLUDE_DIR="$Utf8CppPath" `
          $TagLibSourceDir
    Write-Host "=== 构建 TagLib ===" -ForegroundColor Cyan
    cmake --build . --config Release
    Write-Host "=== 安装 TagLib ===" -ForegroundColor Cyan
    cmake --install .
    Pop-Location
}

# 切换到构建目录
Push-Location $BuildDir

try {
    # 运行 CMake 配置
    Write-Host "=== 运行 CMake ===" -ForegroundColor Cyan
    cmake -G "Ninja" `
          -DCMAKE_BUILD_TYPE=Release `
          -DCMAKE_INSTALL_PREFIX="$PackageDir" `
          -DCMAKE_PREFIX_PATH="$TagLibInstallDir" `
          -DCMAKE_TOOLCHAIN_FILE="$VcpkgRoot/scripts/buildsystems/vcpkg.cmake" `
          $ProjectDir

    # 构建项目
    Write-Host "=== 构建项目 ===" -ForegroundColor Cyan
    cmake --build . --config Release

    # 安装到打包目录
    Write-Host "=== 安装到打包目录 ===" -ForegroundColor Cyan
    cmake --install .

    # 确定可执行文件路径 - 修正目标名称
    $ExePath = Join-Path $PackageDir "bin" "app$AppName.exe"
    if (-Not (Test-Path $ExePath)) {
        # 尝试其他可能的位置
        $ExePath = Join-Path $PackageDir "app$AppName.exe"
        if (-Not (Test-Path $ExePath)) {
            throw "找不到可执行文件: app$AppName.exe"
        }
    }

    # 运行 Qt 部署工具
    Write-Host "=== 运行 windeployqt ===" -ForegroundColor Cyan
    
    # 简化 Qt 路径查找
    $QtBinPath = ""
    
    # 首先检查环境变量
    if ($env:CMAKE_PREFIX_PATH) {
        $QtBinPath = Join-Path $env:CMAKE_PREFIX_PATH "bin"
        if (-Not (Test-Path (Join-Path $QtBinPath "windeployqt.exe"))) {
            $QtBinPath = ""
        }
    }
    
    # 如果还没找到，尝试其他环境变量
    if ([string]::IsNullOrEmpty($QtBinPath)) {
        if ($env:Qt6_DIR) {
            $QtBinPath = Join-Path (Split-Path -Parent $env:Qt6_DIR) "bin"
        } elseif ($env:QTDIR) {
            $QtBinPath = Join-Path $env:QTDIR "bin"
        }
    }
    
    # 最后尝试 PATH 中的 qmake
    if ([string]::IsNullOrEmpty($QtBinPath) -or (-Not (Test-Path (Join-Path $QtBinPath "windeployqt.exe")))) {
        $QMakePath = (Get-Command -ErrorAction SilentlyContinue qmake6, qmake | Select-Object -First 1).Source
        if ($QMakePath) {
            $QtBinPath = Split-Path -Parent $QMakePath
        }
    }

    if ([string]::IsNullOrEmpty($QtBinPath) -or (-Not (Test-Path (Join-Path $QtBinPath "windeployqt.exe")))) {
        throw "无法找到 windeployqt.exe，请确保 Qt bin 目录在 PATH 中或设置了相关环境变量"
    }

    # 运行 windeployqt
    $WinDeployQtPath = Join-Path $QtBinPath "windeployqt.exe"
    & $WinDeployQtPath --release --verbose 2 --compiler-runtime $ExePath

    # 创建 ZIP 文件
    $VersionString = $CmakeVersion -replace '\.', '_'
    $ZipFileName = "$AppName-$VersionString-windows.zip"
    $ZipFilePath = Join-Path $OutputDir $ZipFileName

    Write-Host "=== 创建 ZIP 包 ===" -ForegroundColor Cyan
    Compress-Archive -Path "$PackageDir\*" -DestinationPath $ZipFilePath -Force

    Write-Host "========== 构建完成 ==========" -ForegroundColor Green
    Write-Host "压缩包位于: $ZipFilePath" -ForegroundColor Yellow
} catch {
    Write-Host "构建过程中发生错误: $_" -ForegroundColor Red
    exit 1
} finally {
    # 返回原始目录
    Pop-Location
}