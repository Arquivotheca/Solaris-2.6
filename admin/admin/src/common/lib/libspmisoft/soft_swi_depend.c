#ifndef lint
#pragma ident "@(#)soft_swi_depend.c 1.1 95/10/20 SMI"
#endif
/*
 * Copyright (c) 1995, by Sun Microsystems, Inc.
 * All rights reserved.
 */

#include "spmisoft_api.h"
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
