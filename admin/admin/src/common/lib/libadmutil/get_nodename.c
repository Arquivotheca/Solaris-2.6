#ifndef lint
static  char sccsid[] = "@(#)get_nodename.c 1.1 94/08/25 SMI";
#endif

/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/utsname.h>
#include <sys/systeminfo.h>
#include <errno.h>
#include <libintl.h>
#include "db_entry.h"
#include "admutil.h"

#define UFS_DB	"/etc/nodename"

/*
 * expect two buffers, each SYS_NMLN long.
 *
 * return
 *  0 - ok
 *  > 0 - errno (system error)
 *  < 0 - internal error
 *	ADMUTIL_GETNN_SYS: sysinfo failed
 */

get_nodename(char *curr_nodename, char *perm_nodename)
{
	int status;

	if (sysinfo(SI_HOSTNAME, curr_nodename, SYS_NMLN) == -1)
		return (ADMUTIL_GETNN_SYS);

	if ((status = read_db(UFS_DB, perm_nodename, SYS_NMLN)) != 0)
		return (status);

	return (0);
}
