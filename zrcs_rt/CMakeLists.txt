project(zrcsrt)

set(CMD_SRC_DIR    ${CMAKE_SOURCE_DIR}/zrcs_rt/command)

# 包含目录
include_directories(
    ${CMAKE_SOURCE_DIR}/zrcs_rt
    ${CMAKE_SOURCE_DIR}/3rdParty/magic_enum/include
    ${CMAKE_BINARY_DIR}
)

# 源文件 (暂时假设使用 zrcs_rt/main.cpp)
# 之前的目录结构显示 zrcs_rt 下有 main.cpp
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    "main.cpp"
    "command/*.cpp"
    "controller/*.cpp"
    "model/*.cpp"
    "plc/*.cpp"
    "system/*.cpp"
)

# ============================================================================
# PLC：MatIEC 生成的 C，PlcLogicNode 直接引用生成接口。
#   - ST 源码:    config/<active>/program/plc/plc.st （版本受控，随工程）
#   - 生成目录:   ./plc_generated/                   （build 树，不入库）
#   - 宿主节点:   zrcs_rt/plc/Plc{Input,Logic,Output}Node.cpp （版本受控）
# 注意：当前是构建期选一份 ST 编译进 zrcsrt（非运行时按工程切换）。
#       更换激活工程的 PLC 逻辑时改下面 PLC_ST_FILE 指向即可。
# 详见 docs/matiec-rt-integration.md §9。
# ============================================================================
# 定位/自动构建 MatIEC 编译器，见 cmake/matiec.cmake。
include(${CMAKE_SOURCE_DIR}/cmake/matiec.cmake)
set(PLC_ST_FILE   "${CMAKE_SOURCE_DIR}/config/5axis/program/plc/plc.st")
set(PLC_GEN_DIR   "${CMAKE_CURRENT_BINARY_DIR}/plc_generated")

# 一份 ST 同时包含 PROGRAM 和 CONFIGURATION；周期由 controller.cmake 按模式选择。
# configure_file 自动跟踪该源文件，ST 修改或模式切换后会更新构建目录中的完整 ST。
# 仅替换 @VAR@ 占位符，不运行 C 预处理器，保留 IEC 字符串和注释的原始内容。
set(PLC_CONFIGURED_ST "${PLC_GEN_DIR}/plc.st")
configure_file("${PLC_ST_FILE}" "${PLC_CONFIGURED_ST}" @ONLY)

# 文件名由 ST 中的 CONFIGURATION / RESOURCE 名决定：zrcs_plc / ZRCS。
set(PLC_C_SOURCES "${PLC_GEN_DIR}/zrcs_plc.c" "${PLC_GEN_DIR}/ZRCS.c")
set(PLC_GENERATED_FILES ${PLC_C_SOURCES}
    "${PLC_GEN_DIR}/POUS.h" "${PLC_GEN_DIR}/POUS.c" "${PLC_GEN_DIR}/zrcs_plc.h")
# Windows 的 iec2c 是 MSYS2 宿主工具，PATH 需含 MSYS 的 /usr/bin。
add_custom_command(
    OUTPUT ${PLC_GENERATED_FILES}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PLC_GEN_DIR}"
    COMMAND "${ZRCS_MATIEC_EXE}" -T "${PLC_GEN_DIR}" "${PLC_CONFIGURED_ST}"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/3rdParty/MatIEC"
    DEPENDS "${PLC_CONFIGURED_ST}" "${ZRCS_MATIEC_EXE}"
    COMMENT "[PLC] matiec: plc.st (${ZRCS_CYCLE_TIME_MS} ms) -> ${PLC_GEN_DIR}"
    VERBATIM)
add_custom_target(zrcs_plc_generated DEPENDS ${PLC_GENERATED_FILES})

# ZRCS.c（资源文件）已包含 POUS.c，后者不能再次作为独立翻译单元编译。
add_executable(zrcsrt ${SOURCES} ${PLC_C_SOURCES})
# 首次并行构建时，C++ 节点包含 POUS.h 前必须先完成生成。
add_dependencies(zrcsrt zrcs_plc_generated)
target_include_directories(zrcsrt PRIVATE "${PLC_GEN_DIR}")
target_include_directories(zrcsrt SYSTEM PRIVATE "${CMAKE_SOURCE_DIR}/3rdParty/MatIEC/lib/C")

# 链接库
target_link_libraries(zrcsrt PRIVATE
    zrcscommon
    Eigen3::Eigen
    ${MOTION_CONTROL_LIBS}
)

if(WIN32)
    set_target_properties(zrcsrt PROPERTIES LINK_FLAGS "-mconsole")
endif()

install(TARGETS zrcsrt
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
