#ifndef lint
static  char sccsid[] = "@(#)get_domain.c 1.1 94/08/25 SMI";
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

#define UFS_DB	"/etc/defaultdomain"

/*
 * expect two buffers, each SYS_NMLN long.
 */

get_domain(char *curr_domain, char *perm_domain)
{
	int status;

	curr_domain[0] = perm_domain[0] = '\0';
	
	if (sysinfo(SI_SRPC_DOMAIN, curr_domain, SYS_NMLN) == -1)
		return (ADMUTIL_GETDM_SYS);

	if (((status = read_db(UFS_DB, perm_domain, SYS_NMLN)) != 0) &&
	    (status != ENOENT))
		return (status);

	return (0);
}
