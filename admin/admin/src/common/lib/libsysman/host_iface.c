/*
 * Copyright (c) 1994 by Sun Microsystems, Inc.
 * All rights reserved.
 */

#pragma	ident	"@(#)host_iface.c	1.9	96/09/09 SMI"


#include <stddef.h>
#include "sysman_iface.h"
#include "sysman_impl.h"


int
sysman_add_host(SysmanHostArg *ha_p, char *buf, int bufsiz)
{

	int	status;


	if (ha_p == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	open_sysman_cat();
	init_err_msg(buf, bufsiz, catgets(_catlibsysman,7,11, "add host failed"));

	status = call_function_as_admin(_root_add_host, ha_p,
	    sizeof (SysmanHostArg), buf, bufsiz);

	return (status);
}


int
sysman_delete_host(SysmanHostArg *ha_p, char *buf, int bufsiz)
{

	int	status;


	if (ha_p == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	open_sysman_cat();
	init_err_msg(buf, bufsiz, catgets(_catlibsysman,7,12, "delete host failed"));

	status = call_function_as_admin(_root_delete_host, ha_p,
	    sizeof (SysmanHostArg), buf, bufsiz);

	return (status);
}


int
sysman_modify_host(SysmanHostArg *ha_p, char *buf, int bufsiz)
{

	int	status;


	if (ha_p == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	open_sysman_cat();
	init_err_msg(buf, bufsiz, catgets(_catlibsysman,7,13, "modify host failed"));

	status = call_function_as_admin(_root_modify_host, ha_p,
	    sizeof (SysmanHostArg), buf, bufsiz);

	return (status);
}


int
sysman_get_host(SysmanHostArg *ha_p, char *buf, int bufsiz)
{

	int	status;


	if (ha_p == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	open_sysman_cat();
	init_err_msg(buf, bufsiz, catgets(_catlibsysman,7,14, "get host failed"));

	status = _get_host(ha_p, buf, bufsiz);

	return (status);
}


void
sysman_free_host(SysmanHostArg *ha_p)
{
	_free_host(ha_p);
}


int
sysman_list_host(SysmanHostArg **ha_pp, char *buf, int bufsiz)
{

	int	status;


	if (ha_pp == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	open_sysman_cat();
	init_err_msg(buf, bufsiz, catgets(_catlibsysman,7,15, "list host failed"));

	status = _list_host(ha_pp, buf, bufsiz);

	return (status);
}


void
sysman_free_host_list(SysmanHostArg *ha_p, int cnt)
{
	_free_host_list(ha_p, cnt);
}
