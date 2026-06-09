# 设置CMake策略以处理Boost库查找
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

# 强制所有第三方库生成静态库
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)

#添加tinyxml2库编译
# 检查 tinyxml2 目录是否存在
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/tinyxml2)

#添加ruckig库编译
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/ruckig)

# 添加spdlog日志库
set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/spdlog)

# 添加 cppzmq 头文件封装库
set(CPPZMQ_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/cppzmq)

# 添加 Matplot++ 绘图库（仅构建库本体，示例/测试关闭）
include(GNUInstallDirs)
set(MATPLOT_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/matplotplusplus)
set(MATPLOTPP_BUILD_HIGH_RESOLUTION_WORLD_MAP OFF CACHE BOOL "" FORCE)
set(MATPLOTPP_BUILD_FOR_DOCUMENTATION_IMAGES OFF CACHE BOOL "" FORCE)
set(MATPLOT_TRACE_GNUPLOT_COMMANDS OFF CACHE BOOL "" FORCE)
set(MATPLOTPP_BUILD_INSTALLER OFF CACHE BOOL "" FORCE)
set(MATPLOTPP_BUILD_EXPERIMENTAL_OPENGL_BACKEND OFF CACHE BOOL "" FORCE)
set(MATPLOTPP_WITH_OPENCV OFF CACHE BOOL "" FORCE)
set(MATPLOTPP_WITH_SYSTEM_CIMG OFF CACHE BOOL "" FORCE)
set(MATPLOTPP_WITH_SYSTEM_NODESOUP OFF CACHE BOOL "" FORCE)
function(target_link_libraries_system target)
    target_link_libraries(${target} ${ARGN})
endfunction()
function(target_bigobj_options target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /bigobj)
    endif()
endfunction()
function(target_exception_options target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /EHsc)
    endif()
endfunction()
function(target_utf8_options target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /utf-8)
    endif()
endfunction()
function(target_nominmax_definition target)
    if(MSVC)
        target_compile_definitions(${target} PRIVATE NOMINMAX)
    endif()
endfunction()
function(maybe_target_pedantic_warnings target)
endfunction()
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/matplotplusplus/source)

# 添加 Abseil 库（供 Protobuf 等依赖使用）
set(ABSL_PROPAGATE_CXX_STD ON CACHE BOOL "" FORCE)
set(ABSL_BUILD_TESTING OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/abseil-cpp)

# 添加boost库
# 优先使用BoostConfig.cmake，如果不可用则回退到FindBoost.cmake
find_package(Boost QUIET CONFIG)
if(NOT Boost_FOUND)
    find_package(Boost REQUIRED COMPONENTS system)
endif()
#添加线程库
find_package(Threads REQUIRED)

find_package(Protobuf REQUIRED)
if(Protobuf_FOUND)
    message(STATUS "Found Protobuf: ${Protobuf_VERSION}")
    include_directories(${Protobuf_INCLUDE_DIRS})
endif()

if(TARGET absl::base)
    message(STATUS "Using bundled Abseil from 3rdParty/abseil-cpp")
endif()

# 添加Eigen3线性代数库
find_package(Eigen3 REQUIRED)
if(Eigen3_FOUND)
    message(STATUS "Found Eigen3: ${Eigen3_VERSION}")
endif()



# 添加CoppeliaSim的头文件

