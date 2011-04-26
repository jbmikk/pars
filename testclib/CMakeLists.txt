include_directories("${PROJECT_SOURCE_DIR}/clib")

find_package(GLIB REQUIRED)

include_directories(${GLIB_INCLUDE_DIR})
include_directories(${GLIB_INCLUDE_DIRS})

add_executable(test_cbsearch test_cbsearch.c)

target_link_libraries(test_cbsearch ${GLIB_LIBRARIES} cbsearch)

#file(COPY test.sh DESTINATION .)

