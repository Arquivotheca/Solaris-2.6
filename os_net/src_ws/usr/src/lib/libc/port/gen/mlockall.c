/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mlockall.c	1.7	93/04/28 SMI"	/* SVr4.0 1.3	*/

#ifdef __STDC__
	#pragma weak mlockall = _mlockall
#endif
#include "synonyms.h"
#include <sys/types.h>
#include <sys/mman.h>

/*
 * Function to lock address space in memory.
 */

/*LINTLIBRARY*/
mlockall(flags)
	int flags;
{

	return (memcntl(0, 0, MC_LOCKAS, (caddr_t) flags, 0, 0));
}
