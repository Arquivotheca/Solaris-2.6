/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)adm_amsl_local.c	1.11	95/06/02 SMI"

/*
 * FILE:  adm_amsl_local.c
 *
 *	Admin Framework class agent local dispatch routine.
 */

#include <stdio.h>
#include <string.h>
#include <stropts.h>
#include <signal.h>
#include <time.h>
#include <memory.h>
#include <unistd.h>
#include <sys/types.h>
#include <netmgt/netmgt.h>
#include "adm_fw.h"
#include "adm_fw_impl.h"
#include "adm_amsl.h"
#include "adm_amsl_impl.h"

/* Declare local structures and typedefs */
typedef void(*SIG_PFV)();
struct	loc_sig_vec {			/* Prior signal handler structs */
	struct sigaction ovec_int;	/* SIGINT  */
	struct sigaction ovec_quit;	/* SIGQUIT */
	struct sigaction ovec_term;	/* SIGTERM */
	struct sigaction ovec_chld;	/* SIGCHLD */
};

/* Declare static functions */
static void local_cleanup(struct amsl_req *, struct loc_sig_vec *);
static void local_reset_sigs(struct loc_sig_vec *);
static void local_set_sigs(struct loc_sig_vec *);
static void local_sig_chld(void);
static void local_sig_quit(void);
static void local_sig_term(void);

/*
 * -------------------------------------------------------------------
 *  amsl_local_dispatch - Framework class agent local dispatch routine.
 *	This routine processes a local Admin request call.
 *	Accepts the following call list:
 *		flags		=> control flags
 *		request_idp	=> pointer to an AMCL request structure
 *		classname	=> class name for request
 *		classvers	=> (optional) class version number
 *		methodname	=> method name for request
 *		input_data	=> Adm_data input argument list
 *		output_data	=> Adm_data output argument list struct
 *		error_data	=> Adm_error error structure
 *		clientgroup	=> (optional) client group name
 *		categories	=> (optional) client diag categories
 *	Checks global pointer to AMSL control structure and initializes
 *	a default control structure if not already allocated.
 *	All information is returned in call list parameters.
 *	Returns exit status code of method.
 *
 * !!!WARNING!!! Local dispatch creates a temporary default amsl
 *		 control structure which is freed when the routine
 *		 local_cleanup is called.  You should ONLY return
 *		 after the local_cleanup call (no debug statements!).
 * -------------------------------------------------------------------
 */

#define	PROG_NAME	"amsl_local_dispatch"

int
amsl_local_dispatch(
	u_int  ctl_flags,	/* AMSL control flags */
	Adm_requestID *reqidp,	/* Request identifier */
	char  *classname,	/* Class name for request */
	char  *classvers,	/* Class version for request */
	char  *methodname,	/* Method name for request */
	Adm_data *inp,		/* Input argument list structure */
	Adm_data *outp,		/* Output argument list structure */
	Adm_error *errp,	/* Error return structure */
	char *client_group,	/* Client group name */
	char *client_cats)	/* Client diagnostic categories */
{
	struct amsl_req *reqp;	/* Local pointer to amsl request struct */
	Adm_data *ap;		/* Local argument list struct pointer */
	Adm_error *ep;		/* Local error list struct pointer */
	struct bufctl *bp;	/* Local buffer control struct pointer */
	struct loc_sig_vec ovec;	/* Local old sig handler struct */
	char   errbuf[ADM_MAXERRMSG];	/* Local error message buff */
	u_int  logcode;		/* Log request code */
	u_int  xstat;		/* Method exit status code */
	u_int  stat;		/* Error return status code */

	/*
	 * Do some initial setup to get things rolling...
	 *	Assume failure status code
	 *	Make sure there is a request ID and error argument
	 *		(If no error struct, cannot report any errors!)
	 *	If no control structure, create a default one
	 *	Allocate an initial request structure
	 */

	reqp = (struct amsl_req *)NULL;
	xstat = ADM_FAILURE;

	/* XXX - PATCH: TURN OFF CACHING FOR THIS RELEASE */
	ctl_flags |= AMSL_CACHE_OFF;

	if (amsl_ctlp == (struct amsl_ctl *)NULL)
		if (init_amsl((time_t)ADM_AMSL_TIMEOUT, (u_int)1,
		    (char *)NULL, (char *)NULL, (char *)NULL, ctl_flags)
		    != 0) {
			(void) free_amsl(&amsl_ctlp);
			return(ADM_ERR_INIT_NOMEMORY);
		}
	if ((reqidp == (Adm_requestID *)NULL) || (errp == (Adm_error *)NULL)) {
		local_cleanup(reqp, (struct loc_sig_vec *)NULL);
		return(ADM_ERR_AMSL_BADCALL);
	}
	if ((stat = init_req(&reqp, reqidp, errp) != 0)) {
		(void) amsl_err2(errbuf, ADM_ERR_NOMEMORY, PROG_NAME, 1);
		adm_err_fmt(errp, ADM_ERR_NOMEMORY, ADM_ERR_SYSTEM,
		    ADM_FAILCLEAN, errbuf);
		local_cleanup(reqp, (struct loc_sig_vec *)NULL);
		return(ADM_FAILURE);
	}

	/* Must wait until after amsl control allocated for debug msgs */
	ADM_DBG("d", ("Local:  received request; starting verification"));

	/* Validate call list input and output argument parameters */
	if ((inp == (Adm_data *)NULL) || (outp == (Adm_data *)NULL)) {
		(void) amsl_err2(errbuf, ADM_ERR_AMSL_BADCALL);
		adm_err_fmt(errp, ADM_ERR_AMSL_BADCALL, ADM_ERR_SYSTEM,
		    ADM_FAILCLEAN, errbuf);
		local_cleanup(reqp, (struct loc_sig_vec *)NULL);
		return(ADM_FAILURE);
	}

	/*
	 * Do minimal initialization of request structure to set up for
	 * the verification step (which validates the arguments and
	 * fills in default fields).  We must do...
	 *	Set the call list arguments into the request structure
	 *	Set the input and output argument lists
	 *
	 * Note that request ID and error structures were set in the
	 * request structure when we called the request init routine.
	 */

	if (classname != (char *)NULL)
		reqp->class_name = strdup(classname);
	if (classvers != (char *)NULL)
		reqp->class_version = strdup(classvers);
	if (methodname != (char *)NULL)
		reqp->method_name = strdup(methodname);
	if (client_group != (char *)NULL)
		reqp->client_group_name = strdup(client_group);
	if (client_cats != (char *)NULL)
		reqp->client_diag_categories = strdup(client_cats);
	reqp->inp = inp;
	reqp->outp = outp;

	/*
	 * Establish signal handlers for SIGINT, SIGQUIT, SIGTERM, SIGCHLD.
	 * Save any previous handlers the local application may have set.
	 * We must do signal handling since SNM not here to help us.
	 */

	local_set_sigs(&ovec);
	
	/* Finish building the request structure and verify request */
	if ((stat = amsl_local_verify(reqp)) != 0) {
		local_cleanup(reqp, &ovec);
		return (ADM_FAILURE);
	}

	/* If logging, write request started message */
	(void) amsl_log2(reqp, ADM_ERR_AMSL_STARTED, reqp->request_type,
	    reqp->class_name, reqp->class_version, reqp->method_name);
	ADM_DBG("d", ("Local:  started request; disp=%d, class=%s %s, method=%s",
	    reqp->dispatch_type, reqp->class_name,
	    (reqp->class_version != NULL?reqp->class_version:""),
	    reqp->method_name));

	/* Perform the method */
	stat = amsl_invoke(reqp);
	xstat = reqp->exit_status;

	/* 
	 * Check for unformatted output.  If any exists, simply link
	 * the buffer pointed to by the stdout buffer control structure
	 * into the Adm_data structure for output arguments.  Then set
	 * the buffer pointer to NULL in the buffer control structure
	 * so that it is not freed up by the free_req routine later.
	 */

	if ((bp = reqp->outbuff) != (struct bufctl *)NULL)
		if ((bp->startp != (char *)NULL) && (bp->size != bp->left)) {
			ap = reqp->outp;
			ap->unformatted_len = bp->size - bp->left;
			ap->unformattedp = bp->startp;
			bp->startp = (char *)NULL;
		}

	/* 
	 * Check for unformatted errors.  If any exists, simply link
	 * the buffer pointed to by the stderr buffer control structure
	 * into the Adm_error structure for error returns.  Then set
	 * the buffer pointer to NULL in the buffer control structure
	 * so that it is not freed up by the free_req routine later.
	 */

	if ((bp = reqp->errbuff) != (struct bufctl *)NULL)
		if ((bp->startp != (char *)NULL) && (bp->size != bp->left)) {
			ep = reqp->errp;
			ep->unfmt_len = bp->size - bp->left;
			ep->unfmt_txt = bp->startp;
			bp->startp = (char *)NULL;
		}

	/* If logging, write request completed or terminated */
	logcode = ADM_ERR_AMSL_COMPLETED;
	if (stat != 0)
		logcode = ADM_ERR_AMSL_TERMINATED;
	(void) amsl_log2(reqp, logcode, reqp->request_type,
	    reqp->class_name, reqp->class_version, reqp->method_name);

	/* Reset to previous signal handlers */
	local_reset_sigs(&ovec);

	/* Cleanup after method invocation */
	ADM_DBG("d", ("Local:  %s request",
	    (logcode == 0?"completed":"terminated")));
	local_cleanup(reqp, (struct loc_sig_vec *)NULL);

	/* Return with the method's exit status */
	return (xstat);
}

/*
 * -----------------------------------------------------------------------
 * local_set_sigs - Initialize for local signal handling.
 *	Accepts a pointer to a structure of sigaction structs for SIGINT,
 *	SIGQUIT, SIGTERM, and SIGCHLD signals.
 * ----------------------------------------------------------------------
 */

static void
local_set_sigs(
	struct loc_sig_vec *ovecp) /* Local pointer to sigvec structs */
{
	struct sigaction nvec;	/* New signal handler structure */

	/* Initialize handler for SIGINT and SIGTERM */
	nvec.sa_flags = 0;
	(void) sigemptyset(&nvec.sa_mask);
	nvec.sa_handler = (SIG_PFV)local_sig_term;
	(void) sigaction(SIGINT, &nvec, &ovecp->ovec_int);
	(void) sigaction(SIGTERM, &nvec, &ovecp->ovec_term);

	/* Initialize handler for SIGQUIT */
	nvec.sa_flags = 0;
	(void) sigemptyset(&nvec.sa_mask);
	nvec.sa_handler = (SIG_PFV)local_sig_quit;
	(void) sigaction(SIGQUIT, &nvec, &ovecp->ovec_quit);

	/* Initialize handler for SIGCHLD */
	nvec.sa_flags = 0;
	(void) sigemptyset(&nvec.sa_mask);
	nvec.sa_handler = (SIG_PFV)local_sig_chld;
	(void) sigaction(SIGCHLD, &nvec, &ovecp->ovec_chld);

	/* Return */
	return;
}

/*
 * -----------------------------------------------------------------------
 * local_reset_sigs - Restore previous signal handling.
 *	Accepts a pointer to a structure of sigaction structs for SIGINT,
 *	SIGQUIT, SIGTERM, and SIGCHLD signals.
 * ----------------------------------------------------------------------
 */

static void
local_reset_sigs(
	struct loc_sig_vec *ovecp) /* Local pointer to sigvec structs */
{
	struct sigaction tvec;	/* Temporary signal handler structure */

	/* Reset handler for SIGINT */
	(void) sigaction(SIGINT, &ovecp->ovec_int, &tvec);

	/* Reset handler for SIGTERM */
	(void) sigaction(SIGTERM, &ovecp->ovec_term, &tvec);

	/* Reset handler for SIGQUIT */
	(void) sigaction(SIGQUIT, &ovecp->ovec_quit, &tvec);

	/* Reset handler for SIGCHLD */
	(void) sigaction(SIGCHLD, &ovecp->ovec_chld, &tvec);

	/* Return */
	return;
}

/*
 * -----------------------------------------------------------------------
 * local_cleanup - Cleanup after request attempt.
 *	Accepts a pointer to the request structure and a pointer to a
 *	structure of sigaction structs for SIGINT, SIGQUIT, SIGTERM, and
 *	SIGCHLD signals (which could be null if signals not set).
 * ----------------------------------------------------------------------
 */

static void
local_cleanup(
	struct amsl_req *reqp,	/* Local pointer to amsl request struct */
	struct loc_sig_vec *ovecp) /* Local pointers to sigvec structs */
{
	struct bufctl *bp;	/* Local buffer control struct pointer */

	/* If signal handling was set, reset to previous handlers */
	if (ovecp != (struct loc_sig_vec *)NULL)
		(void) local_reset_sigs(ovecp);

	/* If request structure not yet allocated, just return */
	if (reqp == (struct amsl_req *)NULL)
		return;

	/*
	 * Since we got argument list structures from caller and since
	 * we got the unformatted stdin buffer from caller,
	 * remove pointers to these structures from the request structure
	 * so that the request free routine does not deallocate them.
	 * Note: We leave the fmtbuff, since formatted arguments have
	 *	 already been scavanged from this buffer and we still
	 *	 want to free up the buffer later.
	 */

	reqp->reqIDp = (Adm_requestID *)NULL;
	reqp->inp = (Adm_data *)NULL;
	reqp->outp = (Adm_data *)NULL;
	reqp->errp = (Adm_error *)NULL;
	if ((bp = reqp->inbuff) != (struct bufctl *)NULL)
		bp->startp = (char *)NULL;

	/* Unlink and blow away the request structure */
	(void) unlink_req(reqp);
	free_req(reqp);

	/* Now blow away the default control structure we created */
	(void) free_amsl(&amsl_ctlp);

	/* Return */
	return;
}

/*
 * -----------------------------------------------------------------------
 * local_sig_term - Signal handling routine for SIGTERM shutdown.
 *	If method process has been forked and not reaped, send it a SIGTERM
 *	signal telling the method to gracefully terminate via its cleanup
 *	routine.
 * ----------------------------------------------------------------------
 */

static void
local_sig_term()
{
	struct amsl_req *reqp;	/* Local pointer to amsl request struct */

	ADM_DBG("d", ("Local:  caught SIGTERM signal"));

	/* Get this method request structure */
	reqp = find_req((u_int)AMSL_LOCAL_REQUEST_TAG);

	/* If method process still exists, tell it to terminate itself */
	if (reqp != (struct amsl_req *)NULL) {
		if (reqp->method_pid != (pid_t)0) {
			(void) sigsend((idtype_t)P_PID, (id_t)reqp->method_pid,
			    SIGTERM);

			ADM_DBG("d", ("Local:  terminated method process; pid %ld",
			    (long)reqp->method_pid));
		}
	}

	/* Return */
	return;
}

/*
 * -----------------------------------------------------------------------
 * local_sig_quit - Signal handling routine for SIGQUIT shutdown.
 *	If method process has been forked and not reaped, send it a SIGQUIT
 *	signal telling the method to forcefully terminate. Wait a configured
 *	amount of time and then kill the method child process.
 * ----------------------------------------------------------------------
 */

static void
local_sig_quit()
{
	struct amsl_req *reqp;	/* Local pointer to amsl request struct */

	ADM_DBG("d", ("Local:  caught SIGQUIT signal"));

	/* Mask off SIGTERM signals to avoid graceful shutdowns */
	(void) sighold(SIGTERM);

	/* Get this method request structure */
	reqp = find_req((u_int)AMSL_LOCAL_REQUEST_TAG);

	/* If method process still exists, tell it to terminate */
	if (reqp != (struct amsl_req *)NULL) {
		if (reqp->method_pid != (pid_t)0) {
			(void) sigsend((idtype_t)P_PID, (id_t)reqp->method_pid,
			    SIGTERM);

			ADM_DBG("d", ("Local:  terminated method process; pid %ld",
			    (long)reqp->method_pid));

			/* Sleep a short period and commit infanticide */
			(void) sleep((u_int)AMSL_SHUTDOWN_METHOD_TIME);
			(void) sigsend((idtype_t)P_PID, (id_t)reqp->method_pid,
			    SIGKILL);

			ADM_DBG("d", ("Local:  killed method process; pid %ld",
			    (long)reqp->method_pid));
		}
	}
	
	/* Unblock SIGTERM */
	(void) sigrelse(SIGTERM);

	/* Return */
	return;
}

/*
 * -----------------------------------------------------------------------
 * local_sig_chld - Signal handling routine for SIGCHLD.
 *	Write a debug message and return.  Cannot blow away request
 *	structure since we're in the local application process and
 *	we will ultimately return to the local dispatch routine.
 * ----------------------------------------------------------------------
 */

static void
local_sig_chld()
{
	struct amsl_req *reqp;	/* Local pointer to amsl request struct */
	pid_t  pid;

	/* Get this method request structure */
	reqp = find_req((u_int)AMSL_LOCAL_REQUEST_TAG);
	if (reqp != (struct amsl_req *)NULL)
		pid = reqp->method_pid;
	else
		pid = (pid_t)0;

	/* Write a debug message for now */
	ADM_DBG("d", ("Local:  terminated method process; pid %ld",
		(long)pid));

	/* Return */
	return;
}
