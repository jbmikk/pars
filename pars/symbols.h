#ifndef SYMBOLS_H
#define SYMBOLS_H

#include "radixtree.h"

typedef struct _Symbol {
	int id;
	char *name;
	int length;
} Symbol;

typedef struct _SymbolTable {
	Node symbols;
	int id_base;
} SymbolTable;

void symbol_table_init(SymbolTable *table);
Symbol *symbol_table_add(SymbolTable *table, char *name, unsigned int length);
Symbol *symbol_table_get(SymbolTable *table, char *name, unsigned int length);
void symbol_table_dispose(SymbolTable *table);

#endif //SYMBOLS_H
