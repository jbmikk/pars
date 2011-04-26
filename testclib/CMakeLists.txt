include_directories("${PROJECT_SOURCE_DIR}/clib")

find_package(GLIB REQUIRED)

include_directories(${GLIB_INCLUDE_DIR})
include_directories(${GLIB_INCLUDE_DIRS})

#add_executable(test_cradixtree test_cradixtree.c)

#target_link_libraries(test_cradixtree ${GLIB_LIBRARIES} cradixtree)

#file(COPY test.sh DESTINATION .)

