include_directories("${PROJECT_SOURCE_DIR}/clib")

add_library (lexer lexer.c)
add_library (ebnf_parser ebnf_parser.c)
add_library (fsm fsm.c)
add_library (pars pars.c)

target_link_libraries(lexer cmemory)
target_link_libraries(ebnf_parser lexer)
target_link_libraries(fsm cradixtree)
target_link_libraries(pars cmemory lexer)

