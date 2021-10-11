#ifndef lint
#pragma ident "@(#)soft_swi_admin.c 1.1 95/10/20 SMI"
#endif
/*
 * Copyright (c) 1995, by Sun Microsystems, Inc.
 * All rights reserved.
 */

#include "spmisoft_api.h"
#include "sw_swi.h"

char *
getset_admin_file(char * filename)
{
	char	*cp;

	enter_swlib("getset_admin_file");
	cp = swi_getset_admin_file(filename);
	exit_swlib();
	return (cp);
}

int
admin_write(char * filename, Admin_file * admin)
{
	int	ret;

	enter_swlib("admin_write");
	ret = swi_admin_write(filename, admin);
	exit_swlib();
	return (ret);
}
