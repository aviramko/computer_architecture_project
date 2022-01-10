#ifndef UTILS_HEADER
#define UTILS_HEADER

#include "hard_coded_data.h"
#include "bus_mem.h"

int address_to_integer(address addr);

void initialize_args_files(char* args_files[ARGS_EXPECTED_NUM - 1], int args_num, char* args_values[]);

int initialize_array_from_file(char* file_name, int* memory_array, int max_array_size);

int write_bustrace(msi_bus* bus, int cycle, char* bustrace_file);

int write_files(core* cores, char* args_files[ARGS_EXPECTED_NUM - 1], int main_mem[MAIN_MEM_SIZE]);

void updateStatistics(core* core, int status);

#endif // !UTILS_HEADER

