#pragma ident	"@(#)ether_addr.c	1.17	92/07/20 SMI" 

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 *
 * All routines necessary to deal with the file /etc/ethers.  The file
 * contains mappings from 48 bit ethernet addresses to their corresponding
 * hosts name.  The addresses have an ascii representation of the form
 * "x:x:x:x:x:x" where x is a hex number between 0x00 and 0xff;  the
 * bytes are always in network order.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <net/if_arp.h>
#include <netinet/if_ether.h>

static char *domain; /* NIS domain name */
static int usingyellow;
static char *ethers = "/etc/ethers";

/*
 * Parses a line from /etc/ethers into its components.  The line has the form
 * 8:0:20:1:17:c8	krypton
 * where the first part is a 48 bit ethernet addrerss and the second is
 * the corresponding hosts name.
 * Returns zero if successful, non-zero otherwise.
 */
ether_line(s, e, hostname)
	char *s;		/* the string to be parsed */
	struct ether_addr *e;	/* ethernet address struct to be filled in */
	char *hostname;		/* hosts name to be set */
{
	register int i;
	unsigned int t[6];
	
	i = sscanf(s, " %x:%x:%x:%x:%x:%x %s",
	    &t[0], &t[1], &t[2], &t[3], &t[4], &t[5], hostname);
	if (i != 7) {
		return (7 - i);
	}
	for (i = 0; i < 6; i++)
		e->ether_addr_octet[i] = t[i];
	return (0);
}

/*
 * Converts a 48 bit ethernet number to its string representation.
 */
#define EI(i)	(unsigned int)(e->ether_addr_octet[(i)])
char *
ether_ntoa(e)
	struct ether_addr *e;
{
	static char *s;

	if (s == 0) {
		s = (char *)malloc(18);
		if (s == 0)
			return (0);
	}
	s[0] = 0;
	sprintf(s, "%x:%x:%x:%x:%x:%x",
	    EI(0), EI(1), EI(2), EI(3), EI(4), EI(5));
	return (s);
}

/*
 * Converts a ethernet address representation back into its 48 bits.
 */
struct ether_addr *
ether_aton(s)
	char *s;
{
	static struct ether_addr *ep;
	register int i;
	unsigned int t[6];
	
	if (ep == 0) {
		ep = (struct ether_addr *)calloc(1, sizeof (struct ether_addr));
		if (ep == 0)
			return (0);
	}
	i = sscanf(s, " %x:%x:%x:%x:%x:%x",
	    &t[0], &t[1], &t[2], &t[3], &t[4], &t[5]);
	if (i != 6)
	    return ((struct ether_addr *)NULL);
	for (i = 0; i < 6; i++)
		ep->ether_addr_octet[i] = t[i];
	return (ep);
}
