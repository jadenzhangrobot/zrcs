cmake_minimum_required(VERSION 3.22.1)
project(zrcs-test)

set(CMAKE_CXX_STANDARD 17)
# 强制要求编译器支持 C++17
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(DEVEL_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/..)
include_directories(${DEVEL_INCLUDE_DIRS}/include
                    ${DEVEL_INCLUDE_DIRS}/3rdParty/tinyxml2                   
)

add_executable(ethercat ethercat.cpp)
target_link_libraries(ethercat
PRIVATE
tinyxml2
fakeethercat
)


add_executable(parameter parameter.cpp)
target_link_libraries(parameter
PRIVATE 
tinyxml2
)