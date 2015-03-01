include_directories("${PROJECT_SOURCE_DIR}/clib")

find_package(GLIB REQUIRED)

include_directories(${GLIB_INCLUDE_DIR})
include_directories(${GLIB_INCLUDE_DIRS})

add_executable(test_bsearch test_bsearch.c)
add_executable(test_cradixtree test_cradixtree.c)

target_link_libraries(test_bsearch ${GLIB_LIBRARIES} bsearch)
target_link_libraries(test_cradixtree ${GLIB_LIBRARIES} cradixtree)

#file(COPY test.sh DESTINATION .)

