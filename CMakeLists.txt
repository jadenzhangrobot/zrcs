cmake_minimum_required(VERSION 3.1.2)
project(zrcs)
#add_compile_definitions(Ros)
# 设置 C++ 标准为 C++17
set(CMAKE_CXX_STANDARD 17)

# 强制要求编译器支持 C++17
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置 G++ 编译器路径
#set(CMAKE_CXX_COMPILER "/usr/bin/g++-9")
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
# 添加调试编译选项
option(realtime "Build example programs" ON)
option(ethercat "Build example programs" ON)
      #编译链接xenomai的库
      if(realtime)
      message("realtime ${realtime}")
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
add_subdirectory(${DEVEL_INCLUDE_DIRS}/3rdParty/ruckig)
# 添加tinyxml头文件
include_directories(${DEVEL_INCLUDE_DIRS}/3rdParty/tinyxml2/include)
# 添加open62541头文件
include_directories(${DEVEL_INCLUDE_DIRS}/3rdParty/opcua/include)
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
   link_directories(${DEVEL_INCLUDE_DIRS}/3rdParty/tinyxml2/lib/amd64)
   link_directories(${DEVEL_INCLUDE_DIRS}/3rdParty/opcua/lib/amd64)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
   link_directories(${DEVEL_INCLUDE_DIRS}/3rdParty/tinyxml2/lib/arm64)
   link_directories(${DEVEL_INCLUDE_DIRS}/3rdParty/opcua/lib/arm64)
else()
    message("芯片的架构不符合要求")
endif()

include_directories( 
  /usr/local/include
  /usr/include
  /opt/etherlab/include
  include
)
link_directories(/opt/etherlab/lib/)

#set(ruckig_LIBRARIES /usr/local/liblibruckig.so)
aux_source_directory(./src SOURCE_FILES)
add_executable(motion_control ${SOURCE_FILES})
 if(ethercat)
 target_link_libraries(motion_control
 pthread
 ruckig
 ethercat_rtdm
 tinyxml2
 open62541
)
else()
target_link_libraries(motion_control
pthread
ruckig
tinyxml2
open62541
)
endif()
 
