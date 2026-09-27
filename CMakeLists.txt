cmake_minimum_required (VERSION 3.20.0)
project (pars LANGUAGES C)

include(ExternalProject)


# Traces
# ------

option(ENABLE_FSM_TRACE "Enable fsm traces" OFF)
option(ENABLE_AST_TRACE "Enable ast traces" OFF)
option(TRACE "Enable all traces" OFF)

if(TRACE)
        set(ENABLE_FSM_TRACE ON)
        set(ENABLE_AST_TRACE ON)
endif(TRACE)


# Profiling
# ---------

option(PROFILE "Enable profiling options" OFF)

if(PROFILE)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pg")
endif(PROFILE)


# Include externals
# -----------------

externalproject_add(
        dbg
        GIT_REPOSITORY git@github.com:jbmikk/dbg.git
        GIT_TAG 1.0.0
        DOWNLOAD_NAME dbg
        DOWNLOAD_DIR dbg
        INSTALL_COMMAND ""
        UPDATE_DISCONNECTED 1
)


externalproject_add(
        xdstlib
        GIT_REPOSITORY git@github.com:jbmikk/dstlib.git
        GIT_TAG 0.14.1
        DOWNLOAD_NAME dstlib
        DOWNLOAD_DIR dstlib
        INSTALL_COMMAND ""
        UPDATE_DISCONNECTED 1
)

externalproject_add(
        ctest
        GIT_REPOSITORY git@github.com:jbmikk/ctest.git
        DOWNLOAD_NAME ctest
        DOWNLOAD_DIR ctest
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


# Flags
# -----

if(ENABLE_FSM_TRACE)
        set(FSM_TRACE_FLAGS "-DFSM_TRACE")
endif(ENABLE_FSM_TRACE)

if(ENABLE_AST_TRACE)
        set(AST_TRACE_FLAGS "-DAST_TRACE")
endif(ENABLE_AST_TRACE)

set(PARS_FLAGS "${FSM_TRACE_FLAGS} ${AST_TRACE_FLAGS}")


# Pars
# ----

file(GLOB SOURCES src/*.c)

add_library(parslib ${SOURCES})
target_include_directories(parslib PRIVATE
        "${PROJECT_SOURCE_DIR}/src"
        "${CMAKE_BINARY_DIR}/dbg-prefix/src/dbg"
        "${CMAKE_BINARY_DIR}/xdstlib-prefix/src/xdstlib/src"
)
target_compile_options(parslib PRIVATE -Wall)

add_executable (pars  ${SOURCES})
target_include_directories(pars PRIVATE
        "${PROJECT_SOURCE_DIR}/src"
        "${CMAKE_BINARY_DIR}/dbg-prefix/src/dbg"
        "${CMAKE_BINARY_DIR}/xdstlib-prefix/src/xdstlib/src"
)
target_compile_options(pars PRIVATE -Wall)

target_link_libraries(parslib dstlib)
target_link_libraries(pars dstlib)

# Force download before compiling otherwise can't find headers (why?)
# This was not necessary when including subdirectories.
add_dependencies(parslib xdstlib dbg)
add_dependencies(pars xdstlib dbg)

set_target_properties(parslib PROPERTIES COMPILE_FLAGS "-DLIBRARY ${PARS_FLAGS}")
set_target_properties(pars PROPERTIES COMPILE_FLAGS "${PARS_FLAGS}")


# Tests
# -----

set(TEST_TARGETS
        cli_test
        source_test
        lexer_test
        ebnf_parser_test
        fsm_test
        parser_test
        ast_test
        astquery_test
)

foreach(test_target IN LISTS TEST_TARGETS)
        add_executable(${test_target} test/${test_target}.c)

        target_include_directories(${test_target} PRIVATE
                "${PROJECT_SOURCE_DIR}/src"
                "${CMAKE_BINARY_DIR}/xdstlib-prefix/src/xdstlib/src"
                "${CMAKE_BINARY_DIR}/ctest-prefix/src/ctest/src"
        )
        target_compile_options(${test_target} PRIVATE -Wall)
        target_link_libraries(${test_target} parslib)
endforeach()


message(STATUS "Copy Grammars")
file(COPY test/grammars DESTINATION .)
file(GLOB files "test/grammars/*")
foreach(filename ${files})
	file(COPY ${filename} DESTINATION "./test/grammars")
endforeach()
file(COPY test.sh DESTINATION .)

