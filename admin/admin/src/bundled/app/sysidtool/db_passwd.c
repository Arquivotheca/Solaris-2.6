/*
 *  Copyright (c) 1991 Sun Microsystems, Inc.  All Rights Reserved.
 *  All rights reserved.
 */

#pragma	ident	"@(#)db_passwd.c	1.1	94/11/29 SMI"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/param.h>

#include "cl_database_parms.h"
#include "sysidtool.h"
#include "admldb.h"

#define	ROOT_NAME	"root"
#define	ROOT_UID	"0"
#define	ROOT_GID	"1"
#define	ROOT_PATH	"/"

extern	void	free_table(Table *);

/*
 * Create the password entry for the root user.
 *
 *  Input: Encrypted root password.
 *
 *  Output: If successful, this routine returns 0.  If an error occcurs,
 *	    this routine returns -1, and writes an appropriate error
 *	    message at the location pointed to by errmess.
 */
int
set_root_password(e_passwd, errmess)
	char		*e_passwd, *errmess;
{
	char		*name, *pw, *uid, *gid, *gcos, *path, *shell,
			*last, *min, *max, *warn, *inactive, *expire, *flag;
	int		ret_stat;
	Table		*tbl;
	Db_error	*db_err;

	/*
	 * See if there is an existing entry for the root user.
	 * Use as many existing field values as possible when
	 * setting up the root password entry.
	 */

	(void) fprintf(debugfp, "set_root_password\n");

	tbl = table_of_type(DB_PASSWD_TBL);
	if (testing) {
		ret_stat = (*sim_handle()) (SIM_DB_LOOKUP, UFS, PASSWD_TBL,
		    ROOT_NAME, &name, &pw, &uid,
		    &gid, &gcos, &path, &shell, &last, &min,
		    &max, &warn, &inactive, &expire, &flag);
	} else {
		ret_stat = lcl_list_table(DB_NS_UFS, NULL, NULL,
		    DB_DISABLE_LOCKING | DB_LIST_SHADOW | DB_LIST_SINGLE,
		    &db_err, tbl,
		    ROOT_NAME, &name, &pw, &uid,
		    &gid, &gcos, &path, &shell, &last, &min,
		    &max, &warn, &inactive, &expire, &flag);
		if (ret_stat == -1)
			strcpy(errmess, db_err->msg);
	}

	(void) fprintf(debugfp, "status %d\n", ret_stat);
	if (ret_stat == -1)
		(void) fprintf(debugfp, "message %s\n", errmess);

	if (ret_stat != 0 || gid == NULL)
		gid = ROOT_GID;

	if (ret_stat != 0 || path == NULL)
		path = ROOT_PATH;

	/* Set up the new password entry */

	if (testing) {
		ret_stat = (*sim_handle()) (SIM_DB_MODIFY, UFS,
		    DB_PASSWD_TBL, errmess,
		    ROOT_NAME,  &name, &pw, &uid,
		    &gid, &gcos, &path, &shell, &last, &min,
		    &max, &warn, &inactive, &expire, &flag);
	} else {
		ret_stat = lcl_set_table_entry(DB_NS_UFS, NULL, NULL,
		    DB_DISABLE_LOCKING | DB_ADD_MODIFY,
		    &db_err, tbl, ROOT_NAME,
		    &name, &e_passwd, &uid, &gid, &gcos, &path,
		    &shell, &last, &min, &max, &warn, &inactive,
		    &expire, &flag);
		if (ret_stat == -1)
			strcpy(errmess, db_err->msg);
	}

	(void) fprintf(debugfp, "status %d\n", ret_stat);
	if (ret_stat == -1)
		(void) fprintf(debugfp, "message %s\n", errmess);

	free_table(tbl);
	if (ret_stat == -1) {
		/*
		 * Couldn't set password.
		 */
		return (FAILURE);
	} else {
		return (SUCCESS);
	}
}

