#include <stdio.h>
#include <stdlib.h>

#include "core.h"
#include "parser.h"

void initialize_core(core *core, char *imem_filename)
{
	parse_imem_file(core, imem_filename);
}

//void simulate_cycle(core *core)
//{
//	fetch();
//	decode();
//	execute();
//	memory();
//	write_back();
//	update_stage_buffers();
//}