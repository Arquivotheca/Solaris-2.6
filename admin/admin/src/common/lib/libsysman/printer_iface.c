/*
 * Copyright (c) 1994 by Sun Microsystems, Inc.
 * All rights reserved.
 */

#pragma	ident	"@(#)printer_iface.c	1.10	96/09/09 SMI"


#include <stddef.h>
#include <string.h>
#include "sysman_iface.h"
#include "sysman_impl.h"


int
sysman_add_local_printer(SysmanPrinterArg *pa_p, char *buf, int bufsiz)
{

	int	status;


	if (pa_p == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	open_sysman_cat();
	init_err_msg(buf, bufsiz, catgets(_catlibsysman,7,22, "add local printer failed"));

	status = call_function_as_admin(_root_add_local_printer, pa_p,
	    sizeof (SysmanPrinterArg), buf, bufsiz);

	return (status);
}


int
sysman_add_remote_printer(SysmanPrinterArg *pa_p, char *buf, int bufsiz)
{

	int	status;


	if (pa_p == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	open_sysman_cat();
	init_err_msg(buf, bufsiz, catgets(_catlibsysman,7,23, "add remote printer failed"));

	status = call_function_as_admin(_root_add_remote_printer, pa_p,
	    sizeof (SysmanPrinterArg), buf, bufsiz);

	return (status);
}


int
sysman_delete_printer(SysmanPrinterArg *pa_p, char *buf, int bufsiz)
{

	int	status;


	if (pa_p == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	open_sysman_cat();
	init_err_msg(buf, bufsiz, catgets(_catlibsysman,7,24, "delete printer failed"));

	status = call_function_as_admin(_root_delete_printer, pa_p,
	    sizeof (SysmanPrinterArg), buf, bufsiz);

	return (status);
}


int
sysman_modify_local_printer(SysmanPrinterArg *pa_p, char *buf, int bufsiz)
{

	int	status;


	if (pa_p == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	open_sysman_cat();
	init_err_msg(buf, bufsiz, catgets(_catlibsysman,7,25, "modify local printer failed"));

	status = call_function_as_admin(_root_modify_local_printer, pa_p,
	    sizeof (SysmanPrinterArg), buf, bufsiz);

	return (status);
}


int
sysman_modify_remote_printer(SysmanPrinterArg *pa_p, char *buf, int bufsiz)
{

	int	status;


	if (pa_p == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	open_sysman_cat();
	init_err_msg(buf, bufsiz, catgets(_catlibsysman,7,26, "modify remote printer failed"));

	status = call_function_as_admin(_root_modify_remote_printer, pa_p,
	    sizeof (SysmanPrinterArg), buf, bufsiz);

	return (status);
}


int
sysman_list_printer_devices(char ***devices_pp, char *buf, int bufsiz)
{

	int	status;


	if (devices_pp == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	open_sysman_cat();
	init_err_msg(buf, bufsiz, catgets(_catlibsysman,7,27, "list printer devices failed"));

	status = _list_printer_devices(devices_pp, buf, bufsiz);

	return (status);
}


void
sysman_free_printer_devices_list(char **devices_p, int cnt)
{
	_free_printer_devices_list(devices_p, cnt);
}


int
sysman_get_default_printer_name(char **printer, char *buf, int bufsiz)
{

	int	status;


	if (printer == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	open_sysman_cat();
	init_err_msg(buf, bufsiz, catgets(_catlibsysman,7,28, "get default printer name failed"));

	status = _get_default_printer_name(printer, buf, bufsiz);

	return (status);
}


int
sysman_get_printer(SysmanPrinterArg *pa_p, char *buf, int bufsiz)
{

	SysmanSharedPrinterArg	spa;
	int			status;


	if (pa_p == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	open_sysman_cat();
	init_err_msg(buf, bufsiz, catgets(_catlibsysman,7,29, "get printer failed"));

	strcpy(spa.printername, pa_p->printername);

	status = call_function_as_admin(_root_get_printer, (void *)&spa,
	    sizeof (SysmanSharedPrinterArg), buf, bufsiz);

	if (status == SYSMAN_SUCCESS) {
		pa_p->printername =
		    spa.printername[0] ? strdup(spa.printername) : NULL;
		pa_p->printertype =
		    spa.printertype[0] ? strdup(spa.printertype) : NULL;
		pa_p->printserver =
		    spa.printserver[0] ? strdup(spa.printserver) : NULL;
		pa_p->file_contents =
		    spa.file_contents[0] ? strdup(spa.file_contents) : NULL;
		pa_p->comment = spa.comment[0] ? strdup(spa.comment) : NULL;
		pa_p->device = spa.device[0] ? strdup(spa.device) : NULL;
		pa_p->notify = spa.notify[0] ? strdup(spa.notify) : NULL;
		pa_p->protocol = spa.protocol[0] ? strdup(spa.protocol) : NULL;
		pa_p->num_restarts = spa.num_restarts;
		pa_p->default_p = spa.default_p;
		pa_p->banner_req_p = spa.banner_req_p;
		pa_p->enable_p = spa.enable_p;
		pa_p->accept_p = spa.accept_p;
		pa_p->user_allow_list =
		    spa.user_allow_list[0] ? strdup(spa.user_allow_list) : NULL;
	}

	return (status);
}


void
sysman_free_printer(SysmanPrinterArg *pa_p)
{
	_free_printer(pa_p);
}


int
sysman_list_printer(SysmanPrinterArg **pa_pp, char *buf, int bufsiz)
{

	int	status;


	if (pa_pp == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	open_sysman_cat();
	init_err_msg(buf, bufsiz, catgets(_catlibsysman,7,30, "list printer failed"));

	status = _list_printer(pa_pp, buf, bufsiz);

	return (status);
}


void
sysman_free_printer_list(SysmanPrinterArg *pa_p, int cnt)
{
	_free_printer_list(pa_p, cnt);
}
