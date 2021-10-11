#ifndef lint
#pragma ident "@(#)soft_swi_platform.c 1.1 95/10/20 SMI"
#endif
/*
 * Copyright (c) 1995, by Sun Microsystems, Inc.
 * All rights reserved.
 */

#include "spmisoft_api.h"
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
