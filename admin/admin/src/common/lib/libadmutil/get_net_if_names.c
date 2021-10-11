/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)get_net_if_names.c	1.1	94/08/31 SMI"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <net/if.h>
#include <errno.h>
#include <libintl.h>
#include "admutil.h"

get_net_if_names(struct ifconf *pifc)
{
	char *buf;		/* buffer for socket info */
        int sd;			/* socket descriptor */
        struct ifconf ifc;	/* interface config buffer */
        int max_if;		/* max interfaces */
	
       	sd = socket(AF_INET, SOCK_DGRAM, 0);
       	if (sd < 0)
		return (ADMUTIL_GETIFN_SOCK);

	if (ioctl(sd, SIOCGIFNUM, (char *) &max_if) < 0) { 
		(void) close(sd);
		return (ADMUTIL_GETIFN_IOCTL);
	}

	if ( (buf = (char *)malloc(max_if * sizeof(struct ifreq))) == NULL ) {
		(void) close(sd);
		return (ADMUTIL_GETIFN_MEM);
	}

       	ifc.ifc_len = max_if * sizeof(struct ifreq);
       	ifc.ifc_buf = (caddr_t) buf;
       	if (ioctl(sd, SIOCGIFCONF, (char *) &ifc) < 0) { 
		(void) close(sd);
		return (ADMUTIL_GETIFN_IOCTL);
	}

	(void) close(sd);

	*pifc = ifc;

	return (0);
}
