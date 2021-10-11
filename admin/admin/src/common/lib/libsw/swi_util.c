#ifndef lint
#pragma ident "@(#) swi_util.c 1.3 95/04/20"
#endif
/*
 * Copyright (c) 1995, by Sun Microsystems, Inc.
 * All rights reserved.
 */

#include "sw_api.h"
#include "sw_swi.h"

void
sw_lib_init(void (*alloc_proc)(int), int ptype, int disk_space)
{
	enter_swlib("sw_lib_init");
	swi_sw_lib_init(alloc_proc, ptype, disk_space);
	exit_swlib();
	return;
}

char *
get_err_str(int i)
{
	char *c;

	enter_swlib("get_err_str");
	c = swi_get_err_str(i);
	exit_swlib();
	return (c);
}

void
error_and_exit(int i)
{
	enter_swlib("error_and_exit");
	swi_error_and_exit(i);
	exit_swlib();
	return;
}

void *
xcalloc(size_t size)
{
	void *v;

	v = swi_xcalloc(size);
	return (v);
}

void *
xmalloc(size_t size)
{
	void *v;

	v = swi_xmalloc(size);
	return (v);
}

void *
xrealloc(void *ptr, size_t size)
{
	void *v;

	v = swi_xrealloc(ptr, size);
	return (v);
}

char *
xstrdup(char * str)
{
	char *c;

	c = swi_xstrdup(str);
	return (c);
}

void
deselect_usr_pkgs(Module * prod)
{
	enter_swlib("deselect_usr_pkgs");
	swi_deselect_usr_pkgs(prod);
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

void
set_action_for_machine_type(Module * prod)
{
	enter_swlib("set_action_for_machine_type");
	swi_set_action_for_machine_type(prod);
	exit_swlib();
	return;
}

int
percent_free_space(void)
{
	int i;

	enter_swlib("percent_free_space");
	i = swi_percent_free_space();
	exit_swlib();
	return (i);
}

Space **
sort_space_fs(Space ** space, char ** mntlist)
{
	Space **spp;

	enter_swlib("sort_space_fs");
	spp = swi_sort_space_fs(space, mntlist);
	exit_swlib();
	return (spp);
}

int
set_sw_debug(int state)
{
	int i;

	enter_swlib("set_sw_debug");
	i = swi_set_sw_debug(state);
	exit_swlib();
	return (i);
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
gen_pboot_path(char *rootdir)
{
	char *c;

	enter_swlib("gen_pboot_path");
	c = swi_gen_pboot_path(rootdir);
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
