project(zrcsnrt)

# 查找依赖库
find_package(Threads REQUIRED)
find_package(Protobuf REQUIRED)

# 包含目录
include_directories(
    ${CMAKE_SOURCE_DIR}/zrcsCommon/include
    ${CMAKE_SOURCE_DIR}/zrcsNrt
    ${CMAKE_BINARY_DIR}
)

# 源文件
file(GLOB_RECURSE SOURCES 
    "main.cpp"
)

add_executable(zrcsnrt ${SOURCES} ${PROTO_SRCS} ${PROTO_HDRS})

# 链接库
target_link_libraries(zrcsnrt PRIVATE 
    Threads::Threads
    ruckig
    tinyxml2
    atomic
    protobuf::libprotobuf
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
