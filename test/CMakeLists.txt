cmake_minimum_required(VERSION 3.18.1)
project(zrcs-test)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(DEVEL_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/..)

include_directories(
    ${DEVEL_INCLUDE_DIRS}/zrcs_common/config
    ${DEVEL_INCLUDE_DIRS}/zrcs_common
    ${DEVEL_INCLUDE_DIRS}/zrcs_common/shared_memory
    ${DEVEL_INCLUDE_DIRS}/3rdParty/cereal/cereal-master/include
    ${DEVEL_INCLUDE_DIRS}/3rdParty/tinyxml2
)

# SPSC 环形缓冲区单元测试
add_executable(test_spsc test_spsc.cpp)
target_link_libraries(test_spsc PRIVATE atomic)

# Command 结构体安全性测试
add_executable(test_command test_command.cpp)
target_link_libraries(test_command PRIVATE atomic)

add_executable(test_config_manager test_config_manager.cpp)
target_link_libraries(test_config_manager PRIVATE zrcscommon tinyxml2)

add_executable(test_axis_direction
    test_axis_direction.cpp
    ${DEVEL_INCLUDE_DIRS}/zrcs_rt/controller/ControllerInterface.cpp
)
target_include_directories(test_axis_direction PRIVATE
    ${DEVEL_INCLUDE_DIRS}/zrcs_rt
)
target_link_libraries(test_axis_direction PRIVATE atomic)

add_executable(test_cartesian_rtcp
    test_cartesian_rtcp.cpp
    ${DEVEL_INCLUDE_DIRS}/zrcs_rt/model/XyzacTableRobot.cpp
    ${DEVEL_INCLUDE_DIRS}/zrcs_rt/model/RobotModel.cpp
    ${DEVEL_INCLUDE_DIRS}/zrcs_rt/model/ModelConfig.cpp
)
target_include_directories(test_cartesian_rtcp PRIVATE
    ${DEVEL_INCLUDE_DIRS}/zrcs_rt
)
target_compile_definitions(test_cartesian_rtcp PRIVATE
    ZRCS_SOURCE_DIR="${DEVEL_INCLUDE_DIRS}"
)
target_link_libraries(test_cartesian_rtcp PRIVATE
    Eigen3::Eigen
    zrcscommon
    tinyxml2
)

add_executable(test_status_store
    test_status_store.cpp
    ${DEVEL_INCLUDE_DIRS}/zrcs_nrt/status/StatusStore.cpp
)
target_include_directories(test_status_store PRIVATE
    ${DEVEL_INCLUDE_DIRS}/zrcs_nrt
)
target_link_libraries(test_status_store PRIVATE atomic)

add_executable(test_motion_preprocessing
    test_motion_preprocessing.cpp
    ${DEVEL_INCLUDE_DIRS}/zrcs_nrt/algorithm/path_planning/TrajectoryGeometry.cpp
    ${DEVEL_INCLUDE_DIRS}/zrcs_nrt/algorithm/path_planning/PathSimplifier.cpp
    ${DEVEL_INCLUDE_DIRS}/zrcs_nrt/algorithm/path_planning/CornerBlender.cpp
    ${DEVEL_INCLUDE_DIRS}/zrcs_nrt/algorithm/path_planning/ShortSegmentMerger.cpp
    ${DEVEL_INCLUDE_DIRS}/zrcs_nrt/algorithm/path_planning/LookAheadPlanner.cpp
    ${DEVEL_INCLUDE_DIRS}/zrcs_nrt/algorithm/path_planning/MotionPlanner.cpp
    ${DEVEL_INCLUDE_DIRS}/zrcs_nrt/behavior_tree/nodes/motion/PathMoveSupport.cpp
    ${DEVEL_INCLUDE_DIRS}/zrcs_nrt/rtBridge/RtBridge.cpp
)
target_include_directories(test_motion_preprocessing PRIVATE
    ${DEVEL_INCLUDE_DIRS}/zrcs_nrt
    ${DEVEL_INCLUDE_DIRS}/3rdParty/magic_enum/include
)
target_compile_definitions(test_motion_preprocessing PRIVATE WITHOUT_NUMPY)
target_link_libraries(test_motion_preprocessing PRIVATE
    matplotlib_cpp::matplotlib_cpp
    spdlog::spdlog
    zrcscommon
    ruckig
)

add_executable(test_xyzac_nc_path
    test_xyzac_nc_path.cpp
    ${DEVEL_INCLUDE_DIRS}/zrcs_nrt/core/nc_parser/NcParser.cpp
    ${DEVEL_INCLUDE_DIRS}/zrcs_nrt/algorithm/path_planning/TrajectoryGeometry.cpp
    ${DEVEL_INCLUDE_DIRS}/zrcs_nrt/algorithm/path_planning/PathSimplifier.cpp
    ${DEVEL_INCLUDE_DIRS}/zrcs_nrt/algorithm/path_planning/CornerBlender.cpp
    ${DEVEL_INCLUDE_DIRS}/zrcs_nrt/algorithm/path_planning/ShortSegmentMerger.cpp
    ${DEVEL_INCLUDE_DIRS}/zrcs_nrt/algorithm/path_planning/LookAheadPlanner.cpp
    ${DEVEL_INCLUDE_DIRS}/zrcs_nrt/algorithm/path_planning/MotionPlanner.cpp
)
target_include_directories(test_xyzac_nc_path PRIVATE
    ${DEVEL_INCLUDE_DIRS}/zrcs_nrt
)
target_compile_definitions(test_xyzac_nc_path PRIVATE
    ZRCS_SOURCE_DIR="${DEVEL_INCLUDE_DIRS}"
)
target_link_libraries(test_xyzac_nc_path PRIVATE
    gpr::gpr
    ruckig
)

# ZMQ 通信层测试（Protobuf + ZMQ REQ/REP）
add_executable(test_zmq_comm test_zmq_comm.cpp)
target_link_libraries(test_zmq_comm PRIVATE
    zrcs::proto
    cppzmq
)

# Live integration client. It sends motion only when invoked with --execute.
add_executable(test_move_v_command_client test_move_v_command_client.cpp)
target_link_libraries(test_move_v_command_client PRIVATE
    zrcs::proto
    cppzmq
)

# Windows 线程 sleep_until 周期/抖动测量（仿真选型用，无 zrcs 依赖）
add_executable(test_windows_thread_period test_windows_thread_period.cpp)
if(WIN32)
    target_link_libraries(test_windows_thread_period PRIVATE winmm)
endif()
