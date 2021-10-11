#ifndef lint
#pragma ident "@(#) swi_install.c 1.1 95/02/13"
#endif
/*
 * Copyright (c) 1995, by Sun Microsystems, Inc.
 * All rights reserved.
 */

#include "sw_api.h"
#include "sw_swi.h"

Module *
load_installed(char * rootdir, int service)
{
	Module	*m;

	enter_swlib("load_installed");
	m = swi_load_installed(rootdir, service);
	exit_swlib();
	return (m);
}

Modinfo *
next_patch(Modinfo * mod)
{
	Modinfo	*m;

	enter_swlib("next_patch");
	m = swi_next_patch(mod);
	exit_swlib();
	return(m);
}

Modinfo *
next_inst(Modinfo * mod)
{
	Modinfo	*m;

	enter_swlib("next_inst");
	m = swi_next_inst(mod);
	exit_swlib();
	return (m);
}
