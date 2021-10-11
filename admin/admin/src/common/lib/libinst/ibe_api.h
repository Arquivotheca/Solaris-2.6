#ifndef lint
#pragma ident "@(#)ibe_api.h 1.35 95/01/24"
#endif
/*
 * Copyright (c) 1991 Sun Microsystems, Inc.  All Rights Reserved. Sun
 * considers its source code as an unpublished, proprietary trade secret, and
 * it is available only under strict license provisions.  This copyright
 * notice is placed here only to protect Sun in the event the source is
 * deemed a published work.  Dissassembly, decompilation, or other means of
 * reducing the object code to human readable form is prohibited by the
 * license agreement under which this code is provided to the user or company
 * in possession of this copy.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the Government
 * is subject to restrictions as set forth in subparagraph (c)(1)(ii) of the
 * Rights in Technical Data and Computer Software clause at DFARS 52.227-7013
 * and in similar clauses in the FAR and NASA FAR Supplement.
 */

/*
 * This module contains types and prototype specs for all publicly available
 * interfaces to the installation library
 */

#ifndef _IBE_API_H
#define	_IBE_API_H

/*
 * ERROR/NOERR defines
 */
#define	NOERR		0
#define	ERROR		1

/*
 *
 */
#define	STATMSG		0
#define	ERRMSG		1
#define	WARNMSG		2

/*
 * pkgadd valid returns
 */
#define	PKGREBOOT	10
#define	PKGIREBOOT	20

/*
 * installation status values
 */
#define	UNTOUCHED	0	/* The product was not installed */
#define	INSTALLED	1	/* The product was completly installed */
#define	PARTIALLY	2	/* The product was partially installed */

/* format values used for write_status() */

#define	LOG		0x1	/* write the message to log file */
#define	SCR		0x2	/* write the message to the screen */
#define	LOGSCR		LOG|SCR	/* write the message to the log and screen */

#define	LEVEL0		0x0001	/* message level 0 */
#define	LEVEL1		0x0002	/* message level 1 */
#define	LEVEL2		0x0004	/* message level 2 */
#define	LEVEL3		0x0010	/* message level 3 */

#define	LISTITEM	0x0100	/* list item */
#define	CONTINUE	0x0200	/* continuation line */
#define	PARTIAL		0x0400	/* no newline at end of line */

/* Miscellaneous constants and macros */
#ifndef	BUFSIZE
#define	BUFSIZE		256
#endif

/* Function Prototypes */

/* ibe_sm.c */

extern int 	ibe_sm(Module *, Disk_t *, Remote_FS *);

/* ibe_util.c */

extern int	set_install_debug(int);
extern int	get_install_debug(void);
extern int 	reset_system_state(void);

/* ibe_fileio.c */

extern void 	write_message(u_char, u_int, u_int, char *, ...);
extern void 	write_status(u_char, u_int, char *, ...);
extern void	write_notice(u_int, char *, ...);
extern void 	(*register_func(u_int, void (*)(u_int, char *)))();

#endif	/* _IBE_API_H */
