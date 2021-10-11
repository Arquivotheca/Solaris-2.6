#ifndef lint
#ident   "@(#)admin.c 1.10 95/02/10 SMI"
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

#include "sw_lib.h"

/* Public Function Prototypes */

char *	swi_admin_file(char *);
int     swi_admin_write(char *, Admin_file *);

/* Library Function Prototypes */

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
swi_admin_file(char * filename)
{
	static char	adminfile[MAXPATHLEN+1];

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("admin_file");
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
 *	admin_file().
 *	NOTE:	Data is not logged into the file if sw_debug is set.
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
 *	ERR_SAVE    - call to admin_file() to save 'filename' failed
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
	/* if not debugging, write the file */
	if (get_sw_debug() == 0) {
		fp = fopen(filename, "w");
		if (fp == (FILE *)0)
			return (ERR_INVALID);

		fprintf(fp, "mail=%s\n", admin->mail);
		fprintf(fp, "instance=%s\n", admin->instance);
		fprintf(fp, "partial=%s\n", admin->partial);
		fprintf(fp, "runlevel=%s\n", admin->runlevel);
		fprintf(fp, "idepend=%s\n", admin->idepend);
		fprintf(fp, "rdepend=%s\n", admin->rdepend);
		fprintf(fp, "space=%s\n", admin->space);
		fprintf(fp, "setuid=%s\n", admin->setuid);
		fprintf(fp, "conflict=%s\n", admin->conflict);
		fprintf(fp, "action=%s\n", admin->action);
		fprintf(fp, "basedir=%s\n", admin->basedir);
		(void) fclose(fp);
	}
	/* set pointer to adminfile for future use */
	if(admin_file(filename) == NULL)
		return(ERR_SAVE);

	return (SUCCESS);
}
