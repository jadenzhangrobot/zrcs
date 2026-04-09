project(zrcsrt)

# 查找依赖库
find_package(Threads REQUIRED)
find_package(Protobuf REQUIRED)

# 包含目录
include_directories(
    ${CMAKE_SOURCE_DIR}/zrcs_common/include # Added for common/sharedMemory headers
    ${CMAKE_SOURCE_DIR}/zrcs_rt/include
    ${CMAKE_BINARY_DIR} # For generated proto headers
)

# 源文件 (暂时假设使用 zrcs_rt/main.cpp)
# 之前的目录结构显示 zrcs_rt 下有 main.cpp
file(GLOB_RECURSE SOURCES
    "main.cpp"
    "Trajectory_planning/*.cpp"
    "src/*.cpp"
)

set(PROTO_FILES
    ${CMAKE_SOURCE_DIR}/zrcs_common/message/message.proto
)
protobuf_generate_cpp(ZRCSRT_PROTO_SRCS ZRCSRT_PROTO_HDRS ${PROTO_FILES})

add_executable(zrcsrt ${SOURCES} ${ZRCSRT_PROTO_SRCS} ${ZRCSRT_PROTO_HDRS})
target_include_directories(zrcsrt PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

# 链接库
target_link_libraries(zrcsrt PRIVATE
    Threads::Threads
    ruckig
    tinyxml2
    atomic
    protobuf::libprotobuf
    Eigen3::Eigen
    ${MOTION_CONTROL_LIBS}
)

if(WIN32)
    set_target_properties(zrcsrt PROPERTIES LINK_FLAGS "-mconsole")
endif()

install(TARGETS zrcsrt
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
