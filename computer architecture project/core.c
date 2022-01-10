#define _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_DEPRECATE

//#include <stdio.h>
#include <stdlib.h>

#include "parser.h"
#include "core.h"

#define TRACE_FILE_LINE_LEN 200	// more than enough
#define STAGE_FORMAT 4

//////////////////////// yuval

void initialize_core_statistics(core* core)
{
	core->stat.cycles = 1;
	core->stat.instructions = 0;
	core->stat.read_hit = 0;
	core->stat.write_hit = 0;
	core->stat.read_miss = 0;
	core->stat.write_miss = 0;
	core->stat.decode_stall = 0;
	core->stat.mem_stall = 0;
}

////////////////////////////

void initialize_core_regs(core *core)
{
	for (int i = 0; i < NUM_OF_REGS; i++)
	{
		core->core_registers[i] = 0;
	}
}

void initialize_core_pipeline(core *core) 
{
	for (int i = 0; i < PIPELINE_BUFFERS_NUM; i++)
	{
		core->core_pipeline[i].current_instruction.PC = -1;			// for validity purposes - we need to tell when a first real cmd entered each pipeline stage
		core->core_pipeline[i].new_instruction.PC = -1;
		core->core_pipeline[i].valid = false;
		core->core_pipeline[i].current_ALU_output = 0;
		core->core_pipeline[i].new_ALU_output = 0;
		core->core_pipeline[i].current_mem_output = 0;
		core->core_pipeline[i].new_mem_output = 0;
		core->core_pipeline[i].halt = false;
		core->core_pipeline[i].current_instruction.stalled = false;
		core->core_pipeline[i].new_instruction.stalled = false;
	}
}

void initialize_core(core *core, char *imem_filename)
{
	initialize_core_regs(core);
	parse_imem_file(core, imem_filename);
	initialize_cache_rams(&(core->core_cache));
	initialize_core_statistics(core);
	initialize_core_pipeline(core);
	initialize_bus(&(core->bus_request));

	core->next_PC = 0;
	core->clock_cycle_count = 0;
	core->core_halt = false;
	core->hazard = false;
	core->halt_PC = false;
	core->bus_request_status = NO_BUS_REQUEST_CODE;
	core->mem_stall = false;
}

bool check_stage_validity(core *core, ePIPIELINE_BUFFERS pipe_buffer)
{
	instruction current_instruction = core->core_pipeline[pipe_buffer].current_instruction;

	if (current_instruction.PC != -1)
	{
		core->core_pipeline[pipe_buffer].valid = true;
		return true;
	}

	bool valid_stage = core->core_pipeline[pipe_buffer].valid;
	if (!valid_stage)
		return false;
	return true;
}

void check_hazards(core* core)
{
	if ((core->core_pipeline[EX_MEM].current_instruction.rd == R0 && core->core_pipeline[ID_EX].current_instruction.rs == R0)
		|| (core->core_pipeline[EX_MEM].current_instruction.rd == R0 && core->core_pipeline[ID_EX].current_instruction.rt == R0)
		|| (core->core_pipeline[EX_MEM].current_instruction.rd == R0 && core->core_pipeline[IF_ID].current_instruction.rs == R0)
		|| (core->core_pipeline[MEM_WB].current_instruction.rd == R0 && core->core_pipeline[IF_ID].current_instruction.rt == R0)
		|| (core->core_pipeline[MEM_WB].current_instruction.rd == R0 && core->core_pipeline[IF_ID].current_instruction.rs == R0))
		return;

	if (core->core_pipeline[EX_MEM].current_instruction.rd == core->core_pipeline[ID_EX].current_instruction.rs
		&& core->core_pipeline[EX_MEM].valid && core->core_pipeline[ID_EX].valid && !core->core_pipeline[EX_MEM].current_instruction.stalled
		&& !core->core_pipeline[ID_EX].halt)
	{
		core->hazard = true;
	}

	if (core->core_pipeline[EX_MEM].current_instruction.rd == core->core_pipeline[ID_EX].current_instruction.rt
		&& core->core_pipeline[EX_MEM].valid && core->core_pipeline[ID_EX].valid && !core->core_pipeline[EX_MEM].current_instruction.stalled
		&& !core->core_pipeline[ID_EX].halt)
	{
		core->hazard = true;
	}

	if (core->core_pipeline[EX_MEM].current_instruction.rd == core->core_pipeline[IF_ID].current_instruction.rt
		&& core->core_pipeline[EX_MEM].valid && core->core_pipeline[IF_ID].valid && !core->core_pipeline[EX_MEM].current_instruction.stalled
		&& !core->core_pipeline[IF_ID].halt)
	{
		core->hazard = true;
	}

	if (core->core_pipeline[EX_MEM].current_instruction.rd == core->core_pipeline[IF_ID].current_instruction.rs
		&& core->core_pipeline[EX_MEM].valid && core->core_pipeline[IF_ID].valid && !core->core_pipeline[EX_MEM].current_instruction.stalled
		&& !core->core_pipeline[IF_ID].halt)
	{
		core->hazard = true;
	}

	if (core->core_pipeline[MEM_WB].current_instruction.rd == core->core_pipeline[IF_ID].current_instruction.rs
		&& core->core_pipeline[MEM_WB].valid && core->core_pipeline[IF_ID].valid && !core->core_pipeline[MEM_WB].current_instruction.stalled
		&& !core->core_pipeline[IF_ID].halt)
	{
		core->hazard = true;
	}

	if (core->core_pipeline[MEM_WB].current_instruction.rd == core->core_pipeline[IF_ID].current_instruction.rt
		&& core->core_pipeline[MEM_WB].valid && core->core_pipeline[IF_ID].valid && !core->core_pipeline[MEM_WB].current_instruction.stalled
		&& !core->core_pipeline[IF_ID].halt)
	{
		core->hazard = true;
	}
}

void fetch(core* core)
{
	core->fetch_old_PC = core->next_PC;		// save fetch stage old PC before it's altered, for trace purposes
	core->core_pipeline[IF_ID].new_instruction = core->core_imem[core->next_PC];
	core->next_PC += 1;		// take old PC, given there is no branch taken at this cycle. If branch is taken, next_PC is updated in decode stage
}

void check_branch(core* core)
{
	instruction current_instruction = core->core_pipeline[IF_ID].current_instruction;
	int opcode = current_instruction.opcode;

	if ((opcode < beq) || (opcode > bge))
	{
		return;
	}

	int rs_value = core->core_registers[current_instruction.rs];
	int rt_value = (current_instruction.rt == 0x1) ? current_instruction.immediate : core->core_registers[current_instruction.rt];	// if rt = 1 then take immediate value
	int next_branch_PC = core->core_registers[current_instruction.rt] & 0x3FF; // R[rd][9:0]

	switch (opcode)
	{
	case (beq):
		if (rs_value == rt_value)
		{
			core->next_PC = next_branch_PC;
		}
		break;
	case (bne):
		if (rs_value != rt_value)
		{
			core->next_PC = next_branch_PC;
		}
		break;
	case (blt):
		if (rs_value < rt_value)
		{
			core->next_PC = next_branch_PC;
		}
		break;
	case(bgt):
		if (rs_value > rt_value)
		{
			core->next_PC = next_branch_PC;
		}
		break;
	case (ble):
		if (rs_value <= rt_value)
		{
			core->next_PC = next_branch_PC;
		}
		break;
	case (bge):
		if (rs_value >= rt_value)
		{
			core->next_PC = next_branch_PC;
		}
		break;
	}
}

void decode(core* core)
{
	bool valid_stage = check_stage_validity(core, IF_ID);

	if (!valid_stage)
	{
		return;
	}

	check_hazards(core);
	if (core->hazard)
	{
		core->next_PC -= 1;		// not sure if need to add old PC if after branch
		core->core_pipeline[IF_ID].new_instruction = core->core_pipeline[IF_ID].current_instruction;
		core->core_pipeline[ID_EX].new_instruction = core->core_pipeline[ID_EX].current_instruction;
		core->core_pipeline[ID_EX].new_instruction.stalled = true;

	}
	else
	{
		check_branch(core);
		core->core_pipeline[ID_EX].new_instruction = core->core_pipeline[IF_ID].current_instruction;
	}
}

void execute(core* core)
{
	bool valid_stage = check_stage_validity(core, ID_EX);

	if (!valid_stage)
	{
		return;
	}

	if (core->core_pipeline[ID_EX].current_instruction.opcode == halt)
	{
		core->core_pipeline[IF_ID].halt = true;
	}

	bool stalled = core->core_pipeline[ID_EX].current_instruction.stalled;
	if (stalled)
	{
		core->core_pipeline[EX_MEM].new_instruction = core->core_pipeline[ID_EX].current_instruction;
		return;
	}

	instruction current_instruction = core->core_pipeline[ID_EX].current_instruction;
	int current_opcode = current_instruction.opcode;
	int ALU_output = 0;


	int rs_value = core->core_registers[current_instruction.rs];
	int rt_value = (current_instruction.rt == 0x1) ? current_instruction.immediate : core->core_registers[current_instruction.rt];	// if rt = 1 then take immediate value

	switch (current_opcode)
	{
	case (add):
		ALU_output = rs_value + rt_value;
		break;
	case (sub):
		ALU_output = rs_value - rt_value;
		break;
	case (and):
		ALU_output = rs_value & rt_value;
		break;
	case(or):
		ALU_output = rs_value | rt_value;
		break;
	case(xor):
		ALU_output = rs_value ^ rt_value;
		break;
	case (mul):
		ALU_output = rs_value * rt_value;
		break;
	case (sll):
		ALU_output = rs_value << rt_value;
		break;
	case (sra):
		ALU_output = rs_value >> rt_value;
		break;
	case (srl):	// perform logic shift, fill with zeros regardless of the sign
		ALU_output = (int)((unsigned int)rs_value >> rt_value);
		break;
	case (beq):
	case (bne):
	case (blt):
	case (bgt):
	case (ble):
	case (bge):
		break;
	case (jal):
		break;
	case (lw):
		ALU_output = rs_value + rt_value;
		break;
	case (sw):
		ALU_output = rs_value + rt_value;
		break;
	case (halt):
		break;
	}

	core->core_pipeline[EX_MEM].new_instruction = core->core_pipeline[ID_EX].current_instruction;
	core->core_pipeline[EX_MEM].new_ALU_output = ALU_output;
}

void memory(core* core, int *main_mem)
{
	bool valid_stage = check_stage_validity(core, EX_MEM);
	instruction current_instruction = core->core_pipeline[EX_MEM].current_instruction;
	bool stalled = current_instruction.stalled;

	if (!valid_stage)
	{
		return;
	}

	if (current_instruction.opcode == halt)
	{
		core->core_pipeline[ID_EX].halt = true;
	}

	if (stalled)
	{
		core->core_pipeline[MEM_WB].new_instruction = core->core_pipeline[EX_MEM].current_instruction;
		core->stat.mem_stall++;
		return;
	}

	// memory treatment
	if (current_instruction.opcode == lw)
	{
		// wrap in big if that asks if read access was already done or something like that
		int read_res = read_mem(core, main_mem);
		if (read_res == MISS_CODE)
		{
			//stall core
			//core->next_PC -= 1;		// not sure if need to add old PC if after branch
			//core->core_pipeline[EX_MEM].new_instruction = core->core_pipeline[EX_MEM].current_instruction;
			//core->core_pipeline[MEM_WB].new_instruction = core->core_pipeline[MEM_WB].current_instruction;
			//core->core_pipeline[MEM_WB].new_instruction.stalled = true;
			core->core_pipeline[MEM_WB].new_instruction.stalled = true;
			core->mem_stall = true;
			return;
		}
		else 
		{
			core->core_pipeline[MEM_WB].new_mem_output = read_res; // propogate read result to next stage if access was hit
		}
	}
	else if (current_instruction.opcode == sw)
	{
		int write_result = write_mem(core);
		if (write_result == MISS_CODE)
		{
			// stall core
			core->core_pipeline[MEM_WB].new_instruction.stalled = true;
			core->mem_stall = true;
			return;
		}
	}

	core->core_pipeline[MEM_WB].new_instruction = core->core_pipeline[EX_MEM].current_instruction;
	core->core_pipeline[MEM_WB].new_ALU_output = core->core_pipeline[EX_MEM].current_ALU_output;
}

void write_back(core* core)
{
	bool valid_stage = check_stage_validity(core, MEM_WB);
	bool stalled = core->core_pipeline[MEM_WB].current_instruction.stalled;

	if (!valid_stage)
	{
		return;
	}

	if (core->core_pipeline[MEM_WB].current_instruction.opcode == halt)
	{
		core->core_pipeline[EX_MEM].halt = true;
		core->core_halt = true;
		return;
	}

	if (stalled)
	{
		return;
	}

	instruction current_instruction = core->core_pipeline[MEM_WB].current_instruction;
	core->stat.instructions++;

	if ((current_instruction.opcode >= beq) && (current_instruction.opcode <= bge))
	{
		return;
	}

	int reg = current_instruction.rd;
	if (reg == R0)
	{
		return;
	}

	if (current_instruction.opcode == lw)	// reg-memory operation
	{
		core->core_registers[reg] = core->core_pipeline[MEM_WB].current_mem_output;
		return;
	}

	core->core_registers[reg] = core->core_pipeline[MEM_WB].current_ALU_output;	// reg-reg operation
}

void update_stage_buffers(core* core)
{
	if (core->mem_stall)	//special case for memory stall, WB stage can continue
	{
		core->core_pipeline[MEM_WB].current_instruction = core->core_pipeline[MEM_WB].new_instruction;
		return;
	}
	// update instructions
	core->core_pipeline[IF_ID].current_instruction = core->core_pipeline[IF_ID].new_instruction;
	core->core_pipeline[ID_EX].current_instruction = core->core_pipeline[ID_EX].new_instruction;
	core->core_pipeline[EX_MEM].current_instruction = core->core_pipeline[EX_MEM].new_instruction;
	core->core_pipeline[MEM_WB].current_instruction = core->core_pipeline[MEM_WB].new_instruction;

	// update ALU_output
	core->core_pipeline[EX_MEM].current_ALU_output = core->core_pipeline[EX_MEM].new_ALU_output;
	core->core_pipeline[MEM_WB].current_ALU_output = core->core_pipeline[MEM_WB].new_ALU_output;

	// update mem_output
	core->core_pipeline[MEM_WB].current_mem_output = core->core_pipeline[MEM_WB].new_mem_output;
}

void format_stage_trace(bool valid, bool stalled, bool halt, char* str, int num)
{
	if (!valid || stalled || halt)
	{
		sprintf(str, "---");
	}
	else
	{
		sprintf(str, "%03X", num);
	}
}

// Writes to coretrace file
void write_trace(core* core, FILE* trace_file)
{
	char str[TRACE_FILE_LINE_LEN];
	int clock_count = core->clock_cycle_count;

	pipeline_stage* core_pipeline = core->core_pipeline;
	char fetch[STAGE_FORMAT];
	char decode[STAGE_FORMAT];
	char execute[STAGE_FORMAT];
	char memory[STAGE_FORMAT];
	char write_back[STAGE_FORMAT];

	format_stage_trace(true, core_pipeline[IF_ID].current_instruction.stalled, core_pipeline[IF_ID].halt, fetch, core->fetch_old_PC);
	format_stage_trace(core_pipeline[IF_ID].valid, core_pipeline[IF_ID].current_instruction.stalled, core_pipeline[IF_ID].halt, decode, core_pipeline[IF_ID].current_instruction.PC);
	format_stage_trace(core_pipeline[ID_EX].valid, core_pipeline[ID_EX].current_instruction.stalled, core_pipeline[ID_EX].halt, execute, core_pipeline[ID_EX].current_instruction.PC);
	format_stage_trace(core_pipeline[EX_MEM].valid, core_pipeline[EX_MEM].current_instruction.stalled, core_pipeline[EX_MEM].halt, memory, core_pipeline[EX_MEM].current_instruction.PC);
	format_stage_trace(core_pipeline[MEM_WB].valid, core_pipeline[MEM_WB].current_instruction.stalled, core_pipeline[MEM_WB].halt, write_back, core_pipeline[MEM_WB].current_instruction.PC);

	int* regs = core->current_core_registers;

	sprintf(str, "%d %s %s %s %s %s %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X \n",
		clock_count, fetch, decode, execute, memory, write_back, regs[R2], regs[R3], regs[R4], regs[R5],
		regs[R6], regs[R7], regs[R8], regs[R9], regs[R10], regs[R11], regs[R12], regs[R13], regs[R14], regs[R15]);

	fputs(str, trace_file);
}

void copy_regs(core* core)
{
	for (int i = 0; i < NUM_OF_REGS; i++)
	{
		core->current_core_registers[i] = core->core_registers[i];
	}
}

void simulate_clock_cycle(core* core, FILE* trace_file, int *main_mem)
{
	if (core->core_halt)
		return;
	
	if (core->mem_stall)
	{
		write_trace(core, trace_file);
		core->clock_cycle_count++;
		return; // in this case, stall core completely, only write its trace
	}
	copy_regs(core);				// snapshot core regs at the beginning of the clock cycle
	fetch(core);
	decode(core);
	execute(core);
	memory(core, main_mem);
	write_back(core);
	write_trace(core, trace_file);
	update_stage_buffers(core);
	core->clock_cycle_count++;
	core->hazard = false;
}

bool all_cores_halt(core cores[CORES_NUM])
{
	int count = 0;

	for (int i = 0; i < CORES_NUM; i++)
		if (cores[i].core_halt)
			count++;

	if (count == CORES_NUM)
		return true;

	return false;
}