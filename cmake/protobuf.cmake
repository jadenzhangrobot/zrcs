find_package(Protobuf 3 REQUIRED)

#设置输出路径
SET(MESSAGE_DIR ${CMAKE_BINARY_DIR}/message)
if(EXISTS "${CMAKE_BINARY_DIR}/message" AND IS_DIRECTORY "${CMAKE_BINARY_DIR}/message")
        SET(PROTO_META_BASE_DIR ${MESSAGE_DIR})
else()
        file(MAKE_DIRECTORY ${MESSAGE_DIR})
        SET(PROTO_META_BASE_DIR ${MESSAGE_DIR})
endif()

#设置protoc的搜索路径
LIST(APPEND PROTO_FLAGS -I${CMAKE_SOURCE_DIR}/message)
#获取需要编译的proto文件
file(GLOB_RECURSE MSG_PROTOS ${CMAKE_SOURCE_DIR}/message/*.proto)
set(MESSAGE_SRC "")
set(MESSAGE_HDRS "")

foreach(msg ${MSG_PROTOS})
        get_filename_component(FIL_WE ${msg} NAME_WE)

        list(APPEND MESSAGE_SRC "${PROJECT_BINARY_DIR}/message/${FIL_WE}.pb.cc")
        list(APPEND MESSAGE_HDRS "${PROJECT_BINARY_DIR}/message/${FIL_WE}.pb.h")

		# 使用自定义命令
        add_custom_command(
          OUTPUT "${PROJECT_BINARY_DIR}/message/${FIL_WE}.pb.cc"
                 "${PROJECT_BINARY_DIR}/message/${FIL_WE}.pb.h"
          COMMAND  ${PROTOBUF_PROTOC_EXECUTABLE}
          ARGS --cpp_out  ${PROTO_META_BASE_DIR}
            -I ${CMAKE_SOURCE_DIR}/message
            ${msg}
          DEPENDS ${msg}
          COMMENT "Running C++ protocol buffer compiler on ${msg}"
          VERBATIM
        )
        
endforeach()

# 设置文件属性为 GENERATED
set_source_files_properties(${MESSAGE_SRC} ${MESSAGE_HDRS} PROPERTIES GENERATED TRUE)

# 添加自定义target
add_custom_target(generate_message ALL
                DEPENDS ${MESSAGE_SRC} ${MESSAGE_HDRS}
                COMMENT "generate message target"
                VERBATIM
                )