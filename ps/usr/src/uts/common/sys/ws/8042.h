/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved					*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF		*/
/*	UNIX System Laboratories, Inc.				*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * Copyright (c) 1996 by Sun Microsystems, Inc.  All Rights Reserved.
 */

#ifndef	_SYS_WS_8042_H
#define	_SYS_WS_8042_H

#pragma ident	"@(#)8042.h	1.5	96/03/26 SMI"

#ifdef	__cplusplus
extern "C" {
#endif

/* defines for i8042_program() */
#define	P8042_KBDENAB	1
#define	P8042_KBDDISAB	2
#define	P8042_AUXENAB	3
#define	P8042_AUXDISAB	4

/* defines for i8042_send_cmd */
#define	P8042_TO_KBD	1
#define	P8042_TO_AUX	2


#if	defined(_KERNEL)
extern void	i8042_acquire(unsigned char);
extern void 	i8042_release(unsigned char);
extern int	i8042_send_cmd(unsigned char, unsigned char,
			unsigned char *, unsigned char,
			unsigned char);
extern void	i8042_program(int);
extern int	i8042_aux_port(void);
#endif

#ifdef	__cplusplus
}
#endif

#endif	/* _SYS_WS_8042_H */
