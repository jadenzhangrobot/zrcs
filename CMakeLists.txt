cmake_minimum_required(VERSION 3.22.1)
project(zrcs VERSION 1.0.0)

# 设置 C++ 标准为 C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 启用编译命令导出
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 构建选项
option(realtime "Build realtime programs" OFF)
option(simulation "Build simulation programs" OFF)
option(test "Build test programs" ON)

# 显示构建配置
message(STATUS "Build configuration:")
message(STATUS "  realtime: ${realtime}")
message(STATUS "  simulation: ${simulation}")
message(STATUS "  test: ${test}")

# 包含模块文件
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/compiler.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/3rdParty.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/contorler.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/tool.cmake)

# 设置包含目录
# set(DEVEL_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR})
# include_directories( 
#     ${DEVEL_INCLUDE_DIRS}/include
#     ${DEVEL_INCLUDE_DIRS}/3rdParty/tinyxml2
#     ${DEVEL_INCLUDE_DIRS}/3rdParty/ruckig/include
# )

# 平台特定的包含目录
# if(UNIX)
#     include_directories(
#         /usr/local/include
#         /usr/include
#         /opt/etherlab/include
#         ${DEVEL_INCLUDE_DIRS}/3rdParty/libzmq/include
#     )
#     link_directories(/usr/local/lib)
#     if(ethercat)
#         link_directories(/opt/etherlab/lib/)
#     endif()
# endif()

# 链接目录
# link_directories(${DEVEL_INCLUDE_DIRS}/build/3rdParty/ruckig)

# 源文件
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/build/message PROTO_FILE)
aux_source_directory(./src SOURCE_FILES)

# 主可执行文件
add_executable(motion_control ${SOURCE_FILES} ${PROTO_FILE})

# 依赖关系
add_dependencies(motion_control run_python_scripts)
target_link_libraries(
        motion_control
        PRIVATE 
        Threads::Threads
        ruckig
        tinyxml2
        atomic)
# 链接库配置
if(realtime)
endif()
if(simulation) 
        target_link_libraries(motion_control PRIVATE remoteApi)
endif()

# 测试程序
if(test)
    add_subdirectory(example/terminal)
    add_subdirectory(example/slab)
endif()

# 安装配置
install(TARGETS motion_control
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

