#ifndef lint
#pragma ident "@(#)soft_swi_sp_space.c 1.3 96/06/10 SMI"
#endif
/*
 * Copyright (c) 1995, by Sun Microsystems, Inc.
 * All rights reserved.
 */

#include "spmisoft_api.h"
#include "sw_swi.h"

FSspace **
calc_cluster_space(Module *mod, ModStatus status)
{
	FSspace **fsp;

	enter_swlib("calc_cluster_space");
	fsp = swi_calc_cluster_space(mod, status);
	exit_swlib();
	return (fsp);
}

ulong
calc_tot_space(Product * prod)
{
	ulong l;

	enter_swlib("calc_tot_space");
	l = swi_calc_tot_space(prod);
	exit_swlib();
	return (l);
}

long
tot_pkg_space(Modinfo *m)
{
	long l;

	enter_swlib("tot_pkg_space");
	l = swi_tot_pkg_space(m);
	exit_swlib();
	return (l);
}

void
free_fsspace(FSspace *fsp)
{
	enter_swlib("free_fsspace");
	swi_free_fsspace(fsp);
	exit_swlib();
	return;
}

int
calc_sw_fs_usage(FSspace **fsp, int (*callback)(void *, void *), void *arg)
{
	int i;

	enter_swlib("calc_sw_fs_usage");
	i = swi_calc_sw_fs_usage(fsp, callback, arg);
	exit_swlib();
	return (i);
}
