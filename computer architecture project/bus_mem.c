#define _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_DEPRECATE

//#include <stdio.h>
//#include <stdlib.h>

#include "core.h"
#include "bus_mem.h"
#include "utils.h"

void initialize_main_mem(int *main_mem)
{
	int i = 0;

	for (i = 0; i < MAIN_MEM_SIZE; i++) 
	{
		main_mem[i] = EMPTY_DATA_FIELD;
	}

	initialize_array_from_file("memin.txt", main_mem, MAIN_MEM_SIZE);
}

void initialize_bus(msi_bus *bus) // TODO use
{
	bus->bus_addr.index = EMPTY_DATA_FIELD;
	bus->bus_addr.tag = EMPTY_DATA_FIELD;
	bus->bus_cmd = EMPTY_DATA_FIELD;
	bus->bus_data = EMPTY_DATA_FIELD;
	bus->bus_origid = EMPTY_DATA_FIELD;
}

//void arbitration()
//{
//
//}

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


//// Main Memory Functions
//
//// Read the bus for cores flushes and requests
//void main_memory_bus_snooper(core* core, int cycle) // add support in multiple cores
//{
//	if (bus.bus_origid == MEMORY_ORIGIN_CODE || bus.bus_cmd == BUS_NO_CMD_CODE)
//		return;
//	if (bus.bus_cmd == BUS_FLUSH_CODE)
//	{
//		main_mem[address_to_integer(bus.bus_addr)] = bus.bus_data;
//		return;
//	}
//	memory_bus_request[bus.bus_origid].bus_addr = bus.bus_addr;
//	memory_bus_request[bus.bus_origid].bus_cmd = BUS_FLUSH_CODE;
//	memory_bus_request[bus.bus_origid].bus_data = main_mem[address_to_integer(bus.bus_addr)];
//	memory_bus_request[bus.bus_origid].bus_origid = MEMORY_ORIGIN_CODE;
//	memory_request_cycle = cycle;
//	valid_request = VALID_REQUEST_CODE;
//}
//
//// Checks if there is any ready flush ready under main memory
//int available_memory_to_flush(core* core, int cycle) // add support in multiple cores
//{
//	// if (memory_bus_request[bus.bus_origid].bus_origid == MEMORY_ORIGIN_CODE)
	// {
//		for (int i = 0; i < CORES_NUM; i++)
//			if (valid_request == VALID_REQUEST_CODE && cycle - memory_request_cycle >= 16)
//				return i;
//	//}
//
//	// else
	// {
	// 	for (int i = 0; i < CORES_NUM; i++)
	// 	{
	// 		int num = (i + 1 + memory_bus_request[bus.bus_origid].bus_origid) % 4; // next core in RR
	// 		if (valid_request == VALID_REQUEST_CODE && cycle - memory_request_cycle >= 16) // add multiple cores here
	// 			return num;
	// 	}
	// }
//
//
//	return NO_VALUE_CODE;
//}
//
//// Cancel memory request in the main memory
//void cancel_memory_request(int core_num) // add support in multiple cores
//{
//	valid_request = UNVALID_REQUEST_CODE;
//}
//
//// Bus functions
//
//void update_bus(core* core, int cycle) // add support to multiple cores
//{
//	//for (int i = 0; i < CORES_NUM; i++) { // Checks if any core want to send bus request by order
//	if (core->bus_request_status == PENDING_SEND_CODE || core->bus_request_status == PENDING_WB_SEND_CODE)
//	{
//		bus = core->bus_request;
//		return;
//	}
//	//}
//	if (available_memory_to_flush(core, cycle) != NO_VALUE_CODE) // If no core want to send, check the main memory
//	{
//		bus = memory_bus_request[available_memory_to_flush(core, cycle)];
//		valid_request = UNVALID_REQUEST_CODE;
//	}
//	else
//		bus = empty_request; // If no request
//}
