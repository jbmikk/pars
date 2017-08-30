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
        GIT_TAG 0.3.0
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

add_executable(test_cli tests/test_cli.c)
add_executable(test_input tests/test_input.c)
add_executable(test_lexer tests/test_lexer.c)
add_executable(test_ebnf_parser tests/test_ebnf_parser.c)
add_executable(test_fsm tests/test_fsm.c)
add_executable(test_parser tests/test_parser.c)
add_executable(test_ast tests/test_ast.c)
add_executable(test_astquery tests/test_astquery.c)

target_link_libraries(test_cli parslib)
target_link_libraries(test_input parslib)
target_link_libraries(test_lexer parslib)
target_link_libraries(test_ebnf_parser parslib)
target_link_libraries(test_fsm parslib)
target_link_libraries(test_parser parslib)
target_link_libraries(test_ast parslib)
target_link_libraries(test_astquery parslib)

message(STATUS "Copy Grammars")
file(COPY tests/grammars DESTINATION .)
file(GLOB files "tests/grammars/*")
foreach(filename ${files})
	file(COPY ${filename} DESTINATION "./tests/grammars")
endforeach()
file(COPY test.sh DESTINATION .)

