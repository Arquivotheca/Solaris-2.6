#ifndef lint
static  char sccsid[] = "@(#)get_net_if_ip_addr.c 1.1 94/08/26 SMI";
#endif

/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 */

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <libintl.h>
#include "admutil.h"

get_net_if_ip_addr(char *if_name, char *ip_addr)
{
        int sd;			/* socket descriptor */
	struct ifreq ifr;	/* interface request block */
	char *addrp;
	struct sockaddr_in *sinp;

       	sd = socket(AF_INET, SOCK_DGRAM, 0);
       	if (sd < 0)
		return (ADMUTIL_GETIP_SOCK);

	strcpy(ifr.ifr_name, if_name);
       	if (ioctl(sd, SIOCGIFADDR, (char *) &ifr) < 0) { 
		(void) close(sd);
		return (ADMUTIL_GETIP_IOCTL);
	}

	(void) close(sd);

	sinp = (struct sockaddr_in *)&ifr.ifr_addr;
	addrp = inet_ntoa(sinp->sin_addr);

	strcpy(ip_addr, addrp);
	return (0);
}
