cmake_minimum_required(VERSION 3.22.1)
project(zrcs)
# Add this near the top of your CMakeLists.txt, after project()
find_package(Python3 REQUIRED)

# Add custom command to execute Python scripts
add_custom_target(run_python_scripts ALL
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tool/config.py
    # Add more scripts as needed:
    # COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tool/your_script2.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Executing Python scripts from tool folder"
)

# 添加生成文件的自定义命令到构建过程中
add_custom_target(generate_file_target DEPENDS generated_file.txt)

#add_compile_definitions(Ros)
# 设置 C++ 标准为 C++17
set(CMAKE_CXX_STANDARD 17)

# 强制要求编译器支持 C++17
set(CMAKE_CXX_STANDARD_REQUIRED ON)



# 设置 G++ 编译器路径

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# 添加调试编译选项
option(realtime "Build realtime programs" ON)
message("realtime ${realtime}")
option(test "Build test programs" ON)
#添加单元测试程序
if(test)
  add_subdirectory(example/terminal)
  add_subdirectory(example/slab)
endif(test)
 

      #编译链接xenomai的库
 if(realtime)
      add_definitions(-DREALTIME)
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
set(DEVEL_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR})
include_directories( 
   /usr/local/include
   /usr/include
   /opt/etherlab/include
   ${DEVEL_INCLUDE_DIRS}/include
   ${DEVEL_INCLUDE_DIRS}/3rdParty/libzmq/include
   ${DEVEL_INCLUDE_DIRS}/3rdParty/tinyxml2
   ${DEVEL_INCLUDE_DIRS}/3rdParty/ruckig/include
 )
 find_package(Boost REQUIRED COMPONENTS system)
# 添加CoppeliaSim的头文件
if(NOT realtime)
    if(WIN32)
        include_directories("C:/Program Files/CoppeliaRobotics/CoppeliaSimEdu/programming/legacyRemoteApi/remoteApi")
        include_directories("C:/Program Files/CoppeliaRobotics/CoppeliaSimEdu/programming/include")
        link_directories("C:/Program Files/CoppeliaRobotics/CoppeliaSimEdu/programming/legacyRemoteApi/remoteApiBindings/lib/lib/Windows")
    elseif(UNIX)
        include_directories("/home/zrcs/Downloads/CoppeliaSim_Edu_V4_10_0_rev0_Ubuntu24_04/programming/legacyRemoteApi/remoteApi")
        include_directories("/home/zrcs/Downloads/CoppeliaSim_Edu_V4_10_0_rev0_Ubuntu24_04/programming/include")
        link_directories(/home/zrcs/Downloads/CoppeliaSim_Edu_V4_10_0_rev0_Ubuntu24_04/programming/legacyRemoteApi/remoteApiBindings/lib/lib/Ubuntu20_04)
    endif()
endif()

# 添加tinyxml2库
add_subdirectory(${DEVEL_INCLUDE_DIRS}/3rdParty/tinyxml2)
# 强制 ruckig 生成静态库
set(BUILD_SHARED_LIBS OFF)
# 添加ruckig库
add_subdirectory(${DEVEL_INCLUDE_DIRS}/3rdParty/ruckig)
find_package(Threads REQUIRED)
link_directories(/usr/local/lib)
link_directories(/opt/etherlab/lib/)

link_directories(${DEVEL_INCLUDE_DIRS}/build/3rdParty/ruckig)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/build/message PROTO_FILE)
aux_source_directory(./src SOURCE_FILES)
add_executable(motion_control ${SOURCE_FILES} ${PROTO_FILE})

# Make your main target depend on the Python scripts
add_dependencies(motion_control run_python_scripts)

 if(realtime)
 target_link_libraries(motion_control
 PRIVATE 
 Threads::Threads
 ruckig
 ethercat_rtdm
 tinyxml2
 atomic
)
else()
target_link_libraries(motion_control
PRIVATE 
Threads::Threads
ruckig
tinyxml2
atomic
remoteApi
#fakeethercat
)
endif()

