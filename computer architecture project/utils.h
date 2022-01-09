#ifndef UTILS_HEADER
#define UTILS_HEADER

#include "hard_coded_data.h"
#include "bus_mem.h"

int address_to_integer(address addr);
int initialize_array_from_file(char* file_name, int* memory_array, int max_array_size);
int write_bustrace(msi_bus* bus, int cycle, char* bustrace_file);

#endif // !UTILS_HEADER

