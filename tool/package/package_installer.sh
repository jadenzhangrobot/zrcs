#!/bin/bash
# ============================================================================
# ZRCS GUI Installer Packager
# 自动收集依赖并生成 NSIS 安装包
# 用法:
#   PowerShell: D:\msys2\usr\bin\bash.exe tool/package/package_installer.sh
#   Git Bash / MSYS2: bash tool/package/package_installer.sh
# ============================================================================

# 打包属于发布流程，任何一步失败都不能继续生成“看似成功”的安装包：
#   -E：让 ERR trap 在函数和子 Shell 中同样生效；
#   -e：普通命令失败时立即退出；
#   -u：使用未定义变量时立即退出，避免空路径参与复制或删除；
#   -o pipefail：管道中任意命令失败都视为整条管道失败。
set -Eeuo pipefail

# 确保基础 Unix 工具可用（从 PowerShell 启动 MSYS2 bash 时 /usr/bin 不在 PATH 中）
export PATH="/usr/bin:/bin:$PATH"

# ---------------------------------------------------------------------------
# 配置
# ---------------------------------------------------------------------------
ZRCS_VERSION="2.0.0"
ZRCS_VERSION_QUAD="2.0.0.0"   # 4-part version for Windows VIProductVersion
ZRCS_NAME="ZRCS"
ZRCS_PUBLISHER="ZRCS Project"
EXE_NAME="zrcsgui.exe"
# zrcsnrt/zrcsrt 是完整控制系统运行所必需的后端，缺少时禁止打包。
# motiongui 当前不是仓库中的构建目标，保留为可选工具；存在则打包，不存在则给出提示。
REQUIRED_EXES=("zrcsnrt.exe" "zrcsrt.exe")
OPTIONAL_EXES=("motiongui.exe")
ICON_FILE=""                   # set to .ico path if available, e.g. "resources/zrcs.ico"

# BASH_SOURCE[0] 始终指向当前脚本文件；$0 在脚本被 source 时会变成调用者路径，
# 导致 NSIS 模板和项目根目录定位错误。
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# 必须从脚本所在目录查询 Git 根目录。若直接使用当前工作目录，在另一个仓库中通过
# 绝对路径调用本脚本时，会错误地清理和打包另一个仓库的 build 目录。
if command -v git &>/dev/null && git -C "$SCRIPT_DIR" rev-parse --show-toplevel &>/dev/null 2>&1; then
    PROJECT_ROOT="$(git -C "$SCRIPT_DIR" rev-parse --show-toplevel)"
else
    PROJECT_ROOT="$(cd "$SCRIPT_DIR" && cd ../.. && pwd)"
fi
BUILD_BIN="$PROJECT_ROOT/build/bin"
BUILD_LIB="$PROJECT_ROOT/build/lib"
CMAKE_CACHE="$PROJECT_ROOT/build/CMakeCache.txt"

# 自动检测 MSYS2 UCRT64 路径
detect_msys2() {
    local candidates=(
        "D:/msys2/ucrt64"
        "C:/msys2/ucrt64"
        "C:/tools/msys2/ucrt64"
    )
    # 如果已经在 MSYS2 环境中，优先使用
    if [[ -n "${MSYSTEM_PREFIX:-}" && -d "$MSYSTEM_PREFIX/bin" ]]; then
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

# 捕获没有经过 error() 显式处理的意外失败，并报告脚本行号。ERR trap 会由 set -E
# 传播到函数和命令替换中，排查 CI/同事电脑上的工具链差异时能直接定位失败位置。
on_unhandled_error() {
    local exit_code=$?
    local line_number="${1:-unknown}"
    echo -e "\033[31m[ERROR]\033[0m Unexpected command failure at line $line_number (exit=$exit_code)" >&2
    exit "$exit_code"
}
trap 'on_unhandled_error "$LINENO"' ERR

# ---------------------------------------------------------------------------
# 前置检查
# ---------------------------------------------------------------------------
check_prerequisites() {
    info "Checking prerequisites..."
    info "MSYS2 UCRT64: $MSYS2_PREFIX"

    if [[ ! -f "$BUILD_BIN/$EXE_NAME" ]]; then
        error "$EXE_NAME not found at $BUILD_BIN/$EXE_NAME. Build the project first."
    fi

    if [[ ! -f "$CMAKE_CACHE" ]]; then
        error "CMake cache not found: $CMAKE_CACHE. Configure and build the project first."
    fi

    # MinGW Makefiles 是单配置生成器，`cmake --build --config Release` 不会把一个已经
    # 配置为 Debug 的构建目录切换成 Release。这里直接读取 CMakeCache，防止把带调试
    # 行为和大量调试符号的程序误当成正式安装包发布。
    local build_type
    build_type=$(sed -n 's/^CMAKE_BUILD_TYPE:[^=]*=//p' "$CMAKE_CACHE" | head -n 1)
    if [[ "$build_type" != "Release" ]]; then
        if [[ "${ALLOW_NON_RELEASE_PACKAGE:-0}" == "1" ]]; then
            warn "Packaging non-Release build because ALLOW_NON_RELEASE_PACKAGE=1 (CMAKE_BUILD_TYPE=${build_type:-unset})"
        else
            error "CMAKE_BUILD_TYPE is '${build_type:-unset}', expected 'Release'. Reconfigure with: cmake -S . -B build -DCMAKE_BUILD_TYPE=Release"
        fi
    fi

    # 后端程序不是可选附件。提前检查可以避免完成耗时的 Qt/DLL 收集后才发现安装包不完整。
    local required_exe
    for required_exe in "${REQUIRED_EXES[@]}"; do
        if [[ ! -f "$BUILD_BIN/$required_exe" ]]; then
            error "Required executable not found: $BUILD_BIN/$required_exe"
        fi
    done

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

    # seen 使用小写 DLL 文件名作为键，因为 Windows 文件系统和加载器不区分大小写。
    # unresolved 保存最终无法定位的依赖；只要非空，打包必须失败。
    declare -A seen=()
    declare -A unresolved=()
    local depth=0
    local max_depth=20

    is_windows_system_dll() {
        local lower_name="${1,,}"

        # api-ms-win/ext-ms-win 是 Windows API Set 的虚拟名称，不应随应用分发。
        if [[ "$lower_name" =~ ^(api-ms-win-|ext-ms-win-) ]]; then
            return 0
        fi

        # 下列 DLL 由受支持的 Windows 系统提供。把它们复制进安装目录既没有必要，
        # 也可能用错误版本覆盖系统加载结果。
        [[ "$lower_name" =~ ^(ntdll|kernel32|kernelbase|user32|gdi32|shell32|ole32|comctl32|comdlg32|oleaut32|msvcrt|ucrtbase|advapi32|ws2_32|secur32|crypt32|bcrypt|ncrypt|rpcrt4|shlwapi|version|winmm|imm32|setupapi|cfgmgr32|powrprof|propsys|iphlpapi|dnsapi|netapi32|mpr|winhttp|urlmon|wininet|iertutil|srvcli|wkscli|netutils|samcli|dwmapi|uxtheme|d3d9|d3d11|dxgi|opengl32|gdiplus|msimg32|winspool)\.dll$ ]]
    }

    collect_for_binary() {
        local binary="$1"
        local base_bin
        base_bin=$(basename "$binary")

        depth=$((depth + 1))
        if (( depth > max_depth )); then
            error "DLL dependency recursion exceeded $max_depth levels at $base_bin"
        fi

        # ldd 的退出码必须单独检查。旧逻辑把输出直接接到 grep/sort，导致 ldd 失败时
        # 仍由管道末尾的 sort 返回成功，最终生成缺少依赖的安装包。
        info "  ldd: $base_bin (depth=$depth)"
        local ldd_output
        if ! ldd_output=$("$LDD" "$binary" 2>&1); then
            printf '%s\n' "$ldd_output" >&2
            error "ldd failed while scanning: $binary"
        fi

        _copy_one_dll() {
            local dll_path="$1"
            local base key
            base=$(basename "$dll_path")
            key="${base,,}"

            if [[ -z "$base" ]]; then
                return
            fi

            if is_windows_system_dll "$base"; then
                return
            fi

            if [[ -n "${seen[$key]:-}" ]]; then
                return
            fi
            seen[$key]=1
            unset "unresolved[$key]"

            if [[ -f "$staging/$base" ]]; then
                return
            fi

            # 同名库优先使用当前 UCRT64 工具链中的版本，避免误混用另一套 MinGW ABI。
            local src="$MSYS2_BIN/$base"
            if [[ ! -f "$src" ]]; then
                src="$dll_path"
            fi

            if [[ -f "$src" ]]; then
                cp "$src" "$staging/"
                collect_for_binary "$staging/$base"
            else
                unresolved[$key]="$base (required by $base_bin)"
            fi
        }

        # 使用数组逐行接收路径，避免 DLL 路径中包含空格时被 for 的单词拆分破坏。
        local -a dll_paths=()
        mapfile -t dll_paths < <(
            printf '%s\n' "$ldd_output" \
                | awk 'tolower($0) ~ /(\/ucrt64\/|\/mingw64\/)/ { print $3 }' \
                | sort -fu
        )

        local dll_path
        for dll_path in "${dll_paths[@]}"; do
            _copy_one_dll "$dll_path"
        done

        local -a missing_names=()
        mapfile -t missing_names < <(
            printf '%s\n' "$ldd_output" \
                | awk 'tolower($0) ~ /=> not found/ { print $1 }' \
                | sort -fu
        )

        local base key search_dir found
        for base in "${missing_names[@]}"; do
            if is_windows_system_dll "$base"; then
                continue
            fi

            key="${base,,}"
            if [[ -n "${seen[$key]:-}" ]]; then
                continue
            fi

            # 先检查 staging，再检查项目产物和当前 UCRT64。find -iname 保证文件名
            # 大小写不一致时仍能找到，而不会在 Windows 上误报缺失。
            found=""
            for search_dir in "$staging" "$BUILD_BIN" "$BUILD_LIB" "$MSYS2_BIN"; do
                [[ -d "$search_dir" ]] || continue
                found=$(find "$search_dir" -maxdepth 1 -type f -iname "$base" -print -quit)
                if [[ -n "$found" ]]; then
                    break
                fi
            done

            if [[ -n "$found" ]]; then
                _copy_one_dll "$found"
            else
                unresolved[$key]="$base (required by $base_bin)"
            fi
        done

        depth=$((depth - 1))
    }

    # 递归扫描插件子目录中的 DLL。只扫描 staging 根目录会漏掉 qwindows.dll、
    # 图像插件等二进制文件自身依赖的运行库。
    local binary
    while IFS= read -r -d '' binary; do
        collect_for_binary "$binary"
    done < <(find "$staging" -maxdepth 1 -type f -iname '*.exe' -print0)

    while IFS= read -r -d '' binary; do
        collect_for_binary "$binary"
    done < <(find "$staging" -type f -iname '*.dll' -print0)

    if (( ${#unresolved[@]} > 0 )); then
        local unresolved_key
        for unresolved_key in "${!unresolved[@]}"; do
            warn "Unresolved dependency: ${unresolved[$unresolved_key]}"
        done
        error "Dependency collection failed; refusing to build an incomplete installer"
    fi

    local count
    count=$(find "$staging" -type f -iname '*.dll' | wc -l)
    info "Total DLLs in staging (including Qt plugins): $count"
}

# ---------------------------------------------------------------------------
# 构建 staging 目录
# ---------------------------------------------------------------------------
validate_staging() {
    # 这些文件是 Widgets GUI 在一台未安装 Qt 的 Windows 电脑上启动所需的最小集合。
    # 特别是 platforms/qwindows.dll：缺少它时程序会报“no Qt platform plugin”并退出。
    local required_paths=(
        "$EXE_NAME"
        "Qt6Core.dll"
        "Qt6Gui.dll"
        "Qt6Widgets.dll"
        "platforms/qwindows.dll"
    )

    local required_exe required_path
    for required_exe in "${REQUIRED_EXES[@]}"; do
        required_paths+=("$required_exe")
    done

    for required_path in "${required_paths[@]}"; do
        if [[ ! -f "$STAGING_DIR/$required_path" ]]; then
            error "Required staging file is missing: $required_path"
        fi
    done

    if [[ ! -d "$STAGING_DIR/config" ]]; then
        error "Required configuration directory is missing from staging"
    fi

    info "Staging validation passed"
}

stage_files() {
    info "Preparing staging directory..."
    rm -rf "$STAGING_DIR"
    mkdir -p "$STAGING_DIR"

    # 复制主程序
    info "Copying $EXE_NAME..."
    cp "$BUILD_BIN/$EXE_NAME" "$STAGING_DIR/"

    # windeployqt6 不仅复制 Qt6*.dll，还负责部署 platforms/qwindows.dll 等插件。
    # 必须先保存并检查真实退出码，不能再用 `|| true` 吞掉失败，否则 NSIS 仍会产出
    # 一个双击后无法启动的安装包。
    info "Running windeployqt6..."
    local deploy_output
    if ! deploy_output=$(windeployqt6 --release --no-translations --dir "$STAGING_DIR" "$STAGING_DIR/$EXE_NAME" 2>&1); then
        printf '%s\n' "$deploy_output" >&2
        error "windeployqt6 failed"
    fi
    printf '%s\n' "$deploy_output"

    # 必需后端在前置检查中已经确认存在，此处任何复制失败都会由 set -e 终止脚本。
    local extra_exe
    for extra_exe in "${REQUIRED_EXES[@]}"; do
        info "Copying required executable $extra_exe..."
        cp "$BUILD_BIN/$extra_exe" "$STAGING_DIR/"
    done

    # 可选工具的缺失不阻止发布，但必须明确提示，避免 README 与实际包内容不一致。
    for extra_exe in "${OPTIONAL_EXES[@]}"; do
        if [[ -f "$BUILD_BIN/$extra_exe" ]]; then
            info "Copying optional executable $extra_exe..."
            cp "$BUILD_BIN/$extra_exe" "$STAGING_DIR/"
        else
            warn "Optional executable not found, skipping: $extra_exe"
        fi
    done

    # 最后统一收集一次 DLL 依赖（避免重复扫描）
    collect_dlls "$STAGING_DIR"

    # 配置是后端启动所必需的数据，不再把缺失配置降级成警告。
    # 使用 config/. 可以连同隐藏文件一起复制，并避免空目录时通配符原样展开。
    if [[ -d "$PROJECT_ROOT/config" ]]; then
        info "Copying config directory..."
        mkdir -p "$STAGING_DIR/config"
        cp -r "$PROJECT_ROOT/config/." "$STAGING_DIR/config/"
    else
        error "Config directory not found: $PROJECT_ROOT/config"
    fi

    # 复制图标文件
    if [[ -n "$ICON_FILE" && -f "$PROJECT_ROOT/$ICON_FILE" ]]; then
        info "Copying icon: $ICON_FILE..."
        cp "$PROJECT_ROOT/$ICON_FILE" "$STAGING_DIR/"
    fi

    # 深色 QSS 已通过 Qt Resource System 编译进 zrcsgui.exe，不再复制外部 style 目录。
    # 最后统一验证关键文件，确保依赖收集结果满足最低可运行条件。
    validate_staging

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

    local staging_win output_win nsis_template_win nsis_script_win
    staging_win=$(cygpath -w "$STAGING_DIR" | sed 's/\\/\\\\/g')
    output_win=$(cygpath -w "$OUTPUT_DIR" | sed 's/\\/\\\\/g')
    nsis_template_win=$(cygpath -w "$NSIS_TEMPLATE")
    nsis_script_win=$(cygpath -w "$NSIS_SCRIPT")

    # Prepare icon defines (NSIS !define lines)
    local icon_defines=""
    local icon_reg=""
    if [[ -n "$ICON_FILE" ]]; then
        local icon_basename
        icon_basename=$(basename "$ICON_FILE")
        if [[ -f "$STAGING_DIR/$icon_basename" ]]; then
            local icon_win
            icon_win=$(cygpath -w "$STAGING_DIR/$icon_basename" | sed 's/\\/\\\\/g')
            icon_defines="!define MUI_ICON \"${icon_win}\"
!define MUI_UNICON \"${icon_win}\""
            icon_reg="WriteRegStr HKLM \"Software\\\\Microsoft\\\\Windows\\\\CurrentVersion\\\\Uninstall\\\\\${PRODUCT_NAME}\" \\
        \"DisplayIcon\" \"\$INSTDIR\\\\$icon_basename\""
        fi
    fi
    # If no icon, leave defines empty (NSIS uses default)
    if [[ -z "$icon_defines" ]]; then
        icon_defines="; No custom icon provided - using NSIS default"
        icon_reg="; No custom icon for registry"
    fi

    # Prepare license page
    local license_page=""
    local license_candidates=("$PROJECT_ROOT/LICENSE" "$PROJECT_ROOT/LICENSE.txt" "$PROJECT_ROOT/LICENSE.md")
    for lf in "${license_candidates[@]}"; do
        if [[ -f "$lf" ]]; then
            # NSIS 引用 staging 中的副本，使生成的 installer.nsi 不再依赖源码目录。
            # 这样即使生成脚本后移动或归档 staging，NSIS 脚本仍可独立编译。
            local staged_license="$STAGING_DIR/$(basename "$lf")"
            cp "$lf" "$staged_license"
            local lf_win
            lf_win=$(cygpath -w "$staged_license" | sed 's/\\/\\\\/g')
            license_page="!insertmacro MUI_PAGE_LICENSE \"${lf_win}\""
            info "Using license file: $lf"
            break
        fi
    done
    if [[ -z "$license_page" ]]; then
        license_page="; No license file found - skipping license page"
        warn "No LICENSE file found, skipping license page"
    fi

    # 使用 PowerShell 生成脚本，避免 MSYS2 bash 对内联反引号的错误解析。
    # String.Replace 是普通字符串替换，不会像 -replace 那样把路径中的 `$` 或反斜杠
    # 当作正则表达式替换语法处理。
    local ps_script="$STAGING_DIR/generate_installer.ps1"
    cat > "$ps_script" <<'EOF'
$template = Get-Content $env:NSIS_TEMPLATE -Encoding UTF8 -Raw
$script = $template
$replacements = @{
    '@ZRCS_VERSION@'      = $env:ZRCS_VERSION
    '@ZRCS_VERSION_QUAD@' = $env:ZRCS_VERSION_QUAD
    '@ZRCS_NAME@'         = $env:ZRCS_NAME
    '@ZRCS_PUBLISHER@'    = $env:ZRCS_PUBLISHER
    '@STAGING_DIR@'       = $env:STAGING_WIN
    '@OUTPUT_DIR@'        = $env:OUTPUT_WIN
    '@EXE_NAME@'          = $env:EXE_NAME
    '@ICON_DEFINES@'      = $env:ICON_DEFINES
    '@ICON_REG@'          = $env:ICON_REG
    '@LICENSE_PAGE@'      = $env:LICENSE_PAGE
}
foreach ($entry in $replacements.GetEnumerator()) {
    $script = $script.Replace($entry.Key, [string]$entry.Value)
}
[IO.File]::WriteAllText($env:NSIS_SCRIPT, $script, (New-Object System.Text.UTF8Encoding $true))
EOF

    # 命令放在 `||` 左侧可以在 set -e 模式下取得退出码，并输出统一、明确的错误信息。
    local ps_exit=0
    NSIS_TEMPLATE="$nsis_template_win" \
    NSIS_SCRIPT="$nsis_script_win" \
    ZRCS_VERSION="$ZRCS_VERSION" \
    ZRCS_VERSION_QUAD="$ZRCS_VERSION_QUAD" \
    ZRCS_NAME="$ZRCS_NAME" \
    ZRCS_PUBLISHER="$ZRCS_PUBLISHER" \
    STAGING_WIN="$staging_win" \
    OUTPUT_WIN="$output_win" \
    EXE_NAME="$EXE_NAME" \
    ICON_DEFINES="$icon_defines" \
    ICON_REG="$icon_reg" \
    LICENSE_PAGE="$license_page" \
    powershell -NoProfile -ExecutionPolicy Bypass -File "$(cygpath -w "$ps_script")" 2>&1 || ps_exit=$?

    if (( ps_exit != 0 )); then
        error "Failed to generate NSIS script with PowerShell"
    fi

    rm -f "$ps_script"

    info "NSIS script generated: $NSIS_SCRIPT"
}

# ---------------------------------------------------------------------------
# 编译安装包
# ---------------------------------------------------------------------------
build_installer() {
    info "Building installer with NSIS..."
    local installer="$OUTPUT_DIR/${ZRCS_NAME}-${ZRCS_VERSION}-Setup.exe"

    # 先删除同名旧包，避免本次 NSIS 失败后目录里仍留着旧文件，让发布人员误以为
    # 刚刚的打包已经成功。该路径由固定的 OUTPUT_DIR、产品名和版本号组成。
    rm -f "$installer"
    if ! "$MAKENSIS" "$(cygpath -w "$NSIS_SCRIPT")"; then
        error "makensis failed"
    fi

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

# 直接执行脚本时进入完整打包流程；被测试脚本 source 时只加载函数和配置。
# 这一保护使 generate_nsis/collect_dlls 等步骤可以独立做语法与回归验证，而不会
# 因为加载文件就立即删除 staging 或覆盖正式安装包。
if [[ "${BASH_SOURCE[0]}" == "$0" ]]; then
    main "$@"
fi
