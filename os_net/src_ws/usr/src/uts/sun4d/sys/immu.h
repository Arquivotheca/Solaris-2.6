/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#ifndef _SYS_IMMU_H
#define	_SYS_IMMU_H

#pragma ident	"@(#)immu.h	1.9	96/07/28 SMI"

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * XXX - following stuff from 3b2 immu.h.  this really belongs elsewhere.
 */

/*
 *	The following variables describe the memory managed by
 *	the kernel.  This includes all memory above the kernel
 *	itself.
 */

extern int	maxmem;		/* Maximum available free memory. */
extern int	freemem;	/* Current free memory. */
extern int	availrmem;	/* Available resident (not	*/
				/* swapable) memory in pages.	*/

#define	PAGOFF(x)   (((uint)(x)) & PAGEOFFSET)

#ifdef	__cplusplus
}
#endif

#endif	/* _SYS_IMMU_H */
