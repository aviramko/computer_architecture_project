#define _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_DEPRECATE

#include <stdlib.h>

#include "parser.h"
#include "core.h"

// Initalizing the statistics for each core
void initialize_core_statistics(core* core)
{
	core->stat.cycles = 0;
	core->stat.instructions = 0;
	core->stat.read_hit = 0;
	core->stat.write_hit = 0;
	core->stat.read_miss = 0;
	core->stat.write_miss = 0;
	core->stat.decode_stall = 0;
	core->stat.mem_stall = 0;
}

// Initializing the registers for each core
void initialize_core_regs(core *core)
{
	for (int i = 0; i < NUM_OF_REGS; i++)
	{
		core->core_registers[i] = 0;
	}
}

// Initializing the pipeline for each core
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

// Initializing the entire core struct
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
	//core->bus_request_status = NO_BUS_REQUEST_CODE;
	core->mem_stall = false;
	core->mem_completed = false;
}

// Check if the current pipeline stage is valid
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

// check for pipeline hazards at the current cycle for specific core
void check_hazards(core* core)
{
	if ((core->core_pipeline[EX_MEM].current_instruction.rd == R0 && core->core_pipeline[ID_EX].current_instruction.rs == R0)
		|| (core->core_pipeline[EX_MEM].current_instruction.rd == R0 && core->core_pipeline[ID_EX].current_instruction.rt == R0)
		|| (core->core_pipeline[EX_MEM].current_instruction.rd == R0 && core->core_pipeline[IF_ID].current_instruction.rs == R0)
		|| (core->core_pipeline[MEM_WB].current_instruction.rd == R0 && core->core_pipeline[IF_ID].current_instruction.rt == R0)
		|| (core->core_pipeline[MEM_WB].current_instruction.rd == R0 && core->core_pipeline[IF_ID].current_instruction.rs == R0)
		|| (core->core_pipeline[EX_MEM].current_instruction.rd == R1 && core->core_pipeline[IF_ID].current_instruction.rt == R1)
		//|| (core->core_pipeline[MEM_WB].current_instruction.rd == R1 && core->core_pipeline[IF_ID].current_instruction.rt == R1)
		|| (core->core_pipeline[EX_MEM].current_instruction.rd == R1 && core->core_pipeline[IF_ID].current_instruction.rs == R1)
		|| (core->core_pipeline[MEM_WB].current_instruction.rd == R1 && core->core_pipeline[IF_ID].current_instruction.rs == R1))
		return;

	// Data hazards (NO LOAD)
	//if (core->core_pipeline[EX_MEM].current_instruction.rd == core->core_pipeline[ID_EX].current_instruction.rs
	//	&& core->core_pipeline[EX_MEM].valid && core->core_pipeline[ID_EX].valid && !core->core_pipeline[EX_MEM].current_instruction.stalled
	//	&& !core->core_pipeline[ID_EX].halt)
	//{
	//	core->hazard = true;
	//}
	//
	//if (core->core_pipeline[EX_MEM].current_instruction.rd == core->core_pipeline[ID_EX].current_instruction.rt
	//	&& core->core_pipeline[EX_MEM].valid && core->core_pipeline[ID_EX].valid && !core->core_pipeline[EX_MEM].current_instruction.stalled
	//	&& !core->core_pipeline[ID_EX].halt)
	//{
	//	core->hazard = true;
	//}

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

	// LOAD-USE HAZARDS
	if ((core->core_pipeline[ID_EX].current_instruction.opcode == lw) || (core->core_pipeline[EX_MEM].current_instruction.opcode == lw) ||
		(core->core_pipeline[MEM_WB].current_instruction.opcode == lw))
	{

		if (core->core_pipeline[IF_ID].current_instruction.rd == core->core_pipeline[ID_EX].current_instruction.rd
			&& core->core_pipeline[ID_EX].valid && core->core_pipeline[IF_ID].valid && !core->core_pipeline[ID_EX].current_instruction.stalled
			&& !core->core_pipeline[IF_ID].halt)
		{
			core->hazard = true;
		}

		if (core->core_pipeline[IF_ID].current_instruction.rd == core->core_pipeline[ID_EX].current_instruction.rt
			&& core->core_pipeline[ID_EX].valid && core->core_pipeline[IF_ID].valid && !core->core_pipeline[ID_EX].current_instruction.stalled
			&& !core->core_pipeline[IF_ID].halt)
		{
			core->hazard = true;
		}

		if (core->core_pipeline[IF_ID].current_instruction.rd == core->core_pipeline[ID_EX].current_instruction.rs
			&& core->core_pipeline[ID_EX].valid && core->core_pipeline[IF_ID].valid && !core->core_pipeline[ID_EX].current_instruction.stalled
			&& !core->core_pipeline[IF_ID].halt)
		{
			core->hazard = true;
		}

		////////

		if (core->core_pipeline[IF_ID].current_instruction.rd == core->core_pipeline[EX_MEM].current_instruction.rd
			&& core->core_pipeline[EX_MEM].valid && core->core_pipeline[IF_ID].valid && !core->core_pipeline[EX_MEM].current_instruction.stalled
			&& !core->core_pipeline[IF_ID].halt)
		{
			core->hazard = true;
		}

		if (core->core_pipeline[IF_ID].current_instruction.rd == core->core_pipeline[EX_MEM].current_instruction.rt
			&& core->core_pipeline[EX_MEM].valid && core->core_pipeline[IF_ID].valid && !core->core_pipeline[EX_MEM].current_instruction.stalled
			&& !core->core_pipeline[IF_ID].halt)
		{
			core->hazard = true;
		}

		if (core->core_pipeline[IF_ID].current_instruction.rd == core->core_pipeline[EX_MEM].current_instruction.rs
			&& core->core_pipeline[EX_MEM].valid && core->core_pipeline[IF_ID].valid && !core->core_pipeline[EX_MEM].current_instruction.stalled
			&& !core->core_pipeline[IF_ID].halt)
		{
			core->hazard = true;
		}

		///

		if (core->core_pipeline[IF_ID].current_instruction.rd == core->core_pipeline[MEM_WB].current_instruction.rd
			&& core->core_pipeline[MEM_WB].valid && core->core_pipeline[IF_ID].valid && !core->core_pipeline[MEM_WB].current_instruction.stalled
			&& !core->core_pipeline[IF_ID].halt)
		{
			core->hazard = true;
		}

		if (core->core_pipeline[IF_ID].current_instruction.rd == core->core_pipeline[MEM_WB].current_instruction.rt
			&& core->core_pipeline[MEM_WB].valid && core->core_pipeline[IF_ID].valid && !core->core_pipeline[MEM_WB].current_instruction.stalled
			&& !core->core_pipeline[IF_ID].halt)
		{
			core->hazard = true;
		}

		if (core->core_pipeline[IF_ID].current_instruction.rd == core->core_pipeline[MEM_WB].current_instruction.rs
			&& core->core_pipeline[MEM_WB].valid && core->core_pipeline[IF_ID].valid && !core->core_pipeline[MEM_WB].current_instruction.stalled
			&& !core->core_pipeline[IF_ID].halt)
		{
			core->hazard = true;
		}
	}

	// USE-STORE HAZARDS
	if (core->core_pipeline[IF_ID].current_instruction.opcode == sw)
	{
		if (core->core_pipeline[IF_ID].current_instruction.rd == core->core_pipeline[ID_EX].current_instruction.rd
			&& core->core_pipeline[ID_EX].valid && core->core_pipeline[IF_ID].valid && !core->core_pipeline[ID_EX].current_instruction.stalled
			&& !core->core_pipeline[IF_ID].halt)
		{
			core->hazard = true;
		}

		if (core->core_pipeline[IF_ID].current_instruction.rd == core->core_pipeline[EX_MEM].current_instruction.rd
			&& core->core_pipeline[EX_MEM].valid && core->core_pipeline[IF_ID].valid && !core->core_pipeline[EX_MEM].current_instruction.stalled
			&& !core->core_pipeline[IF_ID].halt)
		{
			core->hazard = true;
		}

		if (core->core_pipeline[IF_ID].current_instruction.rd == core->core_pipeline[MEM_WB].current_instruction.rd
			&& core->core_pipeline[MEM_WB].valid && core->core_pipeline[IF_ID].valid && !core->core_pipeline[MEM_WB].current_instruction.stalled
			&& !core->core_pipeline[IF_ID].halt)
		{
			core->hazard = true;
		}
	}
}

// IF stage in the pipeline
void fetch(core* core)
{
	core->fetch_old_PC = core->next_PC;		// save fetch stage old PC before it's altered, for trace purposes
	core->core_pipeline[IF_ID].new_instruction = core->core_imem[core->next_PC];
	core->next_PC += 1;		// take old PC, given there is no branch taken at this cycle. If branch is taken, next_PC is updated in decode stage
}

// Perform branch resolution in the pipeline
void check_branch(core* core)
{
	instruction current_instruction = core->core_pipeline[IF_ID].current_instruction;
	int opcode = current_instruction.opcode;

	if ((opcode < beq) || (opcode > bge))
	{
		return;
	}

	//int rs_value = core->core_registers[current_instruction.rs];
	int rs_value = (current_instruction.rs == 0x1) ? current_instruction.immediate : core->core_registers[current_instruction.rs];	// if rs = 1 then take immediate value
	int rt_value = (current_instruction.rt == 0x1) ? current_instruction.immediate : core->core_registers[current_instruction.rt];	// if rt = 1 then take immediate value
	int rd_value = (current_instruction.rd == 0x1) ? current_instruction.immediate : core->core_registers[current_instruction.rd];	// if rd = 1 then take immediate value
	int next_branch_PC = rd_value & 0x3FF; // R[rd][9:0]

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

// ID stage in the pipeline
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
		core->next_PC = core->fetch_old_PC;		// not sure if need to add old PC if after branch
		core->core_pipeline[IF_ID].new_instruction = core->core_pipeline[IF_ID].current_instruction;
		core->core_pipeline[ID_EX].new_instruction = core->core_pipeline[ID_EX].current_instruction;
		core->core_pipeline[ID_EX].new_instruction.stalled = true;

		//instruction current_instruction = core->core_pipeline[EX_MEM].current_instruction;
		//bool stalled = current_instruction.stalled;
		//if (!core->mem_stall)
		core->stat.decode_stall++;
	}
	else
	{
		check_branch(core);
		core->core_pipeline[ID_EX].new_instruction = core->core_pipeline[IF_ID].current_instruction;
	}
}

// EX stage in the pipeline
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
	//core->stat.instructions++;
	int current_opcode = current_instruction.opcode;
	int ALU_output = 0;


	//int rs_value = core->core_registers[current_instruction.rs];
	int rt_value = (current_instruction.rt == 0x1) ? current_instruction.immediate : core->core_registers[current_instruction.rt];	// if rt = 1 then take immediate value
	int rs_value = (current_instruction.rs == 0x1) ? current_instruction.immediate : core->core_registers[current_instruction.rs];	// if rs = 1 then take immediate value

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

// MEM stage in the pipeline
void memory(core* core, int *main_mem, int core_num)
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
		//core->stat.mem_stall++;
		return;
	}

	// memory treatment
	if (current_instruction.opcode == lw)
	{
		int read_res = read_mem(core, main_mem, core_num);
		if (read_res == MISS_CODE)
		{
			core->core_pipeline[MEM_WB].new_instruction.stalled = true;
			core->mem_stall = true;
			//core->stat.mem_stall++;
			core->next_PC = core->fetch_old_PC;
			return;
		}
		else 
		{
			core->core_pipeline[MEM_WB].new_mem_output = read_res; // propogate read result to next stage if access was hit
		}
	}
	else if (current_instruction.opcode == sw)
	{
		int write_result = write_mem(core, core_num);
		if (write_result == MISS_CODE)
		{
			// stall core
			core->core_pipeline[MEM_WB].new_instruction.stalled = true;
			core->mem_stall = true;
			//core->stat.mem_stall++;
			core->next_PC = core->fetch_old_PC;
			return;
		}
		else
		{
			//int reg = current_instruction.rd;
			int rd_val = (current_instruction.rd == 0x1) ? current_instruction.immediate : core->core_registers[current_instruction.rd];	// if rd = 1 then take immediate value
			if (current_instruction.rd == R0)
			{
				return;
			}
			int write_address = core->core_pipeline[EX_MEM].current_ALU_output;
			int index = write_address & 0xFF;

			core->core_cache.dsram[index] = rd_val; // MEM[R[rs]+R[rt]] = R[rd]
		}
	}

	core->core_pipeline[MEM_WB].new_instruction = core->core_pipeline[EX_MEM].current_instruction;
	core->core_pipeline[MEM_WB].new_ALU_output = core->core_pipeline[EX_MEM].current_ALU_output;
}

// WB stage in the pipeline
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
		core->stat.instructions++;
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

	//if (current_instruction.opcode == lw)	// reg-memory or reg-reg operation 
	if ((current_instruction.opcode >= add) && (current_instruction.opcode <= srl))
	{
		core->core_registers[reg] = core->core_pipeline[MEM_WB].current_ALU_output;	// reg-reg operation
		return;
	}
	else if (current_instruction.opcode == lw)
	{
		core->core_registers[reg] = core->core_pipeline[MEM_WB].current_mem_output; // mem-reg operation
	}
	else if (current_instruction.opcode == jal)
	{
		core->core_registers[R15] = core->core_pipeline[MEM_WB].new_instruction.PC;
		int rd_val = (current_instruction.rd == 0x1) ? current_instruction.immediate : core->core_registers[current_instruction.rd];
		int next_PC = rd_val & 0x3FF; // R[rd][9:0]
	}

	return;
}

// Moving values to the next stage on pipeline
void update_stage_buffers(core* core)
{
	if (core->mem_stall)	//special case for memory stall, WB stage can continue
	{
		core->core_pipeline[MEM_WB].current_instruction = core->core_pipeline[MEM_WB].new_instruction;
		//core->stat.mem_stall++;
		core->stat.cycles++;
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
	core->stat.cycles++;
}

// Copies the core registers
void copy_regs(core* core)
{
	for (int i = 0; i < NUM_OF_REGS; i++)
	{
		core->current_core_registers[i] = core->core_registers[i];
	}
}

// Activates pipeline for specific core
void simulate_clock_cycle(core* core, FILE* trace_file, int *main_mem, int core_num)
{
	if (core->core_halt)
		return;
	
	if (core->mem_stall)
	{
		write_coretrace(core, trace_file);
		core->clock_cycle_count++;
		core->stat.mem_stall++;
		return; // in this case, stall core completely, only write its trace
	}
	copy_regs(core); // snapshot core regs at the beginning of the clock cycle
	fetch(core);
	decode(core);
	execute(core);
	memory(core, main_mem, core_num);
	write_back(core);
	write_coretrace(core, trace_file);
	update_stage_buffers(core);
	core->clock_cycle_count++;
	core->hazard = false;
}

// Checks if all cores are halted
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