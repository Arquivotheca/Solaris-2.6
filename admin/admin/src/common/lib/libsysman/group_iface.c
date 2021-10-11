/*
 * Copyright (c) 1994 by Sun Microsystems, Inc.
 * All rights reserved.
 */

#pragma	ident	"@(#)group_iface.c	1.11	96/09/09 SMI"


#include <stddef.h>
#include <sys/types.h>
#include "sysman_iface.h"
#include "sysman_impl.h"


int
sysman_add_group(SysmanGroupArg *ga_p, char *buf, int bufsiz)
{

	int	status;


	if (ga_p == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	open_sysman_cat();
	init_err_msg(buf, bufsiz, catgets(_catlibsysman,7,6, "add group failed"));

	status = call_function_as_admin(_root_add_group, ga_p,
	    sizeof (SysmanGroupArg), buf, bufsiz);

	return (status);
}


int
sysman_delete_group(SysmanGroupArg *ga_p, char *buf, int bufsiz)
{

	int	status;


	if (ga_p == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	open_sysman_cat();
	init_err_msg(buf, bufsiz, catgets(_catlibsysman,7,7, "delete group failed"));

	status = call_function_as_admin(_root_delete_group, ga_p,
	    sizeof (SysmanGroupArg), buf, bufsiz);

	return (status);
}


int
sysman_modify_group(SysmanGroupArg *ga_p, char *buf, int bufsiz)
{

	int	status;


	if (ga_p == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	open_sysman_cat();
	init_err_msg(buf, bufsiz, catgets(_catlibsysman,7,8, "modify group failed"));

	status = call_function_as_admin(_root_modify_group, ga_p,
	    sizeof (SysmanGroupArg), buf, bufsiz);

	return (status);
}


int
sysman_get_group(SysmanGroupArg *ga_p, char *buf, int bufsiz)
{

	int	status;


	if (ga_p == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	open_sysman_cat();
	init_err_msg(buf, bufsiz, catgets(_catlibsysman,7,9, "get group failed"));

	status = _get_group(ga_p, buf, bufsiz);

	return (status);
}


void
sysman_free_group(SysmanGroupArg *ga_p)
{
	_free_group(ga_p);
}


int
sysman_list_group(SysmanGroupArg **ga_pp, char *buf, int bufsiz)
{

	int	status;


	if (ga_pp == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	open_sysman_cat();
	init_err_msg(buf, bufsiz, catgets(_catlibsysman,7,10, "list group failed"));

	status = _list_group(ga_pp, buf, bufsiz);

	return (status);
}


void
sysman_free_group_list(SysmanGroupArg *ga_p, int cnt)
{
	_free_group_list(ga_p, cnt);
}


gid_t
sysman_get_next_avail_gid(void)
{
	static gid_t	min_gid = 101;

	return (_get_next_avail_gid(min_gid));
}
