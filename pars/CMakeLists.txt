include_directories("${PROJECT_SOURCE_DIR}/clib")

add_library (lexer lexer.c)
add_library (pars pars.c)

target_link_libraries(lexer cmemory)
target_link_libraries(pars cmemory lexer)

