cmake_minimum_required(VERSION 3.1.2)
project(zrcs)
#add_compile_definitions(Ros)
set(CMAKE_CXX_STANDARD 17)
# 添加调试编译选项
set(Debug OFF)
      if(Debug)
          set(CMAKE_BUILD_TYPE Debug)
          set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
      endif()
set(xenomai ON)
      if(xenomai)
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
      endif()
link_directories(/usr/local/lib)
set(DEVEL_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR})
# 添加tinyxml头文件和库
include_directories(${DEVEL_INCLUDE_DIRS}/3rdParty/tinyxml2/include)
link_directories(${DEVEL_INCLUDE_DIRS}/3rdParty/tinyxml2/lib)


include_directories( 
  /usr/local/include
  /usr/include
  include                       
)
#set(ruckig_LIBRARIES /usr/local/liblibruckig.so)
aux_source_directory(./src SOURCE_FILES)
add_executable(motion_control ${SOURCE_FILES})
 target_link_libraries(motion_control
   pthread
   ruckig
   zmq
   ethercat_rtdm
   tinyxml2
 )