#include "symbols.h"

#include "cmemory.h"
#include <stdlib.h>

void symbol_table_init(SymbolTable *table)
{
	table->id_base = -1;
	rtree_init(&table->symbols);
	rtree_init(&table->symbols_by_id);
}

Symbol *symbol_table_add(SymbolTable *table, char *name, unsigned int length)
{
	Symbol *symbol = rtree_get(&table->symbols, (unsigned char*)name, length);
	if(!symbol) {
		symbol = malloc(sizeof(Symbol));
		symbol->id = table->id_base--;
		symbol->length = length;
		symbol->name = malloc(sizeof(char) * (length + 1)); 
		int i = 0;
		for(; i < length; i++) {
			symbol->name[i] = name[i];
		}
		symbol->name[i] = '\0';
		rtree_set(&table->symbols, (unsigned char*)name, length, symbol);
		rtree_set_int(&table->symbols_by_id, symbol->id, symbol);
	}
	return symbol;
}

Symbol *symbol_table_get(SymbolTable *table, char *name, unsigned int length)
{
	return rtree_get(&table->symbols, (unsigned char*)name, length);
}

Symbol *symbol_table_get_by_id(SymbolTable *table, int id)
{
	return rtree_get_int(&table->symbols_by_id, id);
}

void symbol_table_dispose(SymbolTable *table)
{
	Iterator it;
	Symbol *symbol;

	rtree_iterator_init(&it, &table->symbols);
	while((symbol = (Symbol *)rtree_iterator_next(&it))) {
		free(symbol->name);
		free(symbol);
	}
	rtree_iterator_dispose(&it);
	rtree_dispose(&table->symbols);
	rtree_dispose(&table->symbols_by_id);
}
