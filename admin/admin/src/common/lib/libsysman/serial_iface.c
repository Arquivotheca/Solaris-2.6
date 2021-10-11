/*
 * Copyright (c) 1994 by Sun Microsystems, Inc.
 * All rights reserved.
 */

#pragma	ident	"@(#)serial_iface.c	1.10	96/09/09 SMI"


#include <stddef.h>
#include "sysman_iface.h"
#include "sysman_impl.h"


int
sysman_modify_serial(SysmanSerialArg *sa_p, char *buf, int bufsiz)
{

	int	status;


	if (sa_p == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	open_sysman_cat();
	init_err_msg(buf, bufsiz, catgets(_catlibsysman,7,31, "modify serial port failed"));

	status = call_function_as_admin(_root_modify_serial, sa_p,
	    sizeof (SysmanSerialArg), buf, bufsiz);

	return (status);
}


int
sysman_enable_serial(SysmanSerialArg *sa_p, char *buf, int bufsiz)
{

	int	status;


	if (sa_p == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	open_sysman_cat();
	init_err_msg(buf, bufsiz, catgets(_catlibsysman,7,32, "enable serial port failed"));

	status = call_function_as_admin(_root_enable_serial, sa_p,
	    sizeof (SysmanSerialArg), buf, bufsiz);

	return (status);
}


int
sysman_disable_serial(SysmanSerialArg *sa_p, char *buf, int bufsiz)
{

	int	status;


	if (sa_p == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	open_sysman_cat();
	init_err_msg(buf, bufsiz, catgets(_catlibsysman,7,33, "disable serial port failed"));

	status = call_function_as_admin(_root_disable_serial, sa_p,
	    sizeof (SysmanSerialArg), buf, bufsiz);

	return (status);
}


int
sysman_delete_serial(SysmanSerialArg *sa_p, char *buf, int bufsiz)
{

	int	status;


	if (sa_p == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	open_sysman_cat();
	init_err_msg(buf, bufsiz, catgets(_catlibsysman,7,34, "delete serial port failed"));

	status = call_function_as_admin(_root_delete_serial, sa_p,
	    sizeof (SysmanSerialArg), buf, bufsiz);

	return (status);
}


int
sysman_get_serial(SysmanSerialArg *sa_p, char *buf, int bufsiz)
{

	int	status;


	if (sa_p == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	open_sysman_cat();
	init_err_msg(buf, bufsiz, catgets(_catlibsysman,7,35, "get serial port failed"));

	status = _get_serial(sa_p, NULL, buf, bufsiz);

	return (status);
}


int
sysman_get_alt_serial(
	SysmanSerialArg	*sa_p,
	const char	*alt_dev_dir,
	char		*buf,
	int		bufsiz)
{

	int	status;


	if (sa_p == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	open_sysman_cat();
	init_err_msg(buf, bufsiz, catgets(_catlibsysman,7,36, "get serial port failed"));

	status = _get_serial(sa_p, alt_dev_dir, buf, bufsiz);

	return (status);
}


void
sysman_free_serial(SysmanSerialArg *sa_p)
{
	_free_serial(sa_p);
}


int
sysman_list_serial(SysmanSerialArg **sa_pp, char *buf, int bufsiz)
{

	int	status;


	if (sa_pp == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	open_sysman_cat();
	init_err_msg(buf, bufsiz, catgets(_catlibsysman,7,37, "list serial port failed"));

	status = _list_serial(sa_pp, NULL, buf, bufsiz);

	return (status);
}


int
sysman_list_alt_serial(
	SysmanSerialArg	**sa_pp,
	const char	*alt_dev_dir,
	char		*buf,
	int		bufsiz)
{

	int	status;


	if (sa_pp == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	open_sysman_cat();
	init_err_msg(buf, bufsiz, catgets(_catlibsysman,7,38, "list serial port failed"));

	status = _list_serial(sa_pp, alt_dev_dir, buf, bufsiz);

	return (status);
}


void
sysman_free_serial_list(SysmanSerialArg *sa_p, int cnt)
{
	_free_serial_list(sa_p, cnt);
}
