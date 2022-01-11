#ifndef _CORE_HEADER_
#define _CORE_HEADER_

#include <stdio.h>
#include <stdbool.h>

#include "hard_coded_data.h"
#include "cache.h"
#include "bus_mem.h"
#include "utils.h"

void initialize_core(core *core, char *imem_filename);

void simulate_clock_cycle(core* core, FILE* trace_file, int *main_mem, int core_num);

bool all_cores_halt(core cores[CORES_NUM]);

#endif // !CORE_HEADER