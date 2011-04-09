include_directories("${PROJECT_SOURCE_DIR}/pars")

find_package(GLIB REQUIRED)

include_directories(${GLIB_INCLUDE_DIR})
include_directories(${GLIB_INCLUDE_DIRS})

add_executable(testpars testpars.c)

target_link_libraries(testpars ${GLIB_LIBRARIES} pars)

