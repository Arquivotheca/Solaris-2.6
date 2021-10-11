/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)set_domain.c	1.2	95/09/25 SMI"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/utsname.h>
#include <sys/systeminfo.h>
#include <libintl.h>
#include "db_entry.h"
#include "admutil.h"

#define UFS_DB	"/etc/defaultdomain"

int
set_domain(char *domain, int te_mask)
{
	int status;

	if (!strlen(domain))
		return (ADMUTIL_SETDM_BAD);

	if ((te_mask & TE_NOW_BIT)) {	
		if (sysinfo(SI_SET_SRPC_DOMAIN, domain, strlen(domain)) 
		    == -1)
			return (ADMUTIL_SETDM_SYS);
	}

	if ((te_mask & TE_BOOT_BIT)) {
		if ((status = replace_db(UFS_DB, domain)) != 0)
			return (status);
		if ((status = chmod(UFS_DB,
		     S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) != 0)
			return (status);
	}

	return (0);
}
