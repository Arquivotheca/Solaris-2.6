/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)adm_amsl_log.c	1.11	95/05/08 SMI"

/*
 * FILE:  log.c
 *
 *	Admin Framework class agent logging routines.
 */

#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include "adm_fw.h"
#include "adm_fw_impl.h"
#include "adm_amsl.h"
#include "adm_amsl_impl.h"

#define	REQ_TYPE_PERFORM	"PERFORM "
#define	REQ_TYPE_UNDO		"UNDO    "
#define	REQ_TYPE_IDENTIFY	"IDENTIFY"

#define	LOCK_WAIT	30	/* Number of seconds to wait for lock */

/*
 * -------------------------------------------------------------------
 *  amsl_log - Class agent diagnostic log message routine.
 *	Accepts pointer to the amsl request structure, the message
 *	referrent to the log message format string (in the error
 *	message array), a pointer to the list of categories that apply
 *	to this message, and an optional list of substitutable arguments.
 *	This routine formats gets the message format string, substitutes
 *	the arguments, and calls the diagnostic logging routine to
 *	write the log message into the log file.
 *	Returns zero if successful; otherwise, returns an error code
 *	and writes an error message into the formatted error structure.
 * -------------------------------------------------------------------
 */

int
amsl_log(
	struct amsl_req *reqp,	/* Pointer to the amsl request structure */
	u_int code,		/* Message referrent */
	char *cats,		/* Message categories */
	...)			/* Optional substitutable arguments */
{
	va_list(vp);
	char *msgfmt;		/* Pointer to message format template */
	char *msgbuf;		 /* Local log message buffer */
	time_t msgtime;		/* Time structure for message time */
	int   syserr;		/* Local status for write error */
	int   stat;		/* Local status code */

	/* Position to variable arguments in call list */
	va_start(vp, cats);

	/* Get the log message text and substitute arguments */
	msgfmt = adm_err_msg(code);
	/*
	 * Allocate needed space for msgbuf and then
	 * put vsprintf output into it.
	 * Note: the allocated buffer needs to be freed
	 *	 somewhere below.
	 */
	(void) vsprintf_alloc(&msgbuf, msgfmt, vp);
	va_end(vp);

	/* Write the log message to the log file */
	stat = 0;
	msgtime = time((time_t *)NULL);
	if ((stat = adm_diag_fmt(&syserr, ADM_LOG_STDHDR, msgtime, cats,
	    msgbuf)) != ADM_SUCCESS)
		stat = amsl_err(reqp, ADM_ERR_LOGERR, stat, syserr);

	if (msgbuf) free(msgbuf);
	/* Return with status */
	return (stat);
}

/*
 * -------------------------------------------------------------------
 *  amsl_log1 - Class agent diagnostic log message routine.
 *	Accepts a pointer the the list of catagories that apply to this
 *	message and a pointer to the log message.
 *	This routine simply calls the diagnostic logging routine to
 *	write the log message into the log file.
 *	Returns zero if successful; otherwise, returns an error code
 *	and writes an error message into the formatted error structure.
 * -------------------------------------------------------------------
 */

int
amsl_log1(
	char *cats,		/* Message categories */
	char *msg)		/* Message tesxt string */
{
	time_t msgtime;		/* Time structure for message time */
	int   syserr;		/* System error from write */

	/*
	 * Write the log message to the log file.
	 *
	 * !!!WARNING!!!
	 *
	 * The error routines make a call to this log routine, do
	 * NOT call error routines from here.  If you do, you get an
	 * infinite recursion of log/error calls.
	 */

	msgtime = time((time_t *)NULL);
	adm_diag_fmt(&syserr, ADM_LOG_STDHDR, msgtime, cats, msg);

	/* Return with status */
	return (0);
}

/*
 * -------------------------------------------------------------------
 *  amsl_log2 - Class agent diagnostic request log message routine.
 *	Accepts pointer to the amsl request structure, the message
 *	referrent to the log message format string (in the error
 *	message array), a pointer to the list of categories that apply
 *	to this message, the request type, and an optional list of
 *	substitutable arguments.
 *	This routine gets the request message format string, substitutes
 *	the arguments, and calls the diagnostic logging routine to
 *	write the log message into the log file.
 *	Returns zero if successful; otherwise, returns an error code
 *	and writes an error message into the formatted error structure.
 * -------------------------------------------------------------------
 */

int
amsl_log2(
	struct amsl_req *reqp,	/* Pointer to the amsl request structure */
	u_int code,		/* Message referrent */
	u_int reqtype,		/* Type of request */
	char *class_name,	/* Class name */
	char *class_vers,	/* Class version */
	char *method_name)	/* Method name */
{
	char *msgfmt;		/* Pointer to message format template */
	char *msgbuf;		/* Local log message buffer */
	char  tbuff[16];	/* Local integer conversion buffer */
	time_t msgtime;		/* Time structure for message time */
	char *tp;		/* Local pointer to string */
	int   syserr;		/* Local status for write error */
	int   stat;		/* Local status code */

	/* Convert request type to a string */
	switch (reqtype) {
	case ADM_PERFORM_REQUEST:
		tp = "PERFORM";
		break;
	default:
		(void) sprintf(tbuff, "%d", reqtype);
		tp = tbuff;
		break;
	}

	/* Get the log message text and substitute arguments */
	msgfmt = adm_err_msg(code);
	/*
	 * Allocate needed space for msgbuf and then
	 * put sprintf output into it.
	 * Note: the allocated buffer needs to be freed
	 *	 somewhere below.
	 */
	(void) sprintf_alloc(&msgbuf, msgfmt, tp,
	    (class_name != NULL?class_name:"(nil)"),
	    (class_vers != NULL?class_vers:""),
	    (method_name != NULL?method_name:"(nil)"));

	/* Write the log message to the log file */
	stat = 0;
	msgtime = time((time_t *)NULL);
	if ((stat = adm_diag_fmt(&syserr, ADM_LOG_STDHDR, msgtime,
	    reqp->diag_reqcats, msgbuf)) != ADM_SUCCESS)
		stat = amsl_err(reqp, ADM_ERR_LOGERR, stat, syserr);

	/* If verbose option, write request message to system console */
	if (amsl_ctlp->flags & AMSL_CONSLOG_ON)
		(void) amsl_err_syslog1(msgbuf);

	if (msgbuf) free(msgbuf);
	/* Return with status */
	return (stat);
}

/*
 * -------------------------------------------------------------------
 *  amsl_log3 - Class agent diagnostic agent log message routine.
 *	Accepts a message refferent to the log message format string,
 *	and an optional list of substitutable arguments.  If the message
 *	refferent is zero, the next argument is the message string itself
 *	and no argument substitution is done.
 *	This routine formats a standard class agent system information
 *	header, gets the message format string, substitutes
 *	the arguments, and calls the diagnostic logging routine to
 *	write the log message into the log file with the special header.
 *	Returns zero if successful; otherwise, returns an error code
 *	and writes an error message into the formatted error structure.
 * -------------------------------------------------------------------
 */

int
amsl_log3(
	u_int code,		/* Message referrent */
	...)			/* Optional substitutable arguments */
{
	va_list(vp);
	char *msgfmt;		/* Pointer to message format template */
	char  tmpbuf[ADM_MAXERRMSG*2];
	char *hdrbuf;		/* Local log message header buffer */
	char *msgbuf = NULL;	/* Local log message buffer */
	time_t msgtime;		/* Time structure for message time */
	char *msgp;		/* Pointer to message */
	int   syserr;		/* Local status for write error */

	/* Position to variable arguments in call list */
	va_start(vp, code);

	/* Get the class agent header message */
	msgtime = time((time_t *)NULL);
	tmpbuf[0] = '\0';
	(void) adm_find_object_class(tmpbuf, (ADM_MAXERRMSG+1));
	msgfmt = adm_err_msg(ADM_ERR_AMSL_HEADER);
	/*
	 * Allocate needed space for hdrbuf and then
	 * put sprintf output into it.
	 * Note: the allocated buffer needs to be freed
	 *	 somewhere below.
	 */
	(void) sprintf_alloc(&hdrbuf, msgfmt, tmpbuf);

	/* Get the log message text */
	if (code == 0) 
		msgp = va_arg(vp, char*);
	else {
		msgfmt = adm_err_msg(code);
		/*
		 * Allocate needed space for msgbuf and then
		 * put vsprintf output into it.
		 * Note: the allocated buffer needs to be freed
		 *	 somewhere below.
		 */
		(void) vsprintf_alloc(&msgbuf, msgfmt, vp);
		msgp = msgbuf;
	}
	va_end(vp);

	/* Write the log message to the log file */
	(void) adm_diag_fmt(&syserr, hdrbuf, msgtime, ADM_CAT_INFO, msgp);

	if (msgbuf) free(msgbuf);
	if (hdrbuf) free(hdrbuf);
	/* Return with status */
	return (0);
}
