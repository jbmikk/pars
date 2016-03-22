#include <stddef.h>
#include <string.h>
#include <glib.h>
#include <stdio.h>

#include "arrays.h"

typedef struct {
} Fixture;

void setup(Fixture *fix, gconstpointer data){
}

void teardown(Fixture *fix, gconstpointer data){
}

void int_to_array__one_byte(Fixture *fix, gconstpointer data){

	int symbol = 40;
	unsigned char buffer[sizeof(int)];
	unsigned int size;

	int_to_array(buffer, &size, symbol);

	g_assert(size == 1);
	g_assert(buffer[0] == 40);
}

void int_to_array__two_bytes(Fixture *fix, gconstpointer data){

	int symbol = 258;
	unsigned char buffer[sizeof(int)];
	unsigned int size;

	int_to_array(buffer, &size, symbol);

	g_assert(size == 2);
	g_assert(buffer[0] == 2);
	g_assert(buffer[1] == 1);
}

void int_to_array__negative(Fixture *fix, gconstpointer data){

	int symbol = -1;
	unsigned char buffer[sizeof(int)];
	unsigned int size;

	int_to_array(buffer, &size, symbol);

	g_assert(size == sizeof(int));
	g_assert(buffer[0] == 0xFF);
}

void array_to_int__one_byte(Fixture *fix, gconstpointer data){

	int symbol = 0;
	unsigned char buffer[sizeof(int)];
	unsigned int size;

	size = 1;
	buffer[0] = 40;

	symbol = array_to_int(buffer, size);

	g_assert(symbol == 40);
}

void array_to_int__two_bytes(Fixture *fix, gconstpointer data){

	int symbol = 0;
	unsigned char buffer[sizeof(int)];
	unsigned int size;

	size = 2;
	buffer[0] = 2;
	buffer[1] = 1;

	symbol = array_to_int(buffer, size);

	g_assert(symbol == 258);
}

void array_to_int__negative(Fixture *fix, gconstpointer data){

	int symbol = 0;
	unsigned char buffer[sizeof(int)];
	unsigned int size;

	size = sizeof(int);
	int i;
	for(i = 0; i < size; i++) {
		buffer[i] = 0xFF;
	}

	symbol = array_to_int(buffer, size);

	g_assert(symbol == -1);
}

int main(int argc, char** argv){
	g_test_init(&argc, &argv, NULL);
	g_test_add("/Arrays/int_to_array", Fixture, NULL, setup, int_to_array__one_byte, teardown);
	g_test_add("/Arrays/int_to_array", Fixture, NULL, setup, int_to_array__two_bytes, teardown);
	g_test_add("/Arrays/int_to_array", Fixture, NULL, setup, int_to_array__negative, teardown);
	g_test_add("/Arrays/array_to_int", Fixture, NULL, setup, array_to_int__one_byte, teardown);
	g_test_add("/Arrays/array_to_int", Fixture, NULL, setup, array_to_int__two_bytes, teardown);
	g_test_add("/Arrays/array_to_int", Fixture, NULL, setup, array_to_int__negative, teardown);
	return g_test_run();
}

