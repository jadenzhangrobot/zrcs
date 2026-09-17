cmake_minimum_required(VERSION 3.18.1)

# ============================================================
# 全局构建设置
# ============================================================
if(POLICY CMP0077)
    cmake_policy(SET CMP0077 NEW)
endif()

# 默认安装到 <源码目录>/Install，避免需要管理员权限
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/Install" CACHE PATH "Install path" FORCE)
endif()

project(zrcs VERSION 1.0.0)

# C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 导出 compile_commands.json（供 clangd 等工具使用）
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 构建模式：realtime / simulation / standard（合法值校验见 controller.cmake）
set(BUILD_MODE "simulation" CACHE STRING "Build configuration mode")
option(test "Build test programs" ON)

# 全局头文件搜索路径
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_BINARY_DIR})        # 生成的 proto 头
include_directories(${CMAKE_BINARY_DIR}/generated)      # 生成的命令注册表头

# ============================================================
# 依赖与公共库
#   - controller.cmake : 按 BUILD_MODE 设置编译宏 / xenomai flags
#   - 3rdParty.cmake   : 集中管理所有第三方库（含 Protobuf/Abseil 查找）
# ============================================================
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/controller.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/3rdParty.cmake)

# Abseil 由 3rdParty.cmake 引入，此处仅做存在性校验
if(NOT TARGET absl::base)
    message(FATAL_ERROR "Abseil targets not found. Please ensure 3rdParty/abseil-cpp is available.")
endif()

# ---- 公共 Protobuf 库：message.proto 全工程只生成/编译一次 ----
set(ZRCS_PROTO_FILES ${CMAKE_CURRENT_SOURCE_DIR}/zrcs_common/message/message.proto)
protobuf_generate_cpp(ZRCS_PROTO_SRCS ZRCS_PROTO_HDRS ${ZRCS_PROTO_FILES})
add_library(zrcs_proto STATIC ${ZRCS_PROTO_SRCS} ${ZRCS_PROTO_HDRS})
add_library(zrcs::proto ALIAS zrcs_proto)
# 生成的 message.pb.h 位于本 binary 目录，PUBLIC 传播给所有消费者
target_include_directories(zrcs_proto PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(zrcs_proto PUBLIC
    protobuf::libprotobuf
    absl::log_internal_message
    absl::log_internal_check_op
    absl::log_globals
)

# ---- 运动控制模块公共依赖集：各可执行目标统一链接 ----
set(MOTION_CONTROL_LIBS
    Threads::Threads
    ruckig
    atomic
    zrcs::proto
)
if(TARGET zrcs_mujoco)
    list(APPEND MOTION_CONTROL_LIBS zrcs_mujoco)
endif()

if(BUILD_MODE STREQUAL "realtime")
    list(APPEND MOTION_CONTROL_LIBS ethercat_rtdm)
endif()

# ============================================================
# 子目录编排（顺序有依赖：公共库 → 行为树 → GUI → 进程 → 测试）
# ============================================================
# 1. zrcs_common 公共库
add_subdirectory(zrcs_common)

# Groot 行为树编辑器 + zrcs_gui（realtime 模式不编译）
if(NOT BUILD_MODE STREQUAL "realtime")
    add_subdirectory(3rdParty/Groot)
    add_subdirectory(zrcs_gui)
endif()

# 2. zrcsnrt (非实时进程)
add_subdirectory(zrcs_nrt)

# 3. zrcsrt (实时进程)
add_subdirectory(zrcs_rt)

if(test)
    add_subdirectory(test)
endif()

# ============================================================
# 安装与输出目录
#   install 由各子目录 CMakeLists 自行处理；zrcsgui 的输出目录亦在其内设置。
# ============================================================
# 统一可执行文件输出到 ${CMAKE_BINARY_DIR}/bin
foreach(_zrcs_exe zrcsnrt zrcsrt)
    if(TARGET ${_zrcs_exe})
        set_target_properties(${_zrcs_exe} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
        )
    endif()
endforeach()
