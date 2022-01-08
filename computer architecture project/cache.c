//#include <stdio.h>
//#include <stdlib.h>

#include "cache.h"
#include "bus_mem.h"
//#include "core.h"

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
		if (i < TSRAM_SIZE)
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
	core->bus_request.bus_data = data;
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

void core_snoop_bus(core *core)
{
	for (int i = 0; i < CORES_NUM; i++)
	{

	}
}

void read_mem(core *core)
{
	int read_address = core->core_pipeline[EX_MEM].current_ALU_output;
	int index = read_address & 0xFF;
	int tag = (read_address >> 8) & 0xFFF;

	bool block_in_cache = mem_block_search(core->core_cache.tsram, index, tag);
	address address_format = { index, tag };

	switch (core->core_cache.tsram[index].MESI_state)
	{
	case (invalid):
		create_bus_request(core, 0, address_format, bus_rd, 0x0);
		// return indication to wait for the bus request
		break;
	case (shared):
		core->core_cache.tsram[index].next_MESI_state = shared;
		break;
	case (exclusive):
		core->core_cache.tsram[index].next_MESI_state = exclusive;
		break;
	case (modified):
		core->core_cache.tsram[index].next_MESI_state = modified;
		break;
	}
}

void write_mem(core *core)
{
	int read_address = core->core_pipeline[EX_MEM].current_ALU_output;
	int index = read_address & 0xFF;
	int tag = (read_address >> 8) & 0xFFF;

	bool block_in_cache = mem_block_search(core->core_cache.tsram, index, tag);
	address address_format = { index, tag };

	switch (core->core_cache.tsram[index].MESI_state)
	{
	case (invalid):
		create_bus_request(core, 0, address_format, bus_rdx, core->core_registers[core->core_pipeline[EX_MEM].current_instruction.rd]);
		core->core_cache.tsram[index].next_MESI_state = modified;
		break;
	case (shared):
		create_bus_request(core, 0, address_format, bus_rdx, core->core_registers[core->core_pipeline[EX_MEM].current_instruction.rd]);
		core->core_cache.tsram[index].next_MESI_state = modified;
		break;
	case (exclusive):
		core->core_cache.tsram[index].next_MESI_state = modified;
		//no need for bus request
		break;
	case (modified):
		core->core_cache.tsram[index].next_MESI_state = modified;
		//no need for bus request
		break;
	}
}

void core_bus_snooper(core *core, int core_num, msi_bus *bus)
{
	int read_address = core->core_pipeline[EX_MEM].current_ALU_output;
	int index = read_address & 0xFF;
	int tag = (read_address >> 8) & 0xFFF;
	
	if (bus->bus_cmd == BUS_NO_CMD_CODE)
		return;
	
	if (bus->bus_origid == core_num)
	{
		if (core->bus_request_status == PENDING_WB_SEND_CODE) 
		{
			core->bus_request_status = NO_BUS_REQUEST_CODE;
			// function to clean cache data
		}

		else if (core->bus_request_status == PENDING_SEND_CODE)
		{
			core->bus_request_status = WAITING_FLUSH_CODE;
			
			if (bus->bus_cmd == BUS_FLUSH_CODE)
			{
				// update MESI state
				core->bus_request_status = NO_BUS_REQUEST_CODE;
			}
		}
	}
	else
	{
		if ((bus->bus_cmd == BUS_FLUSH_CODE) && (core->bus_request_status == WAITING_FLUSH_CODE) &&
			(core->bus_request.bus_addr.index == bus->bus_addr.index) && (core->bus_request.bus_addr.tag == bus->bus_addr.tag))
		{
			// check if request not valid
				//cancel_memory_request(core_num);
			
			// function to clean cache data
			// function to add cache data
			core->bus_request_status = NO_BUS_REQUEST_CODE;
		}
		else if (core->bus_request_status == NO_BUS_REQUEST_CODE && mem_block_search(core->core_cache.tsram,index,tag))
		{
			// && core->core_cache.tsram[index].MESI_state)
			// handle each state
		}
	}
}