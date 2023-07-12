cmake_minimum_required(VERSION 3.0.2)
project(motion-control)

#add_compile_options(-std=c++17)
#add_compile_options(c++17)
set(CMAKE_CXX_STANDARD 17)
set(xeno_cflags_params "--alchemy"  "--cflags")
execute_process(COMMAND /usr/xenomai/bin/xeno-config ${xeno_cflags_params} OUTPUT_VARIABLE xeno_cflags OUTPUT_STRIP_TRAILING_WHITESPACE)

set(xeno_ldflags_params "--alchemy"  "--ldflags")
execute_process(COMMAND /usr/xenomai/bin/xeno-config ${xeno_ldflags_params} OUTPUT_VARIABLE xeno_ldflags OUTPUT_STRIP_TRAILING_WHITESPACE)

set(xeno_g++_params "--cc")
execute_process(COMMAND /usr/xenomai/bin/xeno-config ${xeno_g++_params} OUTPUT_VARIABLE xeno_g++)
set(CMAKE_C_FLAGS          "${CMAKE_CXX_FLAGS} ${xeno_cflags}")
set(CMAKE_CXX_FLAGS        "${CMAKE_CXX_FLAGS} ${xeno_cflags} -fpermissive")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${xeno_ldflags}")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${xeno_ldflags}")
include_directories(
  include
  /opt/ros/noetic/include
 # ${PROJECT_SOURCE_DIR}/lib/log4cplus/include
  #${PROJECT_SOURCE_DIR}/lib/zmq
#  ${PROJECT_SOURCE_DIR}/lib/ruckig/include
)
find_package(catkin REQUIRED COMPONENTS
  #trac_ik_lib
  roscpp
  rospy
)





aux_source_directory(./src SOURCE_FILES)
add_executable(motion_control ${SOURCE_FILES})

# 主要作用是来选择是链接x86库还是arm64库
#if(CMAKE_SYSTEM_PROCESSOR STREQUAL arm64)  
#  find_library(logUtils logUtils HINTS ${PROJECT_SOURCE_DIR}/lib/rk)
#  find_library(zmq zmq HINTS ${PROJECT_SOURCE_DIR}/lib/rk)
#  find_library(ruckig ruckig HINTS ${PROJECT_SOURCE_DIR}/lib/ruckig/arm64)
#elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL x86)
  find_library(logUtils logUtils HINTS ${PROJECT_SOURCE_DIR}/lib/rk)
  find_library(zmq zmq HINTS ${PROJECT_SOURCE_DIR}/lib/rk)
  find_library(ruckig ruckig HINTS ${PROJECT_SOURCE_DIR}/lib/ruckig/x86)
#else()
 #   message("Please select a system architecture")
#endif()
#find_library(ruckig ruckig HINTS /usr/local/lib)
 target_link_libraries(motion_control
   ${ruckig}
   pthread
   zmq
   #behaviortree_cpp_v3
   ${catkin_LIBRARIES}
 )