#ifndef _CORE_HEADER_
#define _CORE_HEADER_

#include <stdio.h>
#include <stdbool.h>

typedef struct _core core;

#include "cache.h"
#include "bus_mem.h"
#include "utils.h"


#define NUM_OF_REGS 16
#define MAX_IMEM_LINES 1024
#define PIPELINE_BUFFERS_NUM 4

#define NO_VALUE_CODE -1
#define UNVALID_REQUEST_CODE 0 // yuval
#define VALID_REQUEST_CODE 1 // yuval

#define CORES_NUM 4

//#define BUS_FLUSH_CODE 3
#define IMEM_LINES_NUM 1024

// Core Codes
#define MEMORY_ORIGIN_CODE 4

typedef enum reg {
	R0,
	R1,
	R2,
	R3,
	R4,
	R5,
	R6,
	R7,
	R8,
	R9,
	R10,
	R11,
	R12,
	R13,
	R14,
	R15
} reg;

typedef enum opcodes {
	add		= 0,
	sub		= 1,
	and		= 2,
	or		= 3,
	xor		= 4,
	mul		= 5,
	sll		= 6,
	sra		= 7,
	srl		= 8,
	beq		= 9,
	bne		= 10,
	blt		= 11,
	bgt		= 12,
	ble		= 13,
	bge		= 14,
	jal		= 15,
	lw		= 16,
	sw		= 17,
	halt	= 20
} opcodes;

//typedef enum ePIPELINE_STAGES {
//	FETCH = 0,
//	DECODE = 1,
//	EXECUTE = 2,
//	MEMORY = 3,
//	WRITE_BACK = 4,
//	EMPTY = 5
//} ePIPELINE_STAGES;

typedef enum ePIPIELINE_BUFFERS {
	IF_ID = 0,
	ID_EX = 1,
	EX_MEM = 2,
	MEM_WB = 3
} ePIPIELINE_BUFFERS;

typedef struct instruction {
	int PC;
	int opcode;		// bits 31:24
	int rd;			// bits 23:20
	int rs;			// bits 19:16
	int rt;			// bits 15:12
	int immediate;	// bits 11:0
	bool stalled;	
} instruction;

typedef struct pipeline_stage {
	instruction current_instruction;
	instruction new_instruction;
	bool valid;
	int current_ALU_output;
	int new_ALU_output;
	bool halt;
} pipeline_stage;

////////////////////// yuval

typedef struct statistics
{
	int cycles;
	int instructions;
	int read_hit;
	int write_hit;
	int read_miss;
	int write_miss;
	int decode_stall;
	int mem_stall;

} statistics;

//////////////////////////////////////

struct _core {
	cache core_cache;
	instruction core_imem[MAX_IMEM_LINES];
	statistics stat; // yuval
	msi_bus bus_request; // yuval
	int bus_request_status; // yuval
	int core_registers[NUM_OF_REGS];
	int current_core_registers[NUM_OF_REGS];
	int core_imem_length;
	int clock_cycle_count;
	int next_PC;
	int fetch_old_PC;
	pipeline_stage core_pipeline[PIPELINE_BUFFERS_NUM];
	bool hazard;
	bool core_halt;
	bool halt_PC;
	char *imem_file;
};


void initialize_core(core *core, char *imem_filename);
void simulate_clock_cycle(core *core, FILE *trace_file);

#endif // !CORE_HEADER