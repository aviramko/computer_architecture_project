////////////////////////////////////////////////////////////////
//////////		Computer Architecture Project		////////////
////////////////////////////////////////////////////////////////

#define _CRT_SECURE_NO_DEPRECATE

//#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core.h"

int main(int argc, char* argv[])
{
	core core0;// *core1 = NULL, *core2 = NULL, *core3 = NULL;
	msi_bus bus;
	msi_bus empty_request;
	int main_mem[MAIN_MEM_SIZE];

	int i, cycle = 0;

	initialize_main_mem(main_mem);
	initialize_core(&core0, argv[1]);

	FILE* fp_core0trace = fopen(argv[11], "w");

	while (!core0.core_halt) // all cores halt
	{
		//update_bus(cycle);

		//main_memory_bus_snooper(core,cycle);

		//for(i=0; i<CORES_NUM; i++)
		simulate_clock_cycle(&core0, fp_core0trace);

		//for(i=0; i<CORES_NUM; i++)
		// core_bus_snooper

		cycle++;

		//do_bus_and_main_mem_stuff(&core0, main_mem, bus, memory_bus_request);
		//do_bus_and_main_mem_stuff(cores_array, main_mem_array, bus);
	}

	// print to files

	fclose(fp_core0trace);

	return 0;
}

//int main(int argc, char* argv[])
//{
//	core cores_array[CORES_NUM];
//	initialize_core(&(cores_array[0]), argv[1]);
//	initialize_core(&(cores_array[1]), argv[2]);
//	initialize_core(&(cores_array[2]), argv[3]);
//	initialize_core(&(cores_array[3]), argv[4]);
//	msi_bus bus[CORES_NUM];
//	while (!all_cores_finished())
//	{
//		simulate_clock_cycle(&(cores_array[0]), fp_core0trace, &(bus[0]));
//		simulate_clock_cycle(&(cores_array[1]), fp_core1trace, &(bus[1]));
//		simulate_clock_cycle(&(cores_array[2]), fp_core2trace, &(bus[2]));
//		simulate_clock_cycle(&(cores_array[3]), fp_core3trace, &(bus[3]));
//		do_bus_and_main_mem_stuff(cores_array, main_mem_array, bus);
//	}
//
//	return 0;
//}