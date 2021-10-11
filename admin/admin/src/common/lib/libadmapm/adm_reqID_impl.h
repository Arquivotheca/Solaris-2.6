
/*
 *  Copyright (c) 1990 Sun Microsystems, Inc.  All Rights Reserved.
 */

/*
 *******************************************************************************
 *
 *	This file contains the request ID handling definitions used
 *	internally within the administrative framework.
 *
 *	NOTE: Access to the request counter in the request generator is
 *	      not thread-safe.  A mutex lock should be added to gaurd
 *	      against concurrent access to the counter.
 *
 *******************************************************************************
 */

#ifndef _adm_reqID_impl_h
#define _adm_reqID_impl_h

#pragma	ident	"@(#)adm_reqID_impl.h	1.6	93/05/18 SMI"

#include <sys/types.h>

/*
 *----------------------------------------------------------------------
 * Internal request ID constants.
 *----------------------------------------------------------------------
 */

#define ADM_RIDFMT	"%ld:%ld:%lu"	/* Format of a string request ID */
#define ADM_RIDFMT2	"%ld:%ld:%lu%n"	/* Like ADM_RIDFMT, but returns */
					/* length of ID string */

/*
 *----------------------------------------------------------------------
 * Global variables used to handle request IDs.
 *----------------------------------------------------------------------
 */

extern	u_long	adm_nextIDcnt;	/* Count to place in next generated req ID */

/*
 *----------------------------------------------------------------------
 * Internal request ID handling routines.
 *----------------------------------------------------------------------
 */

#ifdef __cplusplus
extern "C" {
#endif

extern	void	adm_reqID_blank(Adm_requestID *);
extern	int	adm_reqID_init();
extern	int	adm_reqID_str2rid(char *, Adm_requestID *, u_int *);
extern	int	adm_reqID_rid2str(Adm_requestID, char *, u_int, u_int *);

#ifdef __cplusplus
}
#endif

#endif /* !_adm_reqID_impl_h */

