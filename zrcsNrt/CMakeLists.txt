project(zrcsnrt)

# 查找依赖库
find_package(Threads REQUIRED)
find_package(Protobuf REQUIRED)

# 包含目录
include_directories(
    ${CMAKE_SOURCE_DIR}/zrcsCommon/include # Added for common/sharedMemory headers
    ${CMAKE_SOURCE_DIR}/zrcsNrt
    ${CMAKE_BINARY_DIR} # For generated proto headers
)

# 源文件 (暂时假设使用 src/main.cpp 作为入口，或者在 zrcsNrt 目录下有自己的 main)
# 如果 zrcsNrt 目录为空，我们需要创建或指定源文件
# 暂时复用 src/main.cpp，但建议将 src/main.cpp 移动到这里或创建新的
set(SOURCES
    ${CMAKE_SOURCE_DIR}/src/main.cpp
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
