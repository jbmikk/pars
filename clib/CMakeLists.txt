add_library (cmemory cmemory.c)
add_library (stack stack.c)
add_library (bsearch bsearch.c cmemory.c)
add_library (cradixtree cradixtree.c cmemory.c bsearch.c)
