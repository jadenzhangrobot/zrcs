cmake_minimum_required(VERSION 3.16)
project(motiongui VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")
endif()

if(WIN32)
    set(CMAKE_PREFIX_PATH "D:/msys2/ucrt64")
    set(Qt6_DIR "D:/msys2/ucrt64/lib/cmake/Qt6")
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)
find_package(Protobuf REQUIRED)

if(NOT TARGET absl::base)
    message(FATAL_ERROR "Abseil targets not found.")
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR})

include_directories(
    ${CMAKE_SOURCE_DIR}/zrcs_common
    ${CMAKE_BINARY_DIR}
)

set(PROTO_FILES
    ${CMAKE_SOURCE_DIR}/zrcs_common/message/message.proto
)
protobuf_generate_cpp(MOTION_PROTO_SRCS MOTION_PROTO_HDRS ${PROTO_FILES})

set(SOURCES
    main.cpp
    MotionMainWindow.cpp
    MotionZmqClient.cpp
    MotionStatusSubscriber.cpp
    ${MOTION_PROTO_SRCS}
)

set(HEADERS
    MotionMainWindow.h
    MotionZmqClient.h
    MotionStatusSubscriber.h
)

set(UI_FILES
    MainWindow.ui
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${UI_FILES})
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(${PROJECT_NAME}
    zrcscommon
    Qt6::Core
    Qt6::Widgets
    Threads::Threads
    protobuf::libprotobuf
    cppzmq
    atomic
    absl::log_internal_message
    absl::log_internal_check_op
    absl::log_globals
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
