
/*
 *  Copyright (c) 1990 Sun Microsystems, Inc.  All Rights Reserved.
 */

/*
 *******************************************************************************
 *
 *	This file contains the request ID handling routines exported
 *	from the administrative framework.  It contains:
 *
 *		adm_reqID_new()	   Generate a new request ID.
 *	
 *	NOTE: Access to the request counter in the request ID generator
 *	      is not thread-safe.  A mutex lock should be added to gaurded
 *	      against concurrent access to the counter.
 *
 *******************************************************************************
 */

#ifndef _adm_reqID_c
#define _adm_reqID_c

#pragma	ident	"@(#)adm_reqID.c	1.6	95/02/02 SMI"

#include <time.h>
#include <sys/types.h>
#include "adm_fw.h"
#include "adm_fw_impl.h"

/*
 *----------------------------------------------------------------------
 *
 * ADM_REQID_NEW( reqIDp ):
 *
 *	Generate a new administrative request identifier.  The identifier
 *	is written at the location pointed to by reqIDp.  This identifier
 *	will be unique within the generating host.  Thus, combined with
 *	a host identifier, this ID is unique for all administrative
 *	requests.
 *
 *	Upon normal completion, this routine return ADM_SUCCESS.
 *
 *----------------------------------------------------------------------
 */

int 
adm_reqID_new(
	Adm_requestID *reqIDp
	)
{
	time_t current_time;		/* Current system time in seconds */
	int stat;

	stat = adm_init();
	if (stat != ADM_SUCCESS) {
		return(stat);
	}

	current_time = time(NULL);
	if (current_time == (time_t) -1) {
		return(ADM_ERR_BADTIME);
	}
	reqIDp->clnt_pid = adm_pid;
	reqIDp->clnt_time = current_time;
	reqIDp->clnt_count = adm_nextIDcnt++;

	return(ADM_SUCCESS);
}

#endif /* !_adm_reqID_c */

