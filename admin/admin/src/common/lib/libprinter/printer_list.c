/*
 * Copyright (c) 1994 by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)printer_list.c	1.11	95/09/27 SMI"


#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <dirent.h>
#include <sys/systeminfo.h>
#include "printer_impl.h"

#include <stdarg.h>
#include <print/ns.h>
#include <print/list.h>


static int plus_one(void) { return (1); }

static int
convert_printer(ns_printer_t *val, va_list ap)
{
        LpInfo **lpi_pp = va_arg(ap, LpInfo **);
        int *i = va_arg(ap, int *);
        char *localhost = va_arg(ap, char *);
        ns_bsd_addr_t   *addr = dyn_ns_get_value(NS_KEY_BSDADDR, val);
        char *printer;

	if ((strcmp(val->name, NS_NAME_DEFAULT) == 0) ||
	    (strcmp(val->name, NS_NAME_ALL) == 0) ||
	    (addr == NULL))
		return (0);
 
        printer = (addr->printer ? addr->printer : val->name);
        if (strcmp(val->name, NS_NAME_DEFAULT) == 0)
                (*lpi_pp)[*i].printername = strdup(printer);
        else
                (*lpi_pp)[*i].printername = strdup(val->name);
        (*lpi_pp)[*i].printserver = strdup(addr->server);
 
        (*lpi_pp)[*i].comment = (char *)dyn_ns_get_value(NS_KEY_DESCRIPTION, val);
        (*i)++;
	return (1);
}




static int
sp_list_printers(LpInfo **lpi_pp, char *context)
{
	int num_printers = 0;
	ns_printer_t **printers = dyn_ns_printer_get_list(context);

	if (printers != NULL) {
		int i = 0;
		char localhost[SYS_NMLN];
		int (*func)(void **, VFUNC_T, ...);

		func = (int (*)(void **, VFUNC_T, ...))dyn_list_iterate();

		sysinfo(SI_HOSTNAME, localhost, sizeof (localhost));
		num_printers = (func)((void **)printers,
					    (VFUNC_T)plus_one);

		*lpi_pp = (LpInfo *)malloc(num_printers * sizeof (LpInfo));
		memset((void *)*lpi_pp, '\0', num_printers * sizeof (LpInfo));
	
		num_printers = (func)((void **)printers,
					    (VFUNC_T)convert_printer,
					    lpi_pp, &i, localhost);
	}

	return (num_printers);
}


static
int
cheat_list_printers(LpInfo **lpi_pp)
{

	int		i;
	int		num_printers;
	DIR		*dirp;
	dirent_t	*dirent_p;
	FILE		*fp;
	char		localhostname[SYS_NMLN];
	char		fname[PATH_MAX];
	char		buf[PRT_MAXSTRLEN];


	if (lpi_pp == NULL) {
		return (PRINTER_FAILURE);
	}

	if ((dirp = opendir("/etc/lp/printers")) == NULL) {
		return (PRINTER_ERR_DIR_OPEN_FAILED);
	}

	num_printers = 0;
	while ((dirent_p = readdir(dirp)) != NULL) {
		num_printers++;
	}

	/* subtract 2 for "." and ".." */

	num_printers -= 2;

	if (num_printers == 0) {
		*lpi_pp = NULL;
		return (0);
	}

	*lpi_pp = (LpInfo *)malloc(num_printers * sizeof (LpInfo));

	if (*lpi_pp == NULL) {
		return (PRINTER_ERR_MALLOC_FAILED);
	}

	memset((void *)*lpi_pp, '\0', num_printers * sizeof (LpInfo));

	rewinddir(dirp);

	sysinfo(SI_HOSTNAME, localhostname, sizeof (localhostname));

	i = 0;

	while ((dirent_p = readdir(dirp)) != NULL) {
		if (strcmp(dirent_p->d_name, ".") != 0 &&
		    strcmp(dirent_p->d_name, "..") != 0) {

			/* found a printer */

			(*lpi_pp)[i].printername = strdup(dirent_p->d_name);

			sprintf(fname, "/etc/lp/printers/%s/configuration",
			    dirent_p->d_name);

			(*lpi_pp)[i].printserver = NULL;

			if ((fp = fopen(fname, "r")) != NULL) {

				/* look for "Remote:" */

				while (fgets(buf, PRT_MAXSTRLEN, fp) != 0) {
					if (strncmp(buf, "Remote: ", 8) == 0) {

						/* lose the newline*/
						buf[strlen(buf) - 1] = '\0';

						(*lpi_pp)[i].printserver =
						    strdup(buf + 8);

						break;
					}
				}
			}

			(void) fclose(fp);

			if ((*lpi_pp)[i].printserver == NULL) {
				(*lpi_pp)[i].printserver =
				    strdup(localhostname);
			}

			sprintf(fname, "/etc/lp/printers/%s/comment",
			    dirent_p->d_name);

			if ((fp = fopen(fname, "r")) != NULL) {

				/* read comment */

				if (fgets(buf, PRT_MAXSTRLEN, fp) != 0) {
					/* lose the newline*/
					buf[strlen(buf) - 1] = '\0';

					(*lpi_pp)[i].comment = strdup(buf);
				}
			}

			(void) fclose(fp);

			i++;
		}
	}

	closedir(dirp);

	return (num_printers);
}


int
list_printers(LpInfo **lpi_pp, char *context)
{

	int err;
	FILE *result_desc;
	char workbuf[PRT_MAXSTRLEN];
	char msgbuf[PRT_MAXSTRLEN];
	char *tmpstr;
	char *description, *printer_name;
	char *remote_host = "";
	char localhostname[SYS_NMLN];


	if (print_client_sw_installed_p() == 0) {

		/*
		 * Print client software is NOT installed, let's
		 * cheat and go straight to the filesystem for
		 * printer info ...
		 */

		err = cheat_list_printers(lpi_pp);
		return (err);
	} else {
		err = sp_list_printers(lpi_pp, context);
		return (err);
	}

#ifdef NOTDEF
	/*
	 * get the name of the system on which we are running
	 * in case we find out that the printer we are listing
	 * is locally connected
	 */

	sysinfo(SI_HOSTNAME, localhostname, sizeof(localhostname));

	sprintf(workbuf, "set -f ; /bin/env LC_ALL=C %s", PRT_LPSTAT_LIST);
	result_desc = popen(workbuf, "r");

	if (result_desc == NULL) {
		return (PRINTER_ERR_PIPE_FAILED);
	}

	while (fgets(msgbuf, PRT_MAXSTRLEN, result_desc) != 0) {

		if ((strstr(msgbuf, PRT_LISTING_PRINTER)) &&
			(strstr(msgbuf, PRT_LISTING_DESCRIPTION) == 0) &&
			(strstr(msgbuf, PRT_LISTING_BOGUS_PRT) == 0)) {

			/* we've found a printer */

			tmpstr = strchr(msgbuf, ' ');
			printer_name = strtok(tmpstr + 1, " ");

			adm_args_set(PRT_PRINTER_NAME_PAR, ADM_STRING,
				strlen(printer_name), printer_name);
			/*
			 * Now we need to figure out if this is locally
			 * or remotely connected.
			 * For remotely connected printers, we
			 * go into the printer's configuration file
			 * and read out the "remote = " line
			 */

			err = get_host_name(printer_name, &remote_host);
			if (err != 0) {
				/* error already reported */
				adm_err_cleanup(ADM_NOCATS, ADM_FAILCLEAN);
				exit(ADM_FAILURE);
			}
			if (strcmp(remote_host, "") == 0) {
				adm_args_set(PRT_PRINTER_HOST_PAR, ADM_STRING,
					strlen(localhostname), localhostname);
			} else {
				/*
				 * XXXXXX
				 * found a remote host, fgets includes the
				 * \n char, so don't let the framework
				 * return it...
				 * XXXXXX
				 */
				adm_args_set(PRT_PRINTER_HOST_PAR, ADM_STRING,
					(strlen(remote_host)-1), remote_host);
			}
			continue;
		}
		if (strstr(msgbuf, PRT_LISTING_DESCRIPTION)) {
			/* we've found a description for that printer */
			tmpstr = strchr(msgbuf, ' ');
			description = tmpstr+1;
			adm_args_set(PRT_DESCRIPTION_PAR, ADM_STRING,
				(strlen(description)-1), description);
			/*
			 * this code assumes that the description always
			 * follows the actual printer and precedes the next
			 * printer.
			 */
			adm_args_markr();
			continue;
		}
	}

	(void) pclose(result_desc);

timestamp("all done, even pclose, now will exit", "", TS_ELAPSED);

	exit(ADM_SUCCESS);

#endif

#ifdef lint
	return (0);
#endif
}
