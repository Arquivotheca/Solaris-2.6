#ifndef lint
#pragma ident "@(#) swi_media.c 1.1 95/02/13"
#endif
/*
 * Copyright (c) 1995, by Sun Microsystems, Inc.
 * All rights reserved.
 */

#include "sw_api.h"
#include "sw_swi.h"

Module *
add_media(char * dir)
{
	Module *m;

	enter_swlib("add_media");
	m = swi_add_media(dir);
	exit_swlib();
	return (m);
}

Module *
add_specific_media(char * dir, char * dev)
{
	Module *m;

	enter_swlib("add_specific_media");
	m = swi_add_specific_media(dir, dev);
	exit_swlib();
	return (m);
}

int
load_media(Module * mod, int use_packagetoc)
{
	int i;

	enter_swlib("load_media");
	i = swi_load_media(mod, use_packagetoc);
	exit_swlib();
	return (i);
}

int
mount_media(Module * mod, char * mount_pt, MediaType type)
{
	int i;

	enter_swlib("mount_media");
	i = swi_mount_media(mod, mount_pt, type);
	exit_swlib();
	return (i);
}

int
unload_media(Module * mod)
{
	int i;

	enter_swlib("unload_media");
	i = swi_unload_media(mod);
	exit_swlib();
	return (i);
}

void
set_eject_on_exit(int value)
{
	enter_swlib("set_eject_on_exit");
	swi_set_eject_on_exit(value);
	exit_swlib();
	return;
}

Module *
get_media_head(void)
{
	Module *m;

	enter_swlib("get_media_head");
	m = swi_get_media_head();
	exit_swlib();
	return (m);
}

Module *
find_media(char * dir, char * dev)
{
	Module *m;

	enter_swlib("find_media");
	m = swi_find_media(dir, dev);
	exit_swlib();
	return (m);
}
