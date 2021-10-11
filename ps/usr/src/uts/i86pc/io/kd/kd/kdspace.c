/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * Copyright (c) 1996 by Sun Microsystems, Inc.  All Rights Reserved.
 */

#pragma	ident "@(#)kdspace.c	1.3	96/03/20 SMI"

#include "sys/types.h"
#include "sys/param.h"
#include "sys/tty.h"
#include "sys/vt.h"
#include "sys/at_ansi.h"
#include "sys/proc.h"
#include "sys/kd.h"


stridx_t	kdstrmap;	/* Indices into string buffer */

/*
 * use this array to add configurable array of io port addresses
 */
ushort	kdconfiotab[MKDCONFADDR];

/*
 * set "kdioaddrcnt" to count of new io port array elements being added
 * NOTE - new io address count CANNOT exceed MKDCONFADDR
 */
int	kdioaddrcnt = 0;

/* use this array to add configurable video memory array of
 * start and end address pairs. Note, array count CANNOT exceed MKDCONFADDR
 * address pairs
 */
struct kd_range kdvmemtab[MKDCONFADDR];

/*
 * set "kdvmemcnt" to count of new video memory array elements being added
 * NOTE - new video memory array count CANNOT exceed MKDCONFADDR
 */
#ifdef	EVC
int	kdvmemcnt = 1;
#else
int	kdvmemcnt = 0;
#endif

/*
 *  This variable causes kd to assume that the graphics board is
 *  a specific type.  Used to override the special port testing
 *  for board identification.
 */
int AssumeVDCType = 0;	/* edited by install scripts -- do not change format*/
