#ifndef BUS_MEM_HEADER
#define BUS_MEM_HEADER

//#include "core.h"
#include "hard_coded_data.h"

void initialize_bus(msi_bus* bus);
void main_memory_bus_snooper(core* cores[CORES_NUM], msi_bus bus, int cycle, int* main_mem[MAIN_MEM_SIZE], int* valid_request[CORES_NUM], int* memory_request_cycle[CORES_NUM]);
void initialize_main_mem(int* main_mem[MAIN_MEM_SIZE], int* valid_request[CORES_NUM], int* memory_request_cycle[CORES_NUM]);
void update_bus(core cores[CORES_NUM], msi_bus* bus, int cycle, int* next_RR, int* valid_request[CORES_NUM], int memory_request_cycle[CORES_NUM]);


#endif // !BUS_MEM_HEADER