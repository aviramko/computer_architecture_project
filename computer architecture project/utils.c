#define _CRT_SECURE_NO_DEPRECATE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

/*void initialize_args(char* args_files[ARGS_EXPECTED_NUM - 1], int args_num, char* args_values[])
{
	if (args_num != ARGS_EXPECTED_NUM)
	{
		args_files[0] = DEFAULT_FILE_IMEM0;
		args_files[1] = DEFAULT_FILE_IMEM1;
		args_files[2] = DEFAULT_FILE_IMEM2;
		args_files[3] = DEFAULT_FILE_IMEM3;
		args_files[4] = DEFAULT_FILE_MEMIN;
		args_files[5] = DEFAULT_FILE_MEMOUT;
		args_files[6] = DEFAULT_FILE_REGOUT0;
		args_files[7] = DEFAULT_FILE_REGOUT1;
		args_files[8] = DEFAULT_FILE_REGOUT2;
		args_files[9] = DEFAULT_FILE_REGOUT3;
		args_files[10] = DEFAULT_FILE_CORE0TRACE;
		args_files[11] = DEFAULT_FILE_CORE1TRACE;
		args_files[12] = DEFAULT_FILE_CORE2TRACE;
		args_files[13] = DEFAULT_FILE_CORE3TRACE;
		args_files[14] = DEFAULT_FILE_BUSTRACE;
		args_files[15] = DEFAULT_FILE_DSRAM0;
		args_files[16] = DEFAULT_FILE_DSRAM1;
		args_files[17] = DEFAULT_FILE_DSRAM2;
		args_files[18] = DEFAULT_FILE_DSRAM3;
		args_files[19] = DEFAULT_FILE_TSRAM0;
		args_files[20] = DEFAULT_FILE_TSRAM1;
		args_files[21] = DEFAULT_FILE_TSRAM2;
		args_files[22] = DEFAULT_FILE_TSRAM3;
		args_files[23] = DEFAULT_FILE_STATS0;
		args_files[24] = DEFAULT_FILE_STATS1;
		args_files[25] = DEFAULT_FILE_STATS2;
		args_files[26] = DEFAULT_FILE_STATS3;
		return;
	}
	for (int i = 0; i < ARGS_EXPECTED_NUM - 1; i++)
		args_files[i] = args_values[i + 1];
}*/

// Initializing int array from a file. return memory length
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

// Writes to bustrace file
int write_bustrace(msi_bus* bus, int cycle, char* bustrace_file)
{
	if (cycle == 1)
	{
		int ret = remove(bustrace_file);
		if (ret != SUCCESS_CODE)
		{
			printf("ERROR: cannot delete file '%s'\n", bustrace_file);
			return ERROR_CODE;
		}
	}
	FILE* file_pointer = fopen(bustrace_file, "a");

	if (file_pointer == NULL)
	{
		printf("ERROR: cannot open file '%s'\n", bustrace_file);
		return ERROR_CODE;
	}

	if (bus->bus_cmd != BUS_NO_CMD_CODE)
		fprintf(file_pointer, "%d %01X %01X %03X%02X %08X %01X\n", cycle, bus->bus_origid, bus->bus_cmd, bus->bus_addr.tag, bus->bus_addr.index, bus->bus_data, bus->bus_shared);

	fclose(file_pointer);

	return SUCCESS_CODE;
}

// Writes registers values for specific core - regout file
int write_regout(core* core, char* regout_file)
{
	FILE* file_pointer = fopen(regout_file, "w");

	if (file_pointer == NULL)
	{
		printf("ERROR: cannot open file '%s'\n", regout_file);
		return ERROR_CODE;
	}

	for (int i = 2; i < NUM_OF_REGS; i++)
		fprintf(file_pointer, "%08X\n", core->core_registers[i]);

	fclose(file_pointer);
	return SUCCESS_CODE;
}

// Writes stats values for specific core - stats file
int write_stats(core* core, char* stats_file)
{
	FILE* file_pointer = fopen(stats_file, "w");

	if (file_pointer == NULL)
	{
		printf("ERROR: cannot open file '%s'\n", stats_file);
		return ERROR_CODE;
	}

	fprintf(file_pointer, "cycles %d\n", core->stat.cycles);
	fprintf(file_pointer, "instructions %d\n", core->stat.instructions);
	fprintf(file_pointer, "read_hit %d\n", core->stat.read_hit);
	fprintf(file_pointer, "write_hit %d\n", core->stat.write_hit);
	fprintf(file_pointer, "read_miss %d\n", core->stat.read_miss);
	fprintf(file_pointer, "write_miss %d\n", core->stat.write_miss);
	fprintf(file_pointer, "decode_stall %d\n", core->stat.decode_stall);
	fprintf(file_pointer, "mem_stall %d\n", core->stat.mem_stall);

	fclose(file_pointer);
	return SUCCESS_CODE;
}

// Writes dsram values for specific core - dsram file
int write_dsram(core* core, char* dsram_file)
{
	FILE* file_pointer = fopen(dsram_file, "w");

	if (file_pointer == NULL)
	{
		printf("ERROR: cannot open file '%s'\n", dsram_file);
		return ERROR_CODE;
	}

	for (int i = 0; i < DSRAM_SIZE; i++)
		fprintf(file_pointer, "%08X\n", core->core_cache.dsram[i]);

	fclose(file_pointer);
	return SUCCESS_CODE;
}

// Writes tsram values for specific core - tsram file
int write_tsram(core* core, char* tsram_file)
{
	FILE* file_pointer = fopen(tsram_file, "w");

	if (file_pointer == NULL)
	{
		printf("ERROR: cannot open file '%s'\n", tsram_file);
		return ERROR_CODE;
	}

	for (int i = 0; i < TSRAM_SIZE; i++)
		fprintf(file_pointer, "%05X%03X\n", core->core_cache.tsram[i].MESI_state, core->core_cache.tsram[i].tag);

	fclose(file_pointer);
	return SUCCESS_CODE;
}

// Writes main memory values to memout file
int write_memout(int main_mem[MAIN_MEM_SIZE], char* memout_file)
{
	FILE* file_pointer = fopen(memout_file, "w");

	if (file_pointer == NULL)
	{
		printf("ERROR: cannot open file '%s'\n", memout_file);
		return ERROR_CODE;
	}

	int last_filled = 0;

	for (int i = 0; i < MAIN_MEM_SIZE; i++)
		if (main_mem[i] != 0)
			last_filled = i + 1;

	for (int i = 0; i < last_filled; i++)
		fprintf(file_pointer, "%08X\n", main_mem[i]);

	fclose(file_pointer);
	return SUCCESS_CODE;
}

// Activate all output function at the end
int write_files(core* cores, char** output_files)
{

}