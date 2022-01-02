#define _CRT_SECURE_NO_DEPRECATE

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "parser.h"

#define MAX_IMEM_LINE_WIDTH 10

void remove_spaces(char* s) {
	char* d = s;
	do 
	{
		while (*d == ' ' || *d == '\t' || *d == '\r') 
		{
			++d;
		}
	} while (*s++ = *d++);
}

void parse_line_to_mem(core *core, char *line_buffer, int imem_index)
{

	line_buffer[strcspn(line_buffer, "\n")] = '\0';		// replace end-of-line char with end-of-string char
	remove_spaces(line_buffer);							// remove all spaces before parsing
	int instruction_value = strtol(line_buffer, NULL, 16);

	core->core_imem[imem_index].opcode = (instruction_value & (0xFF << 24)) >> 24;
	core->core_imem[imem_index].rd = (instruction_value & (0xF << 20)) >> 20;
	core->core_imem[imem_index].rs = (instruction_value & (0xF << 16)) >> 16;
	core->core_imem[imem_index].rt = (instruction_value & (0xF << 12)) >> 12;
	core->core_imem[imem_index].immediate = instruction_value & 0xFFF;

	core->core_imem[imem_index].PC = imem_index;
	core->core_imem[imem_index].stalled = false;
}

void parse_imem_file(core *core, char *imem_filename)
{
	FILE *ptr_imem_file = fopen(imem_filename, "r");
	int imem_iterator = 0;
	char line_buffer[MAX_IMEM_LINE_WIDTH];

	while (fgets(line_buffer, MAX_IMEM_LINE_WIDTH, ptr_imem_file))
	{
		if (strcmp(line_buffer, "\n") == 0)
			continue;
		parse_line_to_mem(core, line_buffer, imem_iterator);
		imem_iterator++;
	}

	core->core_imem_length = imem_iterator;

	fclose(ptr_imem_file);
}

