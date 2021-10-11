#ifndef lint
#pragma ident "@(#) swi_sp_calc.c 1.1 95/02/13"
#endif
/*
 * Copyright (c) 1995, by Sun Microsystems, Inc.
 * All rights reserved.
 */

#include "sw_api.h"
#include "sw_swi.h"

u_int
min_req_space(u_int size)
{
	u_int	i;

	enter_swlib("min_req_space");
	i = swi_min_req_space(size);
	exit_swlib();
	return (i);
}
