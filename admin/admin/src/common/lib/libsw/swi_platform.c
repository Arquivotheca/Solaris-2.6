#ifndef lint
#pragma ident "@(#) swi_platform.c 1.1 95/02/13"
#endif
/*
 * Copyright (c) 1995, by Sun Microsystems, Inc.
 * All rights reserved.
 */

#include "sw_api.h"
#include "sw_swi.h"

int
write_platform_file(char *rootdir, Module *prod)
{
	int i;

	enter_swlib("write_platform_file");
	i = swi_write_platform_file(rootdir, prod);
	exit_swlib();
	return (i);
}
