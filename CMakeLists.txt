cmake_minimum_required (VERSION 2.8.0)
project (pars)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

option(ENABLE_FSM_TRACE "Enable pars traces" OFF)

add_subdirectory(clib)
add_subdirectory(testclib)
add_subdirectory(pars)
add_subdirectory(testpars)

