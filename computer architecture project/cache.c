#include "cache.h"

// Gets tsram entry and convert it to a line for future use
// bits 0:11 for tag, bits 13:12 for MESI state.
int tsram_entry_to_line(tsram_entry entry)
{
	int result = entry.MESI_state;

	result <<= 12;

	result += entry.tag;

	return result;
}

void initialize_cache_rams(cache *core_cache)
{
	int i = 0;
	while (i < DSRAM_SIZE)
	{
		core_cache->dsram[i] = 0;
		if (i < (TSRAM_SIZE - 1))
		{
			core_cache->tsram[i].tag = 0;
			core_cache->tsram[i].MESI_state = 0;
		}
	}
}