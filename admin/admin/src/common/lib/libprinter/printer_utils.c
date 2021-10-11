/*
 * Copyright (c) 1992 by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)printer_utils.c	1.27	95/09/27 SMI"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <dirent.h>
#include <sys/dirent.h>
#include <sys/stat.h>
#include <libintl.h>
#include "printer_impl.h"

#include <dlfcn.h>
#include <print/ns.h>


static void *handle = NULL;

int
print_client_sw_installed_p(void)
{
	if ((handle == NULL) &&
	    ((handle = dlopen(PRINT_CLIENT_SW_SHARED_LIB,
			     RTLD_LAZY|RTLD_GLOBAL)) == NULL))
		return (0);
	else
		return (1);
}


/*
 * The following routines wrap the routines avaliable in the Print Client
 * library.  This is done to reduce dependencies during compilation and
 * link time.  I'm sure there are some magic compiler options to handle this
 */
static void *
dyn_func(const char *name)
{
	if (handle == NULL)
		return (NULL);

	return (dlsym(handle, name));
}

int
dyn_list_iterate()
{
	static int (*func)(void **, void *, ...) = NULL;

	if ((func != NULL) ||
	    ((func = (int (*)(void **, void *, ...))
		      dyn_func("list_iterate")) != NULL))
		return ((int)func);

	return (NULL);


}

ns_printer_t *
dyn_ns_printer_get_name(const char *name, const char *context)
{
	static ns_printer_t *(*func)(const char *, const char *) = NULL;

	if ((func != NULL) ||
	    ((func = (ns_printer_t *(*)(const char *, const char *))
		      dyn_func("ns_printer_get_name")) != NULL))
		return ((func)(name, context));

	return (NULL);
}

ns_printer_t **
dyn_ns_printer_get_list(const char *context)
{
	static ns_printer_t **(*func)() = NULL;

	if ((func != NULL) ||
	    ((func = (ns_printer_t **(*)())
	      dyn_func("ns_printer_get_list")) != NULL))
		return ((func)(context));

	return (NULL);
}

int
dyn_ns_printer_put(ns_printer_t *printer)
{
	static int (*func)(ns_printer_t *) = NULL;

	if ((func != NULL) ||
	    ((func = (int (*)(ns_printer_t *))
	      dyn_func("ns_printer_put")) != NULL))
		return ((func)(printer));

	return (-1);
}

void
dyn_ns_printer_destroy(ns_printer_t *printer)
{
	static void (*func)(ns_printer_t *) = NULL;

	if ((func != NULL) ||
	    ((func =(void (*)(ns_printer_t *))
	      dyn_func("ns_printer_destroy")) != NULL))
		(func)(printer);

	return ;
}

int
dyn_ns_set_value(const char *key, const void *value, ns_printer_t *printer)
{
	static int (*func)(const char *, const void *, ns_printer_t *) = NULL;

	if ((func != NULL) ||
	    ((func = (int (*)(const char *, const void *, ns_printer_t *))
	      dyn_func("ns_set_value")) != NULL))
		return ((func)(key, value, printer));

	return (-1);
}

int
dyn_ns_set_value_from_string(
	const char	*key,
	const char	*value,
	ns_printer_t	*printer)
{
	static int (*func)(const char *, const char *, ns_printer_t *) = NULL;

	if ((func != NULL) ||
	    ((func = (int (*)(const char *, const char *, ns_printer_t *))
	      dyn_func("ns_set_value_from_string")) != NULL))
		return ((func)(key, value, printer));

	return (-1);
}

void *
dyn_ns_get_value(const char *key, ns_printer_t *printer)
{
	static void *(*func)(const char *, ns_printer_t *) = NULL;

	if ((func != NULL) ||
	    ((func = (void *(*)(const char *, ns_printer_t *))
	      dyn_func("ns_get_value")) != NULL))
		return ((func)(key, printer));

	return (NULL);
}

char *
dyn_ns_get_value_string(const char *key, ns_printer_t *printer)
{
	static char *(*func)(const char *, ns_printer_t *) = NULL;

	if ((func != NULL) ||
	    ((func = (char *(*)(const char *, ns_printer_t *))
	      dyn_func("ns_get_value_string")) != NULL))
		return ((func)(key, printer));

	return (NULL);
}



int
verify_unique_printer_name(const char *printername)
{

	struct stat	stat_buf;
	char		dirname[PATH_MAX];
	char		msgbuf[PRT_MAXSTRLEN];
	FILE		*result_desc;
	char		*tmpstr;


	/*
	 * The "public" interface to discovering the names of printers
	 * already known on the system is via the "lpstat" program.
	 * However, we're going to cheat here and poke around the system
	 * directly.  If for some reason this breaks in the future,
	 * change this code back to doing a popen("lpstat -L -p") and
	 * looking for the printer name.
	 */

	/*
	 * If the unbundled printer client product software is installed,
	 * we'd have to look in the nameservice to see what printer names
	 * are already in use; we don't know how to do that right now.
	 * However, if it's not installed, just look for the directory
	 * /etc/lp/printers/<printername>; if it exists, the printer is
	 * known to the system, otherwise the name is free for the taking.
	 */

	if (print_client_sw_installed_p() == 0) {
		(void) sprintf(dirname, "/etc/lp/printers/%s", printername);
		if (stat(dirname, &stat_buf) == 0) {
			return (PRINTER_ERR_NON_UNIQUE);
		} else {
			/* yes, we verified that it's unique */
			return (PRINTER_SUCCESS);
		}
	} else {

		/*
		 * print client software is installed.  The nameservice
		 * can tell us if the printer name is in use, but I don't
		 * know how to do that right now, so just do the old
		 * lpstat call ...
		 */
		ns_printer_t *printer = NULL;

		if ((printer = dyn_ns_printer_get_name(printername, NULL)) != NULL) {
			dyn_ns_printer_destroy(printer);
			return (PRINTER_ERR_NON_UNIQUE);
		} else
			return (PRINTER_SUCCESS);
	}
}


/*
 * This subroutine obtains the default printer
 * name, if any
 */
int
get_default_printer(char **printer, const char *context)
{

	char msgbuf[PRT_MAXSTRLEN];
	FILE *result_desc;
	char *tmpstr;
	char *tmpptr;
	int len;


        /*
         * Get the default printer for this system:
         */

	/* another quick cheat, see "verify_unique_printer_name" comments */

	if (print_client_sw_installed_p() == 0) {
		if ((result_desc = fopen("/etc/lp/default", "r")) == NULL) {
			*printer = NULL;
			return (PRINTER_SUCCESS);
		} else {
			if (fgets(msgbuf, sizeof (msgbuf),
			    result_desc) != NULL) {
				/* lose newline */
				msgbuf[strlen(msgbuf) -1] = '\0';
				*printer = strdup(msgbuf);
				(void) pclose(result_desc);
				return (PRINTER_SUCCESS);
			} else {
				*printer = NULL;
				(void) pclose(result_desc);
				return (PRINTER_FAILURE);
			}
		}
	} else {
		ns_printer_t *pobj = NULL;

		*printer = NULL;

		if ((pobj = dyn_ns_printer_get_name(NS_NAME_DEFAULT,
						    context)) != NULL) {
			if ((*printer = dyn_ns_get_value_string(NS_KEY_USE, pobj))
			    != NULL) {
				ns_bsd_addr_t *addr;
				if (((addr = dyn_ns_get_value(NS_KEY_BSDADDR,
							  pobj)) != NULL) &&
				    (addr->printer != NULL))
					*printer = strdup(addr->printer);
			}
		}

		return (PRINTER_SUCCESS);
	}
}




/*
 * do_list_devices - subroutine to list the devices available
 * for connecting printers
 */

int
do_list_devices(char ***ports)
{

	int		i;
	int		more;
	int		pnum;
	char		workbuf[PRT_MAXSTRLEN];
	DIR		*dirptr;
	dirent_t	*direntptr;
	int		num_ports;
	struct stat	stat_buf;


	if (ports == NULL) {
		return (PRINTER_FAILURE);
	}

	dirptr = opendir(PRT_SERIAL_PORT_DIRECTORY);

	if (dirptr == NULL) {
		return (PRINTER_ERR_DIR_OPEN_FAILED);
	}

	num_ports = 0;

	while ((direntptr = readdir(dirptr)) != NULL) {
		num_ports++;
	}

	/* subtract 2 for "." and ".." */

	num_ports -= 2;

	rewinddir(dirptr);

	*ports = (char **)malloc((unsigned)(num_ports * sizeof (char *)));

	if (*ports == NULL) {
		return (PRINTER_ERR_MALLOC_FAILED);
	}

	i = 0;

	while ((direntptr = readdir(dirptr)) != NULL) {
		if ((strcmp(direntptr->d_name, ".") != 0) &&
		    (strcmp(direntptr->d_name, "..") != 0)) {

			(void) sprintf(workbuf, PRT_SERIAL_PORT_DIRECTORY"/%s",
			    direntptr->d_name);
			(*ports)[i++] = strdup(workbuf);
		}
	}

	(void) closedir(dirptr);

	more = 0;

	/* Check for SunPics /dev/lpvi */
	(void) sprintf(workbuf, PRT_DEVICE_DIRECTORY"/%s", "lpvi");
	if (stat(workbuf, &stat_buf) == 0) {
		more++;
	}

	/* check for sparc parallel ports */
	for ( pnum = 0; pnum <= 9; pnum++ ) {
		(void) sprintf(workbuf, PRT_DEVICE_DIRECTORY"/%s%d",
		    "bpp", pnum);
		if (stat(workbuf, &stat_buf) == 0) {
			more++;
		}
	}

	/* check for intel parallel ports */
	for ( pnum = 0; pnum <= 9; pnum++ ) {
		(void) sprintf(workbuf, PRT_DEVICE_DIRECTORY"/%s%d",
		    "lp", pnum);
		if (stat(workbuf, &stat_buf) == 0) {
			more++;
		}
	}

	if (more != 0) {

		*ports = (char **)realloc((void *)*ports,
		    (unsigned)((num_ports + more) * sizeof (char *)));

		if (*ports == NULL) {
			return (PRINTER_ERR_MALLOC_FAILED);
		}

		/* Check for SunPics /dev/lpvi */
		(void) sprintf(workbuf, PRT_DEVICE_DIRECTORY"/%s", "lpvi");
		if (stat(workbuf, &stat_buf) == 0) {
			(*ports)[num_ports++] = strdup(workbuf);
		}

		/* check for sparc parallel ports */
		for ( pnum = 0; pnum <= 9; pnum++ ) {
			(void) sprintf(workbuf, PRT_DEVICE_DIRECTORY"/%s%d",
			    "bpp", pnum);
			if (stat(workbuf, &stat_buf) == 0) {
				(*ports)[num_ports++] = strdup(workbuf);
			}
		}

		/* check for intel parallel ports */
		for ( pnum = 0; pnum <= 9; pnum++ ) {
			(void) sprintf(workbuf, PRT_DEVICE_DIRECTORY"/%s%d",
			    "lp", pnum);
			if (stat(workbuf, &stat_buf) == 0) {
				(*ports)[num_ports++] = strdup(workbuf);
			}
		}
	}

	return (num_ports);
}


/* free a list allocated by do_list_devices() */

void
do_free_devices(char **ports, int cnt)
{

	int	i;


	if (ports != NULL) {
		for (i = 0; i < cnt; i++) {
			free((void *)ports[i]);
		}
		free((void *)ports);
	}
}


/*ARGSUSED*/
int
do_user_list(char *printername, char **user_list)
{
	FILE *fp;
	char tmpstr[PRT_MAXSTRLEN];
	char *argptr, *nexttmp;
	int char_amount = 0;
	int current_amount = 0;
	int num_of_allocs = 1;


	(void) sprintf(tmpstr, "%s/%s/users.allow", PRT_PRINTER_PATH,
	    printername);

	if ((fp = fopen(tmpstr, "r")) == NULL) {
		return (PRINTER_ERR_OPEN_USERS_FAILED);
	}

	if ((argptr = malloc(PRT_MAXSTRLEN)) == 0) {
		return (PRINTER_ERR_MALLOC_FAILED);
	}
	*argptr = '\0';

	while (fgets(tmpstr, sizeof(tmpstr), fp) != NULL) {
		char_amount = strlen(tmpstr);
		if (char_amount+current_amount >= num_of_allocs) {
			num_of_allocs++;
			if ((nexttmp = realloc(argptr, PRT_MAXSTRLEN *
			    num_of_allocs)) == 0) {
				return (PRINTER_ERR_MALLOC_FAILED);
			}
			argptr = nexttmp;
		}
		current_amount += char_amount;
		(void) strncat(argptr, tmpstr, strlen(tmpstr)-1);
		(void) strcat(argptr, ",");
	}

	*user_list = argptr;

	(void) fclose(fp);

	return (PRINTER_SUCCESS);
}


/*
 * This subroutine determines if the given printername
 * is connected to the local system (on which this method
 * is running) or a remote system.
 * It does this using lpstat -s
 */
int
get_host_or_device_name(
	const char	*printername,
	char		**hostname,
	char		**devname)
{

	char msgbuf[PRT_MAXSTRLEN];
	char tmpbuf[PRT_MAXSTRLEN];
	char printer_colon[256];
	FILE *result_desc;
	char *tmpstr;
	char *prtptr;
	char *sysptr;
	char *endptr;
        

	sprintf(printer_colon, "%s:", printername);

	(void) sprintf(msgbuf, "set -f ; /bin/env LC_ALL=C %s",
	    PRT_LPSTAT_LIST_DEVS);

	result_desc = popen(msgbuf, "r");

	if (result_desc == NULL) {
		return (PRINTER_ERR_PIPE_FAILED);
	}

	while (fgets(msgbuf, PRT_MAXSTRLEN, result_desc) != 0) {

		if (strstr(msgbuf, printer_colon)) {
			/*
			 * we've found the printer in question
			 */
			if (strstr(msgbuf, PRT_REMOTE_KEYWORD)) {
				/*
				 * it is a remote printer
				 */

				if (strstr(msgbuf, PRT_AS_PRINTER_KEYWORD)) {
					sysptr = strchr(msgbuf, ':') + 2;
					endptr = strchr(sysptr, ' ');

					strncpy(tmpbuf, sysptr,
					    endptr - sysptr);
					tmpbuf[endptr - sysptr] = 0;

					strcat(tmpbuf, "!");

					prtptr = strchr(msgbuf, '(') + 12;
					endptr = strchr(msgbuf, ')');

					strncat(tmpbuf, prtptr,
					    endptr - prtptr);

					*hostname = strdup(tmpbuf);
				} else {
					sysptr = strchr(msgbuf, ':');
					*hostname = strdup(sysptr + 2);
				}
				break;
			} else {
				if (strstr(msgbuf, PRT_LOCAL_KEYWORD)) {
					/*
					 * it is locally connected
					 */	
					sysptr = strchr(msgbuf, ':');
					*devname = strdup(sysptr + 2);
					break;
				}
			}
		}
	}

	/* consume rest of output from command */
	while (fgets(msgbuf, PRT_MAXSTRLEN, result_desc) != 0) ;

	(void) pclose(result_desc);
	return (PRT_SUCCESS);
}
