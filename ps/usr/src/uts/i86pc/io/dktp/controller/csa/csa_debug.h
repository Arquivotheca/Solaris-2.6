/*
 * Copyright (c) 1995, Sun Microsystems, Inc.
 * All Rights Reserved.
 */

#ifndef _CSA_CSA_DEBUG_H
#define	_CSA_CSA_DEBUG_H

#pragma	ident	"@(#)csa_debug.h	1.3	95/05/31 SMI"

#ifdef	__cplusplus
extern "C" {
#endif

#include <sys/varargs.h>

void		csa_err(char *fmt, ...);
extern	ulong	csa_debug_flags;

#ifdef	CSA_DEBUG

#define	CDBG_FLAG_ERROR		0x0001
#define	CDBG_FLAG_INTR		0x0002
#define	CDBG_FLAG_PEND_INTR	0x0004
#define	CDBG_FLAG_RW		0x0008
#define	CDBG_FLAG_SEND		0x0010
#define	CDBG_FLAG_START		0x0020
#define	CDBG_FLAG_BUSCTL	0x0040
#define	CDBG_FLAG_WARN		0x0080

#ifdef CSA_DEBUG_XXX

#define	CDBG_PRF(fmt)	prom_printf fmt
void	prom_printf(char * fmt, ...);
void	debug_enter(char *msg);

#else

#define	CDBG_PRF(fmt)	csa_err fmt

#endif

#define	CDBG_FLAG_CHK(flag, fmt) if (csa_debug_flags & (flag)) CDBG_PRF(fmt)

#else	/* !CSA_DEBUG */

#define	CDBG_FLAG_CHK(flag, fmt)

#endif	/* !CSA_DEBUG */

/*
 * Always print "real" error messages on non-debugging kernels
 */

#ifdef	CSA_DEBUG
#define	CDBG_ERROR(fmt)		CDBG_FLAG_CHK(CDBG_FLAG_ERROR, fmt)
#else
#define	CDBG_ERROR(fmt)	csa_err fmt
#endif

/*
 * Debugging printf macros
 */

#define	CDBG_INTR(fmt)		CDBG_FLAG_CHK(CDBG_FLAG_INTR, fmt)
#define	CDBG_PEND_INTR(fmt)	CDBG_FLAG_CHK(CDBG_FLAG_PEND_INTR, fmt)
#define	CDBG_RW(fmt)		CDBG_FLAG_CHK(CDBG_FLAG_RW, fmt)
#define	CDBG_SEND(fmt)		CDBG_FLAG_CHK(CDBG_FLAG_SEND, fmt)
#define	CDBG_START(fmt)		CDBG_FLAG_CHK(CDBG_FLAG_START, fmt)
#define	CDBG_BUSCTL(fmt)	CDBG_FLAG_CHK(CDBG_FLAG_BUSCTL, fmt)
#define	CDBG_WARN(fmt)		CDBG_FLAG_CHK(CDBG_FLAG_WARN, fmt)


#ifdef	__cplusplus
}
#endif

#endif  /* _CSA_CSA_DEBUG_H */
