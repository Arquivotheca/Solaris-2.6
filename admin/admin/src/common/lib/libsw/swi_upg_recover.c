#ifndef lint
#pragma ident "@(#) swi_upg_recover.c 1.1 95/02/13"
#endif
/*
 * Copyright (c) 1995, by Sun Microsystems, Inc.
 * All rights reserved.
 */

#include "sw_api.h"
#include "sw_swi.h"

int
partial_upgrade(void)
{
	int i;

	enter_swlib("partial_upgrade");
	i = swi_partial_upgrade();
	exit_swlib();
	return (i);
}

int
resume_upgrade(void)
{
	int i;

	enter_swlib("resume_upgrade");
	i = swi_resume_upgrade();
	exit_swlib();
	return (i);
}
