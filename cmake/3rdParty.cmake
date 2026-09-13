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

# Add bundled MuJoCo physics library.
# MuJoCo 实际只在仿真模式下被使用：HardwareFactory 中创建 MujocoSimulation/MujocoBus 的逻辑
# 位于 #if defined(SIMULATION) 内，而 realtime 模式不定义 SIMULATION；zrcs_rt/controller/mujoco/*
# 在未启用时会走 MujocoSimulation.cpp 末尾的 #else 桩实现。因此实时模式默认不编译 MuJoCo，
# 可省下下位机上相当可观的一段时间。确需在实时模式下打开时显式传 -DZRCS_ENABLE_MUJOCO=ON。
set(_zrcs_mujoco_default ON)
if(BUILD_MODE STREQUAL "realtime")
    set(_zrcs_mujoco_default OFF)
endif()
option(ZRCS_ENABLE_MUJOCO "Build bundled MuJoCo support" ${_zrcs_mujoco_default})
unset(_zrcs_mujoco_default)
if(ZRCS_ENABLE_MUJOCO)
    set(ZRCS_MUJOCO_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/mujoco-main")
    if(NOT EXISTS "${ZRCS_MUJOCO_SOURCE_DIR}/CMakeLists.txt")
        message(FATAL_ERROR "ZRCS_ENABLE_MUJOCO is ON, but ${ZRCS_MUJOCO_SOURCE_DIR} is missing.")
    endif()

    set(MUJOCO_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(MUJOCO_BUILD_SIMULATE OFF CACHE BOOL "" FORCE)
    set(MUJOCO_BUILD_STUDIO OFF CACHE BOOL "" FORCE)
    set(MUJOCO_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(MUJOCO_TEST_PYTHON_UTIL OFF CACHE BOOL "" FORCE)
    set(MUJOCO_WITH_USD OFF CACHE BOOL "" FORCE)
    set(MUJOCO_USE_FILAMENT OFF CACHE BOOL "" FORCE)
    set(MUJOCO_ENABLE_AVX OFF CACHE BOOL "" FORCE)

    add_subdirectory("${ZRCS_MUJOCO_SOURCE_DIR}")

    if(NOT TARGET mujoco::mujoco)
        message(FATAL_ERROR "Bundled MuJoCo did not define target mujoco::mujoco.")
    endif()

    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(mujoco PRIVATE -Wno-error)
    endif()

    get_target_property(ZRCS_MUJOCO_TARGET_TYPE mujoco TYPE)
    if(ZRCS_MUJOCO_TARGET_TYPE STREQUAL "STATIC_LIBRARY")
        target_compile_definitions(mujoco PUBLIC MJ_STATIC)
    elseif(TARGET render_noop)
        # render_noop provides local mjr_* stubs; it must not inherit dllimport
        # declarations when MuJoCo itself is built as a shared library.
        target_compile_definitions(render_noop PUBLIC MJ_STATIC)
    endif()

    foreach(ZRCS_MUJOCO_PLUGIN_TARGET elasticity actuator sensor sdf_plugin)
        if(TARGET ${ZRCS_MUJOCO_PLUGIN_TARGET})
            set_target_properties(${ZRCS_MUJOCO_PLUGIN_TARGET} PROPERTIES
                EXCLUDE_FROM_ALL TRUE
                EXCLUDE_FROM_DEFAULT_BUILD TRUE
            )
        endif()
    endforeach()

    add_library(zrcs_mujoco INTERFACE)
    add_library(zrcs::mujoco ALIAS zrcs_mujoco)
    target_link_libraries(zrcs_mujoco INTERFACE mujoco::mujoco)
    target_compile_definitions(zrcs_mujoco INTERFACE ZRCS_HAS_MUJOCO=1)
    message(STATUS "Using bundled MuJoCo from ${ZRCS_MUJOCO_SOURCE_DIR}")
endif()

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

# 添加 Eigen3 线性代数库（随工程分发，避免两平台版本差异）
# 必要性：zrcs_rt 的 MoveCurve / CartesianRobot 用到 Eigen 5 才有的 canonicalEulerAngles，
# 而 Ubuntu 22.04 的 apt 只提供 Eigen 3.4（其 eulerAngles 不保证规范形，不能直接替换）。
# 故统一使用 3rdParty/eigen。纯头文件库，无需 add_subdirectory。
# 导出 Eigen3::Eigen 目标名，既有引用（zrcs_rt / test）无需改动。
set(ZRCS_EIGEN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/eigen")
if(NOT EXISTS "${ZRCS_EIGEN_DIR}/Eigen/Core")
    message(FATAL_ERROR
        "Bundled Eigen not found at ${ZRCS_EIGEN_DIR}.\n"
        "Please run: cd 3rdParty && git clone --depth 1 --branch 5.0.0 https://gitlab.com/libeigen/eigen.git eigen")
endif()
add_library(zrcs_eigen INTERFACE)
add_library(Eigen3::Eigen ALIAS zrcs_eigen)
target_include_directories(zrcs_eigen INTERFACE ${ZRCS_EIGEN_DIR})
# Eigen 作为三方库，其头文件告警不应污染工程自身的告警输出
set_property(TARGET zrcs_eigen APPEND PROPERTY INTERFACE_SYSTEM_INCLUDE_DIRECTORIES ${ZRCS_EIGEN_DIR})
target_compile_features(zrcs_eigen INTERFACE cxx_std_17)
message(STATUS "Using bundled Eigen3 from ${ZRCS_EIGEN_DIR}")

# 添加 BehaviorTree.CPP（zrcsnrt 核心依赖，所有模式都需要）
# 注意：这些 BUILD_* 为通用缓存变量，故置于本文件末尾，避免影响上方其他库
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(BUILD_MANUAL_SELECTOR OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_SOURCE_DIR}/3rdParty/Groot/depend/BehaviorTree.CPP)

# gpr：G-code 词法/句法解析（仅 parser 库，不编其自带 test/exe）
add_library(gpr STATIC
    ${CMAKE_SOURCE_DIR}/3rdParty/gpr/src/parser.cpp
    ${CMAKE_SOURCE_DIR}/3rdParty/gpr/src/gcode_program.cpp
)
add_library(gpr::gpr ALIAS gpr)
target_include_directories(gpr PUBLIC ${CMAKE_SOURCE_DIR}/3rdParty/gpr/src)
target_compile_features(gpr PUBLIC cxx_std_11)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # gpr 源码有若干未使用/有符号比较告警，作三方库不升为 error
    target_compile_options(gpr PRIVATE -Wno-error -Wno-unused-parameter -Wno-sign-compare)
endif()
message(STATUS "Using bundled gpr from 3rdParty/gpr")
