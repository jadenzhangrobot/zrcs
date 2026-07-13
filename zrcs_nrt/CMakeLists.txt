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
    command/CommandDispatcher.cpp
    command/RtBridge.cpp
    log/NrtLogger.cpp
    log/RtLogConsumer.cpp
    statusPublisher/StatusPublisher.cpp
    zmq_server/ZmqServer.cpp
    behaviorTree/BtSharedState.cpp
    behaviorTree/BehaviorTreeRunner.cpp
    behaviorTree/BtCommandNodes.cpp
    motion/MotionPreprocessor.cpp
    motion/PathPreprocessor.cpp
    motion/VelocityPlanner3D.cpp
    dynamicsIdentification/MujocoIdentifyWorker.cpp
)

add_executable(zrcsnrt ${SOURCES})

target_link_libraries(zrcsnrt PRIVATE
    zrcscommon
    cppzmq
    behaviortree_cpp_v3
    spdlog::spdlog
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
