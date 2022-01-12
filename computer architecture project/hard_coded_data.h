#ifndef HARD_CODED_DATA_HEADER
#define HARD_CODED_DATA_HEADER

#include <stdbool.h>

typedef struct _cache cache;

#define MISS_CODE -1
#define HIT_CODE 0

#define SUCCESS_CODE 0
#define ERROR_CODE -1

#define NUM_OF_REGS 16
#define MAX_IMEM_LINES 1024
#define PIPELINE_BUFFERS_NUM 4

#define EMPTY_DATA_FIELD 0

#define CORES_NUM 4

#define MAIN_MEM_SIZE 1048576 // 2^20

#define DSRAM_SIZE 256
#define BLOCK_SIZE 4
#define DSRAM_FRAMES (DSRAM_SIZE/BLOCK_SIZE)

#define TSRAM_SIZE 64

#define BUS_SHARED 1
#define BUS_NOT_SHARED 0

// Status Codes
#define READ_MISS_CODE 0
#define READ_HIT_CODE 1
#define WRITE_MISS_CODE 2
#define WRITE_HIT_CODE 3

#define MAX_IMEM_LINE_WIDTH 10

#define TRACE_FILE_LINE_LEN 200
#define STAGE_FORMAT 4

#define MAIN_MEM_ANSWER_CYCLES 16 
#define FLUSH_CYCLES 4

#define ARGS_EXPECTED_NUM 28

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
	add = 0,
	sub = 1,
	and = 2,
	or = 3,
	xor = 4,
	mul = 5,
	sll = 6,
	sra = 7,
	srl = 8,
	beq = 9,
	bne = 10,
	blt = 11,
	bgt = 12,
	ble = 13,
	bge = 14,
	jal = 15,
	lw = 16,
	sw = 17,
	halt = 20
} opcodes;

typedef enum ePIPIELINE_BUFFERS {
	IF_ID = 0,
	ID_EX = 1,
	EX_MEM = 2,
	MEM_WB = 3
} ePIPIELINE_BUFFERS;

typedef enum bus_requests {
	no_cmd,
	bus_rd,
	bus_rdx,
	flush,
	read_miss_flush_request,
	read_miss_flush,
	write_miss_flush_request,
	write_miss_flush
} bus_requests;

typedef enum _bus_origid {
	core0_origid,
	core1_origid,
	core2_origid,
	core3_origid,
	main_mem_origid
} bus_origid;

typedef enum _foo {
	original_request,
	flush_request,
} foo;

typedef enum MESI_states {
	invalid,
	shared,
	exclusive,
	modified
} MESI_states;

typedef enum _flush_cause {
	no_reason,
	busrd_flush,
	busrdx_flush
} flush_cause;

typedef struct tsram_entry {
	unsigned int tag;
	unsigned int MESI_state;
	unsigned int next_MESI_state; // TOOD delete
	bool valid;
} tsram_entry;

struct _cache {
	tsram_entry tsram[TSRAM_SIZE];
	int dsram[DSRAM_SIZE];
};

typedef struct memory_address
{
	unsigned int index : 8;
	unsigned int tag : 12;

} address;

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
	int new_mem_output;
	int current_mem_output;
	bool halt;
} pipeline_stage;

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

typedef struct MSI_bus
{
	unsigned int bus_shared : 1;
	unsigned int bus_data : 32;
	address bus_addr;
	unsigned int bus_cmd;
	unsigned int bus_origid : 3;
	int cycles_left;
	flush_cause flush_reason;
	int flush_to;
} msi_bus;

typedef struct MSI_bus_FF {
	msi_bus old_bus;
	msi_bus new_bus;
} msi_bus_ff; // TODO delete

struct _core {
	cache core_cache;
	instruction core_imem[MAX_IMEM_LINES];
	statistics stat;
	msi_bus bus_request;
	//int bus_request_status; // TOOD delete
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
	bool mem_stall;
	bool mem_completed;
};
typedef struct _core core;

#endif // !HARD_CODED_DATA_HEADER
