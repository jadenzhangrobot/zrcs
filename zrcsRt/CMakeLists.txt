project(zrcsrt)

# 查找依赖库
find_package(Threads REQUIRED)
find_package(Protobuf REQUIRED)

# 包含目录
include_directories(
    ${CMAKE_SOURCE_DIR}/zrcsCommon/include # Added for common/sharedMemory headers
    ${CMAKE_SOURCE_DIR}/zrcsRt/include
    ${CMAKE_BINARY_DIR} # For generated proto headers
)

# 源文件 (暂时假设使用 zrcsRt/main.cpp)
# 之前的目录结构显示 zrcsRt 下有 main.cpp
file(GLOB_RECURSE SOURCES 
    "main.cpp"
    "Trajectory_planning/*.cpp"
    # 如果还有其他源文件，添加在这里
)

add_executable(zrcsrt ${SOURCES} ${PROTO_SRCS} ${PROTO_HDRS})

# 链接库
target_link_libraries(zrcsrt PRIVATE 
    Threads::Threads
    ruckig
    tinyxml2
    atomic
    protobuf::libprotobuf
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
