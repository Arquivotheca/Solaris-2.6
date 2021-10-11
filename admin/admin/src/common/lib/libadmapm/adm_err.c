
/*
 *  Copyright (c) 1990 Sun Microsystems, Inc.  All Rights Reserved.
 */

/*
 ****************************************************************************
 *
 *	This file contains administrative framework error messages and
 *	application interfaces for handling errors within the administrative
 *	framework.  It includes the following interfaces:
 *
 *		adm_err_cleanup()   Specify a method cleanliness status.
 *		adm_err_freeh()	    Free an administrative error structure.
 *		adm_err_set()	    Specify a method error.
 *		adm_err_setf()	    Specify a method error w/ arguements.
 *
 *	Note: Access to error structures is not thread-safe.  A mutex lock
 *	      should be added to gaurd against concurrent access.
 *
 *      NOTE: Routines similar to adm_err_set() and adm_err_setf() should
 *	      be added that take a localization file key instead of a text
 *	      message.
 * 
 ****************************************************************************
 */

#ifndef _adm_err_c
#define _adm_err_c

#pragma	ident	"@(#)adm_err.c	1.9	95/05/08 SMI"

#include <stdarg.h>
#include <stdio.h>
#include <locale.h>
#include <malloc.h>
#include "adm_fw.h"
#include "adm_fw_impl.h"

/*
 *--------------------------------------------------------------------
 *
 * ADM_ERR_CLEANUP( categories, cleanup ):
 *
 *	Specify the cleanliness status of a method error.  The
 *	specified cleanliness status (cleanup) is written to the
 *	standard formatted output file descriptor in the form:
 *
 *		<size>=<cleanup>\n
 *
 *	where <size> is the total number of bytes between the indicator
 *	character ("=") and the appended newline ("\n"), inclusive.
 *
 *	An automatic tracing message containing the cleanliness
 *	indication is also generated in the specified categories, as well
 *	as in several default categories.
 *
 *	Successive calls to this interface override previously returned
 *	values.  However, successive calls generate multiple tracing
 *	messages.
 *
 *	Upon successful completion, this routine returns ADM_SUCCESS.
 *
 *	Note: This routine is not thread safe.  If multiple threads
 *	      concurrently attempt to write to STDFMT, unpredictable
 *	      results will occur.  A mutex lock on the STDMFT pipe is
 *	      needed to gaurd against concurrent access.
 *
 *--------------------------------------------------------------------
 */

int
adm_err_cleanup(
	char *categories,
	u_int cleanup
	)
{
	int size;
	char buf[ADM_MAXINTLEN+2];
	int stat;

	/*
	 * Initialize method if it has not already been initialized.
	 */

	stat = adm_init();
	if (stat != ADM_SUCCESS) {
		return(stat);
	}

	if (adm_stdfmt == NULL) {
		return(ADM_ERR_METHONLY);
	}

	/*
	 * Format cleanliness status message and write it to STDFMT.
	 */

	size = sprintf(buf, "%u", cleanup);
	if (size < 0) {
		return(ADM_ERR_OUTFAIL);
	}

	size += 2;
	stat = fprintf(adm_stdfmt, "%d%s%s\n", size, ADM_CLEANMARKER, buf);
	if (stat == EOF) {
		return(ADM_ERR_OUTFAIL);
	}

	return(ADM_SUCCESS);
}

/*
 *--------------------------------------------------------------------
 *
 *  ADM_ERR_FREEH( errorp ):
 *
 *	Free the specified error structure.  This routine always
 *	returns ADM_SUCCESS.
 *
 *	Note: The error structure pointer and any pointers to its
 *	      associated  structures will be invalid after this
 *	      routine completes.
 *
 *--------------------------------------------------------------------
 */

int
adm_err_freeh(
	Adm_error *errorp
	)
{
	if (errorp == NULL) {			/* Make sure handle is valid */
		return(ADM_SUCCESS);
	}

	if (errorp->message != NULL) {		/* Free error message */
		free(errorp->message);
	}
	if (errorp->unfmt_txt != NULL) {	/* Free unformatted error */
		free(errorp->unfmt_txt);
	}
	free(errorp);				/* Free error structure */

	return(ADM_SUCCESS);
}
	
/*
 *--------------------------------------------------------------------
 *
 * ADM_ERR_SET( categories, code, message ):
 *
 *	Return the specified error code and message as the (formatted)
 *	error of the invoking administrative method.  The error code
 *	and (optional) message are written to the standard formatted
 *	output file descriptor in the form:
 *
 *		<size>-<code>:<type> <message>\n	(message specified)
 *		<size>-<code>:<type>\n			(no message)
 *
 *	where <size> is the total number of characters between the
 *	error indication character (-) and the appended newline (\n),
 *	inclusize; and, <type> is the integer (ADM_ERR_CLASS) indicating
 *	that this is a method generated error.
 *
 *	An automatic tracing message containing the error is also
 *	generated in the specified categories, as well as in several
 *	default categories.
 *
 *	Successive calls to this interface (or adm_err_setf) override
 *	previously returned errors.  However, successive calls generate
 *	multiple tracing messages.
 *
 *	Upon successful completion, this routine returns ADM_SUCCESS.
 *
 *	Note: This routine is not thread safe.  If multiple threads
 *	      concurrently attempt to write errors (or arguments) to
 *	      STDFMT, unpredictable results will occur.  A mutex lock
 *	      on the STDMFT pipe is needed to gaurd against concurrent
 *	      access.
 *
 *--------------------------------------------------------------------
 */

int
adm_err_set(
	char *categories,
	int code,
	char *message
	)
{
	int stat;

	/*
	 * Initialize method if it has not already been initialized.
	 */

	stat = adm_init();
	if (stat != ADM_SUCCESS) {
		return(stat);
	}

	if (adm_stdfmt == NULL) {
		return(ADM_ERR_METHONLY);
	}

	/*
	 * Write error to STDFMT.
	 */

	return(adm_err_set2(adm_stdfmt, categories, code, ADM_ERR_CLASS,
			    message));
}

/*
 *--------------------------------------------------------------------
 *
 * ADM_ERR_SETF( categories, code, message, arg1, arg2, ... ):
 *
 *	Return the specified error code and message (with substitution
 *	arguments) as the (formatted) error of the invoking administrative
 *	method.  The error code and (optional) message are written to
 *	the standard formatted output file descriptor in the form:
 *
 *		<size>-<code>:<type> <message>\n	(message specified)
 *		<size>-<code>:<type>\n			(no message)
 *
 *	where <size> is the total number of characters between the
 *	error indication character (-) and the appended newline (\n),
 *	inclusize; and, <type> is the integer (ADM_ERR_CLASS) indicating
 *	that this is a method generated error.
 *
 *	An automatic tracing message containing the error is also
 *	generated in the specified categories, as well as in several
 *	default categories.
 *
 *	Successive calls to this interface (or adm_err_set) override
 *	previously returned errors.  However, successive calls generate
 *	multiple tracing messages.
 *
 *	Upon successful completion, this routine returns ADM_SUCCESS.
 *
 *	Note: The formatted error message (with arguments substituted)
 *	      should not exceed ADM_MAXERRMSG bytes in length.
 *
 *	Note: This routine is not thread safe.  If multiple threads
 *	      concurrently attempt to write errors (or arguments) to
 *	      STDFMT, unpredictable results will occur.  A mutex lock
 *	      on the STDMFT pipe is needed to gaurd against concurrent
 *	      access.
 *
 *--------------------------------------------------------------------
 */

int
adm_err_setf(
	char *categories,
	int code,
	char *message,
	...
	)
{
	char *err_msg = NULL;	/* Formatted error message */
	va_list err_args;
	int stat;
	int ret;

	va_start(err_args, message);

	/*
	 * Initialize method if it has not already been initialized.
	 */

	stat = adm_init();
	if (stat != ADM_SUCCESS) {
		va_end(err_args);
		return(stat);
	}

	if (adm_stdfmt == NULL) {
		va_end(err_args);
		return(ADM_ERR_METHONLY);
	}

	/*
	 * Format error message, if one is specified.
	 */

	if (message != NULL) {
		/*
		 * Allocate needed space for err_msg and then
		 * put vsprintf output into it.
		 * Note: the allocated buffer needs to be freed
		 *	 somewhere below.
		 */
		vsprintf_alloc(&err_msg, message, err_args);
	}

	/*
	 * Write error to STDFMT pipe.
	 */

	va_end(err_args);
	ret = adm_err_set2(adm_stdfmt, categories, code, ADM_ERR_CLASS,
			    (message == NULL ? NULL : err_msg));
	if (err_msg) free(err_msg);
	return(ret);
}

#endif /* !_adm_err_c */
