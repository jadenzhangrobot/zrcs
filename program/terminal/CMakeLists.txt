cmake_minimum_required(VERSION 3.10)
project(teminnal)

# 设置CMake策略以处理Boost库查找
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

# 设置C++标准为C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置编译选项
if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")
endif()

set(SOURCES
    main.cpp 
)


# 创建可执行文件
add_executable(${PROJECT_NAME} ${SOURCES} )
 
# 查找Boost库
# 优先使用BoostConfig.cmake，如果不可用则回退到FindBoost.cmake
find_package(Boost QUIET CONFIG)
if(NOT Boost_FOUND)
    find_package(Boost REQUIRED)
endif()
if(Boost_FOUND)
    target_include_directories(${PROJECT_NAME} PRIVATE ${Boost_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES} atomic rt Threads::Threads)
endif()

# 设置输出目录
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)