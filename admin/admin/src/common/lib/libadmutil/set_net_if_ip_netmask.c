/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)set_net_if_ip_netmask.c	1.1	94/08/26 SMI"

#include <unistd.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <libintl.h>
#include <string.h>
#include "valid.h"
#include "admutil.h"

set_net_if_ip_netmask(char *if_name, char *netmask)
{
        int sd;			/* socket descriptor */
	struct ifreq ifr, ifr2;	/* interface request block */
	struct sockaddr_in *sinp = (struct sockaddr_in *)&ifr.ifr_addr;
	
	if ((strlen(netmask) == 0) || !valid_ip_netmask(netmask))
		return (ADMUTIL_SETMASK_BAD);

       	sd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sd < 0)
		return (ADMUTIL_SETMASK_SOCK);

	/*
	 * The sequence below borrows from the logic used in ifconfig(1).  The
	 * act of resetting the IP address of the interface after setting the
	 * netmask has a side-effect of resetting the broadcast address.  So,
	 * we read the address from the interface, set the mask, then set the
	 * address.
	 */
	strcpy(ifr2.ifr_name, if_name);
       	if (ioctl(sd, SIOCGIFADDR, (char *) &ifr2) < 0) { 
		(void) close(sd);
		return (ADMUTIL_SETMASK_IOCTL);
	}

	strcpy(ifr.ifr_name, if_name);
	sinp->sin_family = AF_INET; /* Address will be an Internet address */
	sinp->sin_addr.s_addr = inet_addr(netmask);
	if (ioctl(sd, SIOCSIFNETMASK, (char *) &ifr) < 0) { 
		(void) close(sd);
		return (ADMUTIL_SETMASK_IOCTL);
	}
	
	if (ioctl(sd, SIOCSIFADDR, (char *) &ifr2) < 0) { 
		(void) close(sd);
		return (ADMUTIL_SETMASK_IOCTL);
	}

	(void) close(sd);

	return (0);
}
