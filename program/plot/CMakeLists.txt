cmake_minimum_required(VERSION 3.10)
project(ZmqJsonPublisher)

set(CMAKE_CXX_STANDARD 17)

# Find ZMQ using pkg-config
find_package(PkgConfig REQUIRED)
pkg_check_modules(ZMQ REQUIRED libzmq)

# Find nlohmann_json
find_package(nlohmann_json REQUIRED)

# Add executable
add_executable(publisher src/main.cpp)

# Link libraries
target_include_directories(publisher PRIVATE ${ZMQ_INCLUDE_DIRS})
target_link_libraries(publisher PRIVATE ${ZMQ_LIBRARIES})
target_link_libraries(publisher PRIVATE nlohmann_json::nlohmann_json)
