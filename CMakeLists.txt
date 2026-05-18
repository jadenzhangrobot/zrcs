cmake_minimum_required(VERSION 3.18.1)

if(POLICY CMP0077)
    cmake_policy(SET CMP0077 NEW)
endif()

# 默认安装到 build/install，避免需要管理员权限
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Install path" FORCE)
endif()

project(zrcs VERSION 1.0.0)

# 设置 C++ 标准为 C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 启用编译命令导出
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# realtime simulation standard
set(BUILD_MODE "realtime" CACHE STRING "Build configuration mode")
option(test "Build test programs" ON)
# 包含模块文件
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/controller.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/3rdParty.cmake)
#include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/tool.cmake)
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_BINARY_DIR}) # For generated proto headers
include_directories(${CMAKE_BINARY_DIR}/generated) # For generated command registry headers

# 查找 Protobuf 和 Abseil
find_package(Protobuf REQUIRED)
if(NOT TARGET absl::base)
    message(FATAL_ERROR "Abseil targets not found. Please ensure 3rdParty/abseil-cpp is available.")
endif()

set(MOTION_CONTROL_LIBS
    Threads::Threads
    ruckig
    tinyxml2
    atomic
    protobuf::libprotobuf
    absl::log_internal_message
    absl::log_internal_check_op
    absl::log_globals
)
if(BUILD_MODE STREQUAL "realtime")
    list(APPEND MOTION_CONTROL_LIBS ethercat_rtdm)
endif()

if(BUILD_MODE STREQUAL "simulation")
    list(APPEND MOTION_CONTROL_LIBS remoteApi)
endif()

# 源文件
aux_source_directory(./src SOURCE_FILES)

# 1. zrcs_common 公共库
add_subdirectory(zrcs_common)

# BehaviorTree.CPP（zrcsnrt 核心依赖，所有模式都需要）
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(BUILD_MANUAL_SELECTOR OFF CACHE BOOL "" FORCE)
add_subdirectory(3rdParty/Groot/depend/BehaviorTree.CPP)

# Groot 行为树编辑器 + zrcs_gui（realtime 模式不编译）
if(NOT BUILD_MODE STREQUAL "realtime")
    add_subdirectory(3rdParty/Groot)
    add_subdirectory(zrcs_gui)
    add_subdirectory(motionGui)
endif()

# 2. zrcsnrt (非实时进程)
add_subdirectory(zrcs_nrt)

# 3. zrcsrt (实时进程)
add_subdirectory(zrcs_rt)


# 安装配置
# 注意：install 命令现在应该在各自的子目录 CMakeLists.txt 中处理
# 或者在这里引用子目录中的目标（但如果在子目录 install 了，这里可以简化）
# install(TARGETS zrcsnrt zrcsrt ...) # 已经在子目录处理

if(NOT BUILD_MODE STREQUAL "realtime")
    set_target_properties(zrcsgui motiongui zrcsnrt zrcsrt PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )
else()
    set_target_properties(zrcsnrt zrcsrt PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )
endif()
