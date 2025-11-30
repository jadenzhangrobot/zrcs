cmake_minimum_required(VERSION 3.18.1)
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
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/contorler.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/3rdParty.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/tool.cmake)
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/include)
# 源文件
aux_source_directory(./src SOURCE_FILES)

# 主可执行文件
add_executable(${PROJECT_NAME} ${SOURCE_FILES})
# 依赖关系
add_dependencies(${PROJECT_NAME} run_python_scripts)

set(MOTION_CONTROL_LIBS
    Threads::Threads
    ruckig
    tinyxml2
    atomic
    rt
)
if(BUILD_MODE STREQUAL "realtime")
    list(APPEND MOTION_CONTROL_LIBS ethercat_rtdm)
endif()

if(BUILD_MODE STREQUAL "simulation")
    list(APPEND MOTION_CONTROL_LIBS remoteApi)
endif()


target_link_libraries(${PROJECT_NAME} PRIVATE ${MOTION_CONTROL_LIBS})

# 测试程序
if(test)
    add_subdirectory(program/terminal)
    add_subdirectory(program/zrcsgui)
    add_subdirectory(program/plot)
endif()

# 安装配置
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
