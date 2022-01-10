
#include "cache.h"
#include "bus_mem.h"

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
		i++;
	}
}

// Creates the bus request by given values
void create_bus_request(core *core, int core_num, address bus_addr, int request_type, int data)
{
	//if (core->bus_request_status != NO_BUS_REQUEST_CODE)
	//	return;
	
	// Puts the request in core.
	core->bus_request.bus_origid = core_num;
	core->bus_request.bus_cmd = request_type;
	core->bus_request.bus_addr = bus_addr;
	core->bus_request.bus_data = data;
	core->bus_request_status = PENDING_SEND_CODE;

	//if (request_type == BUS_FLUSH_CODE) // WB
	//{
	//	address address_wb;
	//	address_wb.index = bus_addr.index;
	//	address_wb.tag = core->core_cache.tsram[bus_addr.index].tag;
	//	core->bus_request.bus_addr = address_wb;
	//	core->bus_request_status = PENDING_WB_SEND_CODE;
	//	core->bus_request.bus_data = core->core_cache.dsram[bus_addr.index];
	//}
}


void clean_cache_data(core* core, address bus_addr)
{
	core->core_cache.dsram[bus_addr.index] = EMPTY_DATA_FIELD;
	core->core_cache.tsram->tag = EMPTY_DATA_FIELD;
	core->core_cache.tsram->MESI_state = EMPTY_DATA_FIELD;
	core->core_cache.tsram->next_MESI_state = EMPTY_DATA_FIELD;
	core->core_cache.tsram->valid = true;
}

void add_cache_data(core* core, address bus_addr, int data, int state)
{
	core->core_cache.dsram[bus_addr.index] = data;
	core->core_cache.tsram[bus_addr.index].tag = bus_addr.tag;
	core->core_cache.tsram[bus_addr.index].MESI_state = state;
	core->core_cache.tsram[bus_addr.index].next_MESI_state = NO_STATE_CODE;
}

void update_MESI_state(core* core, address bus_addr, int state)
{
	core->core_cache.tsram[bus_addr.index].MESI_state = state;
	core->core_cache.tsram[bus_addr.index].next_MESI_state = NO_STATE_CODE;
}

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
		// moving to Shared or Exclusive state is updated by the return signal in bus_shared, after 16 cycles
		break;

	case (shared):
		if (block_in_cache)
		{
			//core->core_cache.tsram[tsram_index].next_MESI_state = shared;
			return core->core_cache.dsram[index];	//hit
		}
		else	// conflict miss
		{
			create_bus_request(core, 0, address_format, bus_rd, 0x0);	//miss
			return MISS_CODE;
		}
		break;

	case (exclusive):
		if (block_in_cache)
		{
			//core->core_cache.tsram[tsram_index].next_MESI_state = exclusive;
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
			//core->core_cache.tsram[tsram_index].next_MESI_state = modified;
			return core->core_cache.dsram[index];	//hit
		}
		else
		{
			//// flush old data (no need to wait 16 clock cycles)
			//int block_base_index = index - index % 4;
			//int block_tag = core->core_cache.tsram[block_base_index / 4].tag;
			//int main_mem_address = (block_tag << 8) | (block_base_index);
			//for (int i = 0; i < BLOCK_SIZE; i++)
			//{
			//	main_mem[main_mem_address + i] = core->core_cache.dsram[block_base_index + i];
			//}

			//create_bus_request(core, 0, address_format, bus_rd, 0x0);	//miss

			// first create a flush xaction, win arbitration, and then create bus_rd xaction and win arbitration

			return MISS_CODE;
		}
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
	// using BusRdX is a read request, so no data is supposed to be written on bus_data
	switch (core->core_cache.tsram[tsram_index].MESI_state)
	{
	case (invalid):
		create_bus_request(core, 0, address_format, bus_rdx, 0x0);
		core->core_cache.tsram[tsram_index].next_MESI_state = modified;
		return MISS_CODE;
		break;

	case (shared):
		if (block_in_cache)
		{
			core->core_cache.dsram[index] = core->core_registers[core->core_pipeline[EX_MEM].current_instruction.rd];	//hit, write and move mesi to modified.
			core->core_cache.tsram[tsram_index].next_MESI_state = modified;
			create_bus_request(core, 0, address_format, bus_rdx, 0x0);
			return MISS_CODE; // returned miss because we still need to stall. Officialy, it's not a miss, but it's recommended to wait
			// update data in $ only when data returns
		}
		else
		{
			// in case shared, if there's a write miss (conflict miss), all we have to do is just eviction of current block and stall core until data returns
			create_bus_request(core, 0, address_format, bus_rdx, 0x0);	// get exclusive access to write this block, write only when data arrived
			core->core_cache.tsram[tsram_index].next_MESI_state = modified;
			return MISS_CODE;
		}
		break;

	case (exclusive):

		if (block_in_cache)
		{
			core->core_cache.tsram[tsram_index].MESI_state = modified;
			core->core_cache.dsram[index] = core->core_registers[core->core_pipeline[EX_MEM].current_instruction.rd];	//hit
			return 0;	//hit, no need for bus request
		}
		else
		{
			// in case exclusive, if there's a write miss (conflict miss), all we have to do is just eviction of current block and stall core until data returns
			create_bus_request(core, 0, address_format, bus_rdx, 0x0);	// get exclusive access to write this block, write only when data arrived
			core->core_cache.tsram[tsram_index].next_MESI_state = modified;
			return MISS_CODE;
		}
		break;

	case (modified):

		if (block_in_cache)
		{
			core->core_cache.tsram[tsram_index].MESI_state = modified;
			core->core_cache.dsram[index] = core->core_registers[core->core_pipeline[EX_MEM].current_instruction.rd];	//hit
			return 0; //hit, no need for bus request
		}
		else
		{

			// first create a flush xaction, win arbitration, and then create bus_rdx xaction and win arbitration.
			// then, and only then, write the data you initialy requested to write.
			return MISS_CODE;
		}

		
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

bool core_snoop_bus(core *core, int core_num, msi_bus *bus, bool *shared_flags)	
{
	msi_bus pending_request = core->bus_request;

	int dsram_index = pending_request.bus_addr.index;
	int tsram_index = dsram_index / BLOCK_SIZE;
	tsram_entry core_tsram_entry = core->core_cache.tsram[tsram_index];
	int core_tag = core_tsram_entry.tag;
	int request_tag = pending_request.bus_addr.tag;

	if ((core_tag != request_tag) && core_tsram_entry.valid)	// requested block is not in this core's cache
	{
		return original_request;	// continue (for this core) with original xaction
	}

	if (core_tsram_entry.MESI_state == invalid)
	{
		return original_request;	// block is invalid in other cache that has it. continue (for this core) with original xaction
	}

	if (pending_request.bus_cmd == bus_rdx)
	{
		switch (core_tsram_entry.MESI_state)
		{
		case (modified):
			core->core_cache.tsram[tsram_index].MESI_state = invalid;
			return flush_request;	// flush for this core
			break;
		case (exclusive):
		case (shared):
			core->core_cache.tsram[tsram_index].MESI_state = invalid;
			return original_request;
			break;
		}
	}
	else
	{
		shared_flags[core_num] = true;
		switch (core_tsram_entry.MESI_state)
		{
		case (modified):
			core->core_cache.tsram[tsram_index].MESI_state = shared;
			return flush_request;	// flush for this core
			break;
		case (exclusive):
		case (shared):
			core->core_cache.tsram[tsram_index].MESI_state = shared;
			return original_request;
			break;
		}
	}
}

//now let the cores snoop the request.
	// for BusRdX:
	// 1. if other cache has the block Modified in it, generate Flush xaction from this core and update main_mem at the same time. switch the state of this block to Invalid.
	//	  now, don't go on bus with this xaction! switch this xaction of BusRdX to Flush. main_mem should snoop this xaction and update its contents.
	//    main_mem is supposed to snoop at some point and find out that bus_origid of the Flush cmd is not his, so it has to be updated too!
	// 2. if other cache has the block Exclusive in it, invalidate this block (no actual need to Flush) at this function! you have to execute transition E->I at this cycle.
	// 3. if other cache has the block Shared in it, invalidate this block (no actual need to Flush) at this function! you have to execute transition S->I at this cycle.

	// for BusRd:
	// first initialize bus_shared to '0'. check if any of the caches has the block in it, and if so raise bus_shared to '1'. 
	// 1. if other cache has the block Modified in it, generate Flush xaction from this core and update main_mem at the same time. switch the state of this block to Shared.
	//	  now, don't go on bus with this xaction! switch this xaction of BusRd to Flush. main_mem should snoop this xaction and update its contents.
	//    main_mem is supposed to snoop at some point and find out that bus_origid of the Flush cmd is not his, so it has to be updated too!
	// 2. if other cache has the block Exclusive in it, make transition of this block: E->S at the very same clock cycle (within the scope of the function). no need to flush.
	//    it's very important that the transition of the mesi state will occur instantly!
	// 3. if other cache has the block Shared in it, next mesi_state will be the same (Shared), no flush is needed.
