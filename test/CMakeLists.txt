cmake_minimum_required(VERSION 3.18.1)
project(zrcs-test)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(DEVEL_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/..)

include_directories(
    ${DEVEL_INCLUDE_DIRS}/zrcs_common/config
    ${DEVEL_INCLUDE_DIRS}/zrcs_common
    ${DEVEL_INCLUDE_DIRS}/zrcs_common/shared_memory
    ${DEVEL_INCLUDE_DIRS}/3rdParty/cereal/cereal-master/include
    ${DEVEL_INCLUDE_DIRS}/3rdParty/tinyxml2
)

# SPSC 环形缓冲区单元测试
add_executable(test_spsc test_spsc.cpp)
target_link_libraries(test_spsc PRIVATE atomic)

# Command 结构体安全性测试
add_executable(test_command test_command.cpp)
target_link_libraries(test_command PRIVATE atomic)

add_executable(test_config_manager test_config_manager.cpp)
target_link_libraries(test_config_manager PRIVATE zrcscommon tinyxml2)

add_executable(test_axis_direction
    test_axis_direction.cpp
    ${DEVEL_INCLUDE_DIRS}/zrcs_rt/controller/ControllerInterface.cpp
)
target_include_directories(test_axis_direction PRIVATE
    ${DEVEL_INCLUDE_DIRS}/zrcs_rt
)
target_link_libraries(test_axis_direction PRIVATE atomic)

add_executable(test_motion_preprocessing test_motion_preprocessing.cpp)
target_include_directories(test_motion_preprocessing PRIVATE
    ${DEVEL_INCLUDE_DIRS}/zrcs_nrt
    ${DEVEL_INCLUDE_DIRS}/3rdParty/magic_enum/include
)
target_compile_definitions(test_motion_preprocessing PRIVATE WITHOUT_NUMPY)
target_link_libraries(test_motion_preprocessing PRIVATE
    matplotlib_cpp::matplotlib_cpp
    spdlog::spdlog
)

# ZMQ 通信层测试（Protobuf + ZMQ REQ/REP）
add_executable(test_zmq_comm test_zmq_comm.cpp)
target_link_libraries(test_zmq_comm PRIVATE
    zrcs::proto
    cppzmq
)
