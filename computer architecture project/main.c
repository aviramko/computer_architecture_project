////////////////////////////////////////////////////////////////
//////////		Computer Architecture Project		////////////
////////////////////////////////////////////////////////////////

#define _CRT_SECURE_NO_DEPRECATE

//#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hard_coded_data.h"
#include "core.h"

int main(int argc, char* argv[])
{
	core cores[CORES_NUM];
	msi_bus bus;
	static int main_mem[MAIN_MEM_SIZE];
	int memory_request_cycle[CORES_NUM];

	int cycle = 0;
	int next_RR = 0;

	char* args_files[ARGS_EXPECTED_NUM - 1];
	initialize_args_files(args_files, argc, argv);

	initialize_bus(&bus);
	initialize_main_mem(main_mem, memory_request_cycle);
	FILE *core_traces[CORES_NUM];
	
	FILE *tmp_fp= fopen(args_files[14], "w");

	fclose(tmp_fp);

	for (int i = 0; i < CORES_NUM; i++)
	{
		initialize_core(&cores[i], args_files[i]);
		core_traces[i] = fopen(args_files[10 + i], "w");
	}

	while (!all_cores_halt(cores)) // all cores halt
	{
		for (int core_num = 0; core_num < CORES_NUM; core_num++)
			simulate_clock_cycle(&cores[core_num], core_traces[core_num], main_mem, core_num);

		update_bus(cores, &bus, cycle, &next_RR, memory_request_cycle, main_mem);

		cycle++;
		if (cycle == 35832) 
		{ 
			continue;
		}
	}

	// print to files
	if (write_files(cores, args_files, main_mem) == ERROR_CODE)
		return ERROR_CODE;

	for (int i = 0; i < CORES_NUM; i++)
	{
		fclose(core_traces[i]);
	}

	return SUCCESS_CODE;
}