#ifndef lint
#pragma ident "@(#) swi_client.c 1.1 95/02/13"
#endif
/*
 * Copyright (c) 1995, by Sun Microsystems, Inc.
 * All rights reserved.
 */

#include "sw_api.h"
#include "sw_swi.h"

char *
name2ipaddr(char * hostname)
{
	char	*cp;

	enter_swlib("name2ipaddr");
	cp = swi_name2ipaddr(hostname);
	exit_swlib();
	return (cp);
}

int
test_mount(Remote_FS * rfs, int sec)
{
	int	i;

	enter_swlib("test_mount");
	i = swi_test_mount(rfs, sec);
	exit_swlib();
	return (i);
}

TestMount
get_rfs_test_status(Remote_FS * rfs)
{
	TestMount	tm;

	enter_swlib("get_rfs_test_status");
	tm = swi_get_rfs_test_status(rfs);
	exit_swlib();
	return (tm);
}

int
set_rfs_test_status(Remote_FS * rfs, TestMount status)
{
	int	i;

	enter_swlib("set_rfs_test_status");
	i  = swi_set_rfs_test_status(rfs, status);
	exit_swlib();
	return (i);
}
