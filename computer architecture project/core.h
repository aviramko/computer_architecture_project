#ifndef CORE_HEADER
#define CORE_HEADER

#define NUM_OF_REGS 16
#define MAX_IMEM_LINES 1024

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

typedef struct instruction {
	int opcode;		// bits 31:24
	int rd;			// bits 23:20
	int rs;			// bits 19:16
	int rt;			// bits 15:12
	int immediate;	// bits 11:0
} instruction;

//typedef struct pipeline {
//	
//} pipeline;

typedef struct core {
	instruction core_imem[MAX_IMEM_LINES];
	int core_registers[NUM_OF_REGS];
	int core_imem_length;
	int clock_cycle;
	//int PC;
} core;


void initialize_core(core *core, char *imem_filename);

#endif // !CORE_HEADER