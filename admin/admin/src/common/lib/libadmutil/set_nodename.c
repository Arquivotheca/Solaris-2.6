#ifndef lint
static  char sccsid[] = "@(#)set_nodename.c 1.2 95/09/25 SMI";
#endif

/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/utsname.h>
#include <sys/systeminfo.h>
#include <libintl.h>
#include "db_entry.h"
#include "valid.h"
#include "admutil.h"

#define UFS_DB	"/etc/nodename"

set_nodename(char *nodename, int te)
{
	int status;

	if (!valid_hostname(nodename))
		return (ADMUTIL_SETNN_BAD);

	if ((te & TE_NOW_BIT))
		if (sysinfo(SI_SET_HOSTNAME, nodename, strlen(nodename)) 
		    == -1)
			return (ADMUTIL_SETNN_SYS);

	if ((te & TE_BOOT_BIT)) {
		if ((status = replace_db(UFS_DB, nodename)) != 0)
			return (status);
		if ((status = chmod(UFS_DB,
		     S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) != 0)
			return (status);
	}

	return (0);
}
