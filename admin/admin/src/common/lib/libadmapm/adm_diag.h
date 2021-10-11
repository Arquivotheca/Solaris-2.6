
/*
 *  Copyright (c) 1990 Sun Microsystems, Inc.  All Rights Reserved.
 */

/*
 ****************************************************************************
 *
 *	This file contains the exported definitions for handling
 *	tracing messages in the administrative framework.  It includes
 *	the definitions for:
 *
 *		o Miscellaneous constants.
 *		o Exported interfaces for handling tracing messages.
 *
 ****************************************************************************
 */

#ifndef _adm_diag_h
#define _adm_diag_h

#pragma	ident	"@(#)adm_diag.h	1.4	93/05/18 SMI"

/*
 *---------------------------------------------------------------------
 * Miscellaneous constants.
 *---------------------------------------------------------------------
 */

#define ADM_NOCATS	NULL		/* Empty category specification */
#define ADM_ALLCATS	"*"		/* All categories specification */

#define ADM_MAXDIAGMSG	1024		/* Max. tracing message length */

#define ADM_CATSEP	","		/* Separator char. in category lists */

/*
 *---------------------------------------------------------------------
 * Exported tracing message handling interfaces.
 *---------------------------------------------------------------------
 */

#define adm_is_allcats(cat, len)			\
							\
	((len == (u_int)1) && (cat != NULL) && (strcmp(cat, ADM_ALLCATS) == 0))

#ifdef __cplusplus
extern "C" {
#endif

extern int adm_diag_fmt(int *, char *, time_t, char *, char *);
extern int adm_diag_fmtf(int *, char *, time_t, char *, char *, ...);
extern int adm_diag_set(char *, char *);
extern int adm_diag_setf(char *, char *, ...);

#ifdef __cplusplus
}
#endif

#endif /* !_adm_diag_h */

