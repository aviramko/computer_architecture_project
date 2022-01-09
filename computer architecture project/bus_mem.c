#define _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_DEPRECATE

//#include <stdio.h>
//#include <stdlib.h>

#include "core.h"
#include "bus_mem.h"
#include "utils.h"

void initialize_main_mem(int** main_mem, int** valid_request, int** memory_request_cycle)
{
	int i;

	for (i = 0; i < CORES_NUM; i++)
	{
		(*valid_request)[i] = UNVALID_REQUEST_CODE;
		(*memory_request_cycle)[i] = 0;
	}

	// maybe initialize empty request

	for (i = 0; i < MAIN_MEM_SIZE; i++)
		(*main_mem)[i] = EMPTY_DATA_FIELD;


	initialize_array_from_file("memin.txt", main_mem, MAIN_MEM_SIZE);
}

void initialize_bus(msi_bus *bus)
{
	bus->bus_addr.index = EMPTY_DATA_FIELD;
	bus->bus_addr.tag = EMPTY_DATA_FIELD;
	bus->bus_cmd = EMPTY_DATA_FIELD;
	bus->bus_data = EMPTY_DATA_FIELD;
	bus->bus_origid = EMPTY_DATA_FIELD;
}

void put_xaction_on_bus(msi_bus xaction, msi_bus *bus)
{
	
}

void do_bus_and_main_mem_stuff(core *core, int *main_mem, msi_bus *bus)
{
	// first, check if any xaction is on the bus. Meaning, check if the bus is taken or a previous BusRd or BusRdX xaction hasn't finished by using Flush xaction.
	// if bus is busy, stall cores as necessary (consider taking update_stage_buffers() function for each core and executing it after main_mem and bus actions).
	// if bus just finished answering a request, return the request to the one who asked it (bus_origid etc...).
	// if bus is free, let arbitration take place and send a request on the bus for the winning core (if such one exists in any of the cores for this cycle).
}

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

void update_bus(core *cores, msi_bus* bus, int cycle, int* next_RR, int *valid_request, int *memory_request_cycle)
{
	//check bus availability first
	for (int i = 0; i < CORES_NUM; i++)
	{
		int loc = (i + (*next_RR)) % CORES_NUM;
		if (cores[loc].bus_request_status == PENDING_SEND_CODE || cores[loc].bus_request_status == PENDING_WB_SEND_CODE)
		{
			*bus = cores[loc].bus_request;
			*next_RR = (loc + 1) % CORES_NUM;
			return;
		}
	}

	// maybe we can add valid_request and memory_request_cycle inside core struct

	int mem_to_flush = available_memory_to_flush(cycle, valid_request, memory_request_cycle);
	if (mem_to_flush != NO_VALUE_CODE) // If no core want to send, check the main memory
	{
		*bus = cores[mem_to_flush].bus_request;
		valid_request[mem_to_flush] = UNVALID_REQUEST_CODE;
	}
	else
		initialize_bus(&bus);
}


int find_xaction_origin(int *valid_request)
{
	for (int i = 0; i < CORES_NUM; i++)
		if (valid_request[i] == VALID_REQUEST_CODE)
			return i;
}

void update_bus(core *cores, msi_bus* bus, int cycle, int* next_RR, int *valid_request, int *memory_request_cycle, int *main_mem)
{

	// let bus cycles, if needed, to be promoted.
	// check if xaction ended
	if (bus->cycles_left == 0)
	{
		bus->bus_cmd = BUS_NO_CMD_CODE;
	}

	if (bus->cycles_left == 4)
	{
		// load bus with flush request from main memory
		bus->bus_origid = MEMORY_ORIGIN_CODE;
		bus->bus_cmd = BUS_FLUSH_CODE;
		address word_address = bus->bus_addr;
		int address = address_to_integer(word_address);
		int aligned_address = aligned_address - (aligned_address % 4);
		bus->bus_addr.index = aligned_address & 0xFF;
		bus->bus_addr.tag = (aligned_address >> 8) & 0xFFF;
		bus->bus_data = main_mem[address_to_integer(bus->bus_addr)];
		return;
	}

	if (bus->bus_cmd != BUS_NO_CMD_CODE)
	{
		return;
	}



}
