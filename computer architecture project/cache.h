#ifndef CACHE_HEADER
#define CACHE_HEADER

#define DSRAM_SIZE 256
#define BLOCK_SIZE 4
#define NUM_OF_BLOCKS (DSRAM_SIZE/BLOCK_SIZE)

#define TSRAM_SIZE 64

#define MESI_INVALID 0
#define MESI_SHARED 1
#define MESI_EXCLUSIVE 2
#define MESI_MODIFIED 3

typedef struct tsram_entry {
	unsigned int tag : 12;
	unsigned int MESI_state : 2;
} tsram_entry;

typedef struct cache {
	tsram_entry tsram[TSRAM_SIZE];
	int dsram[DSRAM_SIZE];
} cache;

void initialize_cache_rams(cache *core_cache);

#endif // !CACHE_HEADER
