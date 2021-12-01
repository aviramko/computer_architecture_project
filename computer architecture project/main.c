////////////////////////////////////////////////////////////////
//////////		Computer Architecture Project		////////////
////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core.h"

int main(int argc, char *argv[])
{
	core core0;// *core1 = NULL, *core2 = NULL, *core3 = NULL;
	
	initialize_core(&core0, argv[1]);


	return 0;
}