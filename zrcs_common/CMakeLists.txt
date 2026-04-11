project(zrcscommon)

find_package(Boost REQUIRED COMPONENTS system)

file(GLOB_RECURSE SOURCES
    "config/*.cpp"
    "shared_memory/*.cpp"
    "xml/*.cpp"
)

add_library(zrcscommon STATIC ${SOURCES})

target_include_directories(zrcscommon
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/config
        ${CMAKE_CURRENT_SOURCE_DIR}/shared_memory
        ${CMAKE_CURRENT_SOURCE_DIR}/xml
    PRIVATE
        ${CMAKE_SOURCE_DIR}/3rdParty/tinyxml2
)

target_link_libraries(zrcscommon
    PUBLIC
        Boost::system
    PRIVATE
        tinyxml2
)
