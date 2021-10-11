/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 * All rights reserved.
 */

#pragma	ident	"@(#)alias_iface.c	1.2	96/09/09 SMI"


#include <stddef.h>
#include <sys/types.h>
#include "sysman_iface.h"
#include "sysman_impl.h"


int
sysman_add_alias(SysmanAliasArg *aa_p, char *buf, int bufsiz)
{

	int	status;


	if (aa_p == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	open_sysman_cat();
	init_err_msg(buf, bufsiz, catgets(_catlibsysman,7,1, "add alias failed"));

	status = call_function_as_admin(_root_add_alias, aa_p,
	    sizeof (SysmanAliasArg), buf, bufsiz);

	return (status);
}


int
sysman_delete_alias(SysmanAliasArg *aa_p, char *buf, int bufsiz)
{

	int	status;


	if (aa_p == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	open_sysman_cat();
	init_err_msg(buf, bufsiz, catgets(_catlibsysman,7,2, "delete alias failed"));

	status = call_function_as_admin(_root_delete_alias, aa_p,
	    sizeof (SysmanAliasArg), buf, bufsiz);

	return (status);
}


int
sysman_modify_alias(SysmanAliasArg *aa_p, char *buf, int bufsiz)
{

	int	status;


	if (aa_p == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	open_sysman_cat();
	init_err_msg(buf, bufsiz, catgets(_catlibsysman,7,3, "modify alias failed"));

	status = call_function_as_admin(_root_modify_alias, aa_p,
	    sizeof (SysmanAliasArg), buf, bufsiz);

	return (status);
}


int
sysman_get_alias(SysmanAliasArg *aa_p, char *buf, int bufsiz)
{

	int	status;


	if (aa_p == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	open_sysman_cat();
	init_err_msg(buf, bufsiz, catgets(_catlibsysman,7,4, "get alias failed"));

	status = _get_alias(aa_p, buf, bufsiz);

	return (status);
}


void
sysman_free_alias(SysmanAliasArg *aa_p)
{
	_free_alias(aa_p);
}


int
sysman_list_alias(SysmanAliasArg **aa_pp, char *buf, int bufsiz)
{

	int	status;


	if (aa_pp == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	open_sysman_cat();
	init_err_msg(buf, bufsiz, catgets(_catlibsysman,7,5, "list alias failed"));

	status = _list_alias(aa_pp, buf, bufsiz);

	return (status);
}


void
sysman_free_alias_list(SysmanAliasArg *aa_p, int cnt)
{
	_free_alias_list(aa_p, cnt);
}
