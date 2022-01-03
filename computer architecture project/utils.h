#ifndef UTILS_HEADER
#define UTILS_HEADER

#include "core.h"

#define SUCCESS_CODE 0 // yuval
#define ERROR_CODE -1 //yuval

#define DEFAULT_FILE_IMEM0 "imem0.txt"
#define DEFAULT_FILE_IMEM1 "imem1.txt"
#define DEFAULT_FILE_IMEM2 "imem2.txt"
#define DEFAULT_FILE_IMEM3 "imem3.txt"
#define DEFAULT_FILE_MEMIN "memin.txt"
#define DEFAULT_FILE_MEMOUT "memout.txt"
#define DEFAULT_FILE_REGOUT0 "regout0.txt"
#define DEFAULT_FILE_REGOUT1 "regout1.txt"
#define DEFAULT_FILE_REGOUT2 "regout2.txt"
#define DEFAULT_FILE_REGOUT3 "regout3.txt"
#define DEFAULT_FILE_CORE0TRACE "core0trace.txt"
#define DEFAULT_FILE_CORE1TRACE "core1trace.txt"
#define DEFAULT_FILE_CORE2TRACE "core2trace.txt"
#define DEFAULT_FILE_CORE3TRACE "core3trace.txt"
#define DEFAULT_FILE_BUSTRACE "bustrace.txt"
#define DEFAULT_FILE_DSRAM0 "dsram0.txt"
#define DEFAULT_FILE_DSRAM1 "dsram1.txt"
#define DEFAULT_FILE_DSRAM2 "dsram2.txt"
#define DEFAULT_FILE_DSRAM3 "dsram3.txt"
#define DEFAULT_FILE_TSRAM0 "tsram0.txt"
#define DEFAULT_FILE_TSRAM1 "tsram1.txt"
#define DEFAULT_FILE_TSRAM2 "tsram2.txt"
#define DEFAULT_FILE_TSRAM3 "tsram3.txt"
#define DEFAULT_FILE_STATS0 "stats0.txt"
#define DEFAULT_FILE_STATS1 "stats1.txt"
#define DEFAULT_FILE_STATS2 "stats2.txt"
#define DEFAULT_FILE_STATS3 "stats3.txt"

int address_to_integer(address addr);

int initialize_array_from_file(char* file_name, int* memory_array, int max_array_size);

#endif // !UTILS_HEADER

