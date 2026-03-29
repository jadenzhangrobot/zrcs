cmake_minimum_required(VERSION 3.18.1)
project(zrcs-test)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(DEVEL_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/..)

include_directories(
    ${DEVEL_INCLUDE_DIRS}/zrcsCommon/include
    ${DEVEL_INCLUDE_DIRS}/3rdParty/tinyxml2
)

# 原有的参数测试
add_executable(parameter parameter.cpp)
target_link_libraries(parameter PRIVATE tinyxml2)

# SPSC 环形缓冲区单元测试
add_executable(test_spsc test_spsc.cpp)
target_link_libraries(test_spsc PRIVATE atomic)

# Command 结构体安全性测试
add_executable(test_command test_command.cpp)
target_link_libraries(test_command PRIVATE atomic)

# ZMQ 通信层测试（Protobuf + ZMQ REQ/REP）
find_package(Protobuf REQUIRED)

set(ZMQ_TEST_PROTO ${DEVEL_INCLUDE_DIRS}/zrcsCommon/message/message.proto)
protobuf_generate_cpp(ZMQ_TEST_PROTO_SRCS ZMQ_TEST_PROTO_HDRS ${ZMQ_TEST_PROTO})

add_executable(test_zmq_comm test_zmq_comm.cpp ${ZMQ_TEST_PROTO_SRCS} ${ZMQ_TEST_PROTO_HDRS})
target_include_directories(test_zmq_comm PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(test_zmq_comm PRIVATE
    protobuf::libprotobuf
    cppzmq
)
