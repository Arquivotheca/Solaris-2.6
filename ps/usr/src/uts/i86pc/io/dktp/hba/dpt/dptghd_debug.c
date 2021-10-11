/*
 * Copyright (c) 1996, Sun Microsystems, Inc.
 * All Rights Reserved.
 */

#pragma	ident	"@(#)dptghd_debug.c	1.1	96/06/13 SMI"

#include "dptghd.h"



#ifndef	GHD_DEBUG
ulong	dptghd_debug_flags = 0;
#else
ulong	dptghd_debug_flags = GDBG_FLAG_ERROR
			| GDBG_FLAG_WARN
		/*	| GDBG_FLAG_INTR	*/
		/*	| GDBG_FLAG_PEND_INTR	*/
		/*	| GDBG_FLAG_START	*/
			;
#endif

void
dptghd_err(char *fmt, ...)
{
	va_list	ap;

	va_start(ap, fmt);
	vcmn_err(CE_CONT, fmt, ap);
	va_end(ap);
}
