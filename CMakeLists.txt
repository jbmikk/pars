cmake_minimum_required (VERSION 2.8.0)
project (pars)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

add_subdirectory(clib)
add_subdirectory(pars)
add_subdirectory(testpars)

