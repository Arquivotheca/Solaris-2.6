#ifndef lint
#pragma ident "@(#) swi_admin.c 1.1 95/02/13"
#endif
/*
 * Copyright (c) 1995, by Sun Microsystems, Inc.
 * All rights reserved.
 */

#include "sw_api.h"
#include "sw_swi.h"

char *
admin_file(char * filename)
{
	char	*cp;

	enter_swlib("admin_file");
	cp = swi_admin_file(filename);
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
