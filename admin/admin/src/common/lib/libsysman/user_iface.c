/*
 * Copyright (c) 1994, by Sun Microsystems, Inc.
 * All rights reserved.
 */

#pragma	ident	"@(#)user_iface.c	1.15	96/09/09 SMI"


#include <stddef.h>
#include <string.h>
#include <sys/types.h>
#include "sysman_iface.h"
#include "sysman_impl.h"


int
sysman_add_user(SysmanUserArg *ua_p, char *buf, int bufsiz)
{

	int	status;


	if (ua_p == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	open_sysman_cat();
	init_err_msg(buf, bufsiz, catgets(_catlibsysman,7,42, "add user failed"));

	status = call_function_as_admin(_root_add_user, ua_p,
	    sizeof (SysmanUserArg), buf, bufsiz);

	return (status);
}


int
sysman_delete_user(SysmanUserArg *ua_p, char *buf, int bufsiz)
{

	int	status;


	if (ua_p == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	open_sysman_cat();
	init_err_msg(buf, bufsiz, catgets(_catlibsysman,7,43, "delete user failed"));

	status = call_function_as_admin(_root_delete_user, ua_p,
	    sizeof (SysmanUserArg), buf, bufsiz);

	return (status);
}


int
sysman_modify_user(SysmanUserArg *ua_p, char *buf, int bufsiz)
{

	int	status;


	if (ua_p == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	open_sysman_cat();
	init_err_msg(buf, bufsiz, catgets(_catlibsysman,7,44, "modify user failed"));

	status = call_function_as_admin(_root_modify_user, ua_p,
	    sizeof (SysmanUserArg), buf, bufsiz);

	return (status);
}


int
sysman_get_user(SysmanUserArg *ua_p, char *buf, int bufsiz)
{

	SysmanSharedUserArg	sua;
	int			status;


	if (ua_p == NULL || ua_p->username_key == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	open_sysman_cat();
	init_err_msg(buf, bufsiz, catgets(_catlibsysman,7,45, "get user failed"));

	strcpy(sua.username_key, ua_p->username_key);

	if (ua_p->get_shadow_flag == B_FALSE) {
		status = _get_user(&sua, buf, bufsiz);
	} else {
		status = call_function_as_admin(_root_get_user, (void *)&sua,
		    sizeof (SysmanSharedUserArg), buf, bufsiz);
	}

	if (status == SYSMAN_SUCCESS) {
		ua_p->username = sua.username[0] ? strdup(sua.username) : NULL;
		ua_p->passwd = sua.passwd[0] ? strdup(sua.passwd) : NULL;
		ua_p->uid = sua.uid[0] ? strdup(sua.uid) : NULL;
		ua_p->group = sua.group[0] ? strdup(sua.group) : NULL;
		ua_p->second_grps =
		    sua.second_grps[0] ? strdup(sua.second_grps) : NULL;
		ua_p->comment = sua.comment[0] ? strdup(sua.comment) : NULL;
		ua_p->path = sua.path[0] ? strdup(sua.path) : NULL;
		ua_p->shell = sua.shell[0] ? strdup(sua.shell) : NULL;
		ua_p->lastchanged =
		    sua.lastchanged[0] ? strdup(sua.lastchanged) : NULL;
		ua_p->minimum = sua.minimum[0] ? strdup(sua.minimum) : NULL;
		ua_p->maximum = sua.maximum[0] ? strdup(sua.maximum) : NULL;
		ua_p->warn = sua.warn[0] ? strdup(sua.warn) : NULL;
		ua_p->inactive = sua.inactive[0] ? strdup(sua.inactive) : NULL;
		ua_p->expire = sua.expire[0] ? strdup(sua.expire) : NULL;
		ua_p->flag = sua.flag[0] ? strdup(sua.flag) : NULL;
	}

	return (status);
}


void
sysman_free_user(SysmanUserArg *ua_p)
{
	_free_user(ua_p);
}


int
sysman_list_user(SysmanUserArg **ua_pp, char *buf, int bufsiz)
{

	int	status;


	if (ua_pp == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	open_sysman_cat();
	init_err_msg(buf, bufsiz, catgets(_catlibsysman,7,46, "list user failed"));

	status = _list_user(ua_pp, buf, bufsiz);

	return (status);
}


void
sysman_free_user_list(SysmanUserArg *ua_p, int cnt)
{
	_free_user_list(ua_p, cnt);
}


uid_t
sysman_get_next_avail_uid(void)
{
	static uid_t	min_uid = 1001;

	return (_get_next_avail_uid(min_uid));
}
