#include "symbols.h"

#include "cmemory.h"

void symbol_table_init(SymbolTable *table)
{
	table->id_base = -1;
	radix_tree_init(&table->symbols, 0, 0, NULL);
}

Symbol *symbol_table_add(SymbolTable *table, char *name, unsigned int length)
{
	Symbol *symbol = radix_tree_get(&table->symbols, name, length);
	if(!symbol) {
		symbol = c_new(Symbol, 1);
		symbol->id = table->id_base--;
		symbol->length = length;
		symbol->name = c_new(char, length); 
		int i = 0;
		for(i; i < length; i++) {
			symbol->name[i] = name[i];
		}
		radix_tree_set(&table->symbols, name, length, symbol);
	}
	return symbol;
}

Symbol *symbol_table_get(SymbolTable *table, char *name, unsigned int length)
{
	return radix_tree_get(&table->symbols, name, length);
}

void symbol_table_dispose(SymbolTable *table)
{
	Iterator it;
	Symbol *symbol;

	radix_tree_iterator_init(&it, &table->symbols);
	while(symbol = (Symbol *)radix_tree_iterator_next(&it)) {
		//Get all actions reachable through other rules
		c_delete(symbol->name);
		c_delete(symbol);
	}
	radix_tree_iterator_dispose(&it);
	radix_tree_dispose(&table->symbols);
}
