include_directories("${PROJECT_SOURCE_DIR}/stlib")

add_library (
        parslib
        pars.c
        radixtree_ext.c
        symbols.c
        input.c
        lexer.c
        parser.c
        ebnf_parser.c
        fsm.c
        ast.c
)

add_executable (pars pars.c)

target_link_libraries(parslib stlib)
target_link_libraries(pars parslib stlib)

if(ENABLE_FSM_TRACE)
        set(FSM_TRACE_FLAGS "-DFSM_TRACE")
endif(ENABLE_FSM_TRACE)

if(ENABLE_AST_TRACE)
        set(AST_TRACE_FLAGS "-DAST_TRACE")
endif(ENABLE_AST_TRACE)

set(PARS_COMPILE_FLAGS "-DLIBRARY ${FSM_TRACE_FLAGS} ${AST_TRACE_FLAGS}")

set_target_properties(parslib PROPERTIES COMPILE_FLAGS "${PARS_COMPILE_FLAGS}")


