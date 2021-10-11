/*
 * Copyright (c) 1994 by Sun Microsystems, Inc.
 * All rights reserved.
 */

#pragma	ident	"@(#)printer_add.c	1.19	95/07/21 SMI"


#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <sys/systeminfo.h>
#include "printer_impl.h"

#include <print/ns.h>

static
int
preconfigure_boilerplate(LpInfo *lpi_p)
{

	int	status = 0;

	/*
	** Now check the network access.
	*/
 
	status = setup_network_access(lpi_p->num_restarts);

	if (status != PRINTER_SUCCESS) {
		return (status);
	}

	(void) setup_postscript_filters();

	return (PRINTER_SUCCESS);
}


static
int
postconfigure_boilerplate(LpInfo *lpi_p)
{

	int	status;
	char	workbuf[1024];


	/* Is this the default printer for this system? */

	if (lpi_p->default_p != B_FALSE) {
		sprintf(workbuf, "%s -d %s", PRT_LPADMIN, lpi_p->printername);

		status = do_system(workbuf);

		if (status != PRINTER_SUCCESS) {
			return(PRINTER_ERR_SYSTEM_LPADMIN_FAILED);
		}
	}

	/* enable the printer? */

	if (lpi_p->enable_p != B_FALSE) {
		(void) sprintf(workbuf,
		    "set -f ; /bin/enable %s 1>/dev/null 2>&1",
		    lpi_p->printername);

		status = system(workbuf);

		if (status != PRINTER_SUCCESS) {
			return(PRINTER_ERR_SYSTEM_ENABLE_FAILED);
		}
	}

	/* accept print requests? */

	if (lpi_p->accept_p != B_FALSE) {
		(void) sprintf(workbuf,
		    "set -f ; /usr/sbin/accept %s 1>/dev/null 2>&1",
		    lpi_p->printername);

		status = system(workbuf);

		if (status != PRINTER_SUCCESS) {
			return(PRINTER_ERR_SYSTEM_ACCEPT_FAILED);
		}
	}

	return (PRINTER_SUCCESS);
}


int
lp_add_local_printer(LpInfo *lpi_p)
{

	int	status;
	char	workbuf[1024];


	if (lpi_p == NULL) {
		return (PRINTER_FAILURE);
	}

	status = preconfigure_boilerplate(lpi_p);

	if (status != PRINTER_SUCCESS) {
		return (status);
	}

	/*
	 * Configure the printer:
	 * lpadmin -p ...
	 * if default, lpadmin -d
	 * enable
	 * accept
	 */

	/* lpadmin -p <name> -v <port> -T <type> -I <contents> -A <fault> */

	sprintf(workbuf, "%s -p %s -v %s -T %s -I %s -A %s",
	    PRT_LPADMIN, lpi_p->printername, lpi_p->device,
	    (strcmp(lpi_p->printertype, PRT_DEF_PRINTER_TYPE_NEWSPRINT) ?
	    lpi_p->printertype : "PS"),
	    (strlen(lpi_p->file_contents) ? lpi_p->file_contents : "\"\""),
	    lpi_p->notify);

	/* add NeWSprint interface program */

	if (strcmp(lpi_p->printertype, PRT_DEF_PRINTER_TYPE_NEWSPRINT) == 0) {
		strcat(workbuf, " -i ");
		strcat(workbuf, PRT_INTERFACE_NEWSPRINT);
	}

	/* add description */

	if (lpi_p->comment != NULL && strlen(lpi_p->comment) != 0) {
		strcat(workbuf, " -D \"");
		strcat(workbuf, lpi_p->comment);
		strcat(workbuf, "\"");
	}

	/* banner required? */

	if (lpi_p->banner_req_p == B_FALSE) {
		strcat (workbuf, " -o nobanner");
	}

	if (lpi_p->user_allow_list != NULL &&
	    strlen(lpi_p->user_allow_list) != 0) {

		strcat(workbuf, " -u allow:");
		strcat(workbuf, lpi_p->user_allow_list);
	}

	/* run the lpadmin command we just built */

	status = do_system(workbuf);

	if (status != PRINTER_SUCCESS) {
		return(PRINTER_ERR_SYSTEM_LPADMIN_FAILED);
	}

	status = postconfigure_boilerplate(lpi_p);

	if (status != PRINTER_SUCCESS) {
		return(status);
	}

	/* Add '+' to Systems file to allow any system to print here */

	(void) sprintf(workbuf, "%s -t s5 -T n -R 10 + 1>/dev/null 2>&1",
	    PRT_LPSYSTEM);

	status = system(workbuf);

	if (status != PRINTER_SUCCESS) {
		return(PRINTER_ERR_SYSTEM_LPSYSTEM_FAILED);
	}

	return (PRINTER_SUCCESS);
}


int
lp_add_remote_printer(LpInfo *lpi_p)
{

	int	status;
	char	workbuf[1024];


	if (lpi_p == NULL) {
		return (PRINTER_FAILURE);
	}

	status = preconfigure_boilerplate(lpi_p);

	if (status != PRINTER_SUCCESS) {
		return (status);
	}

	/*
	 * Configure the printer:
	 * lpsystem -t protocol printserver
	 * lpadmin -p ...
	 * if default, lpadmin -d
	 * enable
	 * accept
	 */

	if (strchr(lpi_p->printserver, '!') != 0) {
		char *tmp = strdup(lpi_p->printserver),
	             *tmp2;

		tmp2 = strchr(tmp, '!');
		*tmp2 = NULL;
		sprintf(workbuf, "%s -t bsd %s", PRT_LPSYSTEM, tmp);
		free(tmp);
	} else
		sprintf(workbuf, "%s -t bsd %s",
	    		PRT_LPSYSTEM, lpi_p->printserver);

	status = do_system(workbuf);

	if (status != PRINTER_SUCCESS) {
		return (PRINTER_ERR_SYSTEM_LPSYSTEM_FAILED);
	}

	/* lpadmin -p <name> -s <print_server> -I any */

	sprintf(workbuf, "%s -p %s -s %s -I any",
	    PRT_LPADMIN, lpi_p->printername, lpi_p->printserver);

	/* add description */

	if (lpi_p->comment != NULL && strlen(lpi_p->comment) != 0) {
		strcat(workbuf, " -D \"");
		strcat(workbuf, lpi_p->comment);
		strcat(workbuf, "\"");
	}

	/* run the lpadmin command we just built */

	status = do_system(workbuf);

	if (status != PRINTER_SUCCESS) {
		return (PRINTER_ERR_SYSTEM_LPADMIN_FAILED);
	}

	status = postconfigure_boilerplate(lpi_p);

	return (status);
}

sp_add_printer(LpInfo *lpi_p)
{
	ns_printer_t *printer;
	ns_bsd_addr_t *addr;

	printer = (ns_printer_t *)malloc(sizeof (*printer));
	memset(printer, 0, sizeof (*printer));
	printer->name = (char *)lpi_p->printername;

	if ((lpi_p->context == NULL) || 
	    (strcmp(lpi_p->context, NS_SVC_ETC) == 0))
		printer->source = NS_SVC_ETC;
	else if (strcmp(lpi_p->context, NS_SVC_NIS) == 0)
		printer->source = NS_SVC_NIS;
	else if (strcmp(lpi_p->context, NS_SVC_NISPLUS) == 0)
		printer->source = NS_SVC_XFN;
	else
		printer->source = NS_SVC_ETC;
	
	
       	addr = (ns_bsd_addr_t *)malloc(sizeof (*addr));
	memset(addr, 0, sizeof (*addr));
	addr->printer = (char *)lpi_p->printername;
	if (lpi_p->printserver == NULL) {
		static char localhostname[SYS_NMLN];

		sysinfo(SI_HOSTNAME, localhostname, sizeof (localhostname));
		addr->server = localhostname;
	} else	
		addr->server = (char *)lpi_p->printserver;
	dyn_ns_set_value(NS_KEY_BSDADDR, addr, printer);
	if (lpi_p->comment != NULL && lpi_p->comment[0] != NULL)
		dyn_ns_set_value_from_string(NS_KEY_DESCRIPTION, lpi_p->comment,
					 printer);

	dyn_ns_printer_put(printer);

	if (lpi_p->default_p == B_TRUE) {
		printer->name = NS_NAME_DEFAULT;
		printer->attributes = NULL;
		dyn_ns_set_value_from_string(NS_KEY_USE, lpi_p->printername,
				 		printer);
		dyn_ns_printer_put(printer);
	}

	free(addr);
	free(printer);
	return (PRINTER_SUCCESS);
}


add_remote_printer(LpInfo *lpi_p)
{
	int status = 0;

	/*
	** First verify that the printer name is syntactically correct. 
	*/

	status = valid_printer_name(lpi_p->printername);
        if(status == 0) 
		return(PRINTER_ERR_PRINTER_NAME_ERROR);
	
	status = verify_unique_printer_name(lpi_p->printername);

	if (status != PRINTER_SUCCESS) {
		return (PRINTER_ERR_NON_UNIQUE);
	}

	/*
	**Verify the server name given is syntactically correct.
	*/

	status = valid_hostname(lpi_p->printserver);
	if (status == 0)
		return(PRINTER_ERR_SERVER_NAME_ERROR);
	/*
	** Check the comment for syntax.
	*/

	status = valid_description(lpi_p->comment);
	if (status == 0)
		return(PRINTER_ERR_COMMENT_ERROR);


	if (print_client_sw_installed_p() == 0)
		return (lp_add_remote_printer(lpi_p));
	else
	{
		if((lpi_p->default_p == B_TRUE)
			&& (strcmp(lpi_p->context,NS_SVC_NISPLUS) == 0))
		{
			{
				ns_printer_t *	printer;
				printer = (ns_printer_t *)malloc(sizeof (*printer));
				memset(printer,0,sizeof(*printer));
				printer->name = NS_NAME_DEFAULT;
				printer->attributes = NULL;
				printer->source = NS_SVC_ETC;
				dyn_ns_printer_put(printer);
				free(printer);
			}
		}
			
		status =  (sp_add_printer(lpi_p));
		if(( lpi_p->device != NULL) && 
			(strcmp(lpi_p->context,NS_SVC_ETC) !=0)&&
			(strcmp(lpi_p->printserver,lpi_p->printhost) == 0))
		{
			status = lp_add_remote_printer(lpi_p);
			return(status);
		}
		return(status);
	}
}


add_local_printer(LpInfo *lpi_p)
{
	int rc;
	int status = 0;

	/*
	** First verify that the printer name is syntactically correct. 
	*/

	status = valid_printer_name(lpi_p->printername);

	if(status ==0)
		return(PRINTER_ERR_PRINTER_NAME_ERROR);


	/*
	** Then verify that the printer name is unique.
	*/

	status = verify_unique_printer_name(lpi_p->printername);

	if (status != PRINTER_SUCCESS) {
		return (PRINTER_ERR_NON_UNIQUE);
	}
	/*
	** Check the comment for syntax.
	*/

	status = valid_description(lpi_p->comment);
	if (status == 0)
		return(PRINTER_ERR_COMMENT_ERROR);

	/*
	** Check the printer port for accuracy.
	*/

	status = valid_printerport(lpi_p->device);
	if (status == -1)
		return(PRINTER_ERR_PORT_NAME_ERROR);

	/*
	** Verify the printer type.
	*/

	status = valid_printertype(lpi_p->printertype);
	if(status ==-1)
		return(PRINTER_ERR_PRINTER_TYPE_ERROR);

	/*
	** Verify the name in the user access list.
	*/
	
	if(lpi_p->user_allow_list != NULL)
	{
		status = valid_printer_allow_list(lpi_p->user_allow_list);
		if(status == 0)
			return(PRINTER_ERR_NAME_LIST_ERROR);
	}


	/*
	* If the name service is NIS or NIS+ don't call the dyn_ns...
	* part of the code. We don't want to do it in the method runnin
	* on the remote server. When we return to the printer object, the
	* we add the NIS and NIS+ stuff through a add_remote_printer call
	* on the local host. This solves the inheritance of NIS and NIS+
	* attributes problem.
	*/
	if (((rc = lp_add_local_printer(lpi_p)) == PRINTER_SUCCESS) &&
	    (print_client_sw_installed_p() != 0)
		&& ( strcmp(lpi_p->context,NS_SVC_ETC) == 0))
		rc = sp_add_printer(lpi_p);

	/*
	** Now this is really ugly, but it has to be done. Since FNS evaluates
	** the printer stuff from /etc/printers.conf we have to back out the
	** addition of this printer to /etc/printers.conf so that add_remote
	** printer will function properly. This only happens when a printer
	** is installed locally and has named as its server the host which
	** it is being installed on.
	*/

	if((strcmp(lpi_p->context, NS_SVC_ETC) != 0)
		&& (strcmp(lpi_p->printserver,lpi_p->printhost) == 0))
	{
		{
			ns_printer_t *	printer;

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

