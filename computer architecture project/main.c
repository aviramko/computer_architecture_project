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
	int valid_request[CORES_NUM], memory_request_cycle[CORES_NUM];

	int cycle = 0;
	int next_RR = 0;

	char* args_files[ARGS_EXPECTED_NUM - 1];
	initialize_args_files(args_files, argc, argv);

	initialize_bus(&bus);
	initialize_main_mem(main_mem, valid_request, memory_request_cycle);

	for (int i = 0; i < CORES_NUM; i++)
		initialize_core(&cores[i], args_files[i + 1]);
	//initialize_core(&cores[0], argv[1]);

	FILE* fp_core0trace = fopen(argv[11], "w");

	while (!all_cores_halt(cores)) // all cores halt
	{
		//main_memory_bus_snooper(cores, bus, cycle, main_mem, valid_request, memory_request_cycle);

		for (int core_num = 0; core_num < CORES_NUM; core_num++)
			simulate_clock_cycle(&cores[core_num], args_files[10 + core_num], main_mem, core_num);
		//simulate_clock_cycle(&cores[0], fp_core0trace, main_mem);

		update_bus(cores, &bus, cycle, &next_RR, valid_request, memory_request_cycle, main_mem);

		//for(i=0; i<CORES_NUM; i++)
		// core_bus_snooper

		//write_bustrace?

		cycle++;

		//do_bus_and_main_mem_stuff(&core0, main_mem, bus, memory_bus_request);
		//do_bus_and_main_mem_stuff(cores_array, main_mem_array, bus);
	}

	// print to files
	//if (write_files(cores, args_files, main_mem) == ERROR_CODE)
	//	return ERROR_CODE;

	fclose(fp_core0trace);

	return SUCCESS_CODE;
}