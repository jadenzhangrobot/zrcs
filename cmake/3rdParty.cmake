# 设置CMake策略以处理Boost库查找
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

#添加tinyxml2库编译
# 检查 tinyxml2 目录是否存在
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/tinyxml2")

#添加ruckig库编译
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/ruckig)
# 强制 ruckig 生成静态库
set(BUILD_SHARED_LIBS OFF)

# 添加boost库
# 优先使用BoostConfig.cmake，如果不可用则回退到FindBoost.cmake
find_package(Boost QUIET CONFIG)
if(NOT Boost_FOUND)
    find_package(Boost REQUIRED COMPONENTS system)
endif()
#添加线程库
find_package(Threads REQUIRED)

# 添加CoppeliaSim的头文件



