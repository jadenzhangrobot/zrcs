cmake_minimum_required(VERSION 3.10)
project(plotPublisher)

set(CMAKE_CXX_STANDARD 17)

# Find ZMQ using pkg-config
find_package(PkgConfig REQUIRED)
pkg_check_modules(ZMQ REQUIRED libzmq)

# Find nlohmann_json
find_package(nlohmann_json REQUIRED)

# Add executable
add_executable(${PROJECT_NAME} src/main.cpp)

# Link libraries
target_include_directories(${PROJECT_NAME} PRIVATE ${ZMQ_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${ZMQ_LIBRARIES})
target_link_libraries(${PROJECT_NAME} PRIVATE nlohmann_json::nlohmann_json)

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)