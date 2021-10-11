#ifndef lint
#pragma ident "@(#) swi_sp_util.c 1.1 95/02/13"
#endif
/*
 * Copyright (c) 1995, by Sun Microsystems, Inc.
 * All rights reserved.
 */

#include "sw_api.h"
#include "sw_swi.h"

int
valid_mountp(char * mountp)
{
	int i;

	enter_swlib("valid_mountp");
	i = swi_valid_mountp(mountp);
	exit_swlib();
	return (i);
}
