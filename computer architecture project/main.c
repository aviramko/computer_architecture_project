////////////////////////////////////////////////////////////////
//////////		Computer Architecture Project		////////////
////////////////////////////////////////////////////////////////

#define _CRT_SECURE_NO_DEPRECATE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core.h"

int main(int argc, char *argv[])
{
	core core0;// *core1 = NULL, *core2 = NULL, *core3 = NULL;
	
	initialize_core(&core0, argv[1]);
	FILE *fp_core0trace = fopen(argv[11], "w");
	simulate_core(&core0, fp_core0trace);

	return 0;
}