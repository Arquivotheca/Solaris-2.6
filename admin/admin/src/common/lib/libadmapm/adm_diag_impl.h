
/*
 *  Copyright (c) 1990 Sun Microsystems, Inc.  All Rights Reserved.
 */

/*
 ****************************************************************************
 *
 *	This file contains the private definitions for handling
 *	tracing messages in the administrative framework.  It includes
 *	the definitions for:
 *
 *		o Special standard category names.
 *		o Standard category name prefixes.
 *		o Private interfaces for handling tracing messages.
 *
 ****************************************************************************
 */

#ifndef _adm_diag_impl_h
#define _adm_diag_impl_h

#pragma	ident	"@(#)adm_diag_impl.h	1.4	93/05/18 SMI"

#include <sys/types.h>
#include <stdarg.h>

/*
 *---------------------------------------------------------------------
 * Special standard category names associated with various framework
 * operatons.
 *   - These category names will be added to the appropriate
 *     automatic framework tracing messages along with the
 *     normal standard categories.
 *---------------------------------------------------------------------
 */

#define ADM_CAT_REQUEST "Requests"	/* Method invocation message */
#define ADM_CAT_ERROR   "Errors"	/* Tracing messages for errors */
#define ADM_CAT_DEBUG   "Debug"		/* Debug messages */
#define ADM_CAT_INFO    "System-Info"	/* System information messages */

/*
 *---------------------------------------------------------------------
 * Standard category name prefixes.
 *---------------------------------------------------------------------
 */

#define ADM_PREF_CLASS	"Class:"	/* Class name */
#define ADM_PREF_METHOD	"Method:"	/* Method name */
#define ADM_PREF_REQID	"Req#"		/* Request ID */

/*
 *---------------------------------------------------------------------
 * Private tracing message handling interfaces.
 *---------------------------------------------------------------------
 */

#ifdef __cplusplus
extern "C" {
#endif

extern	char	  *adm_diag_catcats(u_int, ...);
extern	boolean_t  adm_diag_catcmp(boolean_t, char *, char *);
extern	boolean_t  adm_diag_catin(char *, u_int, char *);
extern	int        adm_diag_msg2str(time_t, char *, char *, char *, u_int,
				    char *, u_int *);
extern	boolean_t  adm_diag_nextcat(char **, u_int *, char **);
extern	int	   adm_diag_set2(char *, char *, char *);
extern	char	  *adm_diag_stdcats(char *, char *, char *, char *, char *, char*);
extern	int        adm_diag_str2msg(char *, u_int, time_t *, char **, char **,
				    u_int *, u_int *);
extern	u_int      adm_diag_strsize(time_t, char *, char *, char *, u_int *,
				    u_int *);

#ifdef __cplusplus
}
#endif

#endif /* !_adm_diag_impl_h */

