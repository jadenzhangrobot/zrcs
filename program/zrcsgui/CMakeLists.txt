cmake_minimum_required(VERSION 3.10)
project(zrcsgui VERSION 1.0.0)

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

# 查找Qt5库
find_package(Qt5 REQUIRED COMPONENTS Core Widgets)

# 启用Qt5的自动MOC、UIC和RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
# 设置UI文件搜索路径
set(CMAKE_AUTOUIC_SEARCH_PATHS ui)

# 源文件
set(SOURCES
    main.cpp
    mainwindow.cpp
)

# 头文件
set(HEADERS
    mainwindow.h
)
# UI文件
set(UI_FILES
    ui/zrcsgui.ui
)

# 创建可执行文件
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${UI_FILES})

# 链接Qt5库
target_link_libraries(${PROJECT_NAME} Qt5::Core Qt5::Widgets atomic Threads::Threads)

# 设置输出目录
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)