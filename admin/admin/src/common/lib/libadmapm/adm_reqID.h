
/*
 *  Copyright (c) 1990 Sun Microsystems, Inc.  All Rights Reserved.
 */

/*
 *******************************************************************************
 *
 *	This file contains the exported definitions for handling
 *	request IDs in the administrative framework.
 *
 *******************************************************************************
 */

#ifndef _adm_reqID_h
#define _adm_reqID_h

#pragma	ident	"@(#)adm_reqID.h	1.6	93/05/18 SMI"

#include <sys/types.h>
#include <time.h>
#include "adm_fw.h"

/*
 *----------------------------------------------------------------------
 * Request ID constants.
 *----------------------------------------------------------------------
 */

/* Field values for a blank request ID */
#define	ADM_BLANKPID	(pid_t) -1  /* Indicates PID missing in a request ID */
#define ADM_BLANKSEC	(time_t) 0  /* Indicates time missing in a request ID */
#define ADM_BLANKCOUNT	(u_long) 0  /* Indicates counter missing in a request ID */

/* Max. chars in a string request ID */
#define ADM_MAXRIDLEN	(ADM_MAXINTLEN+(2*ADM_MAXLONGLEN)+2)

/*
 *----------------------------------------------------------------------
 * Request ID structure.
 *----------------------------------------------------------------------
 */

#define adm_reqID_isblank(a) (a.clnt_pid == ADM_BLANKPID)
typedef struct Adm_requestID Adm_requestID;	/* Request identifier. */
struct Adm_requestID {
	pid_t		clnt_pid;	/* PID of client making request */
	time_t		clnt_time;	/* Time at which request was made */
					/* (seconds from time(2)) */
	u_long		clnt_count;	/* Request counter value */
};

/*
 *----------------------------------------------------------------------
 * Exported request ID handling routines.
 *----------------------------------------------------------------------
 */

#ifdef __cplusplus
extern "C" {
#endif

extern	int	adm_reqID_new(Adm_requestID *);

#ifdef __cplusplus
}
#endif

#endif /* !_adm_reqID_h */

