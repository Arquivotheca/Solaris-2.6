/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *		PROPRIETARY NOTICE (Combined)
 *
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 *
 *
 *
 *		Copyright Notice
 *
 * Notice of copyright on this source code product does not indicate
 * publication.
 *
 *	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
 *	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 *		All rights reserved.
 *
 */

#ifndef _ARPA_INET_H
#define	_ARPA_INET_H

#pragma ident	"@(#)inet.h	1.9	96/07/12 SMI"	/* SVr4.0 1.1	*/

#include <sys/feature_tests.h>

#include <netinet/in.h>
#if defined(_XPG4_2) && !defined(__EXTENSIONS__)
#include <sys/byteorder.h>
#endif /* defined(__XPG4_2) && !defined(__EXTENSIONS__) */

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * External definitions for
 * functions in inet(3N)
 */
#ifdef __STDC__
extern in_addr_t inet_addr(const char *);
extern in_addr_t inet_lnaof(struct in_addr);
extern struct in_addr inet_makeaddr(in_addr_t, in_addr_t);
extern in_addr_t inet_netof(struct in_addr);
extern in_addr_t inet_network(const char *);
extern char *inet_ntoa(struct in_addr);
#else
unsigned long inet_addr();
char	*inet_ntoa();
struct	in_addr inet_makeaddr();
unsigned long inet_network();
extern unsigned long inet_lnaof();
extern unsigned long inet_netof();
#endif

#ifdef	__cplusplus
}
#endif

#endif	/* _ARPA_INET_H */
