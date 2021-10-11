#ifndef lint
#pragma ident "@(#)soft_swi_util.c 1.4 96/05/15 SMI"
#endif
/*
 * Copyright (c) 1995, by Sun Microsystems, Inc.
 * All rights reserved.
 */

#include "spmisoft_api.h"
#include "sw_swi.h"

void
sw_lib_init(int ptype)
{
	enter_swlib("sw_lib_init");
	swi_sw_lib_init(ptype);
	exit_swlib();
	return;
}

int
set_instdir_svc_svr(Module * prod)
{
	int i;

	enter_swlib("set_instdir_svc_svr");
	i = swi_set_instdir_svc_svr(prod);
	exit_swlib();
	return (i);
}

void
clear_instdir_svc_svr(Module * prod)
{
	enter_swlib("clear_instdir_svc_svr");
	swi_clear_instdir_svc_svr(prod);
	exit_swlib();
	return;
}

char *
gen_pboot_path(char *rootdir)
{
	char *c;

	enter_swlib("gen_pboot_path");
	c = swi_gen_pboot_path(rootdir);
	exit_swlib();
	return (c);
}

char *
gen_bootblk_path(char *rootdir)
{
	char *c;

	enter_swlib("gen_bootblk_path");
	c = swi_gen_bootblk_path(rootdir);
	exit_swlib();
	return (c);
}

char *
gen_openfirmware_path(char *rootdir)
{
	char *c;

	enter_swlib("gen_openfirmware_path");
	c = swi_gen_openfirmware_path(rootdir);
	exit_swlib();
	return (c);
}

