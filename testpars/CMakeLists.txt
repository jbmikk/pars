include_directories("${PROJECT_SOURCE_DIR}/clib")
include_directories("${PROJECT_SOURCE_DIR}/pars")

find_package(GLIB REQUIRED)

include_directories(${GLIB_INCLUDE_DIR})
include_directories(${GLIB_INCLUDE_DIRS})

add_executable(testpars testpars.c)
add_executable(test_lexer test_lexer.c)
add_executable(test_ebnf_parser test_ebnf_parser.c)
add_executable(test_fsm test_fsm.c)
add_executable(test_ast test_ast.c)

target_link_libraries(testpars ${GLIB_LIBRARIES} parslib)
target_link_libraries(test_lexer ${GLIB_LIBRARIES} lexer)
target_link_libraries(test_ebnf_parser ${GLIB_LIBRARIES} ebnf_parser)
target_link_libraries(test_fsm ${GLIB_LIBRARIES} fsm)
target_link_libraries(test_ast ${GLIB_LIBRARIES} ast)

message(STATUS "Copy Grammars")
file(COPY grammars DESTINATION .)
file(GLOB files "grammars/*")
foreach(filename ${files})
	file(COPY ${filename} DESTINATION "./grammars")
endforeach()
file(COPY test.sh DESTINATION .)

