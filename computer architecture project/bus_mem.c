#define _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_DEPRECATE


#include "core.h"
#include "bus_mem.h"
#include "utils.h"

void initialize_main_mem(int* main_mem, int* valid_request, int* memory_request_cycle)
{
	int i;

	for (i = 0; i < CORES_NUM; i++)
	{
		valid_request[i] = UNVALID_REQUEST_CODE;
		memory_request_cycle[i] = 0;
	}

	// maybe initialize empty request

	for (i = 0; i < MAIN_MEM_SIZE; i++)
		main_mem[i] = EMPTY_DATA_FIELD;


	initialize_array_from_file("memin.txt", main_mem, MAIN_MEM_SIZE);
}

void initialize_bus(msi_bus *bus)
{
	bus->bus_addr.index = EMPTY_DATA_FIELD;
	bus->bus_addr.tag = EMPTY_DATA_FIELD;
	bus->bus_cmd = EMPTY_DATA_FIELD;
	bus->bus_data = EMPTY_DATA_FIELD;
	bus->bus_origid = EMPTY_DATA_FIELD;
	bus->flush_reason = no_reason;
}

void put_xaction_on_bus(msi_bus xaction, msi_bus *bus)
{
	
}

//void do_bus_and_main_mem_stuff(core *core, int *main_mem, msi_bus *bus)
//{
//	// first, check if any xaction is on the bus. Meaning, check if the bus is taken or a previous BusRd or BusRdX xaction hasn't finished by using Flush xaction.
//	// if bus is busy, stall cores as necessary (consider taking update_stage_buffers() function for each core and executing it after main_mem and bus actions).
//	// if bus just finished answering a request, return the request to the one who asked it (bus_origid etc...).
//	// if bus is free, let arbitration take place and send a request on the bus for the winning core (if such one exists in any of the cores for this cycle).
//}

// Main Memory Functions

// Read the bus for cores flushes and requests
void main_memory_bus_snooper(core *cores, msi_bus bus, int cycle, int *main_mem, int *valid_request, int *memory_request_cycle)
{
	if (bus.bus_origid == MEMORY_ORIGIN_CODE || bus.bus_cmd == BUS_NO_CMD_CODE)
		return;
	if (bus.bus_cmd == BUS_FLUSH_CODE)
	{
		main_mem[address_to_integer(bus.bus_addr)] = bus.bus_data;
		return;
	}
	cores[bus.bus_origid].bus_request.bus_addr = bus.bus_addr;
	cores[bus.bus_origid].bus_request.bus_cmd = BUS_FLUSH_CODE;
	cores[bus.bus_origid].bus_request.bus_data = main_mem[address_to_integer(bus.bus_addr)];
	//cores[bus.bus_origid].bus_request.bus_origid = MEMORY_ORIGIN_CODE;
	memory_request_cycle[bus.bus_origid] = cycle;
	valid_request[bus.bus_origid] = VALID_REQUEST_CODE;
}

// Checks if there is any flush ready under main memory
int available_memory_to_flush(int cycle, int *valid_request, int *memory_request_cycle) // add support in multiple cores
{
	for (int i = 0; i < CORES_NUM; i++)
		if (valid_request[i] == VALID_REQUEST_CODE && cycle - memory_request_cycle[i] >= 16)
			return i;

	return NO_VALUE_CODE;
}

// Cancel memory request in the main memory
void cancel_memory_request(int core_num, int *valid_request) // add support in multiple cores
{
	valid_request[core_num] = UNVALID_REQUEST_CODE;
}

// Bus functions

//void update_bus(core *cores, msi_bus* bus, int cycle, int* next_RR, int *valid_request, int *memory_request_cycle)
//{
//	//check bus availability first
//	for (int i = 0; i < CORES_NUM; i++)
//	{
//		int loc = (i + (*next_RR)) % CORES_NUM;
//		if (cores[loc].bus_request_status == PENDING_SEND_CODE || cores[loc].bus_request_status == PENDING_WB_SEND_CODE)
//		{
//			*bus = cores[loc].bus_request;
//			*next_RR = (loc + 1) % CORES_NUM;
//			return;
//		}
//	}
//
//	// maybe we can add valid_request and memory_request_cycle inside core struct
//
//	int mem_to_flush = available_memory_to_flush(cycle, valid_request, memory_request_cycle);
//	if (mem_to_flush != NO_VALUE_CODE) // If no core want to send, check the main memory
//	{
//		*bus = cores[mem_to_flush].bus_request;
//		valid_request[mem_to_flush] = UNVALID_REQUEST_CODE;
//	}
//	else
//		initialize_bus(&bus);
//}




void update_bus(core *cores, msi_bus *bus, int cycle, int* next_RR, int *valid_request, int *memory_request_cycle, int *main_mem)
{
	// first check if bus is busy

	// check BusRd/BusRdX clock cycles left
	if ((bus->bus_cmd == bus_rd) || (bus->bus_cmd == bus_rdx))
	{
		if (bus->cycles_left > 0)	// BusRd or BusRdx xaction finished
		{
			bus->cycles_left -= 1;
			return;
		}
		bus->cycles_left = FLUSH_CYCLES - 1;
		bus->bus_cmd = flush;
		bus->flush_to = bus->bus_origid;
		bus->bus_origid = main_mem_origid;

		int read_address = cores[bus->bus_origid].core_pipeline[EX_MEM].current_ALU_output;
		int index = read_address & 0xFF;
		int tag = (read_address >> 8) & 0xFFF;
		int tsram_index = index / 4;
		int block_base_index = index - index % 4;
		//int tsram_index = block_base_index / 4; // appears twice, decided which one to delete
		int block_tag = cores[bus->bus_origid].core_cache.tsram[tsram_index].tag;
		int main_mem_address = (block_tag << 8) | (block_base_index);
		address main_mem_address_formatted = { block_base_index, block_tag };

		bus->bus_addr = main_mem_address_formatted;

		//write_bustrace(bus, cycle, "bustrace.txt");
	}
	
	// check flush, and execute if needed
	// now (in flush, if needed) let main_mem snoop the request. if flush is needed, check the origid. if origid is main_mem, he'll supply the data. else, he'll take data.
	if (bus->bus_cmd == flush)
	{
		// print the flush for this cycle
		// actual execution of the flush request, let main_mem snoop
		// check if flush finished:
		// if finished, update new mesi_state, new tag in tsram, unfreeze core for next cycle, cancel core_bus_request
		// if not, update dsram and go for the next address flush
		
		//write_bustrace(bus, cycle, "bustrace.txt");
		int main_mem_address = address_to_integer(bus->bus_addr);
		int dsram_index = bus->bus_addr.index;

		int data = (bus->bus_origid != main_mem_origid) ? cores[bus->bus_origid].core_cache.dsram[dsram_index] : main_mem[main_mem_address];
		bus->bus_data = data;

		write_bustrace(bus, cycle, "bustrace.txt");
		cores[bus->flush_to].core_cache.dsram[dsram_index] = data; //flush data to core that initiated this flush xaction

		// main_mem snooping
		if (bus->bus_origid != main_mem_origid) // main_mem takes data, it wasn't the one who flushed the data (modified block data taken from another core)
		{
			main_mem[main_mem_address] = data;	
		}

		if (bus->cycles_left == 0)
		{
			// update new mesi_state, new tag in tsram, unfreeze core for next cycle, cancel core_bus_request
			int tsram_index = dsram_index / 4;
			cores[bus->flush_to].core_cache.tsram[tsram_index].tag = bus->bus_addr.tag;
			cores[bus->flush_to].mem_stall = false;
			
			int old_mesi = cores[bus->flush_to].core_cache.tsram[tsram_index].MESI_state;
			//flush reason, bus_shared
			if (cores[bus->flush_to].bus_request.flush_reason == busrd_flush)
			{
				if (bus->bus_shared)
				{
					cores[bus->flush_to].core_cache.tsram[tsram_index].MESI_state = shared;
				}
				else
				{
					cores[bus->flush_to].core_cache.tsram[tsram_index].MESI_state = exclusive;
				}
			}
			else if (cores[bus->flush_to].bus_request.flush_reason == busrdx_flush)
			{
				cores[bus->flush_to].core_cache.tsram[tsram_index].MESI_state = modified;
			}
			
			initialize_bus(&(cores[bus->flush_to].bus_request));
			initialize_bus(bus);
		}
		else
		{
			// update address
			bus->bus_addr.index++;
			bus->cycles_left -= 1;
		}
		return;

	}
	else if ((bus->bus_cmd == write_miss_flush) || (bus->bus_cmd == read_miss_flush))
	{
		// print the flush for this cycle
		// actually execution of the flush request
		// check if flush finished:
		// if finished, put a new request on the bus: if read then put BusRd, if write then put BusRdX
		// if not, update main_mem and go for the next address flush.

		//write_bustrace(bus, cycle, "bustrace.txt");
		int main_mem_address = address_to_integer(bus->bus_addr);
		int dsram_index = bus->bus_addr.index;
		int temp = bus->bus_cmd;
		bus->bus_cmd = flush;

		int data = cores[bus->bus_origid].core_cache.dsram[cores[bus->bus_origid].bus_request.bus_addr.index];
		bus->bus_data = data;

		write_bustrace(bus, cycle, "bustrace.txt");
		bus->bus_cmd = temp;

		main_mem[main_mem_address] = data;
		main_mem_address++;
		int index = main_mem_address & 0xFF;
		int tag = (main_mem_address >> 8) & 0xFFF;
		address new_main_mem_address = { index, tag };

		bus->bus_addr = new_main_mem_address;
		bus->bus_data = 0x0;

		if (bus->cycles_left == 0)
		{
			// bus_origid stays the same
			bus->bus_addr = cores[bus->bus_origid].bus_request.bus_addr;
			bus->bus_data = 0x0;
			if (bus->flush_reason == busrd_flush)
			{
				bus->bus_cmd = bus_rd;
			}
			else if (bus->flush_reason == busrdx_flush)
			{
				bus->bus_cmd = bus_rdx;
			}
		}
		else
		{
			bus->cycles_left -= 1;
		}
		return;
	}

	// enter this section of function only upon BusRd or BusRdX xactions. flush xactions should be taken care of earlier in this function, and should not reach this far.
	// if no_cmd, also take care of it before this section. if flush due to a conflict miss, treat after arbitration

	int core_num, i;
	//let arbitration take place here. Treat the state in which none of the cores wants to do something
	for (i = 0; i < CORES_NUM; i++)
	{
		core_num = (i + (*next_RR)) % CORES_NUM;	// go cylic and allow Round-Robin arbitration
		if (cores[core_num].bus_request.bus_cmd != no_cmd)
		{
			*next_RR += 1;
			break;
		}
	}
	if (i == CORES_NUM)	// none of the cores request the bus in this clock cycle, no need to update_bus
	{
		return;
	}
	// integer core_num now holds the core that won arbitration. 

	//initialize bus_shared to zero before snooping other cores
	bus->bus_shared = BUS_NOT_SHARED;

	// take care of flush that is caused by conflict miss for a block that is modified
	if ((cores[core_num].bus_request.bus_cmd == write_miss_flush_request) || (cores[core_num].bus_request.bus_cmd == read_miss_flush_request))	
	{
		bus->flush_reason = cores[core_num].bus_request.flush_reason;
		// flush first word
		int main_mem_address = address_to_integer(cores[core_num].bus_request.bus_addr);
		int data = cores[core_num].core_cache.dsram[cores[core_num].bus_request.bus_addr.index];
		main_mem[main_mem_address] = data;
		bus->bus_addr = cores[core_num].bus_request.bus_addr;
		bus->bus_origid = core_num;
		bus->bus_data = data;
		bus->bus_cmd = flush;
		write_bustrace(bus, cycle, "bustrace.txt");
		bus->cycles_left = FLUSH_CYCLES - 2; // now update that we have only 3 cycles left to this flush

		main_mem_address++;
		int index = main_mem_address & 0xFF;
		int tag = (main_mem_address >> 8) & 0xFFF;
		address new_main_mem_address = { index, tag };

		// change xaction to write_miss_flush or read_miss_flush, with only 3 cycles left (3 last word - adjust address as needed)
		bus->bus_cmd = ((cores[core_num].bus_request.bus_cmd == write_miss_flush_request) ? write_miss_flush : read_miss_flush);
		bus->bus_addr = new_main_mem_address;
		bus->bus_data = 0x0;

	}

	//initialize bus_shared to zero before snooping other cores
	bus->bus_shared = BUS_NOT_SHARED;

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

	bool shared_flags[CORES_NUM] = { false, false, false, false };
	bool flush = false;
	int flushing_core;
	
	for (i = 0; i < CORES_NUM; i++)
	{
		if (i == core_num)
		{
			continue;
		}
		flush = core_snoop_bus(&(cores[i]), core_num, bus, shared_flags);
		if (flush)
		{
			flushing_core = i;
			break;
		}
	}

	//put xaction on bus
	*bus = cores[core_num].bus_request;
	write_bustrace(bus, cycle, "bustrace.txt"); // write the bus request, before we might change it to flush 

	if (flush) // change xaction to flush
	{
		bus->flush_to = bus->bus_origid;
		bus->bus_origid = flushing_core;
		if (bus->bus_cmd == bus_rd)	// block was surely in modified state in another cache, so check if it's going to shared state or invalid state
		{
			bus->bus_shared = BUS_SHARED; 
		}
		else
		{
			bus->bus_shared = BUS_NOT_SHARED;
		}
		bus->bus_cmd = flush;

		// calculate beginning of block address
		int index = cores[core_num].bus_request.bus_addr.index;
		int block_base_index = index - index % 4;
		int block_tag = cores[core_num].bus_request.bus_addr.tag;
		int main_mem_address = (block_tag << 8) | (block_base_index);

		bus->bus_addr.index = block_base_index;
		bus->bus_addr.tag = block_tag;
		bus->bus_data = 0x0;
		// exit function with new flush xaction
	}
	else // BusRd or BusRdX is going out on the bus! (and not a flush xaction)
	{
		bus->bus_shared = (shared_flags[0] || shared_flags[1] || shared_flags[2] || shared_flags[3]) ? BUS_SHARED : BUS_NOT_SHARED;
		bus->cycles_left = MAIN_MEM_ANSWER_CYCLES - 1;
	}

	return;	// exit function, after sending the correct xaction (if there is any xaction to be sent) or dealing with flush (if there is any flush xaction to be dealt with)
}
