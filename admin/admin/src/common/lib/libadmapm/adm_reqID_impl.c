
/*
 *  Copyright (c) 1990 Sun Microsystems, Inc.  All Rights Reserved.
 */

/*
 *******************************************************************************
 *
 *	This file contains the request ID handling routines used
 *	internally within the administrative framework.  It contains:
 *
 *		adm_reqID_blank()     Set a request ID to blank (empty).
 *		adm_reqID_init()      Initialize the request ID handler.
 *		adm_reqID_str2rid()   Convert a request ID from string to
 *				      internal format.
 *		adm_reqID_rid2str()   Convert a request ID from interal format
 *				      to a string.
 *	
 *	NOTE: Access to the request counter in the request ID generator
 *	      is not thread-safe.  A mutex lock should be added to gaurded
 *	      against concurrent access to the counter.
 *
 *******************************************************************************
 */

#ifndef _adm_reqID_impl_c
#define _adm_reqID_impl_c

#pragma	ident	"@(#)adm_reqID_impl.c	1.5	92/01/28 SMI"

#include <time.h>
#include <sys/types.h>
#include "adm_fw.h"
#include "adm_fw_impl.h"

/*
 *----------------------------------------------------------------------
 *
 * ADM_REQID_BLANK( reqIDp ):
 *
 *	Set the specified request identifier to blank (empty).  That is,
 *	set the request identifier to a value indicating that it is
 *	not a valid request identifier.
 *
 *----------------------------------------------------------------------
 */

void
adm_reqID_blank(
	Adm_requestID *reqIDp
	)
{
	if (reqIDp == NULL) {
		return;
	}

	reqIDp->clnt_pid = ADM_BLANKPID;
	reqIDp->clnt_time = ADM_BLANKSEC;
	reqIDp->clnt_count = ADM_BLANKCOUNT;

	return;
}

/*
 *----------------------------------------------------------------------
 *
 * ADM_REQID_INIT():
 *
 *	This routine initializes the request ID handler.  It initializes
 *	the count of submitted requests.
 *
 *	Upon normal completion, this routine returns ADM_SUCCESS.
 *
 *----------------------------------------------------------------------
 */

int
adm_reqID_init()
{
	adm_nextIDcnt = ADM_BLANKCOUNT + 1;

	return(ADM_SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * ADM_REQID_STR2RID( strID, reqIDp, lenp ):
 *
 *	Convert the specified string (representing a request ID) to
 *	internal format.  The string format of a request ID is:
 *
 *		<process-ID>:<time-in-seconds>:<counter>
 *
 *	The internal request ID is written at the location pointer to by
 *	reqIDp and the number of bytes in the string format request ID is
 *	returned in *lenp.
 *
 *	Upon normal completion, this routine returns ADM_SUCCESS.
 *
 *----------------------------------------------------------------------
 */

int
adm_reqID_str2rid(
	char *strID,
	Adm_requestID *reqIDp,
	u_int *lenp
	)
{
	int ridlen;		/* Number of bytes in string fmt request ID */
	int matched_args;	/* Number of valid components parsed from */
				/* the request ID string */

	if (reqIDp == NULL) {
		return(ADM_SUCCESS);
	}
	if (strID == NULL) {
		return(ADM_ERR_BADREQID);
	}

	matched_args = sscanf(strID, ADM_RIDFMT2, &reqIDp->clnt_pid,
				&reqIDp->clnt_time,
				&reqIDp->clnt_count,
				&ridlen);
	if (lenp != NULL) {
		*lenp = ridlen;
	}
	if (matched_args != 3) {
		return(ADM_ERR_BADREQID);
	}
	return(ADM_SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * ADM_REQID_RID2STR( reqID, strID, buflen, strlenp):
 *
 *	Convert the specified request ID to a string.  The string format
 *	of a request ID is:
 *
 *		<process-ID>:<time-in-seconds>:<counter>
 *
 *	The resulting string is written to the location pointed at by
 *	strID and the *	length of the written string is returned in
 *	*strlenp.  buflen should be the length of the buffer pointed at
 *	by strID.
 *
 *	Upon normal completion, this routine returns ADM_SUCCESS.  If
 *	the buffer pointed to by strID is not long enough to hold the
 *	request ID, the function returns ADM_ERR_TOOLONG.
 *
 *----------------------------------------------------------------------
 */

int
adm_reqID_rid2str(
	Adm_requestID reqID,
	char *strID,
	u_int buflen,
	u_int *strlenp
	)
{
	int out_len;			/* Length of string format request ID */

	if (strID == NULL) {
		return(ADM_SUCCESS);
	}
	if (buflen <= ADM_MAXRIDLEN) {
		return(ADM_ERR_TOOLONG);
	}

	sprintf(strID, ADM_RIDFMT2, reqID.clnt_pid, reqID.clnt_time,
				    reqID.clnt_count, &out_len);

	if (strlenp != NULL) {
		*strlenp = out_len;
	}
	return(ADM_SUCCESS);
}

#endif /* !_adm_reqID_impl_c */

