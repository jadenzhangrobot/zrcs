project(zrcsrt)

# 查找依赖库
find_package(Threads REQUIRED)
find_package(Protobuf REQUIRED)
find_package(Python3 REQUIRED COMPONENTS Interpreter)

# ── 命令注册表代码生成 ──────────────────────────────────────────
set(CMD_GEN_DIR    ${CMAKE_BINARY_DIR}/generated)
set(CMD_GEN_SCRIPT ${CMAKE_SOURCE_DIR}/tool/gen_cmd_registry.py)
set(CMD_SRC_DIR    ${CMAKE_SOURCE_DIR}/zrcs_rt/command)

file(GLOB CMD_HEADERS ${CMD_SRC_DIR}/*.h)

add_custom_command(
    OUTPUT
        ${CMD_GEN_DIR}/CmdRegistry_gen.h
        ${CMD_GEN_DIR}/CmdHead_gen.h
        ${CMD_GEN_DIR}/CmdNameToId_gen.h
        ${CMD_GEN_DIR}/message_gen.proto
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMD_GEN_DIR}
    COMMAND ${Python3_EXECUTABLE} ${CMD_GEN_SCRIPT}
        --src-dir ${CMD_SRC_DIR}
        --out-dir ${CMD_GEN_DIR}
    DEPENDS ${CMD_HEADERS} ${CMD_GEN_SCRIPT}
    COMMENT "Generating command registry from CMD_DEFINE declarations"
)
add_custom_target(cmd_gen DEPENDS
    ${CMD_GEN_DIR}/CmdRegistry_gen.h
    ${CMD_GEN_DIR}/CmdHead_gen.h
    ${CMD_GEN_DIR}/CmdNameToId_gen.h
    ${CMD_GEN_DIR}/message_gen.proto
)

# 包含目录
include_directories(
    ${CMAKE_SOURCE_DIR}/zrcs_rt
    ${CMAKE_BINARY_DIR}
)

# 源文件 (暂时假设使用 zrcs_rt/main.cpp)
# 之前的目录结构显示 zrcs_rt 下有 main.cpp
file(GLOB_RECURSE SOURCES
    "main.cpp"
    "command/*.cpp"
    "controller/*.cpp"
    "model/*.cpp"
    "system/*.cpp"
)

set(PROTO_FILES
    ${CMAKE_SOURCE_DIR}/zrcs_common/message/message.proto
)
protobuf_generate_cpp(ZRCSRT_PROTO_SRCS ZRCSRT_PROTO_HDRS ${PROTO_FILES})

add_executable(zrcsrt ${SOURCES} ${ZRCSRT_PROTO_SRCS} ${ZRCSRT_PROTO_HDRS})
add_dependencies(zrcsrt cmd_gen)
target_include_directories(zrcsrt PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

# 链接库
target_link_libraries(zrcsrt PRIVATE
    zrcscommon
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
