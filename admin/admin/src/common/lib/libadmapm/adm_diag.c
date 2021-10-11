
/*
 *  Copyright (c) 1990 Sun Microsystems, Inc.  All Rights Reserved.
 */

/*
 *******************************************************************************
 *
 *	This file contains exported functions for handling diagnostic
 *	(or tracing) information in the administrative framework.  The
 *	functions defined here are for framework developers and the
 *	general public.
 *
 *      NOTE: Routines similar to adm_diag_set() and adm_diag_setf()
 *            should be added that take localization file keys instead
 *            of text messages.
 *
 *	BUGS: Access to tracing handles is not thread-safe.  Each handle
 *	      should include a mutex lock to guard against concurrent
 *	      access to the handle.
 *
 *******************************************************************************
 */

#ifndef _adm_diag_c
#define _adm_diag_c

#pragma	ident	"@(#)adm_diag.c	1.3	92/01/28 SMI"

#include <sys/types.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <malloc.h>
#include "adm_fw.h"
#include "adm_fw_impl.h"

/*
 *---------------------------------------------------------------------
 *
 * ADM_DIAG_FMT( syserrp, header, msgtime, categories, message ):
 *
 *	Specify a tracing message.  The specified message, along with
 *	associated category list and message time, are written to
 *	all appropriate logs.  If "header" is ADM_LOG_STDHDR, the
 *	logged message will include the standard header.  Otherwise,
 *	the log entry header will contain the specified string
 *	witht the (localized) time prepended.
 *
 *	If successful, this routine returns ADM_SUCCESS.  If the routine
 *	returns ADM_ERR_CANTWRITE, *syserrp will be set to the system error
 *	that prevented the routine from writing to a log.
 *
 *	NOTE: This routine could later be extended to also write
 *	      the message to syslogd, or anywhere else.
 *
 *---------------------------------------------------------------------
 */

int
adm_diag_fmt(
	int *syserrp,
	char *header,
	time_t msgtime,
	char *categories,
	char *message
	)
{
	return(adm_log_entry(ADM_ALL_LOGS, msgtime, categories, message,
			     header, syserrp));
}

/*
 *---------------------------------------------------------------------
 *
 * ADM_DIAG_FMTF( syserrp, header, msgtime, categories,
 *		  message, arg1, arg2, ... ):
 *
 *	Specify a tracing message.  The specified message template is
 *	formatted, using the specified substitution arguments, and
 *	written to all appropriate logs along with the associated category
 *	list and message time.  If "header" is ADM_LOG_STDHDR, the
 *      logged message will include the standard header.  Otherwise, 
 *      the log entry header will contain the specified string 
 *      witht the (localized) time prepended.
 *
 *	If successful, this routine returns ADM_SUCCESS.  If the routine
 *	returns ADM_ERR_CANTWRITE, *syserrp will be set to the system error
 *	that prevented the routine from writing to a log.
 *
 *	NOTE: This routine could later be extended to also write
 *	      the message to syslogd, or anywhere else.
 *
 *	NOTE: The formatted tracing message should not exceed
 *	      ADM_MAXDIAGMSG bytes in length.
 *
 *---------------------------------------------------------------------
 */

int
adm_diag_fmtf(
	int *syserrp,
	char *header,
	time_t msgtime,
	char *categories,
	char *message,
	...
	)
{
	char msg[ADM_MAXDIAGMSG+1];	/* Formatted tracing message */
	va_list msg_args;
	int stat;

	if (message != NULL) {
		va_start(msg_args, message);
		stat = vsprintf(msg, message, msg_args);
		va_end(msg_args);
		if (stat < 0) {
			return(ADM_ERR_CANTFMT);
		}
	}

	return(adm_diag_fmt(syserrp, header, msgtime, categories,
			    (message == NULL ? message : msg)));
}

/*
 *---------------------------------------------------------------------
 *
 * ADM_DIAG_SET( categories, message ):
 *
 *	Specify a tracing message and associated set of diagnostic
 *	categories to which that message belongs.  In addition to
 *	the specified categories, the tracing message will be included
 *	in the standard diagnostic categories for the current method
 *	invocation.
 *
 *	The tracing message is written to STDFMT as:
 *
 *	    <size>#<time>:<catlen>:<msglen>: <categories> <message>\n
 *
 *	where <size> is the total number of bytes between the tracing
 *	message specification indicator ("#") and the appended newline
 *	("\n") inclusive, <time> is the time(2) of the tracing message,
 *	<catlen> is the total number of bytes in the <categories> string
 *	(or "-" if no categories apply to this message), <msglen>
 *	is the total number of bytes in the <message> string (or "-" if
 *	the message specification is NULL), <categories> is a comma-
 *	separated list of diagnostic categories to which the tracing
 *	message belongs (including the standard categories), and <message>
 *	is the tracing message.
 *
 *	Upon successful completion, this routine returns ADM_SUCCESS.
 *
 *	NOTE: The space preceding the <categories> or <message>
 *	      fields will not be included in the tracing specification
 *	      if either of those fields is undefined (i.e. the
 *	      corresponding length is set to "-").
 *
 *---------------------------------------------------------------------
 */

int
adm_diag_set(
	char *categories,
	char *message
	)
{
	int stat;

	stat = adm_init();
	if (stat != ADM_SUCCESS) {
		return(stat);
	}

	return(adm_diag_set2(categories, adm_stdcats, message));
}

/*
 *---------------------------------------------------------------------
 *
 * ADM_DIAG_SETF( categories, message, arg1, arg2, ... ):
 *
 *	Specify a tracing message (with substitution arguments) and
 *	associated set of diagnostic categories to which that message
 *	belongs.  In addition to the specified categories, the tracing
 *	message will be included in the standard diagnostic categories
 *	for the current method invocation.
 *
 *	The tracing message is written to STDFMT as:
 *
 *	    <size>#<time>:<catlen>:<msglen>: <categories> <message>\n
 *
 *	where <size> is the total number of bytes between the tracing
 *	message specification indicator ("#") and the appended newline
 *	("\n") inclusive, <time> is the time(2) of the tracing message,
 *	<catlen> is the total number of bytes in the <categories> string
 *	(or "-" if no categories apply to this tracing message), <msglen>
 *	is the total number of bytes in the <message> string (or "-" if
 *	the message specification is NULL), <categories> is a comma-
 *	separated list of diagnostic categories to which the tracing
 *	message belongs (including standard categories), and <message>
 *	is the formatted tracing message.
 *
 *	Upon successful completion, this routine returns ADM_SUCCESS.
 *
 *	NOTE: The space preceding the <categories> or <message>
 *	      fields will not be included in the tracing specification
 *	      if either of those fields is undefined (i.e. the
 *	      corresponding length is set to "-").
 *
 *	NOTE: The formatted tracing message (with arguments substituted)
 *	      should not exceed ADM_MAXDIAGMSG bytes in length.
 *
 *---------------------------------------------------------------------
 */

int
adm_diag_setf(
	char *categories,
	char *message,
	...
	)
{
	boolean_t is_msg;		/* Is a tracing message specified? */
	char buf[ADM_MAXDIAGMSG + 1];
	va_list diag_args;
	int stat;

	/*
	 * Format the tracing message.
	 */

	va_start(diag_args, message);

	if (message != NULL) {
		stat = vsprintf(buf, message, diag_args);
		if (stat < 0) {
			return(ADM_ERR_CANTFMT);
		}
		is_msg = B_TRUE;
	} else {
		is_msg = B_FALSE;
	}

	va_end(diag_args);

	/*
	 * Output the formatted trace message.
	 */

	stat = adm_diag_set(categories, (is_msg ? buf : message));

	return(stat);
}

#endif /* !_adm_diag_c */

