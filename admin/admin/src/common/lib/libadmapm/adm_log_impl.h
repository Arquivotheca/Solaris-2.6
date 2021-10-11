
/*
 *  Copyright (c) 1990 Sun Microsystems, Inc.  All Rights Reserved.
 */

/*
 ****************************************************************************
 *
 *	This file contains the private definitions for handling
 *	logging in the administrative framework.  It includes
 *	the definitions for:
 *
 *		o Miscellaneous constants.
 *		o Private global variables.
 *		o Private interfaces for handling logs.
 *
 ****************************************************************************
 */

#ifndef _adm_log_impl_h
#define _adm_log_impl_h

#pragma	ident	"@(#)adm_log_impl.h	1.4	93/05/18 SMI"

/*
 *---------------------------------------------------------------------
 * Miscellaneous constants.
 *---------------------------------------------------------------------
 */

#define ADM_LOG_RIDHDR		"ReqID# " /* ReqID portion of log msg header */
#define ADM_LOG_LOCKWAIT	30	  /* Seconds to wait for lock on log */

/*
 *---------------------------------------------------------------------
 * Private global variables.
 *---------------------------------------------------------------------
 */

extern Adm_logID *adm_first_logIDp;	/* Ptr. to first log info block in */
					/* linked list of valid logs */

/*
 *---------------------------------------------------------------------
 * Private log handling interfaces.
 *---------------------------------------------------------------------
 */

#ifdef __cplusplus
extern "C" {
#endif

extern int        adm_log_end2(Adm_logID *);
extern int        adm_log_entry2(Adm_logID *, char *, char *, char *, char *,
				 int *);
extern int        adm_log_freeh(Adm_logID *);
extern int        adm_log_info2(Adm_logID *, char *, char *, char *, char *,
				char *, char *, Adm_requestID, char *);
extern Adm_logID *adm_log_newh();

#ifdef __cplusplus
}
#endif

#endif /* !_adm_log_impl_h */

