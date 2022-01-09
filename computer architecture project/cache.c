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
			core_cache->tsram[i].valid = true;
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

//int check_address_in_cache(core* cores, int core_num, address bus_addr)
//{
//	if(cores[core_num].core_cache.tsram[bus_addr.index].valid != VALID_BLOCK_CODE || cores[core_num].core_cache.tsram[bus_addr.index].MESI_state == NO_VALUE_CODE)
//
//}

int get_cache_data(core* core, address bus_addr)
{
	return core->core_cache.dsram[bus_addr.index];
}

void clean_cache_data(core* core, address bus_addr)
{
	core->core_cache.dsram[bus_addr.index] = EMPTY_DATA_FIELD;
	core->core_cache.tsram->tag = EMPTY_DATA_FIELD;
	core->core_cache.tsram->MESI_state = EMPTY_DATA_FIELD;
	core->core_cache.tsram->next_MESI_state = EMPTY_DATA_FIELD;
	core->core_cache.tsram->valid = true;
}

/*void update_cache_data(core* core, address bus_addr, int data)
{
	core->core_cache.dsram[bus_addr.index] = data;
}*/

void add_cache_data(core* core, address bus_addr, int data, int state)
{
	core->core_cache.dsram[bus_addr.index] = data;
	core->core_cache.tsram[bus_addr.index].tag = bus_addr.tag;
	core->core_cache.tsram[bus_addr.index].MESI_state = state;
	core->core_cache.tsram[bus_addr.index].next_MESI_state = NO_STATE_CODE;
}

int get_MESI_state(core* core, address bus_addr)
{
	return core->core_cache.tsram[bus_addr.index].MESI_state;
}

void update_MESI_state(core* core, address bus_addr, int state)
{
	core->core_cache.tsram[bus_addr.index].MESI_state = state;
	core->core_cache.tsram[bus_addr.index].next_MESI_state = NO_STATE_CODE;
}

//int generate_tsram_index(int dsram_index)
//{
//	return dsram_index / 4;
//}

bool mem_block_search(tsram_entry *tsram, int tsram_index, int tag)
{
	bool block_valid = tsram[tsram_index].valid;
	if (!block_valid)
	{
		tsram[tsram_index].MESI_state = invalid;
		return false;
	}
	else {
		if (tsram[tsram_index].tag == tag)
		{
			return true;
		}
		else
		{
			//tsram[index].MESI_state = invalid;
			return false;
		}
	}
}

int read_mem(core *core, int *main_mem)
{
	int read_address = core->core_pipeline[EX_MEM].current_ALU_output;
	int index = read_address & 0xFF;
	int tag = (read_address >> 8) & 0xFFF;
	int tsram_index = index / 4;

	bool block_in_cache = mem_block_search(core->core_cache.tsram, tsram_index, tag);
	address address_format = { index, tag };

	switch (core->core_cache.tsram[tsram_index].MESI_state)
	{
	case (invalid):
		create_bus_request(core, 0, address_format, bus_rd, 0x0);
		return MISS_CODE;	// miss
		// return indication to wait for the bus request
		break;

	case (shared):
		if (block_in_cache)
		{
			core->core_cache.tsram[tsram_index].next_MESI_state = shared;
			return core->core_cache.dsram[index];	//hit
		}
		else
		{
			create_bus_request(core, 0, address_format, bus_rd, 0x0);	//miss
			return MISS_CODE;
		}
		break;

	case (exclusive):
		if (block_in_cache)
		{
			core->core_cache.tsram[tsram_index].next_MESI_state = exclusive;
			return core->core_cache.dsram[index];	//hit
		}
		else
		{
			create_bus_request(core, 0, address_format, bus_rd, 0x0);	//miss
			return MISS_CODE;
		}
		break;

	case (modified):
		if (block_in_cache)
		{
			core->core_cache.tsram[tsram_index].next_MESI_state = modified;
			return core->core_cache.dsram[index];	//hit
		}
		else
		{
			// flush old data (no need to wait 16 clock cycles)
			int block_base_index = index - index % 4;
			int block_tag = core->core_cache.tsram[block_base_index / 4].tag;
			int main_mem_address = (block_tag << 8) | (block_base_index);
			for (int i = 0; i < BLOCK_SIZE; i++)
			{
				main_mem[main_mem_address + i] = core->core_cache.dsram[block_base_index + i];
			}

			create_bus_request(core, 0, address_format, bus_rd, 0x0);	//miss
			return MISS_CODE;
		}
		//core->core_cache.tsram[index].next_MESI_state = modified;
		//return core->core_cache.dsram[index];	//hit
		break;
	}
	
}

int write_mem(core *core)
{
	int read_address = core->core_pipeline[EX_MEM].current_ALU_output;
	int index = read_address & 0xFF;
	int tag = (read_address >> 8) & 0xFFF;
	int tsram_index = index / 4;

	bool block_in_cache = mem_block_search(core->core_cache.tsram, tsram_index, tag);
	address address_format = { index, tag };

	switch (core->core_cache.tsram[tsram_index].MESI_state)
	{
	case (invalid):
		create_bus_request(core, 0, address_format, bus_rdx, core->core_registers[core->core_pipeline[EX_MEM].current_instruction.rd]);
		core->core_cache.tsram[tsram_index].next_MESI_state = modified;
		return MISS_CODE;
		break;

	case (shared):
		if (block_in_cache)
		{
			core->core_cache.dsram[index] = core->core_registers[core->core_pipeline[EX_MEM].current_instruction.rd];	//hit, write and move mesi to modified
			core->core_cache.tsram[tsram_index].next_MESI_state = modified;
			create_bus_request(core, 0, address_format, bus_rdx, core->core_registers[core->core_pipeline[EX_MEM].current_instruction.rd]);
			return MISS_CODE; // returned miss because we still need to stall. Officialy, it's not a miss
		}
		else
		{
			create_bus_request(core, 0, address_format, bus_rdx, core->core_registers[core->core_pipeline[EX_MEM].current_instruction.rd]);	// let other cores know 
			return MISS_CODE;
		}
		break;

	case (exclusive):
		core->core_cache.tsram[tsram_index].next_MESI_state = modified;
		//no need for bus request
		break;

	case (modified):
		core->core_cache.tsram[tsram_index].next_MESI_state = modified;
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