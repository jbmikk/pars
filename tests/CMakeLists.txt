include_directories("${PROJECT_SOURCE_DIR}/src")

add_executable(testpars testpars.c)
add_executable(test_input test_input.c)
add_executable(test_lexer test_lexer.c)
add_executable(test_ebnf_parser test_ebnf_parser.c)
add_executable(test_fsm test_fsm.c)
add_executable(test_ast test_ast.c)

target_link_libraries(testpars parslib)
target_link_libraries(test_input parslib)
target_link_libraries(test_lexer parslib)
target_link_libraries(test_ebnf_parser parslib)
target_link_libraries(test_fsm parslib)
target_link_libraries(test_ast parslib)

message(STATUS "Copy Grammars")
file(COPY grammars DESTINATION .)
file(GLOB files "grammars/*")
foreach(filename ${files})
	file(COPY ${filename} DESTINATION "./grammars")
endforeach()
file(COPY test.sh DESTINATION .)

