cmake_minimum_required(VERSION 3.1.2)
project(zrcs)
#add_compile_definitions(Ros)
set(CMAKE_CXX_STANDARD 17)
# 添加调试编译选项
set(VARNAME realtime)
set(VARNAME debug)
set(VARNAME platform)
set(VARNAME ethercat)
      #选择debug模式
      if(${debug})
          set(CMAKE_BUILD_TYPE Debug)
          set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
      endif()
      #编译链接xenomai的库
      if( ${realtime})
      add_compile_definitions(xenomai=1)
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


# 添加ruckig头文件和库
include_directories(${DEVEL_INCLUDE_DIRS}/3rdParty/ruckig/include)

if(platform STREQUAL "amd64")
  link_directories(${DEVEL_INCLUDE_DIRS}/3rdParty/ruckig/lib/amd64)
  link_directories(${DEVEL_INCLUDE_DIRS}/3rdParty/tinyxml2/lib/amd64)
elseif(platform STREQUAL "arm64")
  link_directories(${DEVEL_INCLUDE_DIRS}/3rdParty/ruckig/lib/arm64)
  link_directories(${DEVEL_INCLUDE_DIRS}/3rdParty/tinyxml2/lib/arm64)
else()
    message("选择编译的平台")
endif()

include_directories( 
  /usr/local/include
  /usr/include
  include                       
)

#set(ruckig_LIBRARIES /usr/local/liblibruckig.so)
aux_source_directory(./src SOURCE_FILES)
add_executable(motion_control ${SOURCE_FILES})
 if(${ethercat} )
 add_compile_definitions(ethercat=1)
 target_link_libraries(motion_control
 pthread
 ruckig
 ethercat_rtdm
 tinyxml2
)
else()
target_link_libraries(motion_control
pthread
ruckig
tinyxml2
)
endif()
 
