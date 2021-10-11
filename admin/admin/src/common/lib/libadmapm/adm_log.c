
/*
 *  Copyright (c) 1990 Sun Microsystems, Inc.  All Rights Reserved.
 */

/*
 *******************************************************************************
 *
 *	This file contains exported functions for log handling in
 *	the administrative framework.  The functions defined here are
 *	exported to framework developers and the general public.
 *
 *	BUGS: Access to log ID structures and lists is not thread-safe.
 *	      Each should include a mutex lock to guard against concurrent
 *	      access.
 *
 *******************************************************************************
 */

#ifndef _adm_log_c
#define _adm_log_c

#pragma	ident	"@(#)adm_log.c	1.4	95/02/02 SMI"

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "adm_fw.h"
#include "adm_fw_impl.h"

/*
 *---------------------------------------------------------------------
 *
 * ADM_LOG_END( logIDp ):
 *
 *	End logging activities associated with a log.  The log
 *	corresponding to the specified information block is close,
 *	and the information block is removed from the internal list
 *	of logs.  If logIDp is set to ADM_ALL_LOGS, this routine
 *	ends all logging activities for all logs.
 *
 *	If successful, this routine returns ADM_SUCCESS.
 *
 *---------------------------------------------------------------------
 */

int
adm_log_end(Adm_logID *logIDp)
{
	Adm_logID *anyIDp;
	int stat;

	if (logIDp != ADM_ALL_LOGS) {		/* End one log */

	    stat = adm_log_end2(logIDp);

	} else {				/* End all logs */

	    stat = ADM_SUCCESS;
	    for (anyIDp=adm_first_logIDp; anyIDp != NULL; anyIDp=anyIDp->next) {
		stat = adm_log_end2(anyIDp);
		if (stat != ADM_SUCCESS) {
			break;
		}
	    }

	}

	return(stat);
}

/*
 *---------------------------------------------------------------------
 *
 * ADM_LOG_ENTRY( logIDp, msgtime, categories, message, header, syserrp ):
 *
 *	Attempt to add the specified message to the specified log.
 *	If logIDp is set to ADM_ALL_LOGS, an attempt will be made to
 *	write the message to all logs.  The message will only be
 *	written to a log if the log's category requirements (as
 *	specified through adm_log_start()) match the specified category
 *	list.
 *
 *	"header" specifies the log entry header to use when writing
 *	the message.  If "header" is set to ADM_LOG_STDHDR, the
 *	standard log entry header will be used.  msgtime should indicate
 *	the time at which the message was generated.
 *
 *	If this routine succeeds in writing the message to all
 *	appropriate logs, this routine returns ADM_SUCCESS.  Otherwise,
 *	it returns the first error it encounters and does not
 *	attempt to write the message to any other logs.  If the error
 *	condition is ADM_ERR_CANTWRITE, *syserrp will be set to the
 *	system error condition that caused the failure.
 *
 *	NOTE: Both standard and non-standard log entry headers
 *	      automatically include the time of the message.
 *
 *	NOTE: A newline is automatically added to end of all
 *	      non-standard headers.  Newlines should not therefore
 *	      included in "hdr_fmt".
 *
 *---------------------------------------------------------------------
 */

int
adm_log_entry(
	Adm_logID *logIDp,
	time_t msgtime,
	char *categories,
	char *message,
	char *header,
	int *syserrp
	)
{
	char time_str[ADM_MAXTIMELEN+1];   /* Message time in string format */
	size_t time_len;		   /* Length of time string */
	struct tm *msg_tm;
	Adm_logID *anyIDp;
	int stat;

	/*
	 * Convert the message time to a (localized) string.
	 */

	msg_tm = localtime(&msgtime);
	if (msg_tm == NULL) {
		return(ADM_ERR_TIMEFAIL);
	}
	time_len = strftime(time_str, (size_t)ADM_MAXTIMELEN, NULL, msg_tm);
	if (time_len == (size_t)0) {
		return(ADM_ERR_TIMEFAIL);
	}
	time_str[ADM_MAXTIMELEN] = NULL;

	/*
	 * Loop through all appropriate logs and attempt to write
	 * the log entry.
	 */

	if (logIDp != ADM_ALL_LOGS) {			/* Write to one log */

	    stat = adm_log_entry2(logIDp, categories, message, time_str,
				  header, syserrp);

	} else {					/* Write to all logs */

	    stat = ADM_SUCCESS;
	    for (anyIDp=adm_first_logIDp; anyIDp != NULL; anyIDp=anyIDp->next) {
		stat = adm_log_entry2(anyIDp, categories, message, time_str,
				      header, syserrp);
		if (stat != ADM_SUCCESS) {
			break;
		}
	    }
	}

	return(stat);
}

/*
 *---------------------------------------------------------------------
 *
 * ADM_LOG_INFO( logIDp, class, class_vers, method, host, domain,
 *		 client_host, reqID ):
 *
 *	Record the specified class name, class version, method name,
 *	host, domain, client host, and request ID in the specified log
 *	information block.  If logIDp is set to ADM_ALLCATS, this routine
 *	will set the information in the information blocks of all known logs.
 *
 *	If successful, this routine returns ADM_SUCCESS.
 *
 *---------------------------------------------------------------------
 */

int
adm_log_info(
	Adm_logID *logIDp,
	char *class,
	char *class_vers,
	char *method,
	char *host,
	char *domain,
	char *client_host,
	Adm_requestID reqID
	)
{
	char reqID_str[ADM_MAXRIDLEN+1];	/* Request ID in string form */
	Adm_logID *anyIDp;
	int stat;

	/*
	 * Convert request ID to string format.
	 */

	stat = adm_reqID_rid2str(reqID, reqID_str, ADM_MAXRIDLEN + 1, NULL);
	if (stat != ADM_SUCCESS) {
		return(stat);
	}

	/*
	 * Record data in log info block(s).
	 */

	if (logIDp != ADM_ALL_LOGS) {		/* Record to one info block */

	    stat = adm_log_info2(logIDp, class, class_vers, method, host,
				 domain, client_host, reqID, reqID_str);

	} else {				/* Record to all info blocks */

	    stat = ADM_SUCCESS;
	    for (anyIDp=adm_first_logIDp; anyIDp != NULL; anyIDp=anyIDp->next) {
		stat = adm_log_info2(anyIDp, class, class_vers, method, host,
				     domain, client_host, reqID, reqID_str);
		if (stat != ADM_SUCCESS) {
			break;
		}
	    }
	}

	return(stat);
}

/*
 *---------------------------------------------------------------------
 *
 * ADM_LOG_START( pathname, mode, cat_logging, and_flag, categories,
 *		  logIDpp ):
 *
 *	Initialize an administrative log.  A log with the specified
 *	pathname is opened (or created with the specified mode if it
 *	does not exist) so that tracing messages can be appended to it.
 *	The specified category logging indication, list of categories
 *	to filter on, and filter flag ("filter on all/any category in
 *	the list") are recorded in the log information block.
 *
 *	If successful, this routine sets *logIDpp to point to the
 *	information block for the new log and retrn ADM_SUCCESS.
 *
 *---------------------------------------------------------------------
 */

int
adm_log_start(
	char *pathname,
	int mode,
	int cat_logging,
	boolean_t and_flag,
	char *categories,
	Adm_logID **logIDpp
	)
{
	Adm_logID *newIDp;

	/*
	 * Verify parameters.
	 */

	if (pathname == NULL) {
		return(ADM_ERR_BADLOGNAME);
	}
	if ((cat_logging != ADM_CATLOG_OFF) &&
	    (cat_logging != ADM_CATLOG_BASIC) &&
	    (cat_logging != ADM_CATLOG_ALL)) {
		return(ADM_ERR_BADCATLOG);
	}

	/*
	 * Get a new log info block and record basic info.
	 */

	newIDp = adm_log_newh();
	if (newIDp == NULL) {
		return(ADM_ERR_NOMEM);
	}

	newIDp->cat_logging = cat_logging;
	newIDp->and_flag = and_flag;
	if (categories == NULL) {
		newIDp->categories = categories;
	} else {
		newIDp->categories = strdup(categories);
		if (newIDp->categories == NULL) {
			return(ADM_ERR_NOMEM);
		}
	}

	/*
	 * Open the log file and record the open info.
	 */

	newIDp->fd = open(pathname, O_WRONLY | O_APPEND | O_CREAT, mode);
	if (newIDp->fd == -1) {
		adm_log_freeh(newIDp);
		return(ADM_ERR_CANTOPEN);
	}

	newIDp->pathname = strdup(pathname);
	if (newIDp->pathname == NULL) {
		adm_log_freeh(newIDp);
		return(ADM_ERR_NOMEM);
	}

	/*
	 * Add the new info block to the list of log info blocks.
	 */

	newIDp->prev = NULL;
	newIDp->next = adm_first_logIDp;
	if (adm_first_logIDp != NULL) {
		adm_first_logIDp->prev = newIDp;
	}
	adm_first_logIDp = newIDp;

	/*
	 * Return the new info block.
	 */

	if (logIDpp != NULL) {
		*logIDpp = newIDp;
	}

	return(ADM_SUCCESS);
}

#endif /* !_adm_log_c */

