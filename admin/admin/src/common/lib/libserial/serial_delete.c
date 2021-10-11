/*
 * Copyright (c) 1994 by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)serial_delete.c	1.2	94/10/07 SMI"


int
delete_modem(const char *pmtag, const char *svctag)
{

	char	pmadm_cmd[256];


	sprintf(pmadm_cmd, "set -f ; /usr/sbin/pmadm -r -p %s -s %s",
	    pmtag, svctag);

	return (system(pmadm_cmd));
}
