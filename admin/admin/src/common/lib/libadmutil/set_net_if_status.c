/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)set_net_if_status.c	1.1	94/08/31 SMI"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <libintl.h>
#include "admutil.h"

set_net_if_status(char *if_name, char *status, char *trailers, char *arp,
	char *private)
{
        int sd;			/* socket descriptor */
	struct ifreq ifr;	/* interface request block */

       	sd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sd < 0)
		return (ADMUTIL_SETIFS_SOCK);

	/*
	 * Get current flags from interface, then modify them appropriately
	 */
	strcpy(ifr.ifr_name, if_name);
	if (ioctl(sd, SIOCGIFFLAGS, (char *) &ifr) < 0) { 
		(void) close(sd);
		return (ADMUTIL_SETIFS_IOCTL);
	}

	if (strlen(status))
		if (!strcmp(status, ADMUTIL_UP))
			ifr.ifr_flags |= IFF_UP;
		else
			ifr.ifr_flags &= ~IFF_UP;

	if (strlen(trailers))
		if (!strcmp(trailers, ADMUTIL_YES))
			ifr.ifr_flags &= ~IFF_NOTRAILERS;
		else
			ifr.ifr_flags |= IFF_NOTRAILERS;

	if (strlen(arp))
		if (!strcmp(arp, ADMUTIL_YES))
			ifr.ifr_flags &= ~IFF_NOARP;
		else
			ifr.ifr_flags |= IFF_NOARP;

	if (strlen(private))
		if (!strcmp(private, ADMUTIL_YES))
			ifr.ifr_flags |= IFF_PRIVATE;
		else
			ifr.ifr_flags &= IFF_PRIVATE;

	if (ioctl(sd, SIOCSIFFLAGS, (char *) &ifr) < 0) { 
		(void) close(sd);
		return (ADMUTIL_SETIFS_IOCTL);
	}

	(void) close(sd);

	return (0);
}
