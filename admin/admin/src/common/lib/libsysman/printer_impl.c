/*
 * Copyright (c) 1994 by Sun Microsystems, Inc.
 * All rights reserved.
 */

#pragma	ident	"@(#)printer_impl.c	1.16	95/07/19 SMI"


#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "sysman_impl.h"
#include "printer_iface.h"


int
_root_add_local_printer(void *arg_p, char *buf, int len)
{

	int			status;
	int			retval;
	SysmanPrinterArg	*pa_p = (SysmanPrinterArg *)arg_p;
	LpInfo			lpi;

	if (pa_p == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	memset(&lpi, 0, sizeof (lpi));
	lpi.context = strdup(LOCAL_CONTEXT_NAME);
	lpi.printername = pa_p->printername;
	lpi.printertype = pa_p->printertype;
	lpi.file_contents = pa_p->file_contents;
	lpi.comment = pa_p->comment;
	lpi.device = pa_p->device;
	lpi.notify = pa_p->notify;
	lpi.protocol = pa_p->protocol;
	lpi.num_restarts = pa_p->num_restarts;
	lpi.default_p = pa_p->default_p;
	lpi.banner_req_p = pa_p->banner_req_p;
	lpi.accept_p = pa_p->accept_p;
	lpi.enable_p = pa_p->enable_p;
	lpi.user_allow_list = pa_p->user_allow_list;

	status = add_local_printer(&lpi);

	switch (status) {
	case PRINTER_SUCCESS:
		retval = SYSMAN_SUCCESS;
		break;
	case PRINTER_ERR_NON_UNIQUE:
		retval = SYSMAN_PRINTER_NON_UNIQUE;
		break;
	case PRINTER_ERR_SYSTEM_LPADMIN_FAILED:
		retval = SYSMAN_PRINTER_LPADMIN_FAILED;
		break;
	case PRINTER_ERR_SYSTEM_LPSYSTEM_FAILED:
	case PRINTER_ERR_LPSYSTEM_FAILED:
		retval = SYSMAN_PRINTER_LPSYSTEM_FAILED;
		break;
	case PRINTER_ERR_SAC_LIST_FAILED:
	case PRINTER_ERR_PM_LIST_FAILED:
	case PRINTER_ERR_TCP_PM_CREATE_FAILED:
	case PRINTER_ERR_LS5_CREATE_FAILED:
	case PRINTER_ERR_LBSD_CREATE_FAILED:
	case PRINTER_ERR_L0_CREATE_FAILED:
		/* network access setup failed */
		retval = SYSMAN_PRINTER_NETACCESS_FAILED;
		break;
	case PRINTER_ERR_SYSTEM_ENABLE_FAILED:
		retval = SYSMAN_PRINTER_ENABLE_FAILED;
		break;
	case PRINTER_ERR_SYSTEM_ACCEPT_FAILED:
		retval = SYSMAN_PRINTER_ACCEPT_FAILED;
		break;
	case PRINTER_FAILURE:
	default:
		retval = SYSMAN_PRINTER_FAILED;
		break;
	}

	return (retval);
}


int
_root_add_remote_printer(void *arg_p, char *buf, int len)
{

	int			status;
	int			retval;
	SysmanPrinterArg	*pa_p = (SysmanPrinterArg *)arg_p;
	LpInfo			lpi;


	if (pa_p == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	memset(&lpi, 0, sizeof (lpi));
	lpi.context = strdup(LOCAL_CONTEXT_NAME);
	lpi.printername = pa_p->printername;
	lpi.printserver = pa_p->printserver;
	lpi.comment = pa_p->comment;
	lpi.protocol = pa_p->protocol;
	lpi.num_restarts = pa_p->num_restarts;
	lpi.default_p = pa_p->default_p;
	lpi.accept_p = pa_p->accept_p;
	lpi.enable_p = pa_p->enable_p;

	status = add_remote_printer(&lpi);

	switch (status) {
	case PRINTER_SUCCESS:
		retval = SYSMAN_SUCCESS;
		break;
	case PRINTER_ERR_NON_UNIQUE:
		retval = SYSMAN_PRINTER_NON_UNIQUE;
		break;
	case PRINTER_ERR_SYSTEM_LPADMIN_FAILED:
		retval = SYSMAN_PRINTER_LPADMIN_FAILED;
		break;
	case PRINTER_ERR_SYSTEM_LPSYSTEM_FAILED:
	case PRINTER_ERR_LPSYSTEM_FAILED:
		retval = SYSMAN_PRINTER_LPSYSTEM_FAILED;
		break;
	case PRINTER_ERR_SAC_LIST_FAILED:
	case PRINTER_ERR_PM_LIST_FAILED:
	case PRINTER_ERR_TCP_PM_CREATE_FAILED:
	case PRINTER_ERR_LS5_CREATE_FAILED:
	case PRINTER_ERR_LBSD_CREATE_FAILED:
	case PRINTER_ERR_L0_CREATE_FAILED:
		/* network access setup failed */
		retval = SYSMAN_PRINTER_NETACCESS_FAILED;
		break;
	case PRINTER_ERR_SYSTEM_ENABLE_FAILED:
		retval = SYSMAN_PRINTER_ENABLE_FAILED;
		break;
	case PRINTER_ERR_SYSTEM_ACCEPT_FAILED:
		retval = SYSMAN_PRINTER_ACCEPT_FAILED;
		break;
	case PRINTER_FAILURE:
	default:
		retval = SYSMAN_PRINTER_FAILED;
		break;
	}

	return (retval);
}


int
_root_delete_printer(void *arg_p, char *buf, int len)
{

	int			status;
	int			retval;
	SysmanPrinterArg	*pa_p = (SysmanPrinterArg *)arg_p;
	LpInfo			lpi;


	if (pa_p == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	memset(&lpi, 0, sizeof (lpi));
	lpi.context = strdup(LOCAL_CONTEXT_NAME);
	lpi.printername = pa_p->printername;

	status = delete_printer(&lpi);

	switch (status) {
	case PRINTER_SUCCESS:
		retval = SYSMAN_SUCCESS;
		break;
	case PRINTER_ERR_SYSTEM_DISABLE_FAILED:
		retval = SYSMAN_PRINTER_DISABLE_FAILED;
		break;
	case PRINTER_ERR_SYSTEM_REJECT_FAILED:
		retval = SYSMAN_PRINTER_REJECT_FAILED;
		break;
	case PRINTER_ERR_SYSTEM_LPADMIN_FAILED:
		retval = SYSMAN_PRINTER_LPADMIN_FAILED;
		break;
	case PRINTER_FAILURE:
	default:
		retval = SYSMAN_PRINTER_FAILED;
		break;
	}

	return (retval);
}


int
_root_modify_local_printer(void *arg_p, char *buf, int len)
{

	int			status;
	int			retval;
	SysmanPrinterArg	*pa_p = (SysmanPrinterArg *)arg_p;
	LpInfo			lpi;


	if (pa_p == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	memset(&lpi, 0, sizeof (lpi));
	lpi.context = strdup(LOCAL_CONTEXT_NAME);
	lpi.printername = pa_p->printername;
	lpi.printserver = pa_p->printserver;
	lpi.printertype = pa_p->printertype;
	lpi.file_contents = pa_p->file_contents;
	lpi.comment = pa_p->comment;
	lpi.device = pa_p->device;
	lpi.notify = pa_p->notify;
	lpi.protocol = pa_p->protocol;
	lpi.default_p = pa_p->default_p;
	lpi.banner_req_p = pa_p->banner_req_p;
	lpi.accept_p = pa_p->accept_p;
	lpi.enable_p = pa_p->enable_p;
	lpi.user_allow_list = pa_p->user_allow_list;

	status = modify_local_printer(&lpi);

	switch (status) {
	case PRINTER_SUCCESS:
		retval = SYSMAN_SUCCESS;
		break;
	case PRINTER_ERR_SYSTEM_REJECT_FAILED:
		retval = SYSMAN_PRINTER_REJECT_FAILED;
		break;
	case PRINTER_ERR_SYSTEM_DISABLE_FAILED:
		retval = SYSMAN_PRINTER_DISABLE_FAILED;
		break;
	case PRINTER_ERR_SYSTEM_LPADMIN_FAILED:
		retval = SYSMAN_PRINTER_LPADMIN_FAILED;
		break;
	case PRINTER_ERR_SYSTEM_LPSYSTEM_FAILED:
		retval = SYSMAN_PRINTER_LPSYSTEM_FAILED;
		break;
	case PRINTER_ERR_GET_DEFAULT_FAILED:
		retval = SYSMAN_PRINTER_GET_DEFAULT_FAILED;
		break;
	case PRINTER_FAILURE:
	default:
		retval = SYSMAN_PRINTER_FAILED;
		break;
	}

	return (retval);
}


int
_root_modify_remote_printer(void *arg_p, char *buf, int len)
{

	int			status;
	int			retval;
	SysmanPrinterArg	*pa_p = (SysmanPrinterArg *)arg_p;
	LpInfo			lpi;


	if (pa_p == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	memset(&lpi, 0, sizeof (lpi));
	lpi.context = strdup(LOCAL_CONTEXT_NAME);
	lpi.printername = pa_p->printername;
	lpi.printserver = pa_p->printserver;
	lpi.printertype = pa_p->printertype;
	lpi.file_contents = pa_p->file_contents;
	lpi.comment = pa_p->comment;
	lpi.protocol = pa_p->protocol;
	lpi.default_p = pa_p->default_p;
	lpi.accept_p = pa_p->accept_p;
	lpi.enable_p = pa_p->enable_p;

	status = modify_remote_printer(&lpi);

	switch (status) {
	case PRINTER_SUCCESS:
		retval = SYSMAN_SUCCESS;
		break;
	case PRINTER_ERR_SYSTEM_REJECT_FAILED:
		retval = SYSMAN_PRINTER_REJECT_FAILED;
		break;
	case PRINTER_ERR_SYSTEM_DISABLE_FAILED:
		retval = SYSMAN_PRINTER_DISABLE_FAILED;
		break;
	case PRINTER_ERR_SYSTEM_LPADMIN_FAILED:
		retval = SYSMAN_PRINTER_LPADMIN_FAILED;
		break;
	case PRINTER_ERR_SYSTEM_LPSYSTEM_FAILED:
		retval = SYSMAN_PRINTER_LPSYSTEM_FAILED;
		break;
	case PRINTER_ERR_GET_DEFAULT_FAILED:
		retval = SYSMAN_PRINTER_GET_DEFAULT_FAILED;
		break;
	case PRINTER_FAILURE:
	default:
		retval = SYSMAN_PRINTER_FAILED;
		break;
	}

	return (retval);
}


int
_list_printer_devices(char ***dev_pp, char *buf, int len)
{

	int	status;


	if (dev_pp == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	status = do_list_devices(dev_pp);

	if (status < 0) {
		switch (status) {
		case PRINTER_ERR_MALLOC_FAILED:
			status = SYSMAN_MALLOC_ERR;
			break;
		case PRINTER_ERR_DIR_OPEN_FAILED:
		case PRINTER_FAILURE:
		default:
			status = SYSMAN_PRINTER_FAILED;
			break;
		}
	}

	return (status);
}


void
_free_printer_devices_list(char **dev_p, int cnt)
{

	if (dev_p == NULL) {
		return;
	}

	do_free_devices(dev_p, cnt);
}


int
_get_default_printer_name(char **printer, char *buf, int bufsiz)
{

	int	status;


	if (printer == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	status = get_default_printer(printer, LOCAL_CONTEXT_NAME);

	if (status == PRINTER_SUCCESS) {
		return (SYSMAN_SUCCESS);
	} else {
		return (SYSMAN_PRINTER_FAILED);
	}
}


int
_root_get_printer(void *arg_p, char *buf, int len)
{

	int			status;
	int			retval;
	SysmanSharedPrinterArg	*pa_p = (SysmanSharedPrinterArg *)arg_p;
	const char		*cp;
	LpInfo			lpi;


	if (pa_p == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	/* save and restore printername, zero out rest of structure */
	cp = strdup(pa_p->printername);
	memset((void *)pa_p, '\0', sizeof (SysmanSharedPrinterArg));
	strcpy(pa_p->printername, cp);
	free((void *)cp);

	memset(&lpi, 0, sizeof (lpi));
	lpi.printername = pa_p->printername;
	lpi.context = strdup(LOCAL_CONTEXT_NAME);
	status = view_printer(&lpi);

	if (status == PRINTER_SUCCESS) {
		if (lpi.printertype != NULL) {
			strcpy(pa_p->printertype, lpi.printertype);
		}
		if (lpi.printserver != NULL) {
			strcpy(pa_p->printserver, lpi.printserver);
		}
		if (lpi.file_contents != NULL) {
			strcpy(pa_p->file_contents, lpi.file_contents);
		}
		if (lpi.comment != NULL) {
			strcpy(pa_p->comment, lpi.comment);
		}
		if (lpi.device != NULL) {
			strcpy(pa_p->device, lpi.device);
		}
		if (lpi.notify != NULL) {
			strcpy(pa_p->notify, lpi.notify);
		}
		if (lpi.protocol != NULL) {
			strcpy(pa_p->protocol, lpi.protocol);
		}
		pa_p->num_restarts = lpi.num_restarts;
		pa_p->default_p = lpi.default_p;
		pa_p->banner_req_p = lpi.banner_req_p;
		pa_p->enable_p = lpi.enable_p;
		pa_p->accept_p = lpi.accept_p;
		if (lpi.user_allow_list != NULL) {
			strcpy(pa_p->user_allow_list, lpi.user_allow_list);
		}

		retval = SYSMAN_SUCCESS;

	} else {
		switch (status) {
		case PRINTER_ERR_SYSTEM_LPSTAT_FAILED:
			retval = SYSMAN_PRINTER_LPSTAT_FAILED;
			break;
		case PRINTER_ERR_SYSTEM_LPSYSTEM_FAILED:
			retval = SYSMAN_PRINTER_LPSYSTEM_FAILED;
			break;
		case PRINTER_ERR_GET_DEFAULT_FAILED:
			retval = SYSMAN_PRINTER_GET_DEFAULT_FAILED;
			break;
		case PRINTER_ERR_PIPE_FAILED:
		case PRINTER_FAILURE:
		default:
			retval = SYSMAN_PRINTER_FAILED;
			break;
		}
	}

	return (retval);
}


int
_list_printer(SysmanPrinterArg **pa_pp, char *buf, int len)
{

	int		i;
	int		cnt;
	int		retval;
	LpInfo		*lpi_p;


	if (pa_pp == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	cnt = list_printers(&lpi_p, LOCAL_CONTEXT_NAME);

	if (cnt < 0) {
		switch (cnt) {
		case PRINTER_ERR_MALLOC_FAILED:
			retval = SYSMAN_MALLOC_ERR;
			break;
		case PRINTER_FAILURE:
		default:
			retval = SYSMAN_PRINTER_FAILED;
			break;
		}

		return (retval);
	} else if (cnt == 0) {
		*pa_pp = NULL;
		return (cnt);
	}

	*pa_pp = (SysmanPrinterArg *)malloc(cnt * sizeof (SysmanPrinterArg));

	memset((void *)*pa_pp, '\0', cnt * sizeof (SysmanPrinterArg));

	if (*pa_pp == NULL) {
		return (SYSMAN_MALLOC_ERR);
	}

	for (i = 0; i < cnt; i++) {
		(*pa_pp)[i].printername = strdup(lpi_p[i].printername);

		if (lpi_p[i].printserver == NULL) {
			(*pa_pp)[i].printserver = NULL;
		} else {
			(*pa_pp)[i].printserver = strdup(lpi_p[i].printserver);
		}

		if (lpi_p[i].comment == NULL) {
			(*pa_pp)[i].comment = NULL;
		} else {
			(*pa_pp)[i].comment = strdup(lpi_p[i].comment);
		}
	}

	return (cnt);
}


void
_free_printer(SysmanPrinterArg *pa_p)
{

	if (pa_p->printername != NULL) {
		free((void *)pa_p->printername);
	}
	if (pa_p->printertype != NULL) {
		free((void *)pa_p->printertype);
	}
	if (pa_p->printserver != NULL) {
		free((void *)pa_p->printserver);
	}
	if (pa_p->file_contents != NULL) {
		free((void *)pa_p->file_contents);
	}
	if (pa_p->comment != NULL) {
		free((void *)pa_p->comment);
	}
	if (pa_p->device != NULL) {
		free((void *)pa_p->device);
	}
	if (pa_p->notify != NULL) {
		free((void *)pa_p->notify);
	}
	if (pa_p->user_allow_list != NULL) {
		free((void *)pa_p->user_allow_list);
	}
}


void
_free_printer_list(SysmanPrinterArg *pa_p, int cnt)
{

	int	i;


	if (pa_p == NULL) {
		return;
	}

	for (i = 0; i < cnt; i++) {
		_free_printer(pa_p + i);
	}

	free((void *)pa_p);
}
