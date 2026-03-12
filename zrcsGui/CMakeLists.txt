cmake_minimum_required(VERSION 3.10)
project(zrcsgui VERSION 2.0.0)

# 设置C++标准为C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置编译选项
if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")
endif()

# 设置Qt5路径为MinGW版本
set(CMAKE_PREFIX_PATH "D:/msys2/mingw64")
set(Qt5_DIR "D:/msys2/mingw64/lib/cmake/Qt5")

# 查找库
find_package(Qt5 REQUIRED COMPONENTS Core Widgets OpenGL Multimedia)
find_package(Protobuf REQUIRED)
find_package(cppzmq REQUIRED)
find_package(absl REQUIRED)
find_package(OpenGL REQUIRED)

# 启用Qt5的自动MOC、UIC和RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS resources/ui)

include_directories(
    ${CMAKE_SOURCE_DIR}/zrcsCommon/include
    ${CMAKE_SOURCE_DIR}/zrcsGui/src
    ${CMAKE_SOURCE_DIR}/zrcsGui/modules
    ${CMAKE_SOURCE_DIR}/zrcsGui/resources/style
    ${CMAKE_BINARY_DIR}
)

# Protobuf 文件
set(PROTO_FILES
    ${CMAKE_SOURCE_DIR}/zrcsCommon/message/message.proto
)
protobuf_generate_cpp(GUI_PROTO_SRCS GUI_PROTO_HDRS ${PROTO_FILES})

# 源文件
set(SOURCES
    src/core/main_refactored.cpp
    src/core/mainwindow_refactored.cpp
    src/components/statusIndicator.cpp
    src/components/jogAndIOPanel.cpp
    src/components/alarmPanel.cpp
    src/communication/zmqClient.cpp
    modules/trajectory/trajectoryVisualizer.cpp
    modules/gcode/gcodeEditor.cpp
    modules/remote/remoteMonitor.cpp
    modules/plugin/pluginManager.cpp
    ${GUI_PROTO_SRCS}
)

# 头文件
set(HEADERS
    src/core/mainwindow_refactored.h
    src/communication/zmqClient.h
    src/config/zrcsStyles.h
    src/config/zrcsConfig.h
    resources/style/styleLoader.h
    modules/trajectory/trajectoryVisualizer.h
    modules/gcode/gcodeEditor.h
    modules/remote/remoteMonitor.h
    modules/plugin/pluginInterface.h
    modules/plugin/pluginManager.h
)

# UI文件
set(UI_FILES
    resources/ui/mainwindow_refactored.ui
)

# 创建可执行文件
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${UI_FILES})
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

# 链接库
target_link_libraries(${PROJECT_NAME} 
    Qt5::Core 
    Qt5::Widgets 
    Qt5::OpenGL
    Qt5::Multimedia
    atomic 
    Threads::Threads
    protobuf::libprotobuf
    cppzmq
    absl::log_internal_message
    absl::log_internal_check_op
    absl::log_globals
    OpenGL::GL
)

# 设置输出目录
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
