cmake_minimum_required (VERSION 3.2.0)
project (pars)

include(ExternalProject)

option(ENABLE_FSM_TRACE "Enable fsm traces" OFF)
option(ENABLE_AST_TRACE "Enable ast traces" OFF)


# Depend on dbg
# -------------

add_custom_target(
        dependencies ALL sh install.sh
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
)

include_directories("${PROJECT_SOURCE_DIR}/vendor/dbg")


# Include external dstlib 
# -----------------------

include_directories("${CMAKE_BINARY_DIR}/xdstlib-prefix/src/xdstlib/src")

externalproject_add(
        xdstlib
        GIT_REPOSITORY git@github.com:jbmikk/dstlib.git
        DOWNLOAD_NAME dstlib
        DOWNLOAD_DIR dstlib
        INSTALL_COMMAND ""
        UPDATE_DISCONNECTED 1
)


# Add dstlib static library
# -------------------------

set(DSTLIB_DIR "${CMAKE_BINARY_DIR}/xdstlib-prefix/src/xdstlib-build")

add_library(dstlib STATIC IMPORTED GLOBAL) 

set_property(
        TARGET dstlib
        PROPERTY IMPORTED_LOCATION "${DSTLIB_DIR}/libdstlib.a"
)


# Add dstlib update target
# ------------------------

externalproject_add_steptargets(xdstlib update)

add_custom_target(
        update 
        DEPENDS xdstlib-update
)


# Add subdirectories
# ------------------

add_subdirectory(src)
add_subdirectory(tests)

