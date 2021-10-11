#ifndef lint
#pragma ident "@(#) swi_mount.c 1.1 95/02/13"
#endif
/*
 * Copyright (c) 1995, by Sun Microsystems, Inc.
 * All rights reserved.
 */

#include "sw_api.h"
#include "sw_swi.h"

int
mount_fs(char * special, char * mountp, char * fstype)
{
	int i;

	enter_swlib("mount_fs");
	i = swi_mount_fs(special, mountp, fstype);
	exit_swlib();
	return (i);
}

int
umount_fs(char * mountp)
{
	int i;

	enter_swlib("umount_fs");
	i = swi_umount_fs(mountp);
	exit_swlib();
	return (i);
}

int
share_fs(char * fsname)
{
	int i;

	enter_swlib("share_fs");
	i = swi_share_fs(fsname);
	exit_swlib();
	return (i);
}

int
unshare_fs(char * fsname)
{
	int i;

	enter_swlib("unshare_fs");
	i = swi_unshare_fs(fsname);
	exit_swlib();
	return (i);
}
