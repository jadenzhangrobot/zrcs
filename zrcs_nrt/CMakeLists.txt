project(zrcsnrt)

include_directories(
    ${CMAKE_SOURCE_DIR}/zrcs_nrt
    ${CMAKE_SOURCE_DIR}/zrcs_rt
    ${CMAKE_SOURCE_DIR}/3rdParty/Groot/depend/BehaviorTree.CPP/include
    ${CMAKE_SOURCE_DIR}/3rdParty/spdlog/include
    ${CMAKE_BINARY_DIR}
)

set(SOURCES
    main.cpp
    app/NrtApplication.cpp

    # command routing
    command/CommandService.cpp
    command/TaskService.cpp
    command/CommandRouter.cpp
    command/BehaviorTreeCommandService.cpp

    # rtBridge (NRT↔RT shared-memory communication)
    rtBridge/RtBridge.cpp

    # nrtServer (NRT control/status ZMQ server)
    nrtServer/zmq/ZmqServer.cpp
    nrtServer/zmq/StatusPublisher.cpp

    # status
    status/StatusStore.cpp
    status/StatusCollector.cpp

    # logging
    log/NrtLogger.cpp
    log/RtLogConsumer.cpp

    # behavior tree
    behavior_tree/core/BtSharedState.cpp
    behavior_tree/core/BehaviorTreeRunner.cpp
    behavior_tree/core/BehaviorTreeService.cpp
    behavior_tree/core/BtNodeRegistry.cpp
    behavior_tree/core/RegisterNodes.cpp
    behavior_tree/nodes/rtcommand/BtCommandNodes.cpp
    behavior_tree/nodes/motion/PathMoveSupport.cpp
    behavior_tree/nodes/motion/PathMoveNode.cpp
    behavior_tree/nodes/nc/NcParseNode.cpp

    # core — G-code / NC 解析
    core/nc_parser/NcParser.cpp

    # pure algorithms — path pipeline:
    # simplify → corner blend → short-segment merge → look-ahead
    algorithm/path_planning/TrajectoryGeometry.cpp
    algorithm/path_planning/PathSimplifier.cpp
    algorithm/path_planning/CornerBlender.cpp
    algorithm/path_planning/ShortSegmentMerger.cpp
    algorithm/path_planning/LookAheadPlanner.cpp
    algorithm/path_planning/MotionPlanner.cpp

    algorithm/dynamics_identification/MujocoIdentifyWorker.cpp
)

add_executable(zrcsnrt ${SOURCES})

target_link_libraries(zrcsnrt PRIVATE
    zrcscommon
    cppzmq
    behaviortree_cpp_v3
    spdlog::spdlog
    gpr::gpr
    ${MOTION_CONTROL_LIBS}
)

if(WIN32)
    set_target_properties(zrcsnrt PROPERTIES LINK_FLAGS "-mconsole")
endif()

install(TARGETS zrcsnrt
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
