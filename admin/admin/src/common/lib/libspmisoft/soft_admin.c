#ifndef lint
#pragma ident "@(#)soft_admin.c 1.2 95/12/04 SMI"
#endif  lint
/*
 * Copyright (c) 1991 Sun Microsystems, Inc.  All Rights Reserved.
 * Sun considers its source code as an unpublished, proprietary
 * trade secret, and it is available only under strict license
 * provisions.  This copyright notice is placed here only to protect
 * Sun in the event the source is deemed a published work.  Dissassembly,
 * decompilation, or other means of reducing the object code to human
 * readable form is prohibited by the license agreement under which
 * this code is provided to the user or company in possession of this
 * copy.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in subparagraph
 * (c)(1)(ii) of the Rights in Technical Data and Computer Software
 * clause at DFARS 52.227-7013 and in similar clauses in the FAR and
 * NASA FAR Supplement.
 */

#include "spmisoft_lib.h"
#include <string.h>
#include <stdlib.h>

/* Public Function Prototypes */

char *	swi_getset_admin_file(char *);
int     swi_admin_write(char *, Admin_file *);

/* Library Function Prototypes */

void		_setup_admin_file(Admin_file *);
void		_setup_pkg_params(PkgFlags *);
int		_build_admin(Admin_file *);

/* Local Function Prototypes */

/* ******************************************************************** */
/*			PUBLIC SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

/*
 * admin_file()
 * 	Get/set name of current admin file used during pkgadd/pkgrm. If
 *	'filename' is NULL, get the name of the admin file, otherwise, set
 *	the name of the admin file to 'filename'.
 * Parameters:
 *	filename  - pathname of admin file (for setting only)
 * Return:
 *	NULL	  - default return value for set
 *	char *	  - return value for get; name of admin file
 * Status:
 *	public
 */
char *
swi_getset_admin_file(char * filename)
{
	static char	adminfile[MAXPATHLEN+1];

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("getset_admin_file");
#endif

	if (filename != (char *) NULL)
		(void) strcpy((char *) adminfile, filename);

	if (adminfile[0] == '\0')
		return ((char *) NULL);

	return ((char *) adminfile);
}

/*
 * admin_write()
 *	Writes the data contained in 'admin' to the admin file. If 'filename'
 *	is NULL, a temporary name (/tmp/pkg*) is created. 'filename' (or the
 *	temporary file name) is made the default admin_file name via
 *	getset_admin_file().
 *	NOTE:	Data is not logged into the file if execution
 *		simulation is set
 * Parameters:
 *	filename    - user supplied file name to use for admin file (NULL if
 *		      a temporary file is desired)
 *	admin	    - pointer to structure continaing admin file data to be
 *		      stored
 * Return:
 *	SUCCESS	    - successful write to admin file
 *	ERR_INVALID - 'filename' can't be opened for writing
 *	ERR_NOFILE  - 'filename' was NULL and a temporary filename could not
 *		      be created
 *	ERR_SAVE    - call to getset_admin_file() to save 'filename' failed
 * Status:
 *	public
 */
int
swi_admin_write(char * filename, Admin_file * admin)
{
	FILE	*fp;
	static char tmpname[] = "/tmp/pkgXXXXXX";

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("admin_write");
#endif

	if (filename == (char *)NULL) {
		(void) mktemp(tmpname);
		if (tmpname[0] == '\0')
			return (ERR_NOFILE);

		filename = tmpname;
	}
	/* if not simulating execution, write the file */
	if (!GetSimulation(SIM_EXECUTE)) {
		fp = fopen(filename, "w");
		if (fp == (FILE *)0)
			return (ERR_INVALID);

		(void) fprintf(fp, "mail=%s\n", admin->mail);
		(void) fprintf(fp, "instance=%s\n", admin->instance);
		(void) fprintf(fp, "partial=%s\n", admin->partial);
		(void) fprintf(fp, "runlevel=%s\n", admin->runlevel);
		(void) fprintf(fp, "idepend=%s\n", admin->idepend);
		(void) fprintf(fp, "rdepend=%s\n", admin->rdepend);
		(void) fprintf(fp, "space=%s\n", admin->space);
		(void) fprintf(fp, "setuid=%s\n", admin->setuid);
		(void) fprintf(fp, "conflict=%s\n", admin->conflict);
		(void) fprintf(fp, "action=%s\n", admin->action);
		(void) fprintf(fp, "basedir=%s\n", admin->basedir);
		(void) fclose(fp);
	}
	/* set pointer to adminfile for future use */
	if(getset_admin_file(filename) == NULL)
		return(ERR_SAVE);

	return (SUCCESS);
}


/*
 * Function:	_build_admin
 * Description:	Create the admin file for initial install only.
 * Scope:	Internal
 * Parameters:	admin	- non-NULL pointer to an Admin_file structure
 * Return:	NOERR	- success
 *		ERROR	- setup attempt failed
 */
int
_build_admin(Admin_file *admin)
{
	static char	_lbase[MAXPATHLEN] = "";

	/* verify admin is valid */
	if (admin == NULL)
		return (ERROR);

	/* if the basedir hasn't changed, return success */
	if (admin->basedir != NULL &&
			strcmp(admin->basedir, _lbase) == 0)
		return (NOERR);

	/* create and save admin file */
	if (admin_write(getset_admin_file((char *)NULL), admin))
		return (ERROR);

	if (admin->basedir != NULL)
		(void) strcpy(_lbase, admin->basedir);

	return (NOERR);
}

/*
 * Function:	_setup_admin_file
 * Description:	Initialize the fields of an existing admin structure
 * Scope:	internal
 * Parameters:	admin	- non-NULL pointer to the Admin structure
 *			  to be initialized
 * Return:	none
 */
void
_setup_admin_file(Admin_file *admin)
{
	static char 	nocheck[] = "nocheck";
	static char 	unique[] = "unique";
	static char 	quit[] = "quit";
	static char 	blank[] = " ";

	if (admin != NULL) {
		admin->mail = blank;
		admin->instance = unique;
		admin->partial = nocheck;
		admin->runlevel = nocheck;
		admin->idepend = nocheck;
		admin->rdepend = quit;
		admin->space = nocheck;
		admin->setuid = nocheck;
		admin->action = nocheck;
		admin->conflict = nocheck;
		admin->basedir = blank;
	}
}

/*
 * Function:	_setup_pkg_params
 * Description:	Initialize the package params structure to be used
 *		during pkgadd calls.
 * Scope:	internal
 * Parameters:	params	- non-NULL pointer to the PkgFlags structure to be
 *			  initialized
 * Return:	none
 */
void
_setup_pkg_params(PkgFlags *params)
{
	if (params != NULL) {
		params->silent = 1;
		params->checksum = 1;
		params->notinteractive = 1;
		params->accelerated = 1;
		params->spool = NULL;
		params->admin_file = (char *)getset_admin_file(NULL);
		params->basedir = get_rootdir();
	}
}
