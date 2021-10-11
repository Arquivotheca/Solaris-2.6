/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)remove_lb_ntoa_entry.c	1.1	94/08/31 SMI"

#include <netconfig.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/param.h>
#include <errno.h>
#include <libintl.h>
#include <string.h>
#include "db_entry.h"
#include "valid.h"
#include "admutil.h"

#define	UFS_DB	"/etc/net/%s/hosts"

#define	COLUMN_SEP	UFS_DEFAULT_COLUMN_SEP

remove_lb_ntoa_entry(char *hostname)
{
	Col_list *clp = NULL;
	int count = 0;
	void *handlep;
	struct netconfig *nconf;
	char db_name[MAXPATHLEN];

	if ((handlep = setnetconfig()) == (void *) NULL)
		return (ADMUTIL_REMLB_NET);

	if (new_col_list(&clp, COLUMN_SEP, NULL, NULL) != 0)
		return (ADMUTIL_REMLB_MEM);

	if (new_column(clp, UFS_LB_NTOA_NAME_COL, UFS_LB_NTOA_NAME_COL, 
	    UFS_LB_NTOA_NAME_COL, hostname, NULL, DBE_CASE_INSENSITIVE) != 0) {
		free_col_list(&clp);
		return (ADMUTIL_REMLB_MEM);
	}

	if (new_column(clp, UFS_LB_NTOA_ADDR_COL, UFS_LB_NTOA_ADDR_COL,
	    UFS_LB_NTOA_ADDR_COL, hostname, NULL, DBE_CASE_INSENSITIVE) != 0) {
		free_col_list(&clp);
		return (ADMUTIL_REMLB_MEM);
	}

	while ((nconf = getnetconfig(handlep)) != (struct netconfig *)NULL) {
		if ((nconf->nc_flag & NC_VISIBLE) && 
		    (strcmp(nconf->nc_protofmly, NC_LOOPBACK) == 0)) {
			(void) sprintf(db_name, UFS_DB, nconf->nc_netid);
			if (remove_db_entry(db_name, clp) != 0) {
				endnetconfig(handlep);
				free_col_list(&clp);
				if (count)
					return (ADMUTIL_REMLB_DIRT);
				else
					return (ADMUTIL_REMLB_CLN);
			} else
				++count;
		}
	}

	endnetconfig(handlep);
	free_col_list(&clp);

	return (0);
}
