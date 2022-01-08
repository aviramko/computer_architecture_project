#ifndef _CORE_HEADER_
#define _CORE_HEADER_

#include <stdio.h>
#include <stdbool.h>

#include "hard_coded_data.h"
#include "cache.h"
#include "bus_mem.h"
#include "utils.h"

//typedef enum ePIPELINE_STAGES {
//	FETCH = 0,
//	DECODE = 1,
//	EXECUTE = 2,
//	MEMORY = 3,
//	WRITE_BACK = 4,
//	EMPTY = 5
//} ePIPELINE_STAGES;

void initialize_core(core *core, char *imem_filename);
void simulate_clock_cycle(core *core, FILE *trace_file);
bool all_cores_halt(core cores[CORES_NUM]);

#endif // !CORE_HEADER