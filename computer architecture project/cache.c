#include <stdio.h>
#include <stdlib.h>

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
			core_cache->tsram[i].MESI_state = invalid;
			core_cache->tsram[i].valid = false;
		}
	}
}

// Creates the bus request by given values
void create_bus_request(core *core, int core_num, address bus_addr, int request_type, int data)
{
	if (core->bus_request_status != NO_BUS_REQUEST_CODE)
		return;
	
	// Puts the request in core.
	core->bus_request.bus_origid = core_num;
	core->bus_request.bus_cmd = request_type;
	core->bus_request.bus_addr = bus_addr;
	core->bus_request_status = PENDING_SEND_CODE;

	if (request_type == BUS_FLUSH_CODE) // WB
	{
		address address_wb;
		address_wb.index = bus_addr.index;
		address_wb.tag = core->core_cache.tsram[bus_addr.index].tag;
		core->bus_request.bus_addr = address_wb;
		core->bus_request_status = PENDING_WB_SEND_CODE;
		core->bus_request.bus_data = core->core_cache.dsram[bus_addr.index];
	}
}

//// int check_address_in_cache(core* cores, int core_num, address bus_addr)
//// {
//// 	if(cores[core_num].core_cache.tsram[bus_addr.index].valid != VALID_BLOCK_CODE || cores[core_num].core_cache.tsram[bus_addr.index].MESI_state == NO_VALUE_CODE)
//
//// }
//
//int get_MESI_state(core* cores, int core_num, address bus_addr)
//{
//	return cores[core_num].tsram[bus_addr.index].MESI_state;
//}

bool mem_block_search(tsram_entry *tsram, int index, int tag)
{
	bool block_valid = tsram[index].valid;
	if (!block_valid)
	{
		tsram[index].MESI_state = invalid;
		return false;
	}
	else {
		if (tsram[index].tag == tag)
		{
			return true;
		}
		else
		{
			tsram[index].MESI_state = invalid;
			return false;
		}
	}
}

void read_mem(core *core)
{
	int read_address = core->core_pipeline[EX_MEM].current_ALU_output;
	int index = read_address & 0xFF;
	int tag = (read_address >> 8) & 0xFFF;

	bool block_in_cache = mem_block_search(core->core_cache.tsram, index, tag);
	address address_format = { index, tag };

	switch (core->core_cache.tsram->MESI_state)
	{
	case (invalid):
		create_bus_request(core, 0, address_format, bus_rd, 0x00000000);
		break;
	case (shared):
		break;
	case (exclusive):
		break;
	case (modified):
		break;
	}
}