/*
 * Copyright (c) 1994 by Sun Microsystems, Inc.
 * All rights reserved.
 */

#pragma	ident	"@(#)printer_view.c	1.18	95/09/27 SMI"


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <sys/systeminfo.h>
#include <sys/types.h>
#include "printer_impl.h"

#include <print/ns.h>


int
sp_view_printer(LpInfo *lpi_p)
{
	ns_printer_t 	*pobj;
	ns_bsd_addr_t 	*addr;
	char 		localhostname[SYS_NMLN];
	const char 	*printername;
	char 		*printer_name;
	int		err;
	

	if ((pobj = (ns_printer_t *)
	     dyn_ns_printer_get_name(lpi_p->printername, lpi_p->context)) == NULL)
		return (PRINTER_FAILURE);

	if ((addr = (ns_bsd_addr_t *)
	     dyn_ns_get_value(NS_KEY_BSDADDR, pobj)) == NULL) {
		dyn_ns_printer_destroy(pobj);
		return (PRINTER_FAILURE);
	}
	
	sysinfo(SI_HOSTNAME, localhostname, sizeof (localhostname));

	printername = lpi_p->printername;

	memset(lpi_p, 0, sizeof (*lpi_p));

	lpi_p->printername = printername;
	lpi_p->printertype = strdup("unknown");
	lpi_p->printserver= strdup(addr->server);
	lpi_p->file_contents = NULL;
	lpi_p->comment = (char *)
		dyn_ns_get_value_string(NS_KEY_DESCRIPTION, pobj);
	lpi_p->protocol = strdup("bsd");
	lpi_p->enable_p = B_TRUE;
	lpi_p->accept_p = B_TRUE;
	
	err = get_default_printer(&printer_name, lpi_p->context);

	if (err != PRINTER_SUCCESS) {
		return (PRINTER_ERR_GET_DEFAULT_FAILED);
	}
	else {
		if (printer_name != NULL &&
		    strcmp(printer_name, lpi_p->printername) == 0) {
			lpi_p->default_p = B_TRUE;
		} else {
			lpi_p->default_p = B_FALSE;
		}
	}
	return (PRINTER_SUCCESS);
}


int
lp_view_printer(LpInfo *lpi_p)
{

	int		err;
	int		i;
	FILE		*result_desc;
	char		workbuf[PRT_MAXSTRLEN];
	char		msgbuf[PRT_MAXSTRLEN];
	char		printer_space[256];
	char		*tmpstr;
	char		*view_arg, *printer_name;
	char		*remote_host = "";
	char		*device_name = "";
	boolean_t	remote_host_flag = B_FALSE;
	char		*domainname;
	char		localhostname[SYS_NMLN];
	char		*description;
	char		*temp_type = NULL;
	char		*temp_interface = NULL;
	const char 	*printername;
	const char 	*context;


	/*
	 * get the name of the system on which we are running
	 * in case we find out that the printer we are listing
	 * is locally connected
	 */

	sysinfo(SI_HOSTNAME, localhostname, sizeof (localhostname));

	/***
	 *** NOTE -- redirecting stderr to /dev/null causes some possible
	 *** error text to be discarded.  It used to be returned through
	 *** the framework as unformatted error text, but wasn't used by
	 *** printer mgr anyway (due to a bug) so we're not losing anything.
	 *** At some point this should be cleaned up so all output, both
	 *** stdout and stderr, is returned from this function through
	 *** passed in buffers.
	 ***/

	(void) sprintf(workbuf, "set -f ; /bin/env LC_ALL=C %s %s 2>/dev/null",
	    PRT_LPSTAT_VIEW, lpi_p->printername);

	result_desc = popen(workbuf, "r");

	if (result_desc == NULL) {
		return (PRINTER_ERR_SYSTEM_LPSTAT_FAILED);
	}

	printername = lpi_p->printername;
	context = lpi_p->context;

	memset(lpi_p, 0, sizeof (*lpi_p));

	lpi_p->printername = printername;
	lpi_p->context = context;
	
	/*
	** I need this context value that is passed in so that the 
	** "get_default_printer" function has the correct context value
	** passed in. Otherwise, it will use "etc" as the context and
	** possibly read the wrong information from /etc/printers.conf
	*/

	if(strcmp(lpi_p->context,NS_SVC_NISPLUS) == 0)
		lpi_p->context = NS_SVC_XFN;
	else
		lpi_p->context = context;

	while (fgets(msgbuf, PRT_MAXSTRLEN, result_desc) != 0) {

		if ((strlen(msgbuf) >= strlen(PRT_LISTING_PRINTER)) &&
		    (strncmp(msgbuf, PRT_LISTING_PRINTER,
		    strlen(PRT_LISTING_PRINTER)) == 0)) {

			/*
			 * we've found a printer
			 *
			 * Now we need to figure out if this is locally
			 * or remotely connected.
			 * For remotely connected printers, we
			 * go into the printer's configuration file
			 * and read out the "remote = " line
			 */

			err = get_host_or_device_name(lpi_p->printername,
				&remote_host, &device_name);

			if (err != 0) {
				return (PRINTER_FAILURE);
			}

			if ((remote_host != NULL)
			    && ((tmpstr = strchr(remote_host, '\n')) != NULL)) {
				*tmpstr = '\0';
			}
			if ((device_name != NULL)
			    && ((tmpstr = strchr(device_name, '\n')) != NULL)) {
				*tmpstr = '\0';
			}

			if (strcmp(remote_host, "") != 0) {
				/* found a remote host */
				lpi_p->printserver = strdup(remote_host);
				lpi_p->device = NULL;
				remote_host_flag = B_TRUE;	/* set flag */
			} else {
				/* locally connected */
				lpi_p->printserver = strdup(localhostname);
				lpi_p->device = strdup(device_name);
			}
			if (strstr(msgbuf, PRT_VIEW_ENABLED) != 0) {
				lpi_p->enable_p = B_TRUE;
			} else {
				lpi_p->enable_p = B_FALSE;
			}
			continue;
		}

		if (strstr(msgbuf, PRT_LISTING_DESCRIPTION)) {
			/*
			 * we've found a description for that printer
			 */
			tmpstr = strchr(msgbuf, ' ');
			view_arg = strdup(tmpstr + 1);
			if ((tmpstr = strchr(view_arg, '\n')) != NULL) {
				*tmpstr = '\0';
			}
			lpi_p->comment = view_arg;
			continue;
		}

		if (strstr(msgbuf, PRT_VIEW_PRINTER_TYPE)) {
			/*
			 * we've found the printer type
			 */
			tmpstr = strchr(msgbuf, ' ');
			tmpstr = strchr(tmpstr + 1, ' ');
			view_arg = strdup(tmpstr + 1);
			if ((tmpstr = strchr(view_arg, '\n')) != NULL) {
				*tmpstr = '\0';
			};
			temp_type = strdup(view_arg);
			continue;
		}

		if (strstr(msgbuf, PRT_VIEW_PRINTER_INTERFACE)) {
			/*
			 * we've found the interface
			 */
			tmpstr = strchr(msgbuf, ' ');
			view_arg = strdup(tmpstr + 1);
			if ((tmpstr = strchr(view_arg, '\n')) != NULL) {
				*tmpstr = '\0';
			};
			temp_interface = strdup(view_arg);
			continue;
		}

		if (strstr(msgbuf, PRT_VIEW_CONTENT_TYPE)) {
			/*
			 * we've found the file contents type
			 */
			tmpstr = strchr(msgbuf, ' ');
			tmpstr = strchr(tmpstr + 1, ' ');
			if (tmpstr != NULL) {
				view_arg = strdup(tmpstr + 1);
				if ((tmpstr = strchr(view_arg, '\n')) != NULL) {
					*tmpstr = '\0';
				}
			} else {
				view_arg = strdup("");
			}
			lpi_p->file_contents = view_arg;
			continue;
		}

		if (strstr(msgbuf, PRT_VIEW_USERS) != NULL &&
		    (remote_host_flag == B_FALSE)) {

			/*
			 * we've found the list of allowed users
			 * read the next line to get the list:
			 */

			tmpstr = fgets(msgbuf, PRT_MAXSTRLEN, result_desc);
			if (strstr(tmpstr, PRT_VIEW_USERS_ALL) != 0) {
				tmpstr = strchr(msgbuf, '(');
				view_arg = tmpstr + 1;
				i = strlen(view_arg) - 2;
				strncpy(workbuf, view_arg, i);
				workbuf[i] = '\0';
				lpi_p->user_allow_list = strdup(workbuf);
			} else if (strstr(tmpstr, PRT_VIEW_USERS_NONE) == 0) {
				/*
				 * we have a list of users, so need to
				 * read the file and concatenate them
				 * onto one line for sending back one
				 * parameter
				 */
				err = do_user_list(lpi_p->printername,
				    &view_arg);
				if (err != PRINTER_SUCCESS) {
					return (PRINTER_FAILURE);
				}

				lpi_p->user_allow_list = view_arg;
			} else {
				lpi_p->user_allow_list = strdup("");
			}
			continue;
		}

		if (strstr(msgbuf, PRT_VIEW_BANNER) &&
		    (remote_host_flag == B_FALSE)) {

			/*
			 * we've found whether or not a banner must be printed
			 */

			tmpstr = strchr(msgbuf, ' ');
			view_arg = tmpstr + 1;
			if (strncmp(view_arg,
			    PRT_BANNER_NOT_REQUIRED, 3) == 0) {
				lpi_p->banner_req_p = B_FALSE;
			} else {
				lpi_p->banner_req_p = B_TRUE;
			}
			continue;
		}

		if (strstr(msgbuf, PRT_VIEW_FAULT_NOT) &&
		    (remote_host_flag == B_FALSE)) {

			/*
			 * we've found what to do when printer faults
			 */

			if (strstr(msgbuf, PRT_FAULT_MAIL)) {
				lpi_p->notify = strdup("mail");
				continue;
			}
			if (strstr(msgbuf, PRT_FAULT_WRITE)) {
				lpi_p->notify = strdup("write");
				continue;
			}
			lpi_p->notify = NULL;
			continue;
		}
	}

	if (pclose(result_desc) != 0) {
		return (PRINTER_ERR_SYSTEM_LPSTAT_FAILED);
	}

	/* Now determine the print server's system type via "lpsystem -l" */

	if (remote_host_flag == B_TRUE) {
		(void) sprintf(workbuf,
		    "set -f ; /bin/env LC_ALL=C %s %s 2>/dev/null",
		    PRT_LPSYSTEM_L, remote_host);
	} else {

		/*
		 * The localhost might not have an entry into the Systems
		 * file; in this case, it's probably relying on the '+'
		 * entry, which in the default case is set to s5 protocol.
		 * We'll default to that here in case the popen to lpsystem
		 * doesn't find an entry for this host.
		 */

		lpi_p->protocol = strdup("s5");

		(void) sprintf(workbuf,
		    "set -f ; /bin/env LC_ALL=C %s %s 2>/dev/null",
		    PRT_LPSYSTEM_L, localhostname);
	}

	result_desc = popen(workbuf, "r");

	if (result_desc == NULL) {
		return (PRINTER_ERR_SYSTEM_LPSYSTEM_FAILED);
	}

	while (fgets(msgbuf, PRT_MAXSTRLEN, result_desc) != 0) {

		if (strstr(msgbuf, PRT_SYSTEM_TYPE)) {
			/*
			 * We've found the system type for the printer's
			 * server.
			 */

			tmpstr = strchr(msgbuf, ':') + 1;
			while (isspace((unsigned char)*tmpstr)) {
				tmpstr++;
			}
			view_arg = strdup(tmpstr);
			if ((tmpstr = strchr(view_arg, '\n')) != NULL) {
				*tmpstr = '\0';
			}
			if (strcmp(view_arg, PRT_VIEW_TYPE_BSD) == 0) {
				lpi_p->protocol = strdup("bsd");
			} else {
				lpi_p->protocol = strdup("s5");
			}

			break;
		}
	}

	/* consume remaining output from pipe */
	while (fgets(msgbuf, PRT_MAXSTRLEN, result_desc) != 0) ;

	(void) pclose(result_desc);


	if ((temp_type && (strcmp(temp_type, PRT_DEF_PRINTER_TYPE_PS) == 0)) &&
	    (temp_interface && (strcmp(temp_interface,
	    PRT_INTERFACE_NEWSPRINT) == 0))) {
		lpi_p->printertype = strdup(PRT_DEF_PRINTER_TYPE_NEWSPRINT);
	} else if (temp_type) {
		lpi_p->printertype = strdup(temp_type);
	}

	if (temp_type) {
		free(temp_type);
		temp_type = NULL;
	}
	if (temp_interface) {
		free(temp_interface);
		temp_interface = NULL;
	}

	err = get_default_printer(&printer_name, lpi_p->context);

	if (err != PRINTER_SUCCESS) {
		return (PRINTER_ERR_GET_DEFAULT_FAILED);
	}
	else {
		if (printer_name != NULL &&
		    strcmp(printer_name, lpi_p->printername) == 0) {
			lpi_p->default_p = B_TRUE;
		} else {
			lpi_p->default_p = B_FALSE;
		}
	}

	/*
	 * last, we need to figure out if this printer has
	 * rejected requests or not.
	 * We do this with the -a option to lpstat.
	 * but that gives back all printers, so we need
	 * to find the one we are viewing.
	 */

	(void) sprintf(workbuf, "set -f ; /bin/env LC_ALL=C %s",
	    PRT_LPSTAT_ACCEPT);

	result_desc = popen(workbuf, "r");

	if (result_desc == NULL) {
		return (PRINTER_ERR_SYSTEM_LPSTAT_FAILED);
	}

	/*
	 * In this output format, printername is followed by a space;
	 * we want to make sure that we search for the printername
	 * exactly, not just the first printer who's name contains
	 * this printer's name as a substring.
	 */

	sprintf(printer_space, "%s ", lpi_p->printername);

	while (fgets(msgbuf, PRT_MAXSTRLEN, result_desc) != 0) {
		if (strstr(msgbuf, printer_space) != 0) {
			/* found our printer */
			if (strstr(msgbuf, PRT_VIEW_REJECTED) != 0) {
				lpi_p->accept_p = B_FALSE;
			} else {
				lpi_p->accept_p = B_TRUE;
			}
			break;
		}
	}

	/* consume remaining command output from pipe on break */
	while (fgets(msgbuf, PRT_MAXSTRLEN, result_desc) != 0) ;

	(void) pclose(result_desc);

	return (PRINTER_SUCCESS);
}


view_printer(LpInfo *lpi_p)
{
	char buf[BUFSIZ];
	int rc;

	if (lpi_p == NULL || lpi_p->printername == NULL) {
		return (PRINTER_FAILURE);
	}

	sprintf(buf, "/etc/lp/printers/%s", lpi_p->printername);

	if ((print_client_sw_installed_p() == 0) || (access(buf, F_OK) == 0))
		return (lp_view_printer(lpi_p));
	else
		return (sp_view_printer(lpi_p));
}
