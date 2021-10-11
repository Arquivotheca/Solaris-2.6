#ifndef lint
#pragma ident "@(#)soft_swi_dump.c 1.1 95/10/20 SMI"
#endif
/*
 * Copyright (c) 1995, by Sun Microsystems, Inc.
 * All rights reserved.
 */

#include "spmisoft_api.h"
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
