cmake_minimum_required (VERSION 3.2.0)
project (pars)

include(ExternalProject)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")

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


# Include external dbg
# -----------------------

include_directories("${CMAKE_BINARY_DIR}/dbg-prefix/src/dbg")

externalproject_add(
        dbg
        GIT_REPOSITORY git@github.com:jbmikk/dbg.git
        GIT_TAG 1.0.0
        DOWNLOAD_NAME dbg
        DOWNLOAD_DIR dbg
        INSTALL_COMMAND ""
        UPDATE_DISCONNECTED 1
)


# Include external dstlib 
# -----------------------

include_directories("${CMAKE_BINARY_DIR}/xdstlib-prefix/src/xdstlib/src")

externalproject_add(
        xdstlib
        GIT_REPOSITORY git@github.com:jbmikk/dstlib.git
        GIT_TAG 0.8.1
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
add_executable (pars  ${SOURCES})

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

include_directories("${PROJECT_SOURCE_DIR}/src")

add_executable(cli_test tests/cli_test.c)
add_executable(input_test tests/input_test.c)
add_executable(lexer_test tests/lexer_test.c)
add_executable(ebnf_parser_test tests/ebnf_parser_test.c)
add_executable(fsm_test tests/fsm_test.c)
add_executable(parser_test tests/parser_test.c)
add_executable(ast_test tests/ast_test.c)
add_executable(astquery_test tests/astquery_test.c)

target_link_libraries(cli_test parslib)
target_link_libraries(input_test parslib)
target_link_libraries(lexer_test parslib)
target_link_libraries(ebnf_parser_test parslib)
target_link_libraries(fsm_test parslib)
target_link_libraries(parser_test parslib)
target_link_libraries(ast_test parslib)
target_link_libraries(astquery_test parslib)

message(STATUS "Copy Grammars")
file(COPY tests/grammars DESTINATION .)
file(GLOB files "tests/grammars/*")
foreach(filename ${files})
	file(COPY ${filename} DESTINATION "./tests/grammars")
endforeach()
file(COPY test.sh DESTINATION .)

