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

# 添加 matplotlib-cpp 绘图库（单头文件库，不构建示例）
find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
find_package(Python3 COMPONENTS NumPy QUIET)
add_library(matplotlib_cpp INTERFACE)
add_library(matplotlib_cpp::matplotlib_cpp ALIAS matplotlib_cpp)
target_include_directories(matplotlib_cpp INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/matplotlib-cpp
)
target_compile_features(matplotlib_cpp INTERFACE cxx_std_11)
target_link_libraries(matplotlib_cpp INTERFACE
    Python3::Python
    Python3::Module
)
if(Python3_NumPy_FOUND)
    target_link_libraries(matplotlib_cpp INTERFACE Python3::NumPy)
else()
    target_compile_definitions(matplotlib_cpp INTERFACE WITHOUT_NUMPY)
endif()

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
