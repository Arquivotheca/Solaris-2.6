#ifndef lint
#pragma ident "@(#) swi_mountall.c 1.1 95/02/13"
#endif
/*
 * Copyright (c) 1995, by Sun Microsystems, Inc.
 * All rights reserved.
 */

#include "sw_api.h"
#include "sw_swi.h"

int
mount_and_add_swap(char *diskname)
{
	int i;

	enter_swlib("mount_and_add_swap");
	i = swi_mount_and_add_swap(diskname);
	exit_swlib();
	return (i);
}

int
umount_and_delete_swap(void)
{
	int i;

	enter_swlib("umount_and_delete_swap");
	i = swi_umount_and_delete_swap();
	exit_swlib();
	return (i);
}

int
umount_all(void)
{
	int i;

	enter_swlib("umount_all");
	i = swi_umount_all();
	exit_swlib();
	return (i);
}

int
unswap_all(void)
{
	int i;

	enter_swlib("unswap_all");
	i = swi_unswap_all();
	exit_swlib();
	return (i);
}
