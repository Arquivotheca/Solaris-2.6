#ifndef lint
#pragma ident	 "@(#)upg_recover.c 1.12 95/09/25 SMI"
#endif
/*
 * Copyright (c) 1994, by Sun Microsystems, Inc.
 * All rights reserved.
 */
/*
 * Copyright (c) 1991 Sun Microsystems, Inc.  All Rights Reserved. Sun
 * considers its source code as an unpublished, proprietary trade secret, and
 * it is available only under strict license provisions.  This copyright
 * notice is placed here only to protect Sun in the event the source is
 * deemed a published work.  Dissassembly, decompilation, or other means of
 * reducing the object code to human readable form is prohibited by the
 * license agreement under which this code is provided to the user or company
 * in possession of this copy.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the Government
 * is subject to restrictions as set forth in subparagraph (c)(1)(ii) of the
 * Rights in Technical Data and Computer Software clause at DFARS 52.227-7013
 * and in similar clauses in the FAR and NASA FAR Supplement.
 */
#include "sw_lib.h"

/* Public Function Prototypes */

int		swi_partial_upgrade(void);
int		swi_resume_upgrade(void);

/* ******************************************************************** */
/*			PUBLIC SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

/*
 * partial_upgrade()
 * Parameters:
 *	none
 * Return:
 *
 * Status:
 *	public
 */
int
swi_partial_upgrade(void)
{
	char restart_file[MAXPATHLEN];
	char *root_path;
	struct stat	status;

	root_path = get_rootdir();

	if (*root_path == '\0') {
		strcpy(restart_file,
		    "/var/sadm/system/admin/upgrade_restart");
	} else {
		strcpy(restart_file, root_path);
		strcat(restart_file,
		    "/var/sadm/system/admin/upgrade_restart");
	}

	if (stat(restart_file, &status) == 0)
		return (1);
	else if (is_new_var_sadm("/") != 1) {
		/*
		 * In this case the restart file in the new location is not
		 * present, and we do not have the new var/sadm structure
		 * yet. (thus an interrupted upgrade from pre to post-KBI)
		 * Therefore we need to look in the old location for
		 * completeness. 
		 */
		if (*root_path == '\0') {
			strcpy(restart_file,
			    "/var/sadm/install_data/upgrade_restart");
		} else {
			strcpy(restart_file, root_path);
			strcat(restart_file,
			    "/var/sadm/install_data/upgrade_restart");
		}
	
		if (stat(restart_file, &status) == 0)
			return (1);
	}
	
	/*
	 * The restart file was not found, but there may be a backup laying
	 * around. Look for it.
	 */
	if (*root_path == '\0') {
		strcpy(restart_file,
		    "/var/sadm/system/admin/upgrade_restart.bkup");
	} else {
		strcpy(restart_file, root_path);
		strcat(restart_file,
		    "/var/sadm/system/admin/upgrade_restart.bkup");
	}
	if (stat(restart_file, &status) == 0)
		return (1);
	else if (is_new_var_sadm("/") != 1) {
		if (*root_path == '\0') {
			strcpy(restart_file,
			    "/var/sadm/install_data/upgrade_restart.bkup");
		} else {
			strcpy(restart_file, root_path);
			strcat(restart_file,
			    "/var/sadm/install_data/upgrade_restart.bkup");
		}

		if (stat(restart_file, &status) == 0)
			return (1);
	}
	
	return (0);
}

/*
 * resume_upgrade()
 * Parameters:
 *	none
 * Return:
 *
 * Status:
 *	public
 */
int
swi_resume_upgrade(void)
{
	char	cmd[MAXPATHLEN];
	int	status;
	struct stat	statStatus;
	char	restart_file[MAXPATHLEN];
	char	upg_script[MAXPATHLEN];
	char	upg_log[MAXPATHLEN];

	(void) sprintf(restart_file,
	    "%s/var/sadm/system/admin/upgrade_restart", get_rootdir());

	if (stat(restart_file, &statStatus) == 0) {
		(void) sprintf(upg_script,
		    "%s/var/sadm/system/admin/upgrade_script",
		    get_rootdir());
		(void) sprintf(upg_log,
		    "%s/var/sadm/system/logs/upgrade_log",
		    get_rootdir());
	} else {
		(void) sprintf(restart_file,
		    "%s/var/sadm/install_data/upgrade_restart", get_rootdir());
		(void) sprintf(upg_script,
		    "%s/var/sadm/install_data/upgrade_script", get_rootdir());
		(void) sprintf(upg_log,
		    "%s/var/sadm/install_data/upgrade_log", get_rootdir());
	}
	
	(void) sprintf(cmd, "/bin/mv %s %s.save", upg_log, upg_log);
	status = system(cmd);
	
	(void) sprintf(cmd, "/bin/sh %s %s restart 2>&1 | tee %s",
	    upg_script, get_rootdir(), upg_log);
	status = system(cmd);
			
	if (access("/tmp/diskette_rc.d/inst9.sh", X_OK) == 0) {
		(void) system("/sbin/sh /tmp/diskette_rc.d/inst9.sh");
	}
	return (status);
}
