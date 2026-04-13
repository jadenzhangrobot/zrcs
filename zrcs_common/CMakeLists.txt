project(zrcscommon)

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
        Boost::headers
    PRIVATE
        tinyxml2
)

# Linux/macOS: shm_open / shm_unlink 在 librt 中
if(UNIX)
    target_link_libraries(zrcscommon PUBLIC rt)
endif()
# Windows: CreateFileMapping 在 kernel32 中（CMake 默认已链接，无需显式添加）
