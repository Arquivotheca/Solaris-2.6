
/*
 * Copyright (c) 1996, Sun Microsystems, Inc.
 * All Rights Reserved.
 */

#ifndef _ADP_DEBUG_H
#define	_ADP_DEBUG_H

#pragma	ident	"@(#)adp_debug.h	1.1	96/07/31 SMI"

#ifdef	__cplusplus
extern "C" {
#endif

#include <sys/varargs.h>
#include "../ghd/ghd_debug.h"

#define	ADBG_FLAG_ERROR	0x0001	/* unrecoverable error			*/
#define	ADBG_FLAG_ENT	0x0002	/* Display function names on entry	*/
#define	ADBG_FLAG_PKT	0x0004 	/* Display packet data			*/
#define	ADBG_FLAG_DATA	0x0008	/* Display all data			*/
#define	ADBG_FLAG_TEMP	0x0010	/* Display wrt to currrent debugging	*/
#define	ADBG_FLAG_CHN	0x0020	/* Display channel number on fn. entry	*/
#define	ADBG_FLAG_STUS	0x0040	/* Display interrupt status		*/
#define	ADBG_FLAG_INIT	0x0080	/* Display init data			*/
#define	ADBG_FLAG_TEST	0x0100	/* Display test data			*/
#define	ADBG_FLAG_PROBE	0x0200	/* Display probe data			*/

#ifdef	ADP_DEBUG

#define	ADBG_FLAG_CHK(flag, fmt) if (adp_debug_flags & (flag)) GDBG_PRF(fmt)

#else	/* !ADP_DEBUG */

#define	ADBG_FLAG_CHK(flag, fmt)

#endif	/* !ADP_DEBUG */

/*
 * Always print "real" error messages on non-debugging kernels ...
 */

#ifdef	ADP_DEBUG
#define	ADBG_ERROR(fmt)		ADBG_FLAG_CHK(ADBG_FLAG_ERROR, fmt)
#else
#define	ADBG_ERROR(fmt)	ghd_err fmt
#endif

/*
 * ... everything else is conditional on the ADP_DEBUG preprocessor symbol
 */

#define	ADBG_ENT(fmt)	ADBG_FLAG_CHK(ADBG_FLAG_ENT, fmt)
#define	ADBG_PKT(fmt)	ADBG_FLAG_CHK(ADBG_FLAG_PKT, fmt)
#define	ADBG_DATA(fmt)	ADBG_FLAG_CHK(ADBG_FLAG_DATA, fmt)
#define	ADBG_TEMP(fmt)	ADBG_FLAG_CHK(ADBG_FLAG_TEMP, fmt)
#define	ADBG_CHN(fmt)	ADBG_FLAG_CHK(ADBG_FLAG_CHN, fmt)
#define	ADBG_STUS(fmt)	ADBG_FLAG_CHK(ADBG_FLAG_STUS, fmt)
#define	ADBG_INIT(fmt)	ADBG_FLAG_CHK(ADBG_FLAG_INIT, fmt)
#define	ADBG_TEST(fmt)	ADBG_FLAG_CHK(ADBG_FLAG_TEST, fmt)
#define	ADBG_PROBE(fmt)	ADBG_FLAG_CHK(ADBG_FLAG_PROBE, fmt)


#ifdef	__cplusplus
}
#endif

#endif  /* _ADP_DEBUG_H */
