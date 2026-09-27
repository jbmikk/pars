cmake_minimum_required (VERSION 3.25.0)
project (pars LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON) # Only needed for typeof

include(FetchContent)
include(CTest)

# Traces
# ------

option(ENABLE_FSM_TRACE "Enable fsm traces" OFF)
option(ENABLE_AST_TRACE "Enable ast traces" OFF)
option(TRACE "Enable all traces" OFF)
option(ENABLE_TESTING "Enable test targets" OFF)

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

FetchContent_Declare(
        dbg
        GIT_REPOSITORY git@github.com:jbmikk/dbg.git
        GIT_TAG 1.0.1
)


FetchContent_Declare(
        dstlib
        GIT_REPOSITORY git@github.com:jbmikk/dstlib.git
        GIT_TAG 0.15.1
)

FetchContent_Declare(
        ctest
        GIT_REPOSITORY git@github.com:jbmikk/ctest.git
        GIT_TAG 1.0.1
)

block(SCOPE_FOR VARIABLES)
        # make sure we don't register dependency tests
        set(ENABLE_TESTING OFF)
        FetchContent_MakeAvailable(dbg dstlib ctest)
endblock()


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

# automatically available to any target that links the library
target_include_directories(dstlib PUBLIC "${dstlib_SOURCE_DIR}/src")

add_library(parslib ${SOURCES})
target_include_directories(parslib PRIVATE
        "${PROJECT_SOURCE_DIR}/src"
        "${dbg_SOURCE_DIR}"
)
target_compile_options(parslib PRIVATE -Wall)

add_executable (pars  ${SOURCES})
target_include_directories(pars PRIVATE
        "${PROJECT_SOURCE_DIR}/src"
        "${dbg_SOURCE_DIR}"
)
target_compile_options(pars PRIVATE -Wall)

target_link_libraries(parslib dstlib)
target_link_libraries(pars dstlib)

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

if(ENABLE_TESTING)
        find_program(VALGRIND_EXECUTABLE valgrind)
        find_program(GPROF_EXECUTABLE gprof)

        foreach(test_target IN LISTS TEST_TARGETS)
                add_executable(${test_target} test/${test_target}.c)

                target_include_directories(${test_target} PRIVATE
                        "${PROJECT_SOURCE_DIR}/src"
                        "${ctest_SOURCE_DIR}/src"
                )
                target_compile_options(${test_target} PRIVATE -Wall)
                target_link_libraries(${test_target} parslib)

                add_test(NAME ${test_target} COMMAND ${test_target})
                set_tests_properties(${test_target} PROPERTIES LABELS regular)

                if(GPROF_EXECUTABLE)
                        # TODO: Save output "gprof executable gmon.out > output.txt"
                        add_test(NAME ${test_target}_profile COMMAND "${GPROF_EXECUTABLE}" "$<TARGET_FILE:${test_target}>")
                        set_tests_properties(${test_target}_profile PROPERTIES LABELS profile)
                endif()

                if(VALGRIND_EXECUTABLE)
                        add_test(NAME ${test_target}_valgrind
                                COMMAND "${VALGRIND_EXECUTABLE}" --leak-check=full --show-leak-kinds=all --track-origins=yes --error-exitcode=1 --suppressions=${PROJECT_SOURCE_DIR}/suppressions.supp "$<TARGET_FILE:${test_target}>"
                        )
                        set_tests_properties(${test_target}_valgrind PROPERTIES LABELS valgrind)

                        add_test(NAME ${test_target}_callgrind_calls
                                COMMAND "${VALGRIND_EXECUTABLE}" --tool=callgrind "$<TARGET_FILE:${test_target}>"
                        )
                        set_tests_properties(${test_target}_callgrind_calls PROPERTIES LABELS callgrind_calls)

                        add_test(NAME ${test_target}_callgrind_simulate_cache
                                COMMAND "${VALGRIND_EXECUTABLE}" --tool=callgrind --simulate-cache=yes "$<TARGET_FILE:${test_target}>"
                        )
                        set_tests_properties(${test_target}_callgrind_simulate_cache PROPERTIES LABELS callgrind_simulate_cache)
                endif()
        endforeach()
endif()


message(STATUS "Copy Grammars")
file(COPY test/grammars DESTINATION .)
file(GLOB files "test/grammars/*")
foreach(filename ${files})
	file(COPY ${filename} DESTINATION "./test/grammars")
endforeach()
file(COPY test.sh DESTINATION .)

