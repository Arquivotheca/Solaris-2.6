#ifndef lint
#pragma ident "@(#) swi_dump.c 1.1 95/02/13"
#endif
/*
 * Copyright (c) 1995, by Sun Microsystems, Inc.
 * All rights reserved.
 */

#include "sw_api.h"
#include "sw_swi.h"

int
dumptree(char * filename)
{
	int i;

	enter_swlib("dumptree");
	i = swi_dumptree(filename);
	exit_swlib();
	return (i);
}
