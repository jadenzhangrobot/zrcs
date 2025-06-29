cmake_minimum_required(VERSION 3.22.1)
project(zrcs-test)

set(CMAKE_CXX_STANDARD 17)
# 强制要求编译器支持 C++17
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(DEVEL_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/..)

include_directories(${DEVEL_INCLUDE_DIRS}/include
                    ${DEVEL_INCLUDE_DIRS}/3rdParty/tinyxml2                   
)

#       set(xeno_cflags_params "--alchemy"  "--cflags")
#       execute_process(COMMAND /usr/xenomai/bin/xeno-config ${xeno_cflags_params} OUTPUT_VARIABLE xeno_cflags OUTPUT_STRIP_TRAILING_WHITESPACE)

#       set(xeno_ldflags_params "--alchemy"  "--ldflags" )
#       execute_process(COMMAND /usr/xenomai/bin/xeno-config ${xeno_ldflags_params} OUTPUT_VARIABLE xeno_ldflags OUTPUT_STRIP_TRAILING_WHITESPACE)

#       set(xeno_g++_params "--cc")
#       execute_process(COMMAND /usr/xenomai/bin/xeno-config ${xeno_g++_params} OUTPUT_VARIABLE xeno_g++)
#       set(CMAKE_C_FLAGS          "${CMAKE_CXX_FLAGS} ${xeno_cflags}")
#       set(CMAKE_CXX_FLAGS        "${CMAKE_CXX_FLAGS} ${xeno_cflags} -fpermissive")
#       set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${xeno_ldflags}")
#       set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${xeno_ldflags}")

# add_executable(ethercat ethercat.cpp)
# target_link_libraries(ethercat
# PRIVATE
# tinyxml2
# ethercat_rtdm
# )


add_executable(parameter parameter.cpp)
target_link_libraries(parameter
PRIVATE 
tinyxml2
)