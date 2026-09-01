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

# 设置Qt6路径为UCRT64版本（仅Windows）
if(WIN32)
    set(CMAKE_PREFIX_PATH "D:/msys2/ucrt64")
    set(Qt6_DIR "D:/msys2/ucrt64/lib/cmake/Qt6")
endif()

# 查找库
find_package(Qt6 REQUIRED COMPONENTS Core Widgets OpenGL OpenGLWidgets Multimedia Xml Svg Gui Network)
find_package(OpenGL REQUIRED)

if(NOT TARGET absl::base)
    message(FATAL_ERROR "Abseil targets not found. Please ensure 3rdParty/abseil-cpp is added before zrcsgui.")
endif()

cmake_policy(SET CMP0144 NEW)
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

# 启用Qt6的自动MOC、UIC和RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS resources/ui)

include_directories(
    ${CMAKE_SOURCE_DIR}/zrcs_gui
    ${CMAKE_SOURCE_DIR}/zrcs_gui/modules
    ${CMAKE_SOURCE_DIR}/zrcs_rt
    ${CMAKE_SOURCE_DIR}/zrcs_gui/resources/style
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/3rdParty/Groot
    ${CMAKE_SOURCE_DIR}/3rdParty/Groot/QtNodeEditor/include
    ${CMAKE_SOURCE_DIR}/3rdParty/Groot/depend/BehaviorTree.CPP/include
)

set(ZRCS_GUI_EXTRA_SOURCES)
set(ZRCS_GUI_EXTRA_LIBS)
if(TARGET zrcs_mujoco)
    list(APPEND ZRCS_GUI_EXTRA_SOURCES
        ${CMAKE_SOURCE_DIR}/zrcs_rt/controller/mujoco/MujocoConfig.cpp
    )
    list(APPEND ZRCS_GUI_EXTRA_LIBS
        zrcs_mujoco
        tinyxml2
    )
endif()

# 源文件
set(SOURCES
    core/main.cpp
    core/MainWindow.cpp
    components/StatusIndicator.cpp
    components/JogAndIOPanel.cpp
    components/AlarmPanel.cpp
    communication/ZmqClient.cpp
    communication/ZmqStatusSubscriber.cpp
    modules/mujoco/MujocoVisualizer.cpp
    modules/mujoco/MujocoFrameServer.cpp
    modules/behavior_tree/BehaviorTreePanel.cpp
    modules/command/CommandPanel.cpp
    ${ZRCS_GUI_EXTRA_SOURCES}
)

# 头文件
set(HEADERS
    core/MainWindow.h
    communication/ZmqClient.h
    communication/ZmqStatusSubscriber.h
    config/ZrcsStyles.h
    config/ZrcsConfig.h
    resources/style/StyleLoader.h
    modules/mujoco/MujocoVisualizer.h
    modules/mujoco/MujocoFrameServer.h
    modules/behavior_tree/BehaviorTreePanel.h
    modules/command/CommandPanel.h
)

# UI文件
set(UI_FILES
    resources/ui/main_window.ui
    resources/ui/jog_control_panel.ui
    resources/ui/alarm_panel.ui
    resources/ui/mujoco_panel.ui
    resources/ui/command_panel.ui
    resources/ui/behavior_tree_panel.ui
)

# Application resources embedded into the executable.
set(ZRCS_GUI_RESOURCE_FILES
    resources/style/style.qrc
)

# Groot 资源文件
set(GROOT_RESOURCE_FILES
    ${CMAKE_SOURCE_DIR}/3rdParty/Groot/bt_editor/resources/icons.qrc
    ${CMAKE_SOURCE_DIR}/3rdParty/Groot/bt_editor/resources/style.qrc
)

# 创建可执行文件
add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${HEADERS}
    ${UI_FILES}
    ${ZRCS_GUI_RESOURCE_FILES}
    ${GROOT_RESOURCE_FILES}
)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_compile_definitions(${PROJECT_NAME} PRIVATE NODE_EDITOR_STATIC)


# 链接库
target_link_libraries(${PROJECT_NAME}
    zrcscommon
    Qt6::Core
    Qt6::Widgets
    Qt6::OpenGL
    Qt6::OpenGLWidgets
    Qt6::Multimedia
    Qt6::Xml
    Qt6::Svg
    Qt6::Network
    atomic
    Threads::Threads
    zrcs::proto
    cppzmq
    OpenGL::GL
    behavior_tree_editor
    ${ZRCS_GUI_EXTRA_LIBS}
)


# 设置输出目录
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
