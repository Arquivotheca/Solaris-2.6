/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)etheraddr.c	1.6	93/05/10 SMI"

/*
 *  		PROPRIETARY NOTICE (Combined)
 *
 *  This source code is unpublished proprietary information
 *  constituting, or derived under license from AT&T's Unix(r) System V.
 *  In addition, portions of such source code were derived from Berkeley
 *  4.3 BSD under license from the Regents of the University of
 *  California.
 *
 *
 *
 *  		Copyright Notice
 *
 *  Notice of copyright on this source code product does not indicate
 *  publication.
 *
 *  	(c) 1986,1987,1988,1989-1993  Sun Microsystems, Inc.
 *  	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 *		All rights reserved.
 */

#include <sys/types.h>
#include <sys/sysmacros.h>
#include <sys/ethernet.h>
#include <sys/cmn_err.h>

/*
 * Store and retrieve local individual ethernet address.
 * This is typically initialized (called with 'hint' nonnull)
 * by the boot code.
 */
int
localetheraddr(struct ether_addr *hint, struct ether_addr *result)
{
	static int found = 0;
	static	struct	ether_addr	addr;

	if (!found) {
		found = 1;
		if (hint == NULL)
			return (0);
		addr = *hint;
		cmn_err(CE_CONT, "?Ethernet address = %s\n",
		    ether_sprintf(&addr));
	}
	if (result != NULL)
		*result = addr;
	return (1);
}


/*
 * Convert Ethernet address to printable (loggable) representation.
 */
char *
ether_sprintf(struct ether_addr *addr)
{
	register u_char *ap = (u_char *)addr;
	register int i;
	static char etherbuf[18];
	register char *cp = etherbuf;
	static char digits[] = "0123456789abcdef";

	for (i = 0; i < 6; i++) {
		if (*ap > 0x0f)
			*cp++ = digits[*ap >> 4];
		*cp++ = digits[*ap++ & 0xf];
		*cp++ = ':';
	}
	*--cp = 0;
	return (etherbuf);
}
