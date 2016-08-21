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


# Depend on dstlib 
# ----------------

include_directories("${CMAKE_BINARY_DIR}/dstlib_external-prefix/src/dstlib_external/src")

set_property(DIRECTORY PROPERTY EP_STEP_TARGETS build)
externalproject_add(
        dstlib_external
        GIT_REPOSITORY git@github.com:jbmikk/dstlib.git
        DOWNLOAD_NAME dstlib
        DOWNLOAD_DIR dstlib
        INSTALL_COMMAND ""
        UPDATE_DISCONNECTED 1
)

#externalproject_add_steptargets()

add_library(dstlib STATIC IMPORTED GLOBAL) 

set_property(
        TARGET dstlib
        PROPERTY IMPORTED_LOCATION "${CMAKE_BINARY_DIR}/dstlib_external-prefix/src/dstlib_external-build/src/libdstlib.a"
)


# Add subdirectories
# ------------------

add_subdirectory(src)
add_subdirectory(tests)

