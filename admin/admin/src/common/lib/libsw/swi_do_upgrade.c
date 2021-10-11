#ifndef lint
#pragma ident "@(#) swi_do_upgrade.c 1.3 95/05/31"
#endif
/*
 * Copyright (c) 1995, by Sun Microsystems, Inc.
 * All rights reserved.
 */

#include "sw_api.h"
#include "sw_swi.h"

void
set_debug(char *dumpfile)
{
	enter_swlib("set_debug");
	swi_set_debug(dumpfile);
	exit_swlib();
	return;
}

void
set_skip_mod_search(void)
{
	enter_swlib("set_skip_mod_search");
	swi_set_skip_mod_search();
	exit_swlib();
	return;
}

void
set_pkg_hist_file(char *path)
{
	enter_swlib("set_pkg_hist_file");
	swi_set_pkg_hist_file(path);
	exit_swlib();
	return;
}

void
set_onlineupgrade_mode(void)
{
	enter_swlib("set_onlineupgrade_mode");
	swi_set_onlineupgrade_mode();
	exit_swlib();
	return;
}

int
upgrade_all_envs(void)
{
	int i;

	enter_swlib("upgrade_all_envs");
	i = swi_upgrade_all_envs();
	exit_swlib();
	return (i);
}

int
nonnative_upgrade(StringList *sl)
{
	int i;

	enter_swlib("nonnative_upgrade");
	i = swi_nonnative_upgrade(sl);
	exit_swlib();
	return (i);
}

int
local_upgrade(void)
{
	int i;

	enter_swlib("local_upgrade");
	i = swi_local_upgrade();
	exit_swlib();
	return(i);
}

int
do_upgrade(void)
{
	int i;

	enter_swlib("do_upgrade");
	i = swi_do_upgrade();
	exit_swlib();
	return (i);
}

int
do_find_modified(void)
{
	int i;

	enter_swlib("do_find_modified");
	i = swi_do_find_modified();
	exit_swlib();
	return (i);
}

int
do_final_space_check(void)
{
	int i;

	enter_swlib("do_final_space_check");
	i = swi_do_final_space_check();
	exit_swlib();
	return (i);
}

void
do_write_upgrade_script(void)
{
	enter_swlib("do_write_upgrade_script");
	swi_do_write_upgrade_script();
	exit_swlib();
	return;
}

int
do_product_upgrade(Module *prodmod)
{
	int i;

	enter_swlib("do_product_upgrade");
	i = swi_do_product_upgrade(prodmod);
	exit_swlib();
	return (i);
}
