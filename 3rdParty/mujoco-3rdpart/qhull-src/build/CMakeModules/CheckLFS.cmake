# CheckLFS.cmake - Check for Large File Support
# From the qhull project: https://github.com/qhull/qhull
#
# check_lfs(<var>) — checks whether the system supports Large File Support
# (64-bit file offsets). Sets <var> to TRUE/FALSE in the parent scope.

include(CheckIncludeFile)
include(CheckSymbolExists)
include(CMakePushCheckState)

function(check_lfs LFS_VAR)
  cmake_push_check_state()
  if(MSVC)
    # MSVC always supports 64-bit file I/O
    set(HAVE_LFS TRUE)
  else()
    # Check for Large File Support on Unix-like systems
    set(CMAKE_REQUIRED_DEFINITIONS -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE)
    check_symbol_exists(fseeko "stdio.h" HAVE_FSEEKO)
    if(HAVE_FSEEKO)
      set(HAVE_LFS TRUE)
    else()
      set(HAVE_LFS FALSE)
    endif()
  endif()
  cmake_pop_check_state()

  if(NOT HAVE_LFS)
    message(STATUS "Large File Support (LFS) is not available on this system.")
  endif()

  set(${LFS_VAR} ${HAVE_LFS} PARENT_SCOPE)
endfunction()
