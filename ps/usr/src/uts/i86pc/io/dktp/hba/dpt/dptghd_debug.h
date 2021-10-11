
/*
 * Copyright (c) 1996, Sun Microsystems, Inc.
 * All Rights Reserved.
 */

#ifndef _DPTGHD_DEBUG_H
#define	_DPTGHD_DEBUG_H

#pragma	ident	"@(#)dptghd_debug.h	1.1	96/06/13 SMI"

#ifdef	__cplusplus
extern "C" {
#endif

#include <sys/varargs.h>

void		dptghd_err(char *fmt, ...);
extern	ulong	dptghd_debug_flags;

#define	GDBG_FLAG_ERROR		0x0001
#define	GDBG_FLAG_INTR		0x0002
#define	GDBG_FLAG_PEND_INTR	0x0004
#define	GDBG_FLAG_START		0x0008
#define	GDBG_FLAG_WARN		0x0010

/*
 * Use prom_printf() or vcmn_err()
 */
#ifdef GHD_DEBUG_PROM_PRINTF
#define	GDBG_PRF(fmt)	prom_printf fmt
void	prom_printf(char * fmt, ...);
#else
#define	GDBG_PRF(fmt)	dptghd_err fmt
#endif



#ifdef	GHD_DEBUG

#define	GDBG_FLAG_CHK(flag, fmt) if (dptghd_debug_flags & (flag)) GDBG_PRF(fmt)

#else	/* !GHD_DEBUG */

#define	GDBG_FLAG_CHK(flag, fmt)

#endif	/* !GHD_DEBUG */

/*
 * Always print "real" error messages on non-debugging kernels
 */

#ifdef	GHD_DEBUG
#define	GDBG_ERROR(fmt)		GDBG_FLAG_CHK(GDBG_FLAG_ERROR, fmt)
#else
#define	GDBG_ERROR(fmt)	dptghd_err fmt
#endif

/*
 * Debugging printf macros
 */

#define	GDBG_INTR(fmt)		GDBG_FLAG_CHK(GDBG_FLAG_INTR, fmt)
#define	GDBG_PEND_INTR(fmt)	GDBG_FLAG_CHK(GDBG_FLAG_PEND_INTR, fmt)
#define	GDBG_START(fmt)		GDBG_FLAG_CHK(GDBG_FLAG_START, fmt)
#define	GDBG_WARN(fmt)		GDBG_FLAG_CHK(GDBG_FLAG_WARN, fmt)




#ifdef	__cplusplus
}
#endif

#endif  /* _DPTGHD_DEBUG_H */
