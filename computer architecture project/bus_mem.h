#ifndef BUS_MEM_HEADER
#define BUS_MEM_HEADER

//#include "core.h"
#include "hard_coded_data.h"

void initialize_bus(msi_bus* bus);
void main_memory_bus_snooper(core *cores, msi_bus bus, int cycle, int *main_mem, int *valid_request, int *memory_request_cycle);
void initialize_main_mem(int* main_mem, int* valid_request, int* memory_request_cycle);
//void update_bus(core *cores, msi_bus* bus, int cycle, int* next_RR, int *valid_request, int *memory_request_cycle);
void update_bus(core *cores, msi_bus* bus, int cycle, int* next_RR, int *valid_request, int *memory_request_cycle, int *main_mem);

#endif // !BUS_MEM_HEADER