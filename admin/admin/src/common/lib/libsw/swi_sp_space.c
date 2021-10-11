#ifndef lint
#pragma ident "@(#) swi_sp_space.c 1.1 95/02/13"
#endif
/*
 * Copyright (c) 1995, by Sun Microsystems, Inc.
 * All rights reserved.
 */

#include "sw_api.h"
#include "sw_swi.h"

Space **
calc_cluster_space(Module *mod, ModStatus status, int flags)
{
	Space **spp;

	enter_swlib("calc_cluster_space");
	spp = swi_calc_cluster_space(mod, status, flags);
	exit_swlib();
	return (spp);
}

Space **
calc_tot_space(Product * prod)
{
	Space **spp;

	enter_swlib("calc_tot_space");
	spp = swi_calc_tot_space(prod);
	exit_swlib();
	return (spp);
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

Space **
space_meter(char **mplist)
{
	Space **spp;

	enter_swlib("space_meter");
	spp = swi_space_meter(mplist);
	exit_swlib();
	return (spp);
}

Space **
swm_space_meter(char **mplist)
{
	Space **spp;

	enter_swlib("swm_space_meter");
	spp = swi_swm_space_meter(mplist);
	exit_swlib();
	return (spp);
}

void
free_space_tab(Space **sp)
{
	enter_swlib("free_space_tab");
	swi_free_space_tab(sp);
	exit_swlib();
	return;
}

Space **
upg_space_meter(void)
{
	Space **spp;

	enter_swlib("upg_space_meter");
	spp = swi_upg_space_meter();
	exit_swlib();
	return (spp);
}
