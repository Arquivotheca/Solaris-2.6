/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF		*/
/*	UNIX System Laboratories, Inc.				*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * Copyright (c) 1992 by Sun Microsystems, Inc.  All Rights Reserved.
 */

#ifndef	_SYS_KB_H
#define	_SYS_KB_H

#pragma ident	"@(#)kb.h	1.3	94/09/03 SMI"

#ifdef	__cplusplus
extern "C" {
#endif

#define	SEND2KBD(port, byte) { \
	while (inb(KB_STAT) & KB_INBF) \
		; \
	outb(port, byte); \
}

#ifdef	__cplusplus
}
#endif

#endif /* _SYS_KB_H */
