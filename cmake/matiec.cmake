# ============================================================================
# MatIEC：IEC 61131-3 ST->C 编译器（3rdParty/MatIEC，浅克隆自
# https://github.com/thiagoralves/MatIEC）。
#
# 职责：
#   - 定位编译器：Windows 用 MSYS2 预构建的 iec2c.exe，其他平台用 Linux 版 iec2c。
#   - 若编译器缺失：Linux/其他平台用 autotools 构建期自动编译
#     （autoreconf -i && ./configure && make），Windows 无法自动重建则报错。
#   - 输出 ZRCS_MATIEC_EXE 给 zrcs_rt/CMakeLists.txt 的 PLC 生成步骤使用
#     （该生成步骤已 DEPENDS "${ZRCS_MATIEC_EXE}"，缺失时会先触发本模块的构建命令）。
#
# 注意：编译器产物 iec2c / iec2c.exe 均为本地构建产物，已由 .gitignore 排除，
# 不入库；源码各平台自行编译。详见 docs/matiec-rt-integration.md §9。
# ============================================================================

if(CMAKE_HOST_WIN32)
    set(_matiec_name iec2c.exe)
else()
    set(_matiec_name iec2c)
endif()
set(ZRCS_MATIEC_EXE "${CMAKE_SOURCE_DIR}/3rdParty/MatIEC/${_matiec_name}"
    CACHE FILEPATH "matiec IEC61131-3 ST->C 编译器")

if(NOT EXISTS "${ZRCS_MATIEC_EXE}")
    if(CMAKE_HOST_WIN32)
        # Windows 走 MSYS2 预构建的 iec2c.exe，缺失则无法自动重建，直接报错。
        message(FATAL_ERROR "[PLC] matiec 编译器缺失 (${ZRCS_MATIEC_EXE})。请先构建 3rdParty/MatIEC 或设置 ZRCS_MATIEC_EXE。")
    else()
        # Linux/其他平台：构建期自动编译 MatIEC（上游 README.build 标准流程），
        # clone 后无需手动构建。产物 iec2c 已由 .gitignore 排除，不入库。
        find_program(MATIEC_AUTORECONF_EXECUTABLE autoreconf)
        find_program(MATIEC_MAKE_EXECUTABLE make)
        if(NOT MATIEC_AUTORECONF_EXECUTABLE OR NOT MATIEC_MAKE_EXECUTABLE)
            message(FATAL_ERROR "[PLC] matiec 编译器缺失且无法自动构建：缺少 autoreconf/make。请先安装 autoconf automake make，或设置 ZRCS_MATIEC_EXE。")
        endif()
        add_custom_command(
            OUTPUT "${ZRCS_MATIEC_EXE}"
            COMMAND "${MATIEC_AUTORECONF_EXECUTABLE}" -i
            COMMAND ./configure
            COMMAND "${MATIEC_MAKE_EXECUTABLE}"
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/3rdParty/MatIEC"
            COMMENT "[PLC] 构建 matiec 编译器 (autoreconf -i && ./configure && make)"
            VERBATIM)
        message(STATUS "[PLC] ${ZRCS_MATIEC_EXE} 缺失，将在构建时自动编译 MatIEC（依赖 autoconf/automake/flex/bison/g++）")
    endif()
endif()
