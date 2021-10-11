/*
 * Copyright (c) 1994 by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)printer_delete.c	1.8	95/07/25 SMI"


#include <stdio.h>
#include <unistd.h>
#include "printer_impl.h"

#include <print/ns.h>


int
sp_delete_printer(LpInfo *lpi_p)
{
	ns_printer_t *printer;

	if ((printer = (ns_printer_t *)
	     dyn_ns_printer_get_name(lpi_p->printername,
				     lpi_p->context)) == NULL)
		return (PRINTER_FAILURE);

	if ((lpi_p->context == NULL) || 
	    (strcmp(lpi_p->context, NS_SVC_ETC) == 0))
		printer->source = NS_SVC_ETC;
	else if (strcmp(lpi_p->context, NS_SVC_NIS) == 0)
		printer->source = NS_SVC_NIS;
	else if (strcmp(lpi_p->context, NS_SVC_NISPLUS) == 0)
		printer->source = NS_SVC_XFN;
	else
		printer->source = NS_SVC_ETC;
	printer->attributes = NULL;
	dyn_ns_printer_put(printer);

	if ((printer = (ns_printer_t *)
	     dyn_ns_printer_get_name(NS_NAME_DEFAULT,
				     lpi_p->context)) != NULL) {
		char *dflt = (char *)
			dyn_ns_get_value_string(NS_KEY_USE, printer);

		if ((dflt != NULL) &&
		    (strcmp(dflt, lpi_p->printername) == 0)) {
			printer->attributes = NULL;

			if ((lpi_p->context == NULL) || 
	    		(strcmp(lpi_p->context, NS_SVC_ETC) == 0))
				printer->source = NS_SVC_ETC;
			else if (strcmp(lpi_p->context, NS_SVC_NIS) == 0)
				printer->source = NS_SVC_NIS;
			else if (strcmp(lpi_p->context, NS_SVC_NISPLUS) == 0)
				printer->source = NS_SVC_XFN;
			else
				printer->source = NS_SVC_ETC;
			dyn_ns_printer_put(printer);
		}
	}

	return (PRINTER_SUCCESS);
}


int
lp_delete_printer(LpInfo *lpi_p)
{

	int	status;
	char	workbuf[1024];


	/* reject <printername> */
	(void) sprintf(workbuf, "/usr/sbin/reject %s", lpi_p->printername);

	status = do_system(workbuf);

	if (status != PRINTER_SUCCESS) {
		return (PRINTER_ERR_SYSTEM_REJECT_FAILED);
	}

	/* disable <printername> */
	(void) sprintf(workbuf, "/bin/disable %s", lpi_p->printername);

	status = do_system(workbuf);

	if (status != PRINTER_SUCCESS) {
		return (PRINTER_ERR_SYSTEM_DISABLE_FAILED);
	}

	/* lpadmin -x <printername> */
	(void) sprintf(workbuf, "%s -x %s", PRT_LPADMIN, lpi_p->printername);

	status = do_system(workbuf);

	if (status != PRINTER_SUCCESS) {
		return (PRINTER_ERR_SYSTEM_LPADMIN_FAILED);
	}

	return (PRINTER_SUCCESS);
}

delete_printer(LpInfo *lpi_p)
{
	char 	buf[BUFSIZ];
	char 	*def_printer;
	int  	err;
	int 	rc = PRINTER_SUCCESS;

	if (lpi_p == NULL) {
		return (PRINTER_FAILURE);
	}
	/*
	** If this a local printer and the name service is NISPLUS and this
	** is the default printer then we need to remove the _default entry
	** from /etc/printers.conf. so that the FNS stuff will work correctly.
	*/

	if(lpi_p->device != NULL && strcmp(lpi_p->context,NS_SVC_ETC) !=0)
	{
		err = get_default_printer(&def_printer,lpi_p->context);
		if (err != PRINTER_SUCCESS)
			return(PRINTER_ERR_GET_DEFAULT_FAILED);

		if(def_printer != NULL &&
			strcmp(def_printer,lpi_p->printername) == 0)
		{
			ns_printer_t *	printer;
			printer = (ns_printer_t *)malloc(sizeof(*printer));
			memset(printer,0,sizeof(*printer));
			printer->name = NS_NAME_DEFAULT;
			printer->attributes = NULL;
			printer->source = NS_SVC_ETC;
			dyn_ns_printer_put(printer);
			free(printer);
		}
	}
		
	if (print_client_sw_installed_p() != 0)
	 /* delete client access */
		rc = sp_delete_printer(lpi_p);

	/*
	** The reason I reset this value is that for NIS or NIS+, the first
	** time thru on the remote server and error message will be returned
	** from the sp_delete_printer function. This is due to the user not
	** having privs to delete a printer in NIS or NIS+ on the remote server.
	** The second time through, the method runs on the local host and
	** the return value will be success.
	*/

	if((strcmp(lpi_p->context,NS_SVC_ETC) !=0)
		&& (rc != PRINTER_SUCCESS))
			rc = PRINTER_SUCCESS;

	sprintf(buf, "/etc/lp/printers/%s", lpi_p->printername);

	if ((rc == PRINTER_SUCCESS) && (access(buf, F_OK) == 0))
		rc = lp_delete_printer(lpi_p);

	return (rc);
}
