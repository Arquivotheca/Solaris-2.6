#ifndef lint
#pragma ident "@(#)soft_swi_v_version.c 1.1 95/10/20 SMI"
#endif
/*
 * Copyright (c) 1995, by Sun Microsystems, Inc.
 * All rights reserved.
 */

#include "spmisoft_api.h"
#include "sw_swi.h"

int
prod_vcmp(char * v1, char * v2)
{
	int i;

	enter_swlib("prod_vcmp");
	i = swi_prod_vcmp(v1, v2);
	exit_swlib();
	return (i);
}

int
pkg_vcmp(char * v1, char * v2)
{
	int i;

	enter_swlib("pkg_vcmp");
	i = swi_pkg_vcmp(v1, v2);
	exit_swlib();
	return (i);
}

int
is_patch(Modinfo * mi)
{
	int i;

	enter_swlib("is_patch");
	i = swi_is_patch(mi);
	exit_swlib();
	return (i);
}

int
is_patch_of(Modinfo *mi1, Modinfo *mi2)
{
	int i;

	enter_swlib("is_patch_of");
	i = swi_is_patch_of(mi1, mi2);
	exit_swlib();
	return (i);
}
