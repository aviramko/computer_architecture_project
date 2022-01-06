#define _CRT_SECURE_NO_DEPRECATE

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "utils.h"

// yuval
// Gets address struct and returns its dec value.
int address_to_integer(address addr) 
{
	int result = addr.tag;

	result = result << 8;

	result = result + addr.index;

	return result;
}

// Initilizing int array from a file. return memory length
int initialize_array_from_file(char* file_name, int* memory_array, int max_array_size)
{
	int i = 0;
	FILE* file_pointer = fopen(file_name, "r");

	if (file_pointer == NULL) {
		printf("ERROR: cannot open file '%s'.", file_name);
		return ERROR_CODE;
	}

	while (EOF != fscanf(file_pointer, "%x\n", &memory_array[i]) && i < max_array_size) i++;

	fclose(file_pointer);
	return SUCCESS_CODE;
}
// Aviram asks - what if file is shorter than 2E20 lines? How can we tell which part of the memory array is initialized?


