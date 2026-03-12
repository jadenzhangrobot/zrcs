cmake_minimum_required(VERSION 3.18.1)
project(zrcs VERSION 1.0.0)

# 设置 C++ 标准为 C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 启用编译命令导出
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# realtime simulation standard
set(BUILD_MODE "standard" CACHE STRING "Build configuration mode")
option(test "Build test programs" ON)
# 包含模块文件
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/contorler.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/3rdParty.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/tool.cmake)
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_BINARY_DIR}) # For generated proto headers

# Compile Proto file
file(GLOB PROTO_FILES "zrcsCommon/message/*.proto")
if(PROTO_FILES)
    protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})
else()
    message(STATUS "No proto files found in message/ directory.")
endif()

set(MOTION_CONTROL_LIBS
    Threads::Threads
    ruckig
    tinyxml2
    atomic
    protobuf::libprotobuf
)

# 如果找到了 Abseil，也需要链接它（Protobuf 依赖）
if(absl_FOUND)
    list(APPEND MOTION_CONTROL_LIBS
        absl::log_internal_message
        absl::log_internal_check_op
        absl::log_globals
    )
endif()
if(BUILD_MODE STREQUAL "realtime")
    list(APPEND MOTION_CONTROL_LIBS ethercat_rtdm)
endif()

if(BUILD_MODE STREQUAL "simulation")
    list(APPEND MOTION_CONTROL_LIBS remoteApi)
endif()

# 源文件
aux_source_directory(./src SOURCE_FILES)

# 1. zrcsgui (已经在 subdirectory 中定义，这里不需要重复，但确保它被构建)
add_subdirectory(zrcsGui)

# 2. zrcsnrt (非实时进程)
add_subdirectory(zrcsNrt)

# 3. zrcsrt (实时进程)
add_subdirectory(zrcsRt)


# 安装配置
# 注意：install 命令现在应该在各自的子目录 CMakeLists.txt 中处理
# 或者在这里引用子目录中的目标（但如果在子目录 install 了，这里可以简化）
# install(TARGETS zrcsnrt zrcsrt ...) # 已经在子目录处理

set_target_properties(zrcsgui zrcsnrt zrcsrt PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
