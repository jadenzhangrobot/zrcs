# 编译器配置模块
# 设置编译器特定的标志和选项

# 设置编译器标志
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # GCC 特定选项
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Wno-unused-parameter")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -DDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")
    
    # 实时系统特定的编译选项
    if(realtime)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive")
    endif()
    
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # Clang 特定选项
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -DDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")
    
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # MSVC 特定选项
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od /Zi /DDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /DNDEBUG")
    
    # 禁用特定警告
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4100")  # 未引用的形参
endif()

# 设置默认构建类型
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "C++ Flags: ${CMAKE_CXX_FLAGS}")