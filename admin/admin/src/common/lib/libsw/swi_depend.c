#ifndef lint
#pragma ident "@(#) swi_depend.c 1.1 95/02/13"
#endif
/*
 * Copyright (c) 1995, by Sun Microsystems, Inc.
 * All rights reserved.
 */

#include "sw_api.h"
#include "sw_swi.h"

Depend *
get_depend_pkgs(void)
{
	Depend	*d;

	enter_swlib("get_depend_pkgs");
	d = swi_get_depend_pkgs();
	exit_swlib();
	return (d);
}

int
check_sw_depends(void)
{
	int	i;

	enter_swlib("check_sw_depends");
	i = swi_check_sw_depends();
	exit_swlib();
	return (i);
}
