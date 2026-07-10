project(zrcsrt)

set(CMD_SRC_DIR    ${CMAKE_SOURCE_DIR}/zrcs_rt/command)

# 包含目录
include_directories(
    ${CMAKE_SOURCE_DIR}/zrcs_rt
    ${CMAKE_SOURCE_DIR}/3rdParty/magic_enum/include
    ${CMAKE_BINARY_DIR}
)

# 源文件 (暂时假设使用 zrcs_rt/main.cpp)
# 之前的目录结构显示 zrcs_rt 下有 main.cpp
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    "main.cpp"
    "command/*.cpp"
    "controller/*.cpp"
    "model/*.cpp"
    "system/*.cpp"
)

add_executable(zrcsrt ${SOURCES})

# 链接库
target_link_libraries(zrcsrt PRIVATE
    zrcscommon
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
