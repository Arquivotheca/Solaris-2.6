/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * Copyright (c) 1996, by Sun Microsystems, Inc.
 * All rights reserved.
 */

#ifndef _SYS_MOUNT_H
#define	_SYS_MOUNT_H

#pragma ident	"@(#)mount.h	1.15	96/04/23 SMI"	/* SVr4.0 11.10	*/

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * Flags bits passed to mount(2).
 */
#define	MS_RDONLY	0x01	/* Read-only */
#define	MS_FSS		0x02	/* Old (4-argument) mount (compatibility) */
#define	MS_DATA		0x04	/* 6-argument mount */
#define	MS_SYSSPACE	0x08	/* mounta already in kernel space */
#define	MS_NOSUID	0x10	/* Setuid programs disallowed */
#define	MS_REMOUNT	0x20	/* Remount */
#define	MS_NOTRUNC	0x40	/* Return ENAMETOOLONG for long filenames */
#define	MS_OVERLAY	0x80    /* allow overlay mounts */

#if defined(__STDC__) && !defined(_KERNEL)
int mount(const char *, const char *, int, ...);
int umount(const char *);
#endif

#ifdef	__cplusplus
}
#endif

#endif	/* _SYS_MOUNT_H */
