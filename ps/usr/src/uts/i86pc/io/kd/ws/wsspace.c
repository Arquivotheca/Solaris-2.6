/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * Copyright (c) 1992 by Sun Microsystems, Inc.  All Rights Reserved.
 */

#pragma	ident "@(#)wsspace.c	1.1	92/04/01 SMI"

#ifdef DONT_INCLUDE
#include "config.h"
#endif

/* KJS1 This should be globally defined */
#define MAXMINOR	255

unsigned char ws_compatflgs[MAXMINOR/8 + 1];
