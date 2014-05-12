include_directories("${PROJECT_SOURCE_DIR}/clib")

add_library (lexer lexer.c)

add_library (ebnf_parser ebnf_parser.c)

add_library (fsm fsm.c)

add_library (ast ast.c)

add_library (processor processor.c)

add_library (symbols symbols.c)


if(ENABLE_FSM_TRACE)
	set_target_properties(fsm PROPERTIES COMPILE_FLAGS -DFSM_TRACE)
endif(ENABLE_FSM_TRACE)

add_library (parslib pars.c)
set_target_properties(parslib PROPERTIES COMPILE_FLAGS -DLIBRARY)

add_executable (pars pars.c)

target_link_libraries(lexer cmemory)
target_link_libraries(fsm cradixtree symbols)
target_link_libraries(ast cradixtree symbols stack)
target_link_libraries(ebnf_parser fsm lexer)
target_link_libraries(processor ast)
target_link_libraries(parslib cmemory lexer ebnf_parser processor)
target_link_libraries(pars cmemory lexer ebnf_parser processor)

