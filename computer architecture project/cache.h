#ifndef _CACHE_HEADER_
#define _CACHE_HEADER_

#include <stdbool.h>



#include "bus_mem.h"
#include "hard_coded_data.h"
#include "core.h"

void initialize_cache_rams(cache *core_cache);
int read_mem(core *core, int *main_mem);
int write_mem(core *core);

#endif // !CACHE_HEADER
