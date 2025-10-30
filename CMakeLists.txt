cmake_minimum_required(VERSION 3.22.1)
project(zrcs VERSION 1.0.0)

# 设置 C++ 标准为 C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 启用编译命令导出
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(BUILD_MODE "standard" CACHE STRING "Build configuration mode")
option(test "Build test programs" ON)
# 包含模块文件
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/include)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/3rdParty.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/contorler.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/tool.cmake)


# 源文件
aux_source_directory(./src SOURCE_FILES)

# 主可执行文件
add_executable(motion_control ${SOURCE_FILES})

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

