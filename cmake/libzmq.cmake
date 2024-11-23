cmake_minimum_required(VERSION 3.1.2)
include(ExternalProject)
function(ExternalAutotools AUTOTOOLS_NAME AUTOTOOLS_BUILD INSTALL_DIR) #AUTOTOOLS_NAME 项目名字   AUTOTOOLS_BUILD 构建的路径 INSTALL_DIR 安装路径
    ExternalProject_Add(${AUTOTOOLS_NAME}
        SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/${AUTOTOOLS_NAME}"
        BINARY_DIR "${AUTOTOOLS_BUILD}"
        CONFIGURE_COMMAND ${CMAKE_COMMAND} -E make_directory <${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/zmqlib> 
                          && cd <BINARY_DIR> && ../../3rdParty/${AUTOTOOLS_NAME}/configure --prefix=${INSTALL_DIR}
        BUILD_COMMAND     cd ${AUTOTOOLS_BUILD} && $(MAKE)
        INSTALL_COMMAND   cd ${AUTOTOOLS_BUILD} && $(MAKE) install
    )
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/${AUTOTOOLS_NAME}/include)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/zmqlib/lib)
endfunction()