cmake_minimum_required (VERSION 2.8.0)
project (pars)

include(ExternalProject)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

option(ENABLE_FSM_TRACE "Enable fsm traces" OFF)
option(ENABLE_AST_TRACE "Enable ast traces" OFF)


# Depend on dbg
# -------------

add_custom_target(
        dependencies ALL sh install.sh
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
)

include_directories("${PROJECT_SOURCE_DIR}/vendor/dbg")


# Depend on dstlib 
# ----------------

include_directories("${CMAKE_BINARY_DIR}/dstlib_external-prefix/src/dstlib_external/dstlib")

set_property(DIRECTORY PROPERTY EP_STEP_TARGETS build)
externalproject_add(
        dstlib_external
        GIT_REPOSITORY git@github.com:jbmikk/dstlib.git
        DOWNLOAD_NAME dstlib
        DOWNLOAD_DIR dstlib
        INSTALL_COMMAND ""
)

add_library(dstlib STATIC IMPORTED GLOBAL) 

set_property(
        TARGET dstlib
        PROPERTY IMPORTED_LOCATION "${CMAKE_BINARY_DIR}/dstlib_external-prefix/src/dstlib_external-build/dstlib/libdstlib.a"
)


# Add subdirectories
# ------------------

add_subdirectory(src)
add_subdirectory(tests)

