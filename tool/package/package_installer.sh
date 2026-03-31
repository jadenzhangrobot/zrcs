#!/bin/bash
# ============================================================================
# ZRCS GUI Installer Packager
# 自动收集依赖并生成 NSIS 安装包
# 用法:
#   PowerShell: D:\msys2\usr\bin\bash.exe tool/package/package_installer.sh
#   Git Bash / MSYS2: bash tool/package/package_installer.sh
# ============================================================================

set -e

# 确保基础 Unix 工具可用（从 PowerShell 启动 MSYS2 bash 时 /usr/bin 不在 PATH 中）
export PATH="/usr/bin:/bin:$PATH"

# ---------------------------------------------------------------------------
# 配置
# ---------------------------------------------------------------------------
ZRCS_VERSION="2.0.0"
ZRCS_NAME="ZRCS"
ZRCS_PUBLISHER="ZRCS Project"
EXE_NAME="zrcsgui.exe"

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
# 通过 git 定位项目根目录，回退到相对路径
if command -v git &>/dev/null && git rev-parse --show-toplevel &>/dev/null 2>&1; then
    PROJECT_ROOT="$(git rev-parse --show-toplevel)"
else
    PROJECT_ROOT="$(cd "$SCRIPT_DIR" && cd ../.. && pwd)"
fi
BUILD_BIN="$PROJECT_ROOT/build/bin"

# 自动检测 MSYS2 UCRT64 路径
detect_msys2() {
    local candidates=(
        "D:/msys2/ucrt64"
        "C:/msys2/ucrt64"
        "C:/tools/msys2/ucrt64"
    )
    # 如果已经在 MSYS2 环境中，优先使用
    if [[ -n "$MSYSTEM_PREFIX" && -d "$MSYSTEM_PREFIX/bin" ]]; then
        echo "$MSYSTEM_PREFIX"
        return 0
    fi
    for dir in "${candidates[@]}"; do
        if [[ -d "$dir/bin" ]]; then
            echo "$dir"
            return 0
        fi
    done
    return 1
}

MSYS2_PREFIX="$(detect_msys2)" || { echo "ERROR: Cannot find MSYS2 UCRT64 installation"; exit 1; }
MSYS2_BIN="$MSYS2_PREFIX/bin"

# 将 MSYS2 工具加入 PATH
MSYS2_ROOT="${MSYS2_PREFIX%/ucrt64}"
export PATH="/usr/bin:$MSYS2_ROOT/usr/bin:$MSYS2_BIN:$PATH"
# 验证基础工具可用
if ! command -v rm &>/dev/null; then
    # 尝试 Windows 风格路径
    MSYS2_WIN_ROOT="$(cygpath -u "D:\\msys2" 2>/dev/null || echo "/d/msys2")"
    export PATH="$MSYS2_WIN_ROOT/usr/bin:$PATH"
fi

STAGING_DIR="$PROJECT_ROOT/build/installer_staging"
OUTPUT_DIR="$PROJECT_ROOT/build"
NSIS_TEMPLATE="$SCRIPT_DIR/zrcsgui_installer.nsi.template"
NSIS_SCRIPT="$STAGING_DIR/installer.nsi"

# ---------------------------------------------------------------------------
# 日志
# ---------------------------------------------------------------------------
info()  { echo -e "\033[32m[INFO]\033[0m  $*"; }
warn()  { echo -e "\033[33m[WARN]\033[0m  $*"; }
error() { echo -e "\033[31m[ERROR]\033[0m $*"; exit 1; }

# ---------------------------------------------------------------------------
# 前置检查
# ---------------------------------------------------------------------------
check_prerequisites() {
    info "Checking prerequisites..."
    info "MSYS2 UCRT64: $MSYS2_PREFIX"

    if [[ ! -f "$BUILD_BIN/$EXE_NAME" ]]; then
        error "$EXE_NAME not found at $BUILD_BIN/$EXE_NAME. Build the project first."
    fi

    if ! command -v windeployqt6 &>/dev/null; then
        error "windeployqt6 not found. Install: pacman -S mingw-w64-ucrt-x86_64-qt6-base"
    fi

    # 查找 makensis
    MAKENSIS=""
    if command -v makensis &>/dev/null; then
        MAKENSIS="makensis"
    elif [[ -f "$MSYS2_BIN/makensis.exe" ]]; then
        MAKENSIS="$MSYS2_BIN/makensis.exe"
    elif [[ -f "C:/Program Files (x86)/NSIS/makensis.exe" ]]; then
        MAKENSIS="C:/Program Files (x86)/NSIS/makensis.exe"
    else
        error "makensis not found. Install: pacman -S mingw-w64-ucrt-x86_64-nsis"
    fi
    info "Using makensis: $MAKENSIS"

    # ldd
    if command -v ldd &>/dev/null; then
        LDD="ldd"
    elif [[ -f "$MSYS2_BIN/ldd.exe" ]]; then
        LDD="$MSYS2_BIN/ldd.exe"
    elif [[ -f "${MSYS2_PREFIX%/ucrt64}/usr/bin/ldd" ]]; then
        LDD="${MSYS2_PREFIX%/ucrt64}/usr/bin/ldd"
    else
        error "ldd not found in PATH or $MSYS2_BIN"
    fi

    info "All prerequisites OK"
}

# ---------------------------------------------------------------------------
# 递归收集 MSYS2 DLL 依赖
# ---------------------------------------------------------------------------
collect_dlls() {
    local staging="$1"
    info "Collecting DLL dependencies via ldd..."

    declare -A seen

    collect_for_binary() {
        local binary="$1"
        local dlls
        dlls=$("$LDD" "$binary" 2>/dev/null \
            | grep -iE '/ucrt64/|/mingw64/' \
            | awk '{print $3}' \
            | sort -u)

        for dll_path in $dlls; do
            local base
            base=$(basename "$dll_path")

            # 跳过已处理的
            if [[ -n "${seen[$base]}" ]]; then
                continue
            fi
            seen[$base]=1

            # 跳过 staging 中已有的 (windeployqt 已复制)
            if [[ -f "$staging/$base" ]]; then
                continue
            fi

            # 优先从 ucrt64 复制
            local src="$MSYS2_BIN/$base"
            if [[ ! -f "$src" ]]; then
                src="$dll_path"
            fi

            if [[ -f "$src" ]]; then
                cp "$src" "$staging/"
                # 递归扫描新复制的 DLL
                collect_for_binary "$staging/$base"
            else
                warn "DLL not found: $base"
            fi
        done
    }

    # 扫描主 exe
    collect_for_binary "$staging/$EXE_NAME"

    # 扫描 staging 中所有已有的 DLL (windeployqt 复制的)
    for dll in "$staging"/*.dll; do
        [[ -f "$dll" ]] && collect_for_binary "$dll"
    done

    local count
    count=$(find "$staging" -maxdepth 1 -name "*.dll" | wc -l)
    info "Total DLLs in staging: $count"
}

# ---------------------------------------------------------------------------
# 构建 staging 目录
# ---------------------------------------------------------------------------
stage_files() {
    info "Preparing staging directory..."
    rm -rf "$STAGING_DIR"
    mkdir -p "$STAGING_DIR"

    # 复制主程序
    info "Copying $EXE_NAME..."
    cp "$BUILD_BIN/$EXE_NAME" "$STAGING_DIR/"

    # windeployqt6 收集 Qt 依赖
    info "Running windeployqt6..."
    windeployqt6 --release --no-translations --dir "$STAGING_DIR" "$STAGING_DIR/$EXE_NAME" 2>&1 \
        | grep -v "Cannot open" || true

    # ldd 递归收集剩余 DLL
    collect_dlls "$STAGING_DIR"

    # 统计
    local size
    size=$(du -sh "$STAGING_DIR" | awk '{print $1}')
    info "Staging directory: $size"
}

# ---------------------------------------------------------------------------
# 生成 NSIS 脚本
# ---------------------------------------------------------------------------
generate_nsis() {
    info "Generating NSIS script..."

    if [[ ! -f "$NSIS_TEMPLATE" ]]; then
        error "NSIS template not found: $NSIS_TEMPLATE"
    fi

    local staging_win output_win
    staging_win=$(cygpath -w "$STAGING_DIR" | sed 's/\\/\\\\/g')
    output_win=$(cygpath -w "$OUTPUT_DIR" | sed 's/\\/\\\\/g')

    sed -e "s|@ZRCS_VERSION@|$ZRCS_VERSION|g" \
        -e "s|@ZRCS_NAME@|$ZRCS_NAME|g" \
        -e "s|@ZRCS_PUBLISHER@|$ZRCS_PUBLISHER|g" \
        -e "s|@STAGING_DIR@|${staging_win}|g" \
        -e "s|@OUTPUT_DIR@|${output_win}|g" \
        -e "s|@EXE_NAME@|$EXE_NAME|g" \
        "$NSIS_TEMPLATE" > "$NSIS_SCRIPT"

    info "NSIS script generated: $NSIS_SCRIPT"
}

# ---------------------------------------------------------------------------
# 编译安装包
# ---------------------------------------------------------------------------
build_installer() {
    info "Building installer with NSIS..."
    "$MAKENSIS" "$(cygpath -w "$NSIS_SCRIPT")"

    local installer="$OUTPUT_DIR/${ZRCS_NAME}-${ZRCS_VERSION}-Setup.exe"
    if [[ -f "$installer" ]]; then
        local size size_bytes
        size=$(du -h "$installer" | awk '{print $1}')
        size_bytes=$(stat -c%s "$installer" 2>/dev/null || stat -f%z "$installer" 2>/dev/null || echo 0)
        info "Installer created: $installer ($size, $size_bytes bytes)"
        if [[ "$size_bytes" -lt 1000000 ]]; then
            warn "Installer is suspiciously small - check if antivirus is interfering"
        fi
    else
        error "Installer build failed"
    fi
}

# ---------------------------------------------------------------------------
# 主流程
# ---------------------------------------------------------------------------
main() {
    info "===== ZRCS GUI Installer Packager ====="
    check_prerequisites
    stage_files
    generate_nsis
    build_installer
    info "===== Done ====="
}

main "$@"
