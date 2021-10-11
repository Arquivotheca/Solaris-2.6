/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)adm_amsl_error.c	1.14	95/06/02 SMI"

/*
 * FILE:  error.c
 *
 *	Admin Framework class agent error handling interlude routines.
 */

#include <locale.h>
#include <stdarg.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <netmgt/netmgt.h>
#include "adm_fw.h"
#include "adm_fw_impl.h"
#include "adm_amsl.h"
#include "adm_amsl_impl.h"

/*
 * -------------------------------------------------------------------
 *  amsl_err - Format system error message with substitutible arguments.
 *	Accepts a pointer to an amsl request structure, the error
 *	message referrent (error code), and an optional list of
 *	substitutible arguments to be converted to strings and placed
 *	into the error message text ala sprintf.
 *	This routine sets the Adm_error structure for a system error.
 *	Framework system error referrents are assumed by this routine.
 *	The Adm_error structure type and cleanup fields are set to
 *	ADM_ERR_SYSTEM and ADM_FAILCLEAN, respectively.
 *	The error referrent (error code) is returned, allowing such
 *	coding practices as  "return(amsl_err(...));"
 * -------------------------------------------------------------------
 */

int
amsl_err(
	struct amsl_req *reqp,	/* Pointer to amsl request structure */
	int errcode, 		/* Error message referrent */
	...)			/* Variable arguments... */
{
	va_list vp;		/* Varargs argument list pointer */

	Adm_error *ep;		/* Local ptr to formatted error structure */
	char *msgfmt;		/* Pointer to message text string */
	char *msgbuf;		/* Local error message buffer */

	/* Position to variable arguments in call list */
	va_start(vp, errcode);

	/* Set local error structure pointer from amsl request structure */
	ep = reqp->errp;

	/* Get the error message text and substitute arguments */
	msgfmt = adm_err_msg(errcode);
	/*
	 * Allocate needed space for msgbuf and then
	 * put vsprintf output into it.
	 * Note: the allocated buffer needs to be freed
	 *	 somewhere below.
	 */
	(void) vsprintf_alloc(&msgbuf, msgfmt, vp);
	va_end(vp);

	ADM_DBG("e",("AMSL:   Error %d: %s", errcode, msgbuf));

	/* Modify the formatted error structure to contain the error */
	if (ep != (struct Adm_error *)NULL)
		adm_err_fmt(ep, errcode, ADM_ERR_SYSTEM, ADM_FAILCLEAN, msgbuf);

	/* If error diag categories established, log the error message */
	if (reqp->diag_errcats != (char *)NULL)
		(void) amsl_log1(reqp->diag_errcats, msgbuf);

	if (msgbuf) free(msgbuf);
	/* Return the error code */
	return (errcode);
}

/*
 * -------------------------------------------------------------------
 *  amsl_err1 - Format system error message; canned error text.
 *	Accepts pointer to request structure, error code and
 *	address of message text.
 *	The error referrent (error code) is returned, allowing such
 *	coding practices as  "return(amsl_err(...));"
 * -------------------------------------------------------------------
 */

int
amsl_err1(
	struct amsl_req *reqp,	/* Pointer to amsl request structure */
	int code,		/* Message referrent code */
	char *msgp)		/* Pointer to error message text */
{
	ADM_DBG("e", ("AMSL:   Error %d: %s", code, msgp));

	adm_err_fmt(reqp->errp, code, ADM_ERR_SYSTEM, ADM_FAILCLEAN, msgp);

	/* If error disg categories established, log the error message */
	if (reqp->diag_errcats != (char *)NULL)
		(void) amsl_log1(reqp->diag_errcats, msgp);

	return (code);
}

/*
 * -------------------------------------------------------------------
 *  amsl_err2 - Retrieve error message text and substitute arguments.
 *	Accepts a buffer into which to build the error message, an
 *	error status code (refers to the error message), and a variable
 *	length list of substitutible arguments.
 *	This routine gets the error message by using the error status
 *	code to index the amsl_errmsgs array of string pointers.
 *	!!WARNING!! vsprintf is used to substitute arguments into the
 *		    message text, and NO checking is made as to the
 *		    correct number of arguments or the size of the
 *		    message buffer.
 *	The error status code is returned.
 * -------------------------------------------------------------------
 */

int
amsl_err2(
	char *msgbuf,		/* Error message buffer */
	int errcode, 		/* Error message referrent */
	...)			/* Variable arguments... */
{
	va_list vp2;		/* Varargs argument list pointer */
	char *msgfmt;		/* Error message format string */

	/* Position to start of variable argument list */
	va_start(vp2, errcode);

	/* Get the error message text and substitute arguments */
	msgfmt = adm_err_msg(errcode);
	(void) vsprintf(msgbuf, msgfmt, vp2);
	va_end(vp2);

	ADM_DBG("e", ("AMSL:   Error %d: %s", errcode, msgbuf));

	return (errcode);
}

/*
 * -------------------------------------------------------------------
 *  amsl_err_netmgt - Retrieve netmgt error code and message.
 *	Accepts pointer to error code variable and pointer to error
 *	message text pointer variable.
 *	Nothing returned.
 * -------------------------------------------------------------------
 */

void
amsl_err_netmgt(
	int *codep,		/* Address to return error code */
	char **msgpp)		/* Address to return error message ptr */
{
	static char *nullmsg = "";
	char *mp;

	/* Return netmgt service error */
	if (codep != (int *)NULL)
		*codep = netmgt_error.service_error;

	/* Return pointer to netmgt error message text */
	if (msgpp != (char **)NULL) {
		mp = netmgt_sperror();
		if (mp == (char *)NULL)
			mp = nullmsg;
		*msgpp = mp;
	}

	return;
}

/*
 * -------------------------------------------------------------------
 *  amsl_err_syslog - Error routine to write message to syslog daemon.
 *	Accepts error code and list of substitutable arguments.
 *	Gets message text from array of message strings.
 *	Returns no status.
 * -------------------------------------------------------------------
 */

void
amsl_err_syslog(
	int errcode,		/* Message referrent code */
	...)			/* Substitutable arguments */
{
	va_list vp2;		/* Varargs argument list pointer */
	char *msgfmt;		/* Pointer to message text string */
	char *msgbuf;	/* Local error message buffer */

	/* Position to start of variable argument list */
	va_start(vp2, errcode);

	/* Get the error message text and substitute arguments */
	msgfmt = adm_err_msg(errcode);
	/*
	 * Allocate needed space for msgbuf and then
	 * put vsprintf output into it.
	 * Note: the allocated buffer needs to be freed
	 *	 somewhere below.
	 */
	(void) vsprintf_alloc(&msgbuf, msgfmt, vp2);
	va_end(vp2);

	/* Write message to syslog daemon */
	(void) syslog((LOG_ERR|LOG_USER), "%s: %s", ADM_CLASS_AGENT_NAME,
	    msgbuf == NULL ? "" : msgbuf );

	if (msgbuf) free(msgbuf);
	/* Return */
	return;

}

/*
 * -------------------------------------------------------------------
 *  amsl_err_syslog1 - Error routine to write message to syslog daemon.
 *	Accepts pointer to message text string.
 *	Gets message text from array of message strings.
 *	Returns no status.
 * -------------------------------------------------------------------
 */

void
amsl_err_syslog1(
	char *msgp)		/* Message text string */
{
	char *mp;		/* Local pointer to text string */

	/* Write message to syslog daemon */
	if (msgp == (char *)NULL)
		mp = "??";
	else
		mp = msgp;
	(void) syslog((LOG_ERR|LOG_USER), "%s: %s", ADM_CLASS_AGENT_NAME,
	    mp);

	/* Return */
	return;
}

/*
 * -------------------------------------------------------------------
 * amsl_err_dbg - Error routine to write a debug message text string.
 *	Accepts arguments in "printf" format.
 *	Writes the pid of this process as a header, then the message.
 *	Returns no status.
 * -------------------------------------------------------------------
 */

void
amsl_err_dbg(
	char *msgfmt,		/* Error message format string */
	...)			/* Variable arguments */
{
	va_list vp3;		/* Varargs argument list pointer */
	time_t  dtime;		/* Time argument */
	struct  tm *tm_p;	/* Pointer to time buffer */

	va_start(vp3, msgfmt);
	dtime = time((time_t *)NULL);
	tm_p = localtime(&dtime);
	(void) printf("[%ld; %.2d:%.2d:%.2d] ", (long)getpid(), tm_p->tm_hour,
	    tm_p->tm_min, tm_p->tm_sec);
	(void) vprintf(msgfmt, vp3);
	va_end(vp3);

	return;
}
