
#include "cache.h"
#include "bus_mem.h"
#include "utils.h"

// Gets tsram entry and convert it to a line for future use
// bits 0:11 for tag, bits 12:13 for MESI state.
int tsram_entry_to_line(tsram_entry entry)
{
	int result = entry.MESI_state;

	result <<= 12;

	result += entry.tag;

	return result;
}

// Initalizing DSRAM and TSRAM for the cache
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
	//core->bus_request_status = PENDING_SEND_CODE;

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

// Checks for memory hit or miss
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

// When lw needs access to memory
int read_mem(core *core, int *main_mem, int core_num)
{
	int read_address = core->core_pipeline[EX_MEM].current_ALU_output;
	int index = read_address & 0xFF;
	int tag = (read_address >> 8) & 0xFFF;
	int tsram_index = index / 4;

	bool cache_hit = mem_block_search(core->core_cache.tsram, tsram_index, tag);
	address address_format = { index, tag };

	switch (core->core_cache.tsram[tsram_index].MESI_state)
	{
	case (invalid):
		create_bus_request(core, core_num, address_format, bus_rd, 0x0);
		core->bus_request.flush_reason = busrd_flush;
		update_statistics(core, READ_MISS_CODE);
		return MISS_CODE;	// miss
		// return indication to wait for the bus request
		// moving to Shared or Exclusive state is updated by the return signal in bus_shared, after 16 cycles
		break;

	case (shared):
		if (cache_hit)
		{
			//core->core_cache.tsram[tsram_index].next_MESI_state = shared;
			update_statistics(core, READ_HIT_CODE);
			return core->core_cache.dsram[index];	//hit
		}
		else	// read conflict miss
		{
			core->bus_request.flush_reason = busrd_flush;
			create_bus_request(core, core_num, address_format, bus_rd, 0x0);	//miss
			//core->core_cache.tsram[tsram_index].next_MESI_state = shared;
			update_statistics(core, READ_MISS_CODE);
			return MISS_CODE;
		}
		break;

	case (exclusive):
		if (cache_hit)
		{
			//core->core_cache.tsram[tsram_index].next_MESI_state = exclusive;
			update_statistics(core, READ_HIT_CODE);
			return core->core_cache.dsram[index];	//hit
		}
		else
		{
			core->bus_request.flush_reason = busrd_flush;
			create_bus_request(core, core_num, address_format, bus_rd, 0x0);	//miss
			update_statistics(core, READ_MISS_CODE);
			return MISS_CODE;
		}
		break;

	case (modified):
		if (cache_hit)
		{
			//core->core_cache.tsram[tsram_index].next_MESI_state = modified;
			update_statistics(core, READ_HIT_CODE);
			return core->core_cache.dsram[index];	//hit
		}
		else
		{
			// flush old data (no need to wait 16 clock cycles)
			int block_base_index = index - index % 4;
			int tsram_index = block_base_index / 4;
			int block_tag = core->core_cache.tsram[tsram_index].tag;
			int main_mem_address = (block_tag << 8) | (block_base_index);
			address main_mem_address_formatted = { block_base_index, block_tag };
			
			// first create a flush xaction, win arbitration, and then create bus_rd xaction and win arbitration
			core->bus_request.flush_reason = busrd_flush;
			create_bus_request(core, core_num, main_mem_address_formatted, read_miss_flush_request, 0x0);	//miss
			update_statistics(core, READ_MISS_CODE);
			return MISS_CODE;
		}
		break;
	}
	
}

// When sw needs access to memory
int write_mem(core *core, int core_num)
{
	int read_address = core->core_pipeline[EX_MEM].current_ALU_output;
	int index = read_address & 0xFF;
	int tag = (read_address >> 8) & 0xFFF;
	const int tsram_index = index / 4;

	bool cache_hit = mem_block_search(core->core_cache.tsram, tsram_index, tag);
	address address_format = { index, tag };
	// using BusRdX is a read request, so no data is supposed to be written on bus_data // 2F - tsram_index is changed here
	switch (core->core_cache.tsram[tsram_index].MESI_state)
	{
	case (invalid):
		create_bus_request(core, core_num, address_format, bus_rdx, 0x0);
		core->bus_request.flush_reason = busrdx_flush;
		//core->core_cache.tsram[tsram_index].next_MESI_state = modified;
		update_statistics(core, WRITE_MISS_CODE);
		return MISS_CODE;
		break;

	case (shared):
		if (cache_hit)
		{
			core->core_cache.dsram[index] = core->core_registers[core->core_pipeline[EX_MEM].current_instruction.rd];	//hit, write and move mesi to modified.
			core->core_cache.tsram[tsram_index].next_MESI_state = modified; // TOOD delete
			create_bus_request(core, core_num, address_format, bus_rdx, 0x0);
			core->bus_request.flush_reason = busrdx_flush;
			// maybe update_statistics
			return MISS_CODE; // returned miss because we still need to stall. Officialy, it's not a miss, but it's recommended to wait
			// update data in $ only when data returns
		}

		// in case shared, if there's a write miss (conflict miss), all we have to do is just eviction of current block and stall core until data returns
		create_bus_request(core, core_num, address_format, bus_rdx, 0x0);	// get exclusive access to write this block, write only when data arrived
		core->bus_request.flush_reason = busrdx_flush;
		core->core_cache.tsram[tsram_index].next_MESI_state = modified; // TOOD delete
		update_statistics(core, WRITE_MISS_CODE);
		return MISS_CODE;

	case (exclusive):

		if (cache_hit)
		{
			//printf("tsram_index = %d", tsram_index);
			core->core_cache.tsram[tsram_index].MESI_state = modified;
			core->core_cache.dsram[index] = core->core_registers[core->core_pipeline[EX_MEM].current_instruction.rd];	//hit
			update_statistics(core, WRITE_HIT_CODE);
			return HIT_CODE;	//hit, no need for bus request
		}

		// in case exclusive, if there's a write miss (conflict miss), all we have to do is just eviction of current block and stall core until data returns
		create_bus_request(core, core_num, address_format, bus_rdx, 0x0);	// get exclusive access to write this block, write only when data arrived
		core->bus_request.flush_reason = busrdx_flush;
		core->core_cache.tsram[tsram_index].next_MESI_state = modified; // TOOD delete
		update_statistics(core, WRITE_MISS_CODE);
		return MISS_CODE;

	case (modified):

		if (cache_hit)
		{
			core->core_cache.tsram[tsram_index].MESI_state = modified;
			core->core_cache.dsram[index] = core->core_registers[core->core_pipeline[EX_MEM].current_instruction.rd];	//hit
			update_statistics(core, WRITE_HIT_CODE);
			return HIT_CODE; //hit, no need for bus request
		}

		// first create a flush xaction, win arbitration, and then create bus_rdx xaction and win arbitration.
		// then, and only then, write the data you initialy requested to write.
		int block_base_index = index - index % 4;
		int tsram_index = block_base_index / 4;
		int block_tag = core->core_cache.tsram[tsram_index].tag;
		int main_mem_address = (block_tag << 8) | (block_base_index);
		address main_mem_address_formatted = { block_base_index, block_tag };

		core->bus_request.flush_reason = busrdx_flush;
		create_bus_request(core, core_num, main_mem_address_formatted, write_miss_flush_request, 0x0);	//miss
		update_statistics(core, WRITE_MISS_CODE);
		return MISS_CODE;
	}
}

// Bus snooper, checks if there's anything relevant on bus for specific core.
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
			core->bus_request.flush_reason = busrdx_flush;
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
			core->bus_request.flush_reason = busrd_flush;
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
