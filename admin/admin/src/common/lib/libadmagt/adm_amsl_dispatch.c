/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)adm_amsl_dispatch.c	1.24	95/09/08 SMI"

/*
 * FILE:  dispatch.c
 *
 *	Admin Framework class agent dispatch routines for Admin request
 *	types.
 */

#include <stdio.h>
#include <string.h>
#include <stropts.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <netmgt/netmgt.h>
#include "adm_fw.h"
#include "adm_fw_impl.h"
#include "adm_amsl.h"
#include "adm_amsl_impl.h"

/* Declare static functions */
static void disp_send_error(struct Adm_error *);
static void disp_send_err2(int, int, int, int, char *);
static void disp_sig_quit();
static void disp_sig_term();
static void disp_send_warning(int, struct Adm_error *);
static int  disp_version(struct amsl_req *);
static int  disp_weakauth(struct amsl_req *);

/*
 * -------------------------------------------------------------------
 *  amsl_dispatch - Framework class agent remote dispatch routine
 *	Accepts standard SNM agent call list.
 *	Uses global pointers to AMSL control structure set up by main
 *	and request structure set up by remote verify routine.
 *	This routine processes a remote Admin request RPC call.
 *	Returns an Admin callback message if non-fatal error; otherwise
 *	returns an SNM error callback message.
 * -------------------------------------------------------------------
 */

/*ARGSUSED*/

void
amsl_dispatch(
	u_int  type,		/* SNM call I/F: request type */
	char  *host,		/* SNM call I/F: host system name */
	char  *adm_class,		/* SNM call I/F: class name or group name */
	char  *method,		/* SNM call I/F: method name or key value */
	u_int  count,		/* SNM call I/F: report count  (N/A) */
	struct timeval interval, /* SNM call I/F: report interval  (N/A) */
	u_int  flags)		/* SNM call I/F: request flags */
{
	struct amsl_req *reqp;	/* Local pointer to amsl request struct */
	char   errbuf[ADM_MAXERRMSG];	/* Local error message buff */
	u_int  logcode;		/* Log request code */
	int    tstat;		/* Temporary status code */
	int    stat;		/* Error return status code */

	/*
	 * Input call list arguments have been validated by verify routine.
	 * Process each request type:
	 *	ADM_PERFORM_REQUEST  => NETMGT_ACTION_REQUEST (for now)
	 *	AMSL_VERSION_REPORT  => Return versions supported
	 *	AMSL_WEAKAUTH_REPORT =>	Return authentication flavors
	 *
	 * NOTE: If a non-zero status code is returned from the subordinate
	 *	 function, it is considered a FATAL error and the
	 *	 disp_send_error routine is called to send back an SNM error
	 *	 report.
	 */

	/* Set up for SIGINT, SIGTERM and SIGQUIT signals */
	(void) signal(SIGINT, disp_sig_term);
	(void) signal(SIGTERM, disp_sig_term);
	(void) signal(SIGQUIT, disp_sig_quit);
	
	/* Get pointer to amsl request structure via SNM request tag */
	reqp = find_my_req();
	if (reqp == (struct amsl_req *)NULL) {
		(void) amsl_err2(errbuf, ADM_ERR_NOREQSTRUCT);
		disp_send_err2(ADM_FAILURE, ADM_ERR_NOREQSTRUCT,
		    ADM_ERR_SYSTEM, ADM_FAILCLEAN, errbuf);
		return;
		}

	ADM_DBG("d", ("Dsptch: started request; disp=%d, type=%d, class=%s %s, method=%s",
	    reqp->dispatch_type, reqp->request_type, reqp->class_name,
	    (reqp->class_version != NULL?reqp->class_version:""),
	    reqp->method_name));

	switch (reqp->dispatch_type) {

	case AMSL_DISPATCH_INVOKE:		/* Invoke a method... */
		(void) amsl_log2(reqp, ADM_ERR_AMSL_STARTED,
		    reqp->request_type, reqp->class_name,
		    reqp->class_version, reqp->method_name);

		/* Allocate input argument handle and get input args */
		if ((reqp->inp = adm_args_newh()) == NULL) {
			stat = amsl_err(reqp, ADM_ERR_REQNOHANDLE);
			break;
		}
		if ((stat = get_input_parms(reqp, reqp->inp)) != 0)
			break;

		/* Allocate output argument handle */
		if ((reqp->outp = adm_args_newh()) == NULL) {
			stat = amsl_err(reqp, ADM_ERR_REQNOHANDLE);
			break;
		}

		/* Invoke the method runfile */
		stat = amsl_invoke(reqp);

		/*
		 * If any of the following is true, send a warning error:
		 *	Exit status is non-zero
		 *	Error code is non-zero
		 *	Cleanup status is not ADM_FAILCLEAN
		 */

		if ((reqp->exit_status != 0) || (reqp->errp->code != 0) ||
		    (reqp->errp->cleanup != ADM_FAILCLEAN))
			disp_send_warning(reqp->exit_status, reqp->errp);

		/*
		 * Build return arguments to pass back to the client
		 * Save return status codes.  If a system error occurs
		 * trying to send the callback report, set the stat
		 * completion status to the error code.  This will cause
		 * a fatal error to be sent to the client (by the
		 * module which called perf_invoke).
		 */

		tstat = put_action_header(reqp);
		if (tstat == 0)
			tstat = put_output_parms(reqp, reqp->outp);
		if (tstat == 0)
			tstat = put_callback(reqp, reqp->exit_status);
		if (tstat != 0)
			stat = tstat;

		/* If errors, send back a failure message */
		if ((stat != 0) && (stat != ADM_FAILURE))
			disp_send_error(reqp->errp);

		ADM_DBG("d", ("Dsptch: finished request"));

		/* Cleanup buffers */
		(void) free_buffs(reqp);

		break;

	case AMSL_DISPATCH_VERSION:		/* Return versions... */
		stat = disp_version(reqp);
		if (stat != 0)
			disp_send_error(reqp->errp);
		ADM_DBG("d", ("Dsptch: Returned bad version info"));
		stat = -1;			/* Log denied message */
		break;

	case AMSL_DISPATCH_WEAKAUTH:		/* Return auth flavors... */
		stat = disp_weakauth(reqp);
		if (stat != 0)
			disp_send_error(reqp->errp);
		ADM_DBG("d", ("Dsptch: Returned weak authentication info"));
		stat = -1;			/* Log denied message */
		break;


	default:				/* Unknown request type */
		stat = amsl_err(reqp, ADM_ERR_BADREQTYPE, reqp->request_type);
		disp_send_error(reqp->errp);
		break;
	}

	/* If logging, write request completed or terminated */
	logcode = ADM_ERR_AMSL_COMPLETED;
	if (stat == -1)
		logcode = ADM_ERR_AMSL_DENIED;
	if (stat > 0)
		logcode = ADM_ERR_AMSL_TERMINATED;
	(void) amsl_log2(reqp, logcode, reqp->request_type,
	    reqp->class_name, reqp->class_version, reqp->method_name);

	/* Release request structure */
	(void) unlink_req(reqp);
	free_req(reqp);

	/* Return without status */
	return;
}

/*
 * -----------------------------------------------------------------------
 * disp_version - Routine to return version information about class agent.
 *	Accepts pointer to an amsl_req structure.
 *	This routine returns version information about the agent using
 *	a different set of system parameters in the callback header.
 *	If an error occurs, an error status code is returned and the
 *	associated error is placed in the formatted error structure.
 * ----------------------------------------------------------------------
 */

static int
disp_version(
	struct amsl_req *reqp)	/* Pointer to amsl_req structure */
{
	int    stat;		/* Local status code */

	/* Reset request type so we return a data report */
	reqp->request_type = (u_int)ADM_PERFORM_REQUEST;

	/* Simply build the callback system parameters */
	stat = put_version_header(reqp);
	if (stat == 0)
		stat = put_callback(reqp, (int)0);

	ADM_DBG("de", ("Dsptch: bad version report; version=%d",
	    reqp->request_version));

	/* Return */
	return (stat);
}

/*
 * -----------------------------------------------------------------------
 * disp_weakauth - Routine to return authentication flavors for invoking
 *	a method when too weak a flavor was used in the request.
 *	Accepts pointer to the amsl request structure.
 *	This routine returns the list of authentication flavors which
 *	can be used to successfully invoke the method.
 *	If an error occurs, an error status code is returned and the
 *	associated error is placed in the formatted error structure.
 * ----------------------------------------------------------------------
 */

static int
disp_weakauth(
	struct amsl_req *reqp)	/* Pointer to amsl_req structure */
{
	struct amsl_auth *ap;	/* Local ptr to request auth structure */
	char   authbuff[AMSL_WEAKAUTH_BUFFSIZE+1]; /* Auth flavor names */
	char   namebuff[ADM_AUTH_NAMESIZE+1]; /* Auth flavor name buffer */
	int    len;		/* Local length variable */
	int    i;		/* Local loop variable */
	int    stat;		/* Local status code */

	ap = reqp->authp;

	/* Build a string of authentication flavor names */
	authbuff[0] = '\0';
	for (i = 0; i < ap->auth_flavor_entries; i++) {
		if (i > 0)
			(void) strcat(authbuff, " ");
		if ((adm_auth_flavor2str(namebuff, (u_int)ADM_AUTH_NAMESIZE,
		    ap->auth_flavor_list[i])) != 0)
			namebuff[0] = '\0';
		len = (int)strlen(namebuff);
		if ((len + (int)strlen(authbuff)) < AMSL_WEAKAUTH_BUFFSIZE)
			(void) strcat(authbuff, namebuff);
	}						/* End of for */

	/* Send a warning error back to the client process */
	if (ap->auth_fail & AMSL_AUTH_FAIL_WRONG)
		stat = ADM_ERR_AMSL_AUTHWRONG;
	else
		stat = ADM_ERR_AMSL_AUTHWEAK;
	(void) amsl_err(reqp, stat, amsl_ctlp->server_hostname, authbuff,
	    reqp->class_name, (reqp->class_version != NULL?reqp->class_version:""),
	    reqp->method_name);
	disp_send_warning((int)0, reqp->errp);

	/* Build the callback system parameters */
	ADM_DBG("ds", ("Dsptch: weak auth report; flavors=%s", authbuff));
	stat = put_weakauth_header(reqp, authbuff);
	if (stat == 0)
		stat = put_callback(reqp, (int)0);


	/* Return */
	return (stat);
}

/*
 * -----------------------------------------------------------------------
 * disp_send_error - Routine to make a fatal error callback RPC.
 *	Accepts a formatted error structure.  Assumes ADM_FAILURE exit
 *	status.
 *	Uses disp_send_err2 to send to error.
 * -----------------------------------------------------------------------
 */

static void
disp_send_error(
	struct Adm_error *ep)	/* Formatted error structure */
{
	disp_send_err2(ADM_FAILURE, ep->code, ep->type, ep->cleanup,
	    ep->message);
	return;
}

/*
 * -----------------------------------------------------------------------
 * disp_send_err2 - Routine to make a fatal error callback RPC.
 *	Accepts an exit status, error code, error type, cleanup value,
 *	and error message.
 *	Sends the error to the client process via an SNM error RPC message.
 * ----------------------------------------------------------------------
 */

static void
disp_send_err2(
	int   xstat,		/* Exit status code */
	int   ecode,		/* Error status code */
	int   etype,		/* Error type */
	int   eclean,		/* Cleanup status code */
	char *emsgp)		/* Error message */
{
	Netmgt_error nmerr;	/* Local SNM error structure */
	u_int   len;		/* Temp length */

	ADM_DBG("d", ("Dsptch: Sending fatal error; code %d", ecode));
	nmerr.service_error = NETMGT_FATAL;
	nmerr.agent_error = ecode;
	adm_err_snm2str(xstat, etype, eclean, emsgp, &nmerr.message, &len);
	netmgt_send_error(&nmerr);

	/* Return */
	return;
}

/*
 * -----------------------------------------------------------------------
 * disp_send_warning - Routine to make warning error callback RPC.
 *	Accepts an exit status and formatted error structure..
 *	Sends the error to the client process via an SNM error RPC message.
 * ----------------------------------------------------------------------
 */

static void
disp_send_warning(
	int   xstat,		/* Exit status code */
	struct Adm_error *ep)	/* Formatted error structure */
{
	Netmgt_error nmerr;	/* Local SNM error structure */
	u_int   len;		/* Temp length */

	ADM_DBG("d", ("Dsptch: Sending warning error; code %d", ep->code));

	nmerr.service_error = NETMGT_WARNING;
	nmerr.agent_error = ep->code;
	adm_err_snm2str(xstat, ep->type, ep->cleanup, ep->message,
	    &nmerr.message, &len);
	netmgt_send_error(&nmerr);

	/* Return */
	return;
}

/*
 * -----------------------------------------------------------------------
 * disp_sig_term - Signal handling routine for SIGTERM shutdown.
 *	If running in dispatch process and method process has been
 *	forked and not reaped, send it a SIGTERM signal telling
 *	the method to gracefully terminate via its cleanup routine.
 * ----------------------------------------------------------------------
 */

static void
disp_sig_term()
{
	struct amsl_req *reqp;	/* Local pointer to amsl request struct */

	ADM_DBG("d", ("Dsptch: caught SIGTERM signal"));

	/* Get this method request structure */
	reqp = find_my_req();

	/* If method process still exists, tell it to terminate itself */
	if (reqp != (struct amsl_req *)NULL) {
		if (reqp->method_pid != (pid_t)0) {
			(void) sigsend((idtype_t)P_PID, (id_t)reqp->method_pid,
			    SIGTERM);

			ADM_DBG("d", ("Dsptch: terminated method process; pid %ld",
			    (long)reqp->method_pid));
		}
	}

	/* Return */
	return;
}

/*
 * -----------------------------------------------------------------------
 * disp_sig_quit - Signal handling routine for SIGQUIT shutdown.
 *	If running in dispatch process and method process has been
 *	forked and not reaped, send it a SIGQUIT signal telling
 *	the method to forcefully terminate. Wait a configured amount
 *	of time and then kill the method child process.
 * ----------------------------------------------------------------------
 */

static void
disp_sig_quit()
{
	struct amsl_req *reqp;	/* Local pointer to amsl request struct */

	ADM_DBG("d", ("Dsptch: caught SIGQUIT signal"));

	/* Mask off SIGTERM signals to avoid graceful shutdowns */
	(void) sighold(SIGTERM);

	/* Get this method request structure */
	reqp = find_my_req();

	/* If method process still exists, tell it to terminate */
	if (reqp != (struct amsl_req *)NULL) {
		if (reqp->method_pid != (pid_t)0) {
			(void) sigsend((idtype_t)P_PID, (id_t)reqp->method_pid,
			    SIGTERM);

			ADM_DBG("d", ("Dsptch: terminated method process; pid %ld",
			    (long)reqp->method_pid));

			/* Sleep a short period and commit infanticide */
			(void) sleep((u_int)AMSL_SHUTDOWN_METHOD_TIME);
			(void) sigsend((idtype_t)P_PID, (id_t)reqp->method_pid,
			    SIGKILL);

			ADM_DBG("d", ("Dsptch: killed method process; pid %ld",
			    (long)reqp->method_pid));
		}
	}
	
	/* Unblock SIGTERM */
	(void) sigrelse(SIGTERM);

	/* Return */
	return;
}
