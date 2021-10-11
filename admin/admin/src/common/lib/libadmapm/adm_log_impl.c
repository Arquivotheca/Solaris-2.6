
/*
 *  Copyright (c) 1990 Sun Microsystems, Inc.  All Rights Reserved.
 */

/*
 *******************************************************************************
 *
 *	This file contains private functions for log handling in
 *	the administrative framework.  The functions defined here are
 *	exported to framework developers, but not the general public.
 *
 *	BUGS: Access to log ID structures and lists is not thread-safe.
 *	      Each should include a mutex lock to guard against concurrent
 *	      access.
 *
 *******************************************************************************
 */

#ifndef _adm_log_impl_c
#define _adm_log_impl_c

#pragma	ident	"@(#)adm_log_impl.c	1.8	95/09/08 SMI"

#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/uio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "adm_fw.h"
#include "adm_fw_impl.h"
#include "adm_amsl_impl.h"
#include "adm_om_impl.h"

extern int writev(int, const struct iovec *, int);

/*
 *---------------------------------------------------------------------
 *
 * ADM_LOG_END2( logIDp ):
 *
 *	Close the log associated with the specified log information
 *	block and remove the block from the list of information
 *	blocks.
 *
 *	If successful, this routine always returns ADM_SUCCESS.
 *
 *---------------------------------------------------------------------
 */

int
adm_log_end2(Adm_logID *logIDp)
{
	int stat;

	if (logIDp == NULL) {
		return(ADM_SUCCESS);
	}

	/*
	 * Remove the block from the list of info blocks.
	 */

	if (logIDp->next != NULL) {
		logIDp->next->prev = logIDp->prev;
	}
	if (logIDp->prev != NULL) {
		logIDp->prev->next = logIDp->next;
	} else {
		adm_first_logIDp = logIDp->next;
	}

	/*
	 * Close the log file if its open
	 */

	if (logIDp->fd != -1) {
		close(logIDp->fd);
		logIDp->fd = -1;
	}

	/*
	 * De-allocated the info block.
	 */

	stat = adm_log_freeh(logIDp);

	return(stat);
}

/*
 *---------------------------------------------------------------------
 *
 * ADM_LOG_ENTRY2( logIDp, categories, message, time_str, header, syserrp ):
 *
 *	Attempt to add the specified message to the specified log.
 *	The message will only be written to the log if the log's
 *	category requirements (as specified through adm_log_start())
 *	match the specified category list.
 *
 *	The format of the log message will be:
 *
 *		<time_str> <header>
 *		  <message>
 *		  <categories>
 *
 *	where <header> is the specified header string or the standard
 *	header if "header" is set to ADM_LOG_STDHDR, and the <categories>
 *	list is only included if the log information block specifies so
 *	(see the cat_logging field in the log information block).
 *
 *	If successful, this routine returns ADM_SUCCESS.  If an error
 *	occurs writing to the log file, the routine returns ADM_ERR_CANTWRITE
 *	and sets *syserrp to the system error condition causing the
 *	failure.
 *
 *	NOTE: time_str should represent the (printable) localized
 *	      time at which the message was generated.
 *
 *	NOTE: A newline is automatically added to end of all
 *	      lines.
 *
 *	NOTE: Filtering should be added to limit the list of categories
 *	      included with a message.  For example, filtering should
 *	      be added so that only standard categories are included
 *	      with the message entry.
 *
 *	NOTE: Should host and domain be included in the log header?
 *
 *---------------------------------------------------------------------
 */

int
adm_log_entry2(
	Adm_logID *logIDp,
	char *categories,
	char *message,
	char *time_str,
	char *header,
	int *syserrp
	)
{
	struct iovec iov[30];
	int i;
	int j;
	int stat;

/*
 *------------------------
 * The following macro is used within this routine to repeatedly add
 * entries to a iov vector of information to write to a log.
 *------------------------
 */

#define ADM_IOV_SET(iov, i, val)				\
								\
		iov[i].iov_base = val;				\
		iov[i].iov_len = strlen(iov[i].iov_base);	\
		i++;
/*
 *------------------------
 */
	/*
	 * Determine if the specified categories match those that
	 * that should be included in the specified log.
	 */

	if (!adm_diag_catcmp(logIDp->and_flag, logIDp->categories, categories)) {
		return(ADM_SUCCESS);
	}

	stat = adm_lock(logIDp->fd, ADM_LOCK_AP, ADM_LOG_LOCKWAIT);
	if (stat != 0) {
		return(ADM_ERR_LOCKWAIT);
	}

	/*
	 * Set up the time and header portion of the log entry.  If the
	 * standard header is used, it will look like:
	 *
 	 *	<time>  <method>(<class>.<vers>)  ReqID# <reqID>  <client_host>
	 *
	 * In either case, the time is included.
	 */

	i = 0;
	ADM_IOV_SET(iov, i, time_str);
	ADM_IOV_SET(iov, i, "  ");

	if (header != ADM_LOG_STDHDR) {		/* Non-standard header */

	    ADM_IOV_SET(iov, i, header);

	} else {				/* Standard header */

	    ADM_IOV_SET(iov, i, (logIDp->method == NULL ?
				    ADM_BLANK : logIDp->method));
	    ADM_IOV_SET(iov, i, "(")
	    ADM_IOV_SET(iov, i, (logIDp->adm_class == NULL ?
				    ADM_BLANK : logIDp->adm_class));
	    if (logIDp->class_vers != NULL) {
	    	ADM_IOV_SET(iov, i, EXTENSION);
	    	ADM_IOV_SET(iov, i, logIDp->class_vers);
	    }
	    ADM_IOV_SET(iov, i, ")  ");
	    ADM_IOV_SET(iov, i, ADM_LOG_RIDHDR);
	    ADM_IOV_SET(iov, i, (logIDp->reqID_str == NULL ?
				    ADM_BLANK : logIDp->reqID_str));
	    ADM_IOV_SET(iov, i, "  ");
	    ADM_IOV_SET(iov, i, (logIDp->client_host == NULL ?
				    ADM_BLANK : logIDp->client_host));
	}

	ADM_IOV_SET(iov, i, "\n");

	/*
	 * Set up the log message and (if appropriate) list of categories.
	 * For now, either all or none * of the categories are included.
	 * Later, filtering could be added to limit the categories printed.
	 */

	ADM_IOV_SET(iov, i, "  ");
	ADM_IOV_SET(iov, i, (message == NULL ? ADM_BLANK : message));
	ADM_IOV_SET(iov, i, "\n");

	if ((logIDp->cat_logging != ADM_CATLOG_OFF) &&
	    (categories != ADM_NOCATS)) {
		ADM_IOV_SET(iov, i, "  [");
		ADM_IOV_SET(iov, i, categories);
		ADM_IOV_SET(iov, i, "]\n");
	}


	/*
	 * Write the complete entry to the log.  Write IOV_MAX elements
	 * of the vector at a time.
	 */

#define IOV_MAX 16
	stat = 0;
	for (j = 0; j < i; j += IOV_MAX) {
		stat = writev(logIDp->fd, &(iov[j]),
			      ((i-j) > IOV_MAX ? IOV_MAX : (i-j)));
		if (stat == -1) {
			break;
		}
	}
	adm_unlock(logIDp->fd);
	if (stat == -1) {
		if (syserrp != NULL) {
			*syserrp = errno;
		}
		return(ADM_ERR_CANTWRITE);
	}

	return(ADM_SUCCESS);
}

/*
 *---------------------------------------------------------------------
 *
 * ADM_LOG_FREEH( logIDp ):
 *
 *	Free the space occupied by the specified log information
 *	block.
 *
 *	This routine returns always returns ADM_SUCCESS.
 *
 *	NOTE: The specified log information block pointer will
 *	      no longer be valid after this call completes.
 *
 *---------------------------------------------------------------------
 */

int
adm_log_freeh(Adm_logID *logIDp)
{
	if (logIDp == NULL) {
		return(ADM_SUCCESS);
	}

	/*
	 * Free the components of the information block.
	 */

	if (logIDp->pathname != NULL) {
		free(logIDp->pathname);
	}
	if (logIDp->categories != NULL) {
		free(logIDp->categories);
	}
	if (logIDp->adm_class != NULL) {
		free(logIDp->adm_class);
	}
	if (logIDp->class_vers != NULL) {
		free(logIDp->class_vers);
	}
	if (logIDp->method != NULL) {
		free(logIDp->method);
	}
	if (logIDp->host != NULL) {
		free(logIDp->host);
	}
	if (logIDp->domain != NULL) {
		free(logIDp->domain);
	}
	if (logIDp->client_host != NULL) {
		free(logIDp->client_host);
	}
	if (logIDp->reqID_str != NULL) {
		free(logIDp->reqID_str);
	}

	/*
	 * Free the information block.
	 */

	free(logIDp);

	return(ADM_SUCCESS);
}

/*
 *---------------------------------------------------------------------
 *
 * ADM_LOG_INFO2( logIDp, adm_class, class_vers, method, host, domain,
 *		  client_host, reqID, reqID_str ):
 *
 *	Specify data for a log information block.  The specified
 *	class name, class version, method name, host, domain, client
 *	host, request ID, and string format request ID are recored in
 *	the specified log information block to be used when writing
 *	entries to log file corresponding to this information block.
 *
 *	If successful, this routine returns ADM_SUCCESS.
 *
 *---------------------------------------------------------------------
 */

int
adm_log_info2(
	Adm_logID *logIDp,
	char *adm_class,
	char *class_vers,
	char *method,
	char *host,
	char *domain,
	char *client_host,
	Adm_requestID reqID,
	char *reqID_str
	)
{

/*
 *------------------------------
 *
 * The following macro is used within this routine to set to values
 * of most fields in the specified log information block.
 */

#define ADM_LOG_SETINFO2(field)						\
									\
	    if (logIDp->field != NULL) {				\
		free(logIDp->field);					\
	    }								\
	    logIDp->field = (field == NULL ? field : strdup(field));	\
	    if ((logIDp->field == NULL) && (field != NULL)) {		\
		    return(ADM_ERR_NOMEM);					\
	    }

/*------------------------------
 */

	/*
	 * Verify the log info.
	 */

	if (logIDp == NULL) {
		return(ADM_ERR_BADLOGID);
	}

	/*
	 * Record the data in the log info block.
	 */

	ADM_LOG_SETINFO2(adm_class);
	ADM_LOG_SETINFO2(class_vers);
	ADM_LOG_SETINFO2(method);
	ADM_LOG_SETINFO2(host);
	ADM_LOG_SETINFO2(domain);
	ADM_LOG_SETINFO2(client_host);
	ADM_LOG_SETINFO2(reqID_str);
	logIDp->reqID = reqID;

	return(ADM_SUCCESS);
}

/*
 *---------------------------------------------------------------------
 *
 * ADM_LOG_NEWH():
 *
 *	Create a new (initialized) log information block.  The block
 *	will initially be empty, but later may be filled in with
 *	information about a real log.
 *
 *	This routine returns a pointer to the new block.
 *
 *---------------------------------------------------------------------
 */

Adm_logID *
adm_log_newh()
{
	Adm_logID *newIDp;

	newIDp = (Adm_logID *) malloc(sizeof(Adm_logID));
	if (newIDp == NULL) {
		return(newIDp);
	}

	newIDp->fd          = -1;
	newIDp->pathname    = NULL;
	newIDp->cat_logging = ADM_CATLOG_OFF;
	newIDp->and_flag    = B_FALSE;
	newIDp->categories  = NULL;
	newIDp->adm_class	    = NULL;
	newIDp->class_vers  = NULL;
	newIDp->method	    = NULL;
	newIDp->host	    = NULL;
	newIDp->domain	    = NULL;
	newIDp->client_host = NULL;
	newIDp->reqID_str   = NULL;

	adm_reqID_blank(&(newIDp->reqID));

	return(newIDp);
}

#endif /* !_adm_log_impl_c */

