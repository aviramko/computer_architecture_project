#define _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_DEPRECATE

#include <stdio.h>
#include <stdlib.h>

#include "core.h"
#include "parser.h"

#define TRACE_FILE_LINE_LEN 152

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
		core->core_pipeline[i].halt = false;
		core->core_pipeline[i].stalled = false;
	}
}

void initialize_core(core *core, char *imem_filename)
{
	initialize_core_regs(core);
	parse_imem_file(core, imem_filename);
	initialize_core_pipeline(core);
	core->next_PC = 0;
	core->clock_cycle_count = 0;
	core->core_halt = false;
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
}

void fetch(core *core)
{
	core->fetch_old_PC = core->next_PC;		// save fetch stage old PC before it's altered, for trace purposes
	core->core_pipeline[IF_ID].new_instruction = core->core_imem[core->next_PC];
	core->next_PC += 1;		// take old PC, given there is no branch taken at this cycle. If branch is taken, next_PC is updated in decode stage
}

void check_branch(core *core)
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

void decode(core *core)
{
	bool valid_stage = check_stage_validity(core, IF_ID);
	if (!valid_stage)
	{
		return;
	}

	check_branch(core);

	core->core_pipeline[ID_EX].new_instruction = core->core_pipeline[IF_ID].current_instruction;
}

void execute(core *core)
{
	bool valid_stage = check_stage_validity(core, ID_EX);
	if (!valid_stage)
	{
		return;
	}

	instruction current_instruction = core->core_pipeline[ID_EX].current_instruction;
	int current_opcode = current_instruction.opcode;
	int ALU_output = 0;

	if (core->core_pipeline[ID_EX].current_instruction.opcode == halt)
	{
		core->core_pipeline[IF_ID].stalled = true;
		core->core_pipeline[IF_ID].halt = true;
	}

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
		break;
	case (bne):
		break;
	case (blt):
		break;
	case (bgt):
		break;
	case (ble):
		break;
	case (bge):
		break;
	case (jal):
		break;
	case (lw):
		break;
	case (sw):
		break;
	case (halt):
		break;
	}

	core->core_pipeline[EX_MEM].new_instruction = core->core_pipeline[ID_EX].current_instruction;
	core->core_pipeline[EX_MEM].new_ALU_output = ALU_output;
}

void memory(core *core)
{
	bool valid_stage = check_stage_validity(core, EX_MEM);
	if (!valid_stage)
	{
		return;
	}

	if (core->core_pipeline[EX_MEM].current_instruction.opcode == halt)
	{
		core->core_pipeline[ID_EX].stalled = true;
		core->core_pipeline[ID_EX].halt = true;
	}

	core->core_pipeline[MEM_WB].new_instruction = core->core_pipeline[EX_MEM].current_instruction;
	core->core_pipeline[MEM_WB].new_ALU_output = core->core_pipeline[EX_MEM].current_ALU_output;
}

void write_back(core *core)
{
	bool valid_stage = check_stage_validity(core, MEM_WB);
	if (!valid_stage)
	{
		return;
	}

	if (core->core_pipeline[MEM_WB].current_instruction.opcode == halt)
	{
		core->core_pipeline[EX_MEM].stalled = true;
		core->core_pipeline[EX_MEM].halt = true;
		core->core_halt = true;
		return;
	}

	instruction current_instruction = core->core_pipeline[MEM_WB].current_instruction;
	if ((current_instruction.opcode >= beq) && (current_instruction.opcode <= bge))
	{
		return;
	}
	
	int reg = current_instruction.rd;
	if (reg == R0)
	{
		return;
	}
	core->core_registers[reg] = core->core_pipeline[MEM_WB].current_ALU_output;
}

void update_stage_buffers(core *core)
{
	// update instructions
	core->core_pipeline[IF_ID].current_instruction = core->core_pipeline[IF_ID].new_instruction;
	core->core_pipeline[ID_EX].current_instruction = core->core_pipeline[ID_EX].new_instruction;
	core->core_pipeline[EX_MEM].current_instruction = core->core_pipeline[EX_MEM].new_instruction;
	core->core_pipeline[MEM_WB].current_instruction = core->core_pipeline[MEM_WB].new_instruction;

	// update ALU_output
	core->core_pipeline[EX_MEM].current_ALU_output = core->core_pipeline[EX_MEM].new_ALU_output;
	core->core_pipeline[MEM_WB].current_ALU_output = core->core_pipeline[MEM_WB].new_ALU_output;
}

void write_trace(core *core, FILE *trace_file)
{
	char str[TRACE_FILE_LINE_LEN];
	int clock_count = core->clock_cycle_count;

	bool valid_stage = core->core_pipeline[IF_ID].valid;
	bool stalled_stage = core->core_pipeline[IF_ID].stalled;
	char fetch[4];
	if (stalled_stage)
	{
		sprintf(fetch, "---");
	}
	else
	{
		sprintf(fetch, "%03X", core->fetch_old_PC);
	}

	valid_stage = core->core_pipeline[IF_ID].valid;
	stalled_stage = core->core_pipeline[IF_ID].stalled;
	char decode[4];
	if (stalled_stage || !valid_stage)
	{
		sprintf(decode, "---");
	}
	else
	{
		sprintf(decode, "%03X", core->core_pipeline[IF_ID].current_instruction.PC);
	}

	valid_stage = core->core_pipeline[ID_EX].valid;
	stalled_stage = core->core_pipeline[ID_EX].stalled;
	char execute[4];
	if (stalled_stage || !valid_stage)
	{
		sprintf(execute, "---");
	}
	else
	{
		sprintf(execute, "%03X", core->core_pipeline[ID_EX].current_instruction.PC);
	}

	valid_stage = core->core_pipeline[EX_MEM].valid;
	stalled_stage = core->core_pipeline[EX_MEM].stalled;
	char memory[4];
	if (stalled_stage || !valid_stage)
	{
		sprintf(memory, "---");
	}
	else
	{
		sprintf(memory, "%03X", core->core_pipeline[EX_MEM].current_instruction.PC);
	}

	valid_stage = core->core_pipeline[MEM_WB].valid;
	stalled_stage = core->core_pipeline[MEM_WB].stalled;
	char write_back[4];
	if (stalled_stage || !valid_stage)
	{
		sprintf(write_back, "---");
	}
	else
	{
		sprintf(write_back, "%03X", core->core_pipeline[MEM_WB].current_instruction.PC);
	}

	int *regs = core->current_core_registers;

	sprintf(str,"%d %s %s %s %s %s %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X \n",
			clock_count, fetch, decode, execute, memory, write_back, regs[R2], regs[R3], regs[R4], regs[R5],
			regs[R6], regs[R7], regs[R8], regs[R9], regs[R10], regs[R11], regs[R12], regs[R13], regs[R14], regs[R15]);
	fputs(str, trace_file);
}

void copy_regs(core *core)
{
	for (int i = 0; i < NUM_OF_REGS; i++)
	{
		core->current_core_registers[i] = core->core_registers[i];
	}
}

void clock_cycle(core *core, FILE *trace_file)
{
	copy_regs(core);			// snapshot core regs at the beginning of the clock cycle
	fetch(core);
	decode(core);
	execute(core);
	memory(core);
	write_back(core);
	write_trace(core, trace_file);
	update_stage_buffers(core);
	core->clock_cycle_count++;
}

void simulate_core(core *core, FILE *trace_file)
{
	while (!core->core_halt)
	{
		clock_cycle(core,trace_file);
	}
}