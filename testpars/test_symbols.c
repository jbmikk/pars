#include <stddef.h>
#include <string.h>
#include <glib.h>
#include <stdio.h>

#include "symbols.h"

typedef struct {
} Fixture;

void setup(Fixture *fix, gconstpointer data){
}

void teardown(Fixture *fix, gconstpointer data){
}

void symbol_to_buffer__one_byte(Fixture *fix, gconstpointer data){

	int symbol = 40;
	unsigned char buffer[sizeof(int)];
	unsigned int size;

	symbol_to_buffer(buffer, &size, symbol);

	g_assert(size == 1);
	g_assert(buffer[0] == 40);
}

void symbol_to_buffer__two_bytes(Fixture *fix, gconstpointer data){

	int symbol = 258;
	unsigned char buffer[sizeof(int)];
	unsigned int size;

	symbol_to_buffer(buffer, &size, symbol);

	g_assert(size == 2);
	g_assert(buffer[0] == 2);
	g_assert(buffer[1] == 1);
}

void symbol_to_buffer__negative(Fixture *fix, gconstpointer data){

	int symbol = -1;
	unsigned char buffer[sizeof(int)];
	unsigned int size;

	symbol_to_buffer(buffer, &size, symbol);

	g_assert(size == sizeof(int));
	g_assert(buffer[0] == 0xFF);
}

void buffer_to_symbol__one_byte(Fixture *fix, gconstpointer data){

	int symbol = 0;
	unsigned char buffer[sizeof(int)];
	unsigned int size;

	size = 1;
	buffer[0] = 40;

	symbol = buffer_to_symbol(buffer, size);

	g_assert(symbol == 40);
}

void buffer_to_symbol__two_bytes(Fixture *fix, gconstpointer data){

	int symbol = 0;
	unsigned char buffer[sizeof(int)];
	unsigned int size;

	size = 2;
	buffer[0] = 2;
	buffer[1] = 1;

	symbol = buffer_to_symbol(buffer, size);

	g_assert(symbol == 258);
}

void buffer_to_symbol__negative(Fixture *fix, gconstpointer data){

	int symbol = 0;
	unsigned char buffer[sizeof(int)];
	unsigned int size;

	size = sizeof(int);
	int i;
	for(i = 0; i < size; i++) {
		buffer[i] = 0xFF;
	}

	symbol = buffer_to_symbol(buffer, size);

	g_assert(symbol == -1);
}

int main(int argc, char** argv){
	g_test_init(&argc, &argv, NULL);
	g_test_add("/Symbols/symbol_to_buffer", Fixture, NULL, setup, symbol_to_buffer__one_byte, teardown);
	g_test_add("/Symbols/symbol_to_buffer", Fixture, NULL, setup, symbol_to_buffer__two_bytes, teardown);
	g_test_add("/Symbols/symbol_to_buffer", Fixture, NULL, setup, symbol_to_buffer__negative, teardown);
	g_test_add("/Symbols/buffer_to_symbol", Fixture, NULL, setup, buffer_to_symbol__one_byte, teardown);
	g_test_add("/Symbols/buffer_to_symbol", Fixture, NULL, setup, buffer_to_symbol__two_bytes, teardown);
	g_test_add("/Symbols/buffer_to_symbol", Fixture, NULL, setup, buffer_to_symbol__negative, teardown);
	return g_test_run();
}

