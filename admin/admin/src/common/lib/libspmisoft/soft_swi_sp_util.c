#ifndef lint
#pragma ident "@(#)soft_swi_sp_util.c 1.1 95/10/20 SMI"
#endif
/*
 * Copyright (c) 1995, by Sun Microsystems, Inc.
 * All rights reserved.
 */

#include "spmisoft_api.h"
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
