////////////////////////////////////////////////////////////////
//////////		Computer Architecture Project		////////////
////////////////////////////////////////////////////////////////

#define _CRT_SECURE_NO_DEPRECATE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core.h"

int main(int argc, char* argv[])
{
	core core0;// *core1 = NULL, *core2 = NULL, *core3 = NULL;

	initialize_core(&core0, argv[1]);
	FILE* fp_core0trace = fopen(argv[11], "w");

	while (!core0.core_halt)
	{
		simulate_clock_cycle(&core0, fp_core0trace);
	}

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
//
//	while (!all_cores_finished())
//	{
//		simulate_clock_cycle(&(cores_array[0]), fp_core0trace);
//		simulate_clock_cycle(&(cores_array[1]), fp_core1trace);
//		simulate_clock_cycle(&(cores_array[2]), fp_core2trace);
//		simulate_clock_cycle(&(cores_array[3]), fp_core3trace);
//		do_bus_and_main_mem_stuff(cores_array, main_mem_array, bus);
//	}
//
//	return 0;
//}