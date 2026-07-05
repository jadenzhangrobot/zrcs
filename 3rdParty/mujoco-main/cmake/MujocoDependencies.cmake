# Copyright 2021 DeepMind Technologies Limited
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Build configuration for third party libraries used in MuJoCo.

set(MUJOCO_DEP_VERSION_lodepng
    17d08dd26cac4d63f43af217ebd70318bfb8189c
    CACHE STRING "Version of `lodepng` to be fetched."
)
set(MUJOCO_DEP_VERSION_tinyxml2
    e6caeae85799003f4ca74ff26ee16a789bc2af48
    CACHE STRING "Version of `tinyxml2` to be fetched."
)
set(MUJOCO_DEP_VERSION_tinyobjloader
    1421a10d6ed9742f5b2c1766d22faa6cfbc56248
    CACHE STRING "Version of `tinyobjloader` to be fetched."
)
set(MUJOCO_DEP_VERSION_MarchingCubeCpp
    f03a1b3ec29b1d7d865691ca8aea4f1eb2c2873d
    CACHE STRING "Version of `MarchingCubeCpp` to be fetched."
)
set(MUJOCO_DEP_VERSION_ccd
    7931e764a19ef6b21b443376c699bbc9c6d4fba8 # v2.1
    CACHE STRING "Version of `ccd` to be fetched."
)
set(MUJOCO_DEP_VERSION_qhull
    62ccc56af071eaa478bef6ed41fd7a55d3bb2d80
    CACHE STRING "Version of `qhull` to be fetched."
)
set(MUJOCO_DEP_VERSION_miniz
    d10b03cc73475af673df40f06e5cefd1d5f940d9
    CACHE STRING "Version of `miniz` to be fetched."
)
set(MUJOCO_DEP_VERSION_Eigen3
    ea13a98decd497a8c5588fb5de71b57bcf10d864
    CACHE STRING "Version of `Eigen3` to be fetched."
)

set(MUJOCO_DEP_VERSION_abseil
    255c84dadd029fd8ad25c5efb5933e47beaa00c7 # LTS 20250814.1
    CACHE STRING "Version of `abseil` to be fetched."
)

set(MUJOCO_DEP_VERSION_gtest
    52eb8108c5bdec04579160ae17225d66034bd723 # v1.17.0
    CACHE STRING "Version of `gtest` to be fetched."
)

set(MUJOCO_DEP_VERSION_benchmark
    834a61fc65e8b7885fcf177f1230ae4b897118fa
    CACHE STRING "Version of `benchmark` to be fetched."
)

mark_as_advanced(MUJOCO_DEP_VERSION_lodepng)
mark_as_advanced(MUJOCO_DEP_VERSION_MarchingCubeCpp)
mark_as_advanced(MUJOCO_DEP_VERSION_tinyxml2)
mark_as_advanced(MUJOCO_DEP_VERSION_tinyobjloader)
mark_as_advanced(MUJOCO_DEP_VERSION_ccd)
mark_as_advanced(MUJOCO_DEP_VERSION_qhull)
mark_as_advanced(MUJOCO_DEP_VERSION_Eigen3)
mark_as_advanced(MUJOCO_DEP_VERSION_abseil)
mark_as_advanced(MUJOCO_DEP_VERSION_gtest)
mark_as_advanced(MUJOCO_DEP_VERSION_benchmark)

include(FetchContent)
include(FindOrFetch)

# ─── Use local third-party sources (no network download) ──────────────────
# Set FETCHCONTENT_SOURCE_DIR_<name> as CACHE variables so FetchContent uses
# the pre-downloaded sources instead of cloning from GitHub.
set(MUJOCO_3RDPART_DIR "${mujoco_SOURCE_DIR}/../mujoco-3rdpart")
get_filename_component(MUJOCO_3RDPART_DIR "${MUJOCO_3RDPART_DIR}" ABSOLUTE)
foreach(_dep IN ITEMS lodepng marchingcubecpp qhull tinyobjloader ccd miniz)
  set(_src_dir "${MUJOCO_3RDPART_DIR}/${_dep}-src")
  if(EXISTS "${_src_dir}")
    set(FETCHCONTENT_SOURCE_DIR_${_dep} "${_src_dir}" CACHE STRING "Local source for ${_dep}" FORCE)
    message(STATUS "Using local source for ${_dep}: ${_src_dir}")
  endif()
endforeach()
unset(_dep)
unset(_src_dir)
# ──────────────────────────────────────────────────────────────────────────

# Override the BUILD_SHARED_LIBS setting, just for building third party libs (since we always want
# static libraries). The ccd CMakeLists.txt doesn't expose an option to build a static ccd library,
# unless BUILD_SHARED_LIBS is set.

# We force all the dependencies to be compiled as static libraries.
# TODO(fraromano) Revisit this choice when adding support for install.
set(BUILD_SHARED_LIBS_OLD ${BUILD_SHARED_LIBS})
set(BUILD_SHARED_LIBS
    OFF
    CACHE INTERNAL "Build SHARED libraries"
)

if(NOT TARGET lodepng)
  FetchContent_Declare(
    lodepng
    SOURCE_DIR "${MUJOCO_3RDPART_DIR}/lodepng-src"
  )

  FetchContent_GetProperties(lodepng)
  if(NOT lodepng_POPULATED)
    FetchContent_Populate(lodepng)
    # This is not a CMake project.
    set(LODEPNG_SRCS ${lodepng_SOURCE_DIR}/lodepng.cpp)
    set(LODEPNG_HEADERS ${lodepng_SOURCE_DIR}/lodepng.h)
    add_library(lodepng STATIC ${LODEPNG_HEADERS} ${LODEPNG_SRCS})
    target_compile_options(lodepng PRIVATE ${MUJOCO_MACOS_COMPILE_OPTIONS})
    target_link_options(lodepng PRIVATE ${MUJOCO_MACOS_LINK_OPTIONS})
    if(NOT EMSCRIPTEN)
      target_include_directories(lodepng PUBLIC ${lodepng_SOURCE_DIR})
    else()
      target_include_directories(lodepng PUBLIC  $<BUILD_INTERFACE:${lodepng_SOURCE_DIR}> $<INSTALL_INTERFACE:include>)
    endif()
  endif()
endif()

if(NOT TARGET marchingcubecpp)
  FetchContent_Declare(
    marchingcubecpp
    SOURCE_DIR "${MUJOCO_3RDPART_DIR}/marchingcubecpp-src"
  )

  FetchContent_GetProperties(marchingcubecpp)
  if(NOT marchingcubecpp_POPULATED)
    FetchContent_Populate(marchingcubecpp)
    include_directories(${marchingcubecpp_SOURCE_DIR})
  endif()
endif()

# qhull — use local source via add_subdirectory (no FetchContent, no PATCH_COMMAND)
set(QHULL_ENABLE_TESTING OFF)
if(NOT TARGET qhull)
  set(qhull_SOURCE_DIR "${MUJOCO_3RDPART_DIR}/qhull-src")
  set(qhull_BINARY_DIR "${CMAKE_BINARY_DIR}/_deps/qhull-build")
  add_subdirectory("${qhull_SOURCE_DIR}" "${qhull_BINARY_DIR}" EXCLUDE_FROM_ALL)
  message(STATUS "Using local source for qhull: ${qhull_SOURCE_DIR}")
endif()
# MuJoCo includes a file from libqhull_r which is not exported by the qhull include directories.
# Add it to the target.
target_include_directories(
  qhullstatic_r INTERFACE $<BUILD_INTERFACE:${qhull_SOURCE_DIR}/src/libqhull_r>
)
target_compile_options(qhullstatic_r PRIVATE ${MUJOCO_MACOS_COMPILE_OPTIONS})
target_link_options(qhullstatic_r PRIVATE ${MUJOCO_MACOS_LINK_OPTIONS})

set(tinyxml2_BUILD_TESTING OFF)
findorfetch(
  USE_SYSTEM_PACKAGE
  OFF
  PACKAGE_NAME
  tinyxml2
  LIBRARY_NAME
  tinyxml2
  GIT_REPO
  https://github.com/leethomason/tinyxml2.git
  GIT_TAG
  ${MUJOCO_DEP_VERSION_tinyxml2}
  TARGETS
  tinyxml2
  EXCLUDE_FROM_ALL
)
target_compile_options(tinyxml2 PRIVATE ${MUJOCO_MACOS_COMPILE_OPTIONS})
target_link_options(tinyxml2 PRIVATE ${MUJOCO_MACOS_LINK_OPTIONS})

# tinyobjloader — use local source via add_subdirectory
if(NOT DEFINED CMAKE_POLICY_VERSION_MINIMUM)
  set(CMAKE_POLICY_VERSION_MINIMUM ${MUJOCO_CMAKE_MIN_REQ})
  set(CMAKE_POLICY_VERSION_MINIMUM_LOCALLY_DEFINED ON)
endif()
if(NOT TARGET tinyobjloader)
  set(tinyobjloader_SOURCE_DIR "${MUJOCO_3RDPART_DIR}/tinyobjloader-src")
  set(tinyobjloader_BINARY_DIR "${CMAKE_BINARY_DIR}/_deps/tinyobjloader-build")
  add_subdirectory("${tinyobjloader_SOURCE_DIR}" "${tinyobjloader_BINARY_DIR}" EXCLUDE_FROM_ALL)
  message(STATUS "Using local source for tinyobjloader: ${tinyobjloader_SOURCE_DIR}")
endif()
if(CMAKE_POLICY_VERSION_MINIMUM_LOCALLY_DEFINED)
  unset(CMAKE_POLICY_VERSION_MINIMUM)
  unset(CMAKE_POLICY_VERSION_MINIMUM_LOCALLY_DEFINED)
endif()

# ccd — use local source via add_subdirectory
set(ENABLE_DOUBLE_PRECISION ON)
set(CCD_HIDE_ALL_SYMBOLS ON)
if(NOT DEFINED CMAKE_POLICY_VERSION_MINIMUM)
  set(CMAKE_POLICY_VERSION_MINIMUM ${MUJOCO_CMAKE_MIN_REQ})
  set(CMAKE_POLICY_VERSION_MINIMUM_LOCALLY_DEFINED ON)
endif()
if(NOT TARGET ccd)
  set(ccd_SOURCE_DIR "${MUJOCO_3RDPART_DIR}/ccd-src")
  set(ccd_BINARY_DIR "${CMAKE_BINARY_DIR}/_deps/ccd-build")
  add_subdirectory("${ccd_SOURCE_DIR}" "${ccd_BINARY_DIR}" EXCLUDE_FROM_ALL)
  message(STATUS "Using local source for ccd: ${ccd_SOURCE_DIR}")
endif()
if(CMAKE_POLICY_VERSION_MINIMUM_LOCALLY_DEFINED)
  unset(CMAKE_POLICY_VERSION_MINIMUM)
  unset(CMAKE_POLICY_VERSION_MINIMUM_LOCALLY_DEFINED)
endif()
target_compile_options(ccd PRIVATE ${MUJOCO_MACOS_COMPILE_OPTIONS})
target_link_options(ccd PRIVATE ${MUJOCO_MACOS_LINK_OPTIONS})

# libCCD has an unconditional `#define _CRT_SECURE_NO_WARNINGS` on Windows.
# TODO(stunya): Remove this after https://github.com/danfis/libccd/pull/77 is merged.
if(WIN32)
  if(MSVC)
    # C4005 is the MSVC equivalent of -Wmacro-redefined.
    target_compile_options(ccd PRIVATE /wd4005)
  else()
    target_compile_options(ccd PRIVATE -Wno-macro-redefined)
  endif()
endif()

# miniz — use local source via add_subdirectory
if(DEFINED BUILD_TESTS)
  set(_OLD_BUILD_TESTS "${BUILD_TESTS}")
  set(_BUILD_TESTS_WAS_DEFINED TRUE)
else()
  set(_BUILD_TESTS_WAS_DEFINED FALSE)
endif()
set(BUILD_TESTS OFF)
if(NOT TARGET miniz)
  set(miniz_SOURCE_DIR "${MUJOCO_3RDPART_DIR}/miniz-src")
  set(miniz_BINARY_DIR "${CMAKE_BINARY_DIR}/_deps/miniz-build")
  add_subdirectory("${miniz_SOURCE_DIR}" "${miniz_BINARY_DIR}" EXCLUDE_FROM_ALL)
  message(STATUS "Using local source for miniz: ${miniz_SOURCE_DIR}")
endif()
if(_BUILD_TESTS_WAS_DEFINED)
  set(BUILD_TESTS "${_OLD_BUILD_TESTS}")
else()
  unset(BUILD_TESTS)
endif()
unset(_BUILD_TESTS_WAS_DEFINED)


if(MUJOCO_BUILD_TESTS OR MUJOCO_BUILD_STUDIO OR MUJOCO_USE_FILAMENT)
  set(ABSL_PROPAGATE_CXX_STD ON)

  # This specific version of Abseil does not have the following variable. We need to work with BUILD_TESTING
  set(BUILD_TESTING_OLD ${BUILD_TESTING})
  set(BUILD_TESTING
      OFF
      CACHE INTERNAL "Build tests."
  )

  set(ABSL_BUILD_TESTING OFF)
  findorfetch(
    USE_SYSTEM_PACKAGE
    OFF
    PACKAGE_NAME
    absl
    LIBRARY_NAME
    abseil-cpp
    GIT_REPO
    https://github.com/abseil/abseil-cpp.git
    GIT_TAG
    ${MUJOCO_DEP_VERSION_abseil}
    TARGETS
    absl::core_headers
    EXCLUDE_FROM_ALL
  )

  set(BUILD_TESTING
      ${BUILD_TESTING_OLD}
      CACHE BOOL "Build tests." FORCE
  )
endif()

if(MUJOCO_BUILD_TESTS)

  # Avoid linking errors on Windows by dynamically linking to the C runtime.
  set(gtest_force_shared_crt
      ON
      CACHE BOOL "" FORCE
  )

  findorfetch(
    USE_SYSTEM_PACKAGE
    OFF
    PACKAGE_NAME
    GTest
    LIBRARY_NAME
    googletest
    GIT_REPO
    https://github.com/google/googletest.git
    GIT_TAG
    ${MUJOCO_DEP_VERSION_gtest}
    TARGETS
    gtest
    gmock
    gtest_main
    EXCLUDE_FROM_ALL
  )

  set(BENCHMARK_EXTRA_FETCH_ARGS "")
  if(WIN32 AND NOT MSVC)
    set(BENCHMARK_EXTRA_FETCH_ARGS
        PATCH_COMMAND
        "sed"
        "-i"
        "-e"
        "s/-Wformat=2/-Wformat/g"
        "${CMAKE_BINARY_DIR}/_deps/benchmark-src/CMakeLists.txt"
    )
  endif()

  set(BENCHMARK_ENABLE_TESTING OFF)

  findorfetch(
    USE_SYSTEM_PACKAGE
    OFF
    PACKAGE_NAME
    benchmark
    LIBRARY_NAME
    benchmark
    GIT_REPO
    https://github.com/google/benchmark.git
    GIT_TAG
    ${MUJOCO_DEP_VERSION_benchmark}
    TARGETS
    benchmark::benchmark
    benchmark::benchmark_main
    ${BENCHMARK_EXTRA_FETCH_ARGS}
    EXCLUDE_FROM_ALL
  )
endif()

if(MUJOCO_TEST_PYTHON_UTIL)
  add_compile_definitions(EIGEN_MPL2_ONLY)
  if(NOT TARGET eigen)
    # Support new IN_LIST if() operator.
    set(CMAKE_POLICY_DEFAULT_CMP0057 NEW)

    FetchContent_Declare(
      Eigen3
      GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
      GIT_TAG ${MUJOCO_DEP_VERSION_Eigen3}
    )

    FetchContent_GetProperties(Eigen3)
    if(NOT Eigen3_POPULATED)
      FetchContent_Populate(Eigen3)

      # Mark the library as IMPORTED as a workaround for https://gitlab.kitware.com/cmake/cmake/-/issues/15415
      add_library(Eigen3::Eigen INTERFACE IMPORTED)
      set_target_properties(
        Eigen3::Eigen PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${eigen3_SOURCE_DIR}"
      )
    endif()
  endif()
endif()

# Reset BUILD_SHARED_LIBS to its previous value
set(BUILD_SHARED_LIBS
    ${BUILD_SHARED_LIBS_OLD}
    CACHE BOOL "Build MuJoCo as a shared library" FORCE
)
