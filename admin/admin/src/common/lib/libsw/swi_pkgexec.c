#ifndef lint
#pragma ident "@(#) swi_pkgexec.c 1.1 95/02/13"
#endif
/*
 * Copyright (c) 1995, by Sun Microsystems, Inc.
 * All rights reserved.
 */

#include "sw_api.h"
#include "sw_swi.h"

int
add_pkg(char * pkg_dir, PkgFlags * pkg_params, char * prod_dir)
{
	int i;

	enter_swlib("add_pkg");
	i = swi_add_pkg(pkg_dir, pkg_params, prod_dir);
	exit_swlib();
	return (i);
}

int
remove_pkg(char * pkg_dir, PkgFlags * pkg_params)
{
	int i;

	enter_swlib("remove_pkg");
	i = swi_remove_pkg(pkg_dir, pkg_params);
	exit_swlib();
	return (i);
}
