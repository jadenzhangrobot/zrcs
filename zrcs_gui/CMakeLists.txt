cmake_minimum_required(VERSION 3.16)
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

# 设置Qt6路径为UCRT64版本
set(CMAKE_PREFIX_PATH "D:/msys2/ucrt64")
set(Qt6_DIR "D:/msys2/ucrt64/lib/cmake/Qt6")

# 查找库
find_package(Qt6 REQUIRED COMPONENTS Core Widgets OpenGL OpenGLWidgets Multimedia Xml Svg Gui)
find_package(Protobuf REQUIRED)
find_package(OpenGL REQUIRED)

if(NOT TARGET absl::base)
    message(FATAL_ERROR "Abseil targets not found. Please ensure 3rdParty/abseil-cpp is added before zrcsgui.")
endif()

cmake_policy(SET CMP0144 NEW)
set(OPENCASCADE_ROOT "${CMAKE_SOURCE_DIR}/3rdParty/OpenCASCADE")
if(EXISTS "${OPENCASCADE_ROOT}")
    list(APPEND CMAKE_PREFIX_PATH "${OPENCASCADE_ROOT}")
endif()
find_package(VTK QUIET COMPONENTS
    CommonCore
    RenderingCore
    RenderingFreeType
    FiltersGeneral
    IOImage
    ImagingCore
    InteractionStyle
    RenderingOpenGL2
    RenderingGL2PSOpenGL2
)
find_package(OpenCASCADE QUIET)

# 启用Qt6的自动MOC、UIC和RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS resources/ui)

include_directories(
    ${CMAKE_SOURCE_DIR}/zrcs_common/include
    ${CMAKE_SOURCE_DIR}/zrcs_gui/src
    ${CMAKE_SOURCE_DIR}/zrcs_gui/modules
    ${CMAKE_SOURCE_DIR}/zrcs_gui/resources/style
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/3rdParty/Groot
    ${CMAKE_SOURCE_DIR}/3rdParty/Groot/QtNodeEditor/include
    ${CMAKE_SOURCE_DIR}/3rdParty/Groot/depend/BehaviorTree.CPP/include
)

# Protobuf 文件
set(PROTO_FILES
    ${CMAKE_SOURCE_DIR}/zrcs_common/message/message.proto
)
protobuf_generate_cpp(GUI_PROTO_SRCS GUI_PROTO_HDRS ${PROTO_FILES})

# 源文件
set(SOURCES
    src/core/main.cpp
    src/core/MainWindow.cpp
    src/components/statusIndicator.cpp
    src/components/jogAndIOPanel.cpp
    src/components/alarmPanel.cpp
    src/communication/zmqClient.cpp
    src/communication/zmqStatusSubscriber.cpp
    modules/trajectory/trajectoryVisualizer.cpp
    modules/gcode/gcodeEditor.cpp
    modules/remote/remoteMonitor.cpp
    modules/plugin/pluginManager.cpp
    modules/behavior_tree/behaviorTreePanel.cpp
    modules/command/commandPanel.cpp
    ${GUI_PROTO_SRCS}
)

# 头文件
set(HEADERS
    src/core/MainWindow.h
    src/communication/zmqClient.h
    src/communication/zmqStatusSubscriber.h
    src/config/zrcsStyles.h
    src/config/zrcsConfig.h
    resources/style/styleLoader.h
    modules/trajectory/trajectoryVisualizer.h
    modules/gcode/gcodeEditor.h
    modules/remote/remoteMonitor.h
    modules/plugin/pluginInterface.h
    modules/plugin/pluginManager.h
    modules/behavior_tree/behaviorTreePanel.h
    modules/command/commandPanel.h
)

# UI文件
set(UI_FILES
    resources/ui/main_window.ui
    resources/ui/jog_control_panel.ui
    resources/ui/io_panel.ui
    resources/ui/alarm_panel.ui
    resources/ui/gcode_panel.ui
    resources/ui/remote_data_monitor.ui
    resources/ui/remote_monitor_panel.ui
    resources/ui/plugin_panel.ui
    resources/ui/trajectory_panel.ui
)

# Groot 资源文件
set(GROOT_RESOURCE_FILES
    ${CMAKE_SOURCE_DIR}/3rdParty/Groot/bt_editor/resources/icons.qrc
    ${CMAKE_SOURCE_DIR}/3rdParty/Groot/bt_editor/resources/style.qrc
)

# 创建可执行文件
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${UI_FILES} ${GROOT_RESOURCE_FILES})
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_compile_definitions(${PROJECT_NAME} PRIVATE ZRCSGUI_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_definitions(${PROJECT_NAME} PRIVATE NODE_EDITOR_STATIC)

if(OpenCASCADE_FOUND)
    target_compile_definitions(${PROJECT_NAME} PRIVATE ZRCSGUI_HAS_OPENCASCADE)
    if(DEFINED OpenCASCADE_INCLUDE_DIR)
        target_include_directories(${PROJECT_NAME} PRIVATE ${OpenCASCADE_INCLUDE_DIR})
    endif()
    if(DEFINED OpenCASCADE_INCLUDE_DIRS)
        target_include_directories(${PROJECT_NAME} PRIVATE ${OpenCASCADE_INCLUDE_DIRS})
    endif()
endif()

# 链接库
target_link_libraries(${PROJECT_NAME}
    Qt6::Core
    Qt6::Widgets
    Qt6::OpenGL
    Qt6::OpenGLWidgets
    Qt6::Multimedia
    Qt6::Xml
    Qt6::Svg
    atomic
    Threads::Threads
    protobuf::libprotobuf
    cppzmq
    absl::log_internal_message
    absl::log_internal_check_op
    absl::log_globals
    OpenGL::GL
    behavior_tree_editor
)

if(OpenCASCADE_FOUND)
    set(OCC_LINK_LIBS "")
    foreach(occ_lib IN LISTS OpenCASCADE_LIBRARIES)
        if("${occ_lib}" MATCHES "^[A-Za-z0-9_]+::[A-Za-z0-9_]+$")
            if(TARGET ${occ_lib})
                list(APPEND OCC_LINK_LIBS ${occ_lib})
            else()
                message(WARNING "Skip unresolved OpenCASCADE dependency target: ${occ_lib}")
            endif()
        else()
            list(APPEND OCC_LINK_LIBS ${occ_lib})
        endif()
    endforeach()
    if(OCC_LINK_LIBS)
        target_link_libraries(${PROJECT_NAME} ${OCC_LINK_LIBS})
    endif()
endif()

# 设置输出目录
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
