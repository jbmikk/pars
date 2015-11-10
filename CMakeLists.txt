cmake_minimum_required (VERSION 2.8.0)
project (pars)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

option(ENABLE_FSM_TRACE "Enable fsm traces" OFF)
option(ENABLE_AST_TRACE "Enable ast traces" OFF)

include_directories("${PROJECT_SOURCE_DIR}/vendor/dbg")

add_subdirectory(stlib)
add_subdirectory(teststlib)
add_subdirectory(pars)
add_subdirectory(testpars)

