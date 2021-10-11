/*
 * Copyright (c) 1994 by Sun Microsystems, Inc.
 * All rights reserved.
 */

#pragma	ident	"@(#)sw_iface.c	1.10	96/09/09 SMI"


#include <stddef.h>
#include "sysman_iface.h"
#include "sysman_impl.h"


static char* script_file;


void
sysman_sw_do_gui(boolean_t do_gui, const char *display_string)
{
	_sw_set_gui(do_gui, display_string);
}


int
sysman_sw_start_script(void)
{
	int fd;

	script_file = _start_batch_cmd(&fd);

	return fd;
}


int
sysman_sw_add_cmd_to_script(int fd, SysmanSWArg *swa_p)
{
	return _add_batch_cmd(fd, swa_p, script_file);
}


int
sysman_sw_finish_script(int fd)
{
	return _finish_batch_cmd(fd);
}


int
sysman_add_sw_by_script(char *buf, int bufsiz)
{
	int	status;


	if (script_file == NULL) {
		return (-1);
	}

	open_sysman_cat();
	init_err_msg(buf, bufsiz, catgets(_catlibsysman,7,39, "add software failed"));

	status = call_function_as_admin(_root_add_sw_by_script, script_file,
	    strlen (script_file), buf, bufsiz);

	free((void *)script_file);
	script_file = NULL;

	return (status);
}


int
sysman_add_sw(SysmanSWArg *swa_p, char *buf, int bufsiz)
{

	int	status;


	if (swa_p == NULL) {
		return (-1);
	}

	open_sysman_cat();
	init_err_msg(buf, bufsiz, catgets(_catlibsysman,7,40, "add software failed"));

	status = call_function_as_admin(_root_add_sw, swa_p,
	    sizeof (SysmanSWArg), buf, bufsiz);

	return (status);
}


int
sysman_delete_sw(SysmanSWArg *swa_p, char *buf, int bufsiz)
{

	int	status;


	if (swa_p == NULL) {
		return (-1);
	}

	open_sysman_cat();
	init_err_msg(buf, bufsiz, catgets(_catlibsysman,7,41, "delete software failed"));

	status = call_function_as_admin(_root_delete_sw, swa_p,
	    sizeof (SysmanSWArg), buf, bufsiz);

	return (status);
}
