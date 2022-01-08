#ifndef _CACHE_HEADER_
#define _CACHE_HEADER_

#include <stdbool.h>

typedef struct _cache cache;

#include "bus_mem.h"


#define DSRAM_SIZE 256
#define BLOCK_SIZE 4
#define DSRAM_FRAMES (DSRAM_SIZE/BLOCK_SIZE)

#define TSRAM_SIZE 64

#define MESI_INVALID 0
#define MESI_SHARED 1
#define MESI_EXCLUSIVE 2
#define MESI_MODIFIED 3

// Blocks Status Codes
#define VALID_BLOCK_CODE 0
#define DIRTY_BLOCK_CODE 1

typedef enum MESI_states {
	invalid,
	shared,
	exclusive,
	modified
} MESI_states;

typedef struct tsram_entry {
	unsigned int tag : 12;
	unsigned int MESI_state : 2;
	bool valid;
} tsram_entry;

struct _cache {
	tsram_entry tsram[TSRAM_SIZE];
	int dsram[DSRAM_SIZE];
};

#include "core.h"

void initialize_cache_rams(cache *core_cache);
void read_mem(core *core);

#endif // !CACHE_HEADER
