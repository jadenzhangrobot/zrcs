
# 设置可选值范围
set_property(CACHE BUILD_MODE PROPERTY STRINGS "realtime" "simulation" "standard")

if(BUILD_MODE STREQUAL "realtime")
    message(STATUS "realtime模式")
    add_compile_definitions(REALTIME ETHERCAT)
    # 控制周期 (ms)：真机 1 ms
    set(ZRCS_CYCLE_TIME_MS 1)
    set(xeno_cflags_params "--alchemy"  "--cflags")
    execute_process(COMMAND /usr/xenomai/bin/xeno-config ${xeno_cflags_params} OUTPUT_VARIABLE xeno_cflags OUTPUT_STRIP_TRAILING_WHITESPACE)

    set(xeno_ldflags_params "--alchemy"  "--ldflags")
    execute_process(COMMAND /usr/xenomai/bin/xeno-config ${xeno_ldflags_params} OUTPUT_VARIABLE xeno_ldflags OUTPUT_STRIP_TRAILING_WHITESPACE)

    set(xeno_g++_params "--cc")
    execute_process(COMMAND /usr/xenomai/bin/xeno-config ${xeno_g++_params} OUTPUT_VARIABLE xeno_g++)
    set(CMAKE_C_FLAGS          "${CMAKE_C_FLAGS} ${xeno_cflags}")
    set(CMAKE_CXX_FLAGS        "${CMAKE_CXX_FLAGS} ${xeno_cflags} -fpermissive")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${xeno_ldflags}")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${xeno_ldflags}")

elseif(BUILD_MODE STREQUAL "simulation")
    message(STATUS "simulation模式")
    add_compile_definitions(SIMULATION)
    # 控制周期 (ms)：仿真/非实时 10 ms
    set(ZRCS_CYCLE_TIME_MS 1)
elseif(BUILD_MODE STREQUAL "standard")
    message(STATUS "standard模式")
    add_compile_definitions(STANDARD)
    set(ZRCS_CYCLE_TIME_MS 10)
else()
    message(FATAL_ERROR "不支持的构建模式: ${BUILD_MODE}")
endif()

# 周期宏由 CMake 统一注入；业务代码可直接使用 cycletime / ZRCS_CYCLE_TIME_MS
add_compile_definitions(
    ZRCS_CYCLE_TIME_MS=${ZRCS_CYCLE_TIME_MS}
    cycletime=${ZRCS_CYCLE_TIME_MS}
)
message(STATUS "ZRCS_CYCLE_TIME_MS=${ZRCS_CYCLE_TIME_MS}")
