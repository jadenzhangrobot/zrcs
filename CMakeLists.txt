cmake_minimum_required(VERSION 3.22.1)
project(zrcs)
# 添加生成文件的自定义命令到构建过程中
add_custom_target(generate_file_target DEPENDS generated_file.txt)

#add_compile_definitions(Ros)
# 设置 C++ 标准为 C++17
set(CMAKE_CXX_STANDARD 17)

# 强制要求编译器支持 C++17
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# 设置 G++ 编译器路径
#set(CMAKE_CXX_COMPILER "/usr/bin/g++-9")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# 添加调试编译选项
option(realtime "Build realtime programs" ON)
message("realtime ${realtime}")
option(ethercat "Build ethercat programs" ON)
message("ethercat ${ethercat}")
      #编译链接xenomai的库
      if(realtime)
      set(xeno_cflags_params "--alchemy"  "--cflags")
      execute_process(COMMAND /usr/xenomai/bin/xeno-config ${xeno_cflags_params} OUTPUT_VARIABLE xeno_cflags OUTPUT_STRIP_TRAILING_WHITESPACE)

      set(xeno_ldflags_params "--alchemy"  "--ldflags" "--no-auto-init")
      execute_process(COMMAND /usr/xenomai/bin/xeno-config ${xeno_ldflags_params} OUTPUT_VARIABLE xeno_ldflags OUTPUT_STRIP_TRAILING_WHITESPACE)

      set(xeno_g++_params "--cc")
      execute_process(COMMAND /usr/xenomai/bin/xeno-config ${xeno_g++_params} OUTPUT_VARIABLE xeno_g++)
      set(CMAKE_C_FLAGS          "${CMAKE_CXX_FLAGS} ${xeno_cflags}")
      set(CMAKE_CXX_FLAGS        "${CMAKE_CXX_FLAGS} ${xeno_cflags} -fpermissive")
      set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${xeno_ldflags}")
      set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${xeno_ldflags}")
      endif()
set(DEVEL_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR})
include_directories( 
   /usr/local/include
   /usr/include
   /opt/etherlab/include
   ${DEVEL_INCLUDE_DIRS}/include
   ${DEVEL_INCLUDE_DIRS}/3rdParty/libzmq/include
   ${DEVEL_INCLUDE_DIRS}/3rdParty/tinyxml2
   ${DEVEL_INCLUDE_DIRS}/3rdParty/ruckig/include
   ${DEVEL_INCLUDE_DIRS}/build/message
 )
 include(${DEVEL_INCLUDE_DIRS}/cmake/protobuf.cmake) 
# 添加tinyxml2库
add_subdirectory(${DEVEL_INCLUDE_DIRS}/3rdParty/tinyxml2)
# 添加ruckig库
add_subdirectory(${DEVEL_INCLUDE_DIRS}/3rdParty/ruckig)
# 添加zmq库
add_subdirectory(${DEVEL_INCLUDE_DIRS}/3rdParty/libzmq)
# include(${DEVEL_INCLUDE_DIRS}/cmake/libzmq.cmake)
# ExternalAutotools(libzmq "${CMAKE_BINARY_DIR}/build_autotools" "${CMAKE_BINARY_DIR}/3rdParty")

link_directories(/usr/local/lib)
link_directories(/opt/etherlab/lib/)
link_directories(${DEVEL_INCLUDE_DIRS}/build/3rdParty/libzmq/lib)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/build/message PROTO_FILE)
aux_source_directory(./src SOURCE_FILES)
add_executable(motion_control ${SOURCE_FILES} ${PROTO_FILE})
 if(ethercat)
 target_link_libraries(motion_control
 PRIVATE 
 pthread
 ruckig
 ethercat_rtdm
 tinyxml2
 zmq
 protobuf::libprotobuf
)
else()
target_link_libraries(motion_control
PRIVATE 
pthread
ruckig
tinyxml2
zmq
protobuf::libprotobuf
)
endif()
 
