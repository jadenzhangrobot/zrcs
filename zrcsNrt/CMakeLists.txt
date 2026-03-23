project(zrcsnrt)

# 查找依赖库
find_package(Threads REQUIRED)
find_package(Protobuf REQUIRED)
find_package(cppzmq REQUIRED)

# 包含目录
include_directories(
    ${CMAKE_SOURCE_DIR}/zrcsCommon/include
    ${CMAKE_SOURCE_DIR}/zrcsNrt
    ${CMAKE_SOURCE_DIR}/3rdParty/Groot/depend/BehaviorTree.CPP/include
    ${CMAKE_SOURCE_DIR}/3rdParty/spdlog/include
    ${CMAKE_BINARY_DIR}
)

# 源文件
file(GLOB_RECURSE SOURCES 
    "main.cpp"
)

set(PROTO_FILES
    ${CMAKE_SOURCE_DIR}/zrcsCommon/message/message.proto
)
protobuf_generate_cpp(ZRCSNRT_PROTO_SRCS ZRCSNRT_PROTO_HDRS ${PROTO_FILES})

add_executable(zrcsnrt ${SOURCES} ${ZRCSNRT_PROTO_SRCS} ${ZRCSNRT_PROTO_HDRS})
target_include_directories(zrcsnrt PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

# 链接库
target_link_libraries(zrcsnrt PRIVATE
    Threads::Threads
    ruckig
    tinyxml2
    atomic
    protobuf::libprotobuf
    cppzmq
    behaviortree_cpp_v3
    spdlog::spdlog
    ${MOTION_CONTROL_LIBS}
)

if(WIN32)
    set_target_properties(zrcsnrt PROPERTIES LINK_FLAGS "-mconsole")
endif()

install(TARGETS zrcsnrt
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
