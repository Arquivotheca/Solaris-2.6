/*
 * Copyright (c) 1994 by Sun Microsystems, Inc.
 * All rights reserved.
 */

#pragma	ident	"@(#)printer_modify.c	1.28	95/08/30 SMI"


#include <stdio.h>
#include <string.h>
#include "printer_impl.h"

#include <print/ns.h>

static
int
premodify_boilerplate(LpInfo *lpi_p)
{

	int	err;
	char	workbuf[PRT_MAXSTRLEN];


	/*
	 * Before we play with the configuration, we should
	 * disable the printer and reject any more requests
	 * until we are done changing the configuration
	 */

	/* reject <printername> */
	(void) sprintf(workbuf, "set -f ; /usr/sbin/reject %s 1>/dev/null 2>&1",
	    lpi_p->printername);

	err = system(workbuf);
	if (err != PRINTER_SUCCESS) {
		return (PRINTER_ERR_SYSTEM_REJECT_FAILED);
	}

	/* disable <printername> */
	(void) sprintf(workbuf, "set -f ; /bin/disable %s 1>/dev/null 2>&1",
	    lpi_p->printername);

	err = system(workbuf);
	if (err != PRINTER_SUCCESS) {
		return (PRINTER_ERR_SYSTEM_DISABLE_FAILED);
	}

	return (PRINTER_SUCCESS);
}


static
int
postmodify_boilerplate(LpInfo *lpi_p)
{

	int	err;
	char	workbuf[PRT_MAXSTRLEN];


	/*
	 * accept/reject requests, enable/disable printer
	 * don't worry about errors from these commands, since if
	 * you try to enable an already enabled printer or accept
	 * requests for an already accepting printer they return
	 * status 1, even though there is no harm.
	 */

	if (lpi_p->accept_p != B_FALSE) {
		sprintf(workbuf, "/usr/sbin/accept %s", lpi_p->printername);
	} else {
		sprintf(workbuf, "/usr/sbin/reject %s", lpi_p->printername);
	}

	(void) do_system(workbuf);

	if (lpi_p->enable_p != B_FALSE) {
		sprintf(workbuf, "/bin/enable %s", lpi_p->printername);
	} else {
		sprintf(workbuf, "/bin/disable %s", lpi_p->printername);
	}

	(void) do_system(workbuf);

	return (PRINTER_SUCCESS);
}


int
modify_local_printer(LpInfo *lpi_p)
{

	int	err;
	char	workbuf[PRT_MAXSTRLEN];
	char	*def_printer;
	int	status = 0;

	/*
	**	Check the comment. This is for the bundled admintool.
	*/
	
	if(lpi_p->comment != NULL)
		status = valid_description(lpi_p->comment);
	if(status == 0)
		return(PRINTER_ERR_COMMENT_ERROR);

	if ((err = premodify_boilerplate(lpi_p)) != PRINTER_SUCCESS) {
		return (err);
	}

	sprintf(workbuf, "%s -p %s", PRT_LPADMIN, lpi_p->printername);

	/* if device change, lpadmin -p -v ... */

	if (lpi_p->device != NULL) {
		strcat(workbuf, " -v ");
		strcat(workbuf, lpi_p->device);
	}

	/* if printertype change, lpadmin -p -T ... */

	if (lpi_p->printertype != NULL &&
	    strlen(lpi_p->printertype) > (size_t)0) {
		strcat(workbuf, " -T ");
		if (strcmp(lpi_p->printertype,
		    PRT_DEF_PRINTER_TYPE_NEWSPRINT) == 0) {
			/* call it PS and add NeWSprint interface program */
			strcat(workbuf, PRT_DEF_PRINTER_TYPE_PS);
			strcat(workbuf, " -i ");
			strcat(workbuf, PRT_INTERFACE_NEWSPRINT);
		} else {
			strcat(workbuf, lpi_p->printertype);
		}
	}

	/* if file contents change, lpadmin -p -I ... */

	if (lpi_p->file_contents != NULL) {
		strcat(workbuf, " -I ");
		strcat(workbuf, (strlen(lpi_p->file_contents) != 0 ?
		    lpi_p->file_contents : "\"\""));
	}

	/* if comment change, lpadmin -p -D ... */

	if (lpi_p->comment != NULL)  
	{
		strcat(workbuf, " -D \"");
		strcat(workbuf, lpi_p->comment);
		strcat(workbuf, "\"");
	}

	strcat(workbuf, " -o ");
	if (lpi_p->banner_req_p) {
		strcat(workbuf, "banner");
	} else {
		strcat(workbuf, "nobanner");
	}

	if (lpi_p->notify != NULL) {
		strcat(workbuf, " -A ");
		strcat(workbuf, lpi_p->notify);
	}

	err = do_system(workbuf);

	if (err != PRINTER_SUCCESS) {
		return (PRINTER_ERR_SYSTEM_LPADMIN_FAILED);
	}

	err = get_default_printer(&def_printer, lpi_p->context);
	if (err != PRINTER_SUCCESS) {
		return (PRINTER_ERR_GET_DEFAULT_FAILED);
	}

	if (lpi_p->default_p != B_FALSE) {
	/*
	** Add this printer as the default, no matter what, if the user
	** has selected this. The reason for this is that with the methods
	** running on the local host and the remote server, and since
	** I have to back out the modify to make sure that FNS does
	** the right thing with the default printer, I will come
	** through this function possibly twice. If this was the
	** default printer, I need to be sure it is added to
	** the /etc/printers.conf. If it is already there, this will
	** do no harm. 
	*/

		sprintf(workbuf, "%s -d %s",
		    	PRT_LPADMIN, lpi_p->printername);

		err = do_system(workbuf);

		if (err != PRINTER_SUCCESS) {
			return (PRINTER_ERR_SYSTEM_LPADMIN_FAILED);
		}
	} else {
		/*
		 * We don't want this printer to be the default printer;
		 * if it already is, turn it off, else do nothing.
		 */

		if (def_printer != NULL &&
		    strcmp(def_printer, lpi_p->printername) == 0) {
			ns_printer_t *	printer;
			printer = (ns_printer_t *) malloc(sizeof(*printer));	
			memset(printer,0,sizeof(*printer));
			printer->name = NS_NAME_DEFAULT;
			printer->attributes = NULL;
			printer->source = NS_SVC_ETC;	
			dyn_ns_printer_put(printer);
			free(printer);	
		    }
	}

	/* Setup allow lists */

	sprintf(workbuf, "%s -p %s -u allow:none", PRT_LPADMIN,
		lpi_p->printername);

	if (lpi_p->user_allow_list != NULL) {
		strcat(workbuf, " ; " PRT_LPADMIN " -p ");
		strcat(workbuf, lpi_p->printername);
		strcat(workbuf, " -u allow:");
		strcat(workbuf, lpi_p->user_allow_list);
	}

	err = do_system(workbuf);

	if (err != PRINTER_SUCCESS) {
		return (PRINTER_ERR_SYSTEM_LPADMIN_FAILED);
	}

	/* protocol -- lpsystem -t ... */

	if (lpi_p->protocol != NULL && lpi_p->printserver != NULL) {
		sprintf(workbuf, "lpsystem -t %s %s", lpi_p->protocol,
		    lpi_p->printserver);

		err = do_system(workbuf);

		if (err != PRINTER_SUCCESS) {
			return (PRINTER_ERR_SYSTEM_LPSYSTEM_FAILED);
		}
	}

	err = postmodify_boilerplate(lpi_p);

	return (err);
}


int
lp_modify_remote_printer(LpInfo *lpi_p)
{

	int	err;
	char	workbuf[PRT_MAXSTRLEN];
	char	*def_printer;


	if ((err = premodify_boilerplate(lpi_p)) != PRINTER_SUCCESS) {
		return (err);
	}

	sprintf(workbuf, "%s -p %s", PRT_LPADMIN, lpi_p->printername);

	/* if printertype change, lpadmin -p -T ... */

	if (lpi_p->printertype != NULL &&
	    strlen(lpi_p->printertype) > (size_t)0) {
		strcat(workbuf, " -T ");
		strcat(workbuf, lpi_p->printertype);
	}

	/* if file contents change, lpadmin -p -I ... */

	if (lpi_p->file_contents != NULL) {
		strcat(workbuf, " -I ");
		strcat(workbuf, (strlen(lpi_p->file_contents) != 0 ?
		    lpi_p->file_contents : "\"\""));
	}

	/* if comment change, lpadmin -p -D ... */

	if (lpi_p->comment != NULL)  
	{
		strcat(workbuf, " -D \"");
		strcat(workbuf, lpi_p->comment);
		strcat(workbuf, "\"");
	}

	err = do_system(workbuf);

	if (err != PRINTER_SUCCESS) {
		return (PRINTER_ERR_SYSTEM_LPADMIN_FAILED);
	}

	err = get_default_printer(&def_printer, lpi_p->context);
	if (err != PRINTER_SUCCESS) {
		return (PRINTER_ERR_GET_DEFAULT_FAILED);
	}

	if (lpi_p->default_p != B_FALSE) {

		/*
		** We want to make this the default printer. Because of the 
		** other things that we need to do, such as backing out the
		** "etc" part of the printer configuration, we need to do
		** this command no matter what. Since, at this point it would
		** be possible that the current printer we are trying to configure
		** would already be listed as the default in the NIS+ maps,
		** we need to reinput this information into the /etc/printers.conf
		** file.
		*/

		sprintf(workbuf, "%s -d %s",
			    PRT_LPADMIN, lpi_p->printername);
		err = do_system(workbuf);

		if (err != PRINTER_SUCCESS) {
			return (PRINTER_ERR_SYSTEM_LPADMIN_FAILED);
		}
	} else {
		/*
		 * We don't want this printer to be the default printer;
		 * if it already is, turn it off, else do nothing.
		 */

		if (def_printer != NULL &&
		    strcmp(def_printer, lpi_p->printername) == 0) {
			sprintf(workbuf, "%s -d", PRT_LPADMIN);

			err = do_system(workbuf);

			if (err != PRINTER_SUCCESS) {
				return (PRINTER_ERR_SYSTEM_LPADMIN_FAILED);
			}
		}
	}

	/* protocol -- lpsystem -t ... */

	if (lpi_p->protocol != NULL && lpi_p->printserver != NULL) {
		sprintf(workbuf, "lpsystem -t %s %s", lpi_p->protocol,
		    lpi_p->printserver);

		err = do_system(workbuf);

		if (err != PRINTER_SUCCESS) {
			return (PRINTER_ERR_SYSTEM_LPSYSTEM_FAILED);
		}
	}

	err = postmodify_boilerplate(lpi_p);

	return (err);
}


sp_modify_printer(LpInfo *lpi_p)
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
	if (lpi_p->comment != NULL)
		dyn_ns_set_value_from_string(NS_KEY_DESCRIPTION,
					    lpi_p->comment, printer);

	dyn_ns_printer_put(printer);

	if ((printer = (ns_printer_t *)
	     dyn_ns_printer_get_name(NS_NAME_DEFAULT,
				     lpi_p->context)) != NULL) {
		char *dflt = 
			(char *)dyn_ns_get_value_string(NS_KEY_USE, printer);

		if ((lpi_p->context == NULL) || 
	    		(strcmp(lpi_p->context, NS_SVC_ETC) == 0))
			printer->source = NS_SVC_ETC;
		else if (strcmp(lpi_p->context, NS_SVC_NIS) == 0)
			printer->source = NS_SVC_NIS;
		else if (strcmp(lpi_p->context, NS_SVC_NISPLUS) == 0)
			printer->source = NS_SVC_XFN;
		else
			printer->source = NS_SVC_ETC;

		if (strcmp(lpi_p->printername, dflt) == 0) {
			if (lpi_p->default_p == B_FALSE) {
				printer->attributes = NULL;
				dyn_ns_printer_put(printer);
			}
		} else {
			if (lpi_p->default_p == B_TRUE) {
				dyn_ns_set_value_from_string(NS_KEY_USE,
							 lpi_p->printername,
							 printer);
				dyn_ns_printer_put(printer);
			}
		}
	} else {
		if (lpi_p->default_p == B_TRUE) {
			printer = (ns_printer_t *)malloc(sizeof (*printer));
			memset(printer, 0, sizeof (*printer));

			printer->name = NS_NAME_DEFAULT;

			if ((lpi_p->context == NULL) || 
	    			(strcmp(lpi_p->context, NS_SVC_ETC) == 0))
				printer->source = NS_SVC_ETC;
			else if (strcmp(lpi_p->context, NS_SVC_NIS) == 0)
				printer->source = NS_SVC_NIS;
			else if (strcmp(lpi_p->context, NS_SVC_NISPLUS) == 0)
				printer->source = NS_SVC_XFN;
			else
				printer->source = NS_SVC_ETC;
			dyn_ns_set_value_from_string(NS_KEY_USE,
						 lpi_p->printername,
						 printer);
			dyn_ns_printer_put(printer);
			free(printer);
		}
	}

	return (PRINTER_SUCCESS);
}


modify_remote_printer(LpInfo *lpi_p)
{
	int	status = 0;

	if(lpi_p->comment != NULL)
		status = valid_description(lpi_p->comment);
	if(status == 0)
		return(PRINTER_ERR_COMMENT_ERROR);

	if (print_client_sw_installed_p() == 0)
		return (lp_modify_remote_printer(lpi_p));
	else
	{
		int status = 0;
	
 		status = sp_modify_printer(lpi_p);
		if(( lpi_p->device != NULL) &&
                        (strcmp(lpi_p->context,NS_SVC_ETC) !=0 )&&
                        (strcmp(lpi_p->printserver,lpi_p->printhost) == 0))
                {
                        status = modify_local_printer(lpi_p);
                        return(status);
                }
                return(status);
        }
}

modify_printer(LpInfo *lpi_p)
{
	int rc = PRINTER_SUCCESS;
	int status = 0;


	/*
	** Do some error handling before making a call to modify.
	*/

	if(lpi_p->comment != NULL)
	{
		status = valid_description(lpi_p->comment);
		if(status == 0)
			return(PRINTER_ERR_COMMENT_ERROR);
	}

	/*
	** This if statement is for a local printer modify since
	** if a device is present then it was added as an install local
	** printer.
	*/
	
	if ((lpi_p->device != NULL) && (lpi_p->device[0] != NULL))
	{
		status = valid_printerport(lpi_p->device);
		if(status == -1)
			return(PRINTER_ERR_PORT_NAME_ERROR);
		if(lpi_p->printertype != NULL)
		{
			status = valid_printertype(lpi_p->printertype);
			if(status == -1)
				return(PRINTER_ERR_PRINTER_TYPE_ERROR);
		}
		if(lpi_p->user_allow_list != NULL)	
		{
			status = valid_printer_allow_list(
					lpi_p->user_allow_list);
			if(status == 0)
				return(PRINTER_ERR_NAME_LIST_ERROR);
		}
			
		rc = modify_local_printer(lpi_p);
	}
	
	else if (print_client_sw_installed_p() == 0)
		rc = lp_modify_remote_printer(lpi_p);

	if ((rc == PRINTER_SUCCESS) && (print_client_sw_installed_p() != 0)&&
		(strcmp(lpi_p->context,NS_SVC_ETC) == 0))
		rc = sp_modify_printer(lpi_p);

	 /*
        ** Now this is really ugly, but it has to be done. Since FNS evaluates
        ** the printer stuff from /etc/printers.conf we have to back out the
        ** modification of this printer to /etc/printers.conf so that modify_remote
        ** printer will function properly. This only happens when a printer
        ** is installed locally and has named as its server the host which
        ** it is being installed on.
        */

 	if((strcmp(lpi_p->context, NS_SVC_ETC) != 0)
                && (strcmp(lpi_p->printserver,lpi_p->printhost) == 0))
        {
                {
                        ns_printer_t *  printer;
 
                        printer = (ns_printer_t *)malloc(sizeof (*printer));
                        memset(printer,0,sizeof(*printer));
                        printer->name = (char *)lpi_p->printername;
                        printer->source = NS_SVC_ETC;
                        printer->attributes = NULL;
                        dyn_ns_printer_put(printer);
			 /*
                        ** If this is the default printer I need to back out
                        ** the _default from /etc/printers.conf, otherwise it
                        ** screws up the fndestroy for the default printer.
                        */
                        if(lpi_p->default_p == B_TRUE)
                        {
                                printer->name = NS_NAME_DEFAULT;
                                printer->attributes = NULL;
                                printer->source = NS_SVC_ETC;
                                dyn_ns_printer_put(printer);
                        }
                        free(printer);
                }
 	}


	return (rc);
}
