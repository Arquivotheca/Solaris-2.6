
/*
 *  Copyright (c) 1990 Sun Microsystems, Inc.  All Rights Reserved.
 */

/*
 ****************************************************************************
 *
 *	This file contains private error handling routines used
 *	within the administrative framework.  It includes the following
 *	interfaces:
 *
 *		adm_err_fmt()	    Set fmt'd error entry in an error struct.
 *		adm_err_fmtf()	    Set fmt'f error entry w/ arguments.
 *		adm_err_fmt2()      Worker routine for adm_err_fmt() and
 *				    adm_err_fmtf().
 *		adm_err_hdr2str()   Create STDFMT error spec. header.
 *		adm_err_isentry()   Does error struct contain a fmt'd error?
 *		adm_err_msg()	    Return error messages for admin error codes.
 *		adm_err_newh()	    Create a new admin error structure.
 *		adm_err_reset()	    Set an admin error structure to indicate
 *				    success.
 *		adm_err_set2()	    Create formatted error spec. and write
 *				    it to a file ptr.
 *		adm_err_snm2str()   Create a SNM error message containing
 *				    an admin error type, cleanup, and
 *				    optional message.
 *		adm_err_str2cln()   Convert cleanup status from string
 *				    form to C form.
 *		adm_err_str2err()   Convert error in string form to C form.
 *		adm_err_str2snm()   Convert a SNM error message to its
 *				    component admin error type, cleanup,
 *				    and optional message.
 *		adm_err_unfmt()	    Set unfmt'd entry in an error struct.
 *
 *	NOTE: Routines similar to adm_err_fmt() and adm_err_fmtf()
 *	      should be added that take localization file keys instead
 *	      of text messages.
 *
 *	NOTE: A "categories" specification should be added to the
 *	      adm_err_fmt(), adm_err_fmtf(), and adm_err_fmt2() routines.
 *
 ****************************************************************************
 */

#ifndef _adm_err_impl_c
#define _adm_err_impl_c

#pragma	ident	"@(#)adm_err_impl.c	1.17	95/06/02 SMI"

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <locale.h>
#include <sys/types.h>
#include "adm_fw.h"
#include "adm_fw_impl.h"

/*
 *--------------------------------------------------------------------
 *
 * ADM_ERR_FMT( errorp, code, type, cleanup, message ):
 *
 *	Set the value of the formatted error entry in the specified
 *	administrative error structure.  The specifed error code,
 *	type, cleanliness indication, and optional error message
 *	are placed in the corresponding fields of the error
 *	structure.
 *
 *	Upon successful completion, this routine returns ADM_SUCCESS.
 *
 *	Note: The error message (if specified) is copied and a pointer
 *	      to the copy is placed in the error structure.
 *
 *--------------------------------------------------------------------
 */

int
adm_err_fmt(
	Adm_error *errorp,
	int code,
	u_int type,
	u_int cleanup,
	char *message
	)
{
	char *msg_cpy;		/* Copy of formatted error message */
	int stat;

	/*
	 * Copy the error message.
	 */

	if (message != NULL) {
		msg_cpy = strdup(message);
		if (msg_cpy == NULL) {
			return(ADM_ERR_NOMEM);
		}
	} else {
		msg_cpy = NULL;
	}

	/*
	 * Copy error information to error structure.
	 */

	stat = adm_err_fmt2(errorp, code, type, cleanup, msg_cpy);
	if ((stat != ADM_SUCCESS) && (msg_cpy != NULL)) {
		free(msg_cpy);
	}

	return(stat);
}

/*
 *--------------------------------------------------------------------
 *
 * ADM_ERR_FMTF( errorp, code, type, cleanup, message, arg1, arg2, ... ):
 *
 *	Set the value of the formatted error entry in the specified
 *	administrative error structure.  The routines processes the
 *	specified error message format string and corresponding
 *	substitution arguments.  The resulting error message, along
 *	with the specifed error code, type, and cleanliness indication,
 *	are placed in the corresponding fields of the error structure.
 *
 *	Upon successful completion, this routine returns ADM_SUCCESS.
 *
 *--------------------------------------------------------------------
 */

int
adm_err_fmtf(
	Adm_error *errorp,
	int code,
	u_int type,
	u_int cleanup,
	char *message,
	...
	)
{
	char *err_msg = NULL;		/* Formatted error message */
	va_list err_args;
	int stat;

	va_start(err_args, message);

	/*
	 * Format the error message.
	 */

	if (message != NULL) {
		/*
		 * Allocate needed space for err_msg and then
		 * put vsprintf output into it.
		 * Note: the allocated buffer needs to be freed
		 *	 somewhere below.
		 */
		if (vsprintf_alloc(&err_msg, message, err_args) == NULL) {
		    va_end(err_args);
		    return(ADM_ERR_NOMEM);
		}
	}

	/*
	 * Copy error information to error structure.
	 */

	stat = adm_err_fmt2(errorp, code, type, cleanup, err_msg);
	if ((stat != ADM_SUCCESS) && (err_msg != NULL)) {
		free(err_msg);
	}

	va_end(err_args);
	return(stat);
}

/*
 *--------------------------------------------------------------------
 *
 * ADM_ERR_FMT2( errorp, code, type, cleanup, message ):
 *
 *	This is the worker routine for adm_err_fmt() and adm_err_fmt2().
 *	It uses the specified error code, type, cleanliness status,
 *	and message to fill in the formatted error entry in the specified
 *	administrative error strucutre.  The routine assumes that
 *	"message" points to a copy of an error message that may safely
 *	be freed by this routine if the routine is subsequently called.
 *
 *	Upon successful completion, this routine returns ADM_SUCCESS.
 *
 *--------------------------------------------------------------------
 */

int
adm_err_fmt2(
	Adm_error *errorp,
	int code,
	u_int type,
	u_int cleanup,
	char *message
	)
{
	if (errorp == NULL) {
		return(ADM_ERR_BADERRPTR);
	}

	if (errorp->message != NULL) {
		free(errorp->message);
	}
	errorp->code    = code;
	errorp->type    = type;
	errorp->cleanup = cleanup;
	errorp->message = message;

	return(ADM_SUCCESS);
}

/*
 *--------------------------------------------------------------------
 *
 * ADM_ERR_HDR2STR( code, type, bufp, lengthp ):
 *
 *	Create the header portion of an error message string as used
 *	to send error information on the STDFMT pipe.  "code" should
 *	be the error code of the desired message, "type" should
 *	indicate the type of error (ADM_ERR_SYSTEM or ADM_ERR_CLASS),
 *	and "bufp" and "lengthp" should pointer to a buffer (at least
 *	ADM_MAXERRDHR+1 bytes in length) and integer in which to place
 *	the header and its actual length, respectively.
 *
 *	This routine always returns ADM_SUCCESS.
 *
 *--------------------------------------------------------------------
 */

int
adm_err_hdr2str(
	int code,
	u_int type,
	char *bufp,
	u_int *lengthp
	)
{
	u_int len;

	len = sprintf(bufp, "%s%d:%u", ADM_ERRMARKER, code, type);
	*lengthp = len;

	return(ADM_SUCCESS);
}

/*
 *--------------------------------------------------------------------
 *
 * ADM_ERR_ISENTRY( errorp ):
 *
 *	Does the specified error structure contain a formatted
 *	error entry?  If so, this routine returns B_TRUE.  Otherwise,
 *	the routine returns B_FALSE.
 *
 *--------------------------------------------------------------------
 */

boolean_t
adm_err_isentry(Adm_error *errorp)
{
	if (errorp == NULL) {
		return(B_FALSE);
	}
	if ((errorp->code != ADM_SUCCESS) ||
	    (errorp->type != ADM_ERR_CLASS) ||
	    (errorp->cleanup != ADM_FAILCLEAN) ||
	    (errorp->message != NULL)) {
		return(B_TRUE);
	}
	return(B_FALSE);
}

/*
 *--------------------------------------------------------------------
 *
 * ADM_ERR_MSG( code ):
 *
 *	Return the (localized) error message associated with the
 *	specified error code.  If the error code is unknown, this routine
 *	returns an "unknown error" message.
 *
 *	NOTE: Some error messages have format fields that are used
 *	      to fill in substitution arguments.  These will not be
 *	      filled in when this routine is called.
 *
 *--------------------------------------------------------------------
 */

char *
adm_err_msg(
	int code
	)
{
	char *err_msg;		/* Error message to return */

	adm_msgs_init();

	if (((code >= ADM_ERR_MSGS_BASE_STATUS)
			&& (code <= ADM_ERR_MSGS_END_STATUS)) ||
	    ((code > ADM_ERR_MSGS_BASE_AMCL)
			&& (code <= ADM_ERR_MSGS_END_AMCL))   ||
	    ((code > ADM_ERR_MSGS_BASE_AMSL)
			&& (code <= ADM_ERR_MSGS_END_AMSL))   ||
	    ((code > ADM_ERR_MSGS_BASE_OM)
			&& (code <= ADM_ERR_MSGS_END_OM))     ||
	    ((code > ADM_ERR_MSGS_BASE_AUTH)
			&& (code <= ADM_ERR_MSGS_END_AUTH))) {
		err_msg = ADM_ERR_MSGS(code);
	} else {
		err_msg = ADM_ERR_MSGS(ADM_ERR_UNKNOWNERR);
	}

	return(err_msg);
}

/*
 *--------------------------------------------------------------------
 *
 * ADM_ERR_NEWH():
 *
 *	Create a new adminstrative error structure and initialize
 *	it.  This routine returns a pointer to the new structure, or
 *	NULL if an error occurs.
 *
 *	NOTE: If any of the default error structure field values
 *	      (included below) change, adm_err_isentry() should also
 *	      be updated to reflect the change.
 *
 *--------------------------------------------------------------------
 */

Adm_error
*adm_err_newh()
{
	Adm_error *newh;

	newh = (Adm_error *) malloc(sizeof(Adm_error));
	if (newh != NULL) {
		newh->message = NULL;
		newh->unfmt_txt = NULL;
		adm_err_reset(newh);
	}

	return(newh);
}

/*
 *--------------------------------------------------------------------
 *
 * ADM_ERR_RESET( errorp ):
 *
 *	Reset the specified error structure to indicate success.
 *	If successful, this routine returns ADM_SUCCESS.
 *
 *--------------------------------------------------------------------
 */

int
adm_err_reset(Adm_error *errorp)
{
	errorp->unfmt_len = 0;
	if (errorp->unfmt_txt != NULL) {
		free(errorp->unfmt_txt);
	}
	errorp->unfmt_txt = NULL;
	return(adm_err_fmt2(errorp, ADM_SUCCESS, ADM_ERR_CLASS,
			    ADM_FAILCLEAN, NULL));
}

/*
 *--------------------------------------------------------------------
 *
 * ADM_ERR_SET2( stream, categories, code, type, message ):
 *
 *	This is the work routine for adm_err_set() and adm_err_setf().
 *	A formatted error specification is created containing the
 *	specified error code, type (ADM_ERR_SYSTEM or ADM_ERR_CLASS),
 *	and (optional) message.  The formatted error specification is
 *	written to the specified stream (usually the STDFMT pipe).
 *	The format of the string written to the stream is:
 *
 *		<size>-<code>:<type> <message>\n	(message specified)
 *		<size>-<code>:<type>\n			(no message)
 *
 *	where <size> is the total number of characters between the
 *	error indication character (-) and the appended newline (\n),
 *	inclusize.
 *
 *	An automatic tracing message containing the error is also
 *	generated in the specified categories, as well as in several
 *	default categories.
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
adm_err_set2(
	FILE *stream,
	char *categories,
	int code,
	u_int type,
	char *message
	)
{
	char err_hdr[ADM_MAXERRHDR+1];	/* Header for STDFMT error string */
	u_int size;			/* Size of STDFMT error string */
	int stat;
	int msg_len;

	/*
	 * Compute size of formatted error message (for STDFMT).
	 */

	stat = adm_err_hdr2str(code, type, err_hdr, &size);
	if (stat != ADM_SUCCESS) {
		return(stat);
	}
	if (message != NULL) {
	    msg_len = strlen(message);
	    /*
	     * Since there is some extra junk prepended to an error msg
	     * we need to leave room for it (it should only be a max extra
	     * of 14 bytes, but to be safe we reserve 25).  Specifically it
	     * is the stuff created by routine adm_err_snm2str.
	     */
	    if (msg_len > (ADM_MAXERRMSG - 25)) msg_len = ADM_MAXERRMSG - 25;
	}
	/* The + 2 is for the space before and the \n after the error msg. */
	size += (message == NULL ? 1 : msg_len + 2);

	/*
	 * Write error to STDFMT.
	 */

	stat = fprintf(stream, "%u%s", size, err_hdr);
	if (stat == EOF) {
		return(ADM_ERR_OUTFAIL);
	}
	if (message != NULL) {
		stat = fprintf(stream, " %.*s", msg_len, message);
		if (stat == EOF) {
			return(ADM_ERR_OUTFAIL);
		}
	}
	stat = fprintf(stream, "\n");
	if (stat == EOF) {
		return(ADM_ERR_OUTFAIL);
	}

	return(ADM_SUCCESS);
}

/*
 *--------------------------------------------------------------------
 *
 * ADM_ERR_SNM2STR( exit_stat, type, cleanup, message, bufpp, lengthp ):
 *
 *	Create a SunNet Manager error message containing the specified
 *	method exit status, administrative error type, cleanliness
 *	indication, and optional error message.  The format of the
 *	SunNet Manager error message will be:
 *
 *	    [<exit_stat>,<type>,<cleanup>]\n            No admin error msg.
 *
 *	    [<exit_stat>,<type>,<cleanup>] <message>\n  With error msg.
 *
 *	This routine will allocate a buffer of the appropriate size
 *	and place the (null-terminated) SunNet Manager error message
 *	in it.  A pointer to the buffer and its length (without the
 *	appended null) are returned in *bufpp and *lengthp,
 *	respectively.
 *
 *	Upon successful completion, this routine return ADM_SUCCESS.
 *
 *--------------------------------------------------------------------
 */

int
adm_err_snm2str(
	int exit_stat,
	u_int type,
	u_int cleanup,
	char *message,
	char **bufpp,
	u_int *lengthp
	)
{
	char *err_msg;	/* Buffer containing SNM error message */
	u_int len;	/* Length of buffer required to hold SNM error msg */
	u_int msg_len;	/* Length of admin error message */

	if (bufpp == NULL) {
		return(ADM_SUCCESS);
	}

	/*
	 * Allocate buffer to hold SNM error message.
	 */

	len = (3 * ADM_MAXINTLEN) + 6;
	if (message != NULL) {
		msg_len = strlen(message);
		len += msg_len + 1;
	}
	if ((err_msg = malloc((size_t) len)) == NULL) {
		return(ADM_ERR_NOMEM);
	}

	/*
	 * Create SNM error message.
	 */

	len = sprintf(err_msg, ADM_ERR_SNMFMT, exit_stat, type, cleanup,
					  (message == NULL ? "" : " "),
					  (message == NULL ? "" : message));
	*bufpp = err_msg;
	if (lengthp != NULL) {
		*lengthp = len;
	}

	return(ADM_SUCCESS);
}	

/*
 *--------------------------------------------------------------------
 *
 * ADM_ERR_STR2CLN( strp, size, cleanupp ):
 *
 *	Parse a STDFMT cleanliness status specification in the form:
 *
 *		=<cleanup>\n
 *
 *	strp should point to the leading "=" character in the
 *	specification, and "size" should be the total number of bytes
 *	between the "=" character and appended newline ("\n"),
 *	inclusive.  The cleanliness status is returned in *cleanupp.
 *
 *	Upon successful completion, this routine returns ADM_SUCCESS.
 *	If "strp" points to an invalid cleanliness status specification,
 *	this routine returns ADM_ERR_BADCLEANUP.
 *
 *--------------------------------------------------------------------
 */

int
adm_err_str2cln(
	char *strp,
	u_int size,
	u_int *cleanupp
	)
{
	char *tokptr;
	char *oldptr;

	/*
	 * Check for leading "=".
	 */

	if (*strp != *ADM_CLEANMARKER) {
		return(ADM_ERR_BADCLEANUP);
	}

	/*
	 * Parse cleanliness status.
	 */

	oldptr = strp + 1;
	*cleanupp = strtol(oldptr, &tokptr, 10);
	if ((u_int)(tokptr - strp) != (size - 1)) {
		return(ADM_ERR_BADCLEANUP);
	}

 	/*
	 * Check for terminating newline.
	 */

	if (*tokptr != '\n') {
		return(ADM_ERR_BADCLEANUP);
	}

	return(ADM_SUCCESS);
}

/*
 *--------------------------------------------------------------------
 *
 * ADM_ERR_STR2ERR( strp, size, codep, typep, messagepp ):
 *
 *	Parse a STDFMT error specification.  Valid error specifications
 *	are:
 *
 *		-<code>:<type>\n		(no error message)
 *		-<code>:<type> <message>\n	(error message)
 *
 *	strp should point to the leading "-" character in the error
 *	specification and "size" should be the total number of bytes
 *	between the "-" and terminating new-line, inclusive.  The error
 *	code, type, and (copy of) message from the error specification
 *	will be returned in "*codep", "*typep", and "*messagepp", respectively.
 *
 *	Upon successful completion, this routine returns ADM_SUCCESS.
 *	If "strp" points to an invalid error specification, this
 *	routine returns ADM_ERR_BADERROR.  If no memory can be malloc'd
 *	for copying the error message, the routine returns ADM_ERR_NOMEM.
 *
 *--------------------------------------------------------------------
 */

int
adm_err_str2err(
	char *strp,
	u_int size,
	int *codep,
	u_int *typep,
	char **messagepp
	)
{
	u_int msglen;			/* Length of error message */
	u_int prelen;			/* # of chars before error message */
	char *tokptr;			/* Current parsing pos. in string */
	char *oldtokptr;

	if (strp == NULL) {
		return(ADM_ERR_BADERROR);
	}

	/*
	 * Check for initial "-" and terminating "\n" in error spec.
	 */

	if ((*strp != *ADM_ERRMARKER) ||
	    (*(char *)(strp + size - 1) != '\n')) {
		return(ADM_ERR_BADERROR);
	}

	/*
	 * Parse the error code.
	 */

	oldtokptr = (char *)(strp + 1);
	*codep = strtol(oldtokptr, &tokptr, 10);
	if (oldtokptr == tokptr) {
		return(ADM_ERR_BADERROR);
	}
	if (*(tokptr++) != ':') {
		return(ADM_ERR_BADERROR);
	}

	/*
	 * Parse the error type.
	 */

	oldtokptr = tokptr;
	*typep = strtol(oldtokptr, &tokptr, 10);
	if (oldtokptr == tokptr) {
		return(ADM_ERR_BADERROR);
	}
	prelen = ((u_int)(tokptr - strp)) + 1;

	/*
	 * Parse the optional error message.
	 */

	if (prelen == size) {
		*messagepp = NULL;
	} else {
		if (*(tokptr++) != ' ') {
			return(ADM_ERR_BADERROR);
		}
		msglen = size - prelen - 1;
		*messagepp = malloc((size_t)(msglen + 1));
		if (*messagepp == NULL) {
			return(ADM_ERR_NOMEM);
		}
		memcpy(*messagepp, tokptr, (size_t) msglen);
		*((char *)(*messagepp + msglen)) = NULL;
	}

	return(ADM_SUCCESS);
}

/*
 *--------------------------------------------------------------------
 *
 * ADM_ERR_STR2SNM( strp, exit_statp, typep, cleanp, messagepp ):
 *
 *	Convert a SunNet Manager error message into its component
 *	exit status, administrative error type, cleanliness indication,
 *	and message.  This routine assumes that the SunNet Manager error
 *	message pointed to by strp is in one of the following formats:
 *
 *	    [<exit_stat>,<type>,<cleanup>]\n	         No admin error msg.
 *
 *	    [<exit_stat>,<type>,<cleanup>] <message>\n	 With error msg.
 *
 *	The exit status, administrative error type, cleanliness
 *	indication, and COPY of the error message are returned in
 *	*exit_statp, *typep, *cleanp, and *messagepp, respectively.
 *
 *	Upon normal completion, this routine returns ADM_SUCCESS.
 *
 *--------------------------------------------------------------------
 */

int
adm_err_str2snm(
	char *strp,
	int *exit_statp,
	u_int *typep,
	u_int *cleanp,
	char **messagepp
	)
{
	int exit_stat;	/* Exit status */
	u_int type;	/* Error type */
	u_int clean;	/* Error cleanliness indication */
	char *msg;	/* Administrative error message */
	u_int msg_len;	/* Administrative error message length */
	int hdr_len;	/* Length of header portion of SNM error message */
	char *tok_ptr;
	int nitems;

	if (strp == NULL) {
		return(ADM_ERR_BADERRMSG);
	}

	/*
	 * Parse header ("[<exit_stat>,<type>,<cleanup>]") of SNM error message.
	 */

	hdr_len = 0;
	nitems = sscanf(strp, ADM_ERR_SNMHDR2, &exit_stat, &type,
					       &clean, &hdr_len);
	if ((nitems != 3) || (hdr_len == 0)) {
		return(ADM_ERR_BADERRMSG);
	}
	if (exit_statp != NULL) {
		*exit_statp = exit_stat;
	}
	if (typep != NULL) {
		*typep = type;
	}
	if (cleanp != NULL) {
		*cleanp = clean;
	}

	/*
	 * Copy the administrative error message body.
	 */

	tok_ptr = (char *)(strp + hdr_len);
	msg = NULL;
	switch(*tok_ptr) {

	    case '\n':				/* No admin error message */

		tok_ptr++;
		break;

	    case ' ':				/* With admin error message */

		tok_ptr++;
		msg_len = strlen(tok_ptr) - 1;
		if (msg_len == 0) {
			return(ADM_ERR_BADERRMSG);
		}
		msg = malloc((size_t)(msg_len + 1));
		if (msg == NULL) {
			return(ADM_ERR_NOMEM);
		}
		memcpy(msg, tok_ptr, (size_t) msg_len);
		*(char *)(msg + msg_len) = NULL;
		tok_ptr += msg_len;
		if (*tok_ptr != '\n') {
			free(msg);
			return(ADM_ERR_BADERRMSG);
		} else {
			tok_ptr++;
		}
		break;

	    default:				/* Unknown format */

		return(ADM_ERR_BADERRMSG);
	}

	if (*tok_ptr != NULL) {
		if (msg != NULL) {
			free(msg);
		}
		return(ADM_ERR_BADERRMSG);
	}
	if (messagepp != NULL) {
		*messagepp = msg;
	} else if (msg != NULL) {
		free(msg);
	}
	return(ADM_SUCCESS);
}

/*
 *--------------------------------------------------------------------
 *
 * ADM_ERR_UNFMT( errorp, length, txt ):
 *
 *	Set the value of the unformatted error block in an administrative
 *	error structure.  The specified error block is first copied
 *	and a NULL is appended to it.  A pointer to this copy, along
 *	with the specified length (before NULL) , are placed in the
 *	error structure.
 *
 *	Upon successful completion, this routine return ADM_SUCCESS.
 *
 *--------------------------------------------------------------------
 */

int
adm_err_unfmt(
	Adm_error *errorp,
	u_int length,
	caddr_t txt
	)
{
	caddr_t txt_cpy;	/* NULL-terminated copy of error text */

	if (errorp == NULL) {
		return(ADM_ERR_BADERRPTR);
	}

	/*
	 * Copy error text and append a NULL.
	 */

	if (txt != NULL) {
		txt_cpy = malloc((size_t)(length + 1));
		if (txt_cpy == NULL) {
			return(ADM_ERR_NOMEM);
		}
		memcpy(txt_cpy, txt, (size_t) length);
		*((char *)(txt_cpy + length)) = NULL;
	} else {
		txt_cpy = NULL;
	}

	/*
	 * Set unformatted error entry.
	 */

	if (errorp->unfmt_txt != NULL) {
		free(errorp->unfmt_txt);
	}
	errorp->unfmt_len = (txt_cpy == NULL ? 0 : length);
	errorp->unfmt_txt = txt_cpy;

	return(ADM_SUCCESS);
}

#endif /* !_adm_err_impl_c */

