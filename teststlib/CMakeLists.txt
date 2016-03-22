include_directories("${PROJECT_SOURCE_DIR}/stlib")

find_package(GLIB REQUIRED)

include_directories(${GLIB_INCLUDE_DIR})
include_directories(${GLIB_INCLUDE_DIRS})

add_executable(test_bsearch test_bsearch.c)
add_executable(test_radixtree test_radixtree.c)
add_executable(test_arrays test_arrays.c)

target_link_libraries(test_bsearch ${GLIB_LIBRARIES} bsearch radixtree)
target_link_libraries(test_radixtree ${GLIB_LIBRARIES} radixtree)
target_link_libraries(test_arrays ${GLIB_LIBRARIES} arrays)

#file(COPY test.sh DESTINATION .)

