/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)byteorder.c	1.6	94/03/07 SMI"	/* SVr4.0 1.1	*/

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 *
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 *
 *
 *
 * 		Copyright Notice
 *
 * Notice of copyright on this source code product does not indicate
 * publication.
 *
 * 	(c) 1986,1987,1988,1989,1994  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 *		All rights reserved.
 *
 */

/*
 *
 * Copyright 1987, 1988 Lachman Associates, Incorporated (LAI)
 *		All Rights Reserved.
 *
 * The copyright above and this notice must be preserved in all copies of this
 * source code.  The copyright above does not evidence any actual or intended
 * publication of this source code.
 *
 * This is unpublished proprietary trade secret source code of Lachman
 * Associates.  This source code may not be copied, disclosed, distributed,
 * demonstrated or licensed except as expressly authorized by Lachman
 * Associates.
 *
 * System V STREAMS TCP was jointly developed by Lachman Associates and
 * Convergent Technologies.
 */

#include <sys/isa_defs.h>

unsigned long
htonl(hl)
	unsigned long		hl;
{
#if defined(vax) || defined(_LITTLE_ENDIAN)
	char		nl[4];

	nl[0] = ((char *) &hl)[3];
	nl[1] = ((char *) &hl)[2];
	nl[2] = ((char *) &hl)[1];
	nl[3] = ((char *) &hl)[0];
	return (*(unsigned long *) nl);
#else
	return (hl);
#endif
}

unsigned short
htons(hs)
	unsigned short		hs;
{
#if defined(vax) || defined(_LITTLE_ENDIAN)
	char		ns[2];

	ns[0] = ((char *) &hs)[1];
	ns[1] = ((char *) &hs)[0];
	return (*(unsigned short *) ns);
#else
	return (hs);
#endif
}

unsigned long
ntohl(nl)
	unsigned long	nl;
{
#if defined(vax) || defined(_LITTLE_ENDIAN)
	char		hl[4];

	hl[0] = ((char *) &nl)[3];
	hl[1] = ((char *) &nl)[2];
	hl[2] = ((char *) &nl)[1];
	hl[3] = ((char *) &nl)[0];
	return (*(unsigned long *) hl);
#else
	return (nl);
#endif
}

unsigned short
ntohs(ns)
	unsigned short  ns;
{
#if defined(vax) || defined(_LITTLE_ENDIAN)
	char		hs[2];

	hs[0] = ((char *) &ns)[1];
	hs[1] = ((char *) &ns)[0];
	return (*(unsigned short *) hs);
#else
	return (ns);
#endif
}
