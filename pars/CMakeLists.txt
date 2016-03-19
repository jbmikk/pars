include_directories("${PROJECT_SOURCE_DIR}/stlib")

add_library (symbols symbols.c)

add_library (radixtree_ext radixtree_ext.c)

add_library (input input.c)

add_library (lexer lexer.c)

add_library (parser parser.c)

add_library (ebnf_parser ebnf_parser.c)

add_library (fsm fsm.c)

add_library (ast ast.c)


if(ENABLE_FSM_TRACE)
	set_target_properties(fsm PROPERTIES COMPILE_FLAGS -DFSM_TRACE)
endif(ENABLE_FSM_TRACE)

if(ENABLE_AST_TRACE)
	set_target_properties(ast PROPERTIES COMPILE_FLAGS -DAST_TRACE)
endif(ENABLE_AST_TRACE)

add_library (parslib pars.c)
set_target_properties(parslib PROPERTIES COMPILE_FLAGS -DLIBRARY)

add_executable (pars pars.c)

target_link_libraries(input cmemory)
target_link_libraries(lexer input)
target_link_libraries(fsm radixtree radixtree_ext symbols stack)
target_link_libraries(ast radixtree radixtree_ext symbols stack)
target_link_libraries(ebnf_parser fsm lexer ast)
target_link_libraries(parslib cmemory input parser ebnf_parser)
target_link_libraries(pars cmemory input parser ebnf_parser)

