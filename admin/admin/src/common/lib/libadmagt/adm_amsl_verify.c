/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)adm_amsl_verify.c	1.24	95/09/08 SMI"

/*
 * FILE:  verify.c
 *
 *	Admin Framework class agent verify procedures
 */

#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/systeminfo.h>
#include <netmgt/netmgt.h>
#include "adm_fw.h"
#include "adm_fw_impl.h"
#include "adm_amsl.h"
#include "adm_amsl_impl.h"

/* Declare static functions */
static void verify_finish(struct amsl_req *);
static void verify_logmsg(struct amsl_req *, int);
static int  verify_validate(struct amsl_req *);
static int  verify_vers_1(struct amsl_req *);
static void verify_err(struct amsl_req *);
static void verify_err2(int, char *);

/*
 * -------------------------------------------------------------------
 *  amsl_verify - Framework class agent remote request verify procedure
 *	Input arguments are those defined by SNM Manager/Agent Services
 *	library for the agent verify and dispatch procedures.
 *	Returns TRUE if request verified, FALSE if not.
 *	If not verified, returns error via SNM error handling.
 * -------------------------------------------------------------------
 */

#define	PROG_NAME	"amsl_verify"

/*ARGSUSED*/

boolean_t
amsl_verify(
	u_int  type,		/* SNM call I/F: request type */
	char  *host,		/* SNM call I/F: host system name */
	char  *adm_class,		/* SNM call I/F: class name or group name */
	char  *method,		/* SNM call I/F: method name or key value */
	u_int  count,		/* SNM call I/F: report count */
	struct timeval interval, /* SNM call I/F: report interval */
	u_int  flags)		/* SNM call I/F: request flags */
{
	struct amsl_req *reqp;	/* Local pointer for amsl request struct */
	char   errbuf[ADM_MAXERRMSG];	/* Local error text buffer */
	int    stat;		/* Local status code */
	boolean_t retval;		/* Return boolean value */

	ADM_DBG("d", ("Verify: received request; starting verification"));

	/* Use do while statement so can exit early via break statements */
	stat = 0;
	do {

		/* allocate amsl request structure and set verify pointer */
		reqp = (struct amsl_req *)NULL;
		if ((stat = init_req(&reqp, (Adm_requestID *)NULL,
		    (Adm_error *)NULL)) != 0) {
			stat = amsl_err2(errbuf, ADM_ERR_NOMEMORY,
			    PROG_NAME, 1);
			verify_err2(ADM_ERR_NOMEMORY, errbuf);
			break;
		}

		/* Since remote call, use SNM request tag */
		reqp->request_tag = _netmgt_get_request_sequence();

		/* Link request structure into list in amsl_ctl structure */
		(void) link_req(reqp);

		/* Get the Admin protocol system parameters */
		reqp->request_type = type;
		reqp->dispatch_type = (u_int)AMSL_DISPATCH_INVOKE;
		if ((stat = get_system_parms(reqp)) != 0) {
			verify_err(reqp);
			break;
		}

		/*
		 * Validate Admin protocol version number.
		 * If request version lower than we can handle, return an
		 * error.
		 * If request version higher than we can handle, accept the
		 * request (return TRUE), but reset the request type to a
		 * request for the range of protocol versions this class
		 * agent can handle.  We depend on the amsl_dispatch routine
		 * making the actual version range callback RPC message.
		 */

		if (reqp->request_version < ADM_LOW_VERSION) {
			stat = amsl_err(reqp, ADM_ERR_BADVERSION,
			    reqp->request_version);
			verify_err(reqp);
			break;
		}
		if (reqp->request_version > ADM_HIGH_VERSION) {
			reqp->dispatch_type = (u_int)AMSL_DISPATCH_VERSION;
			stat = 0;
			break;
		}

		/* Retrieve authentication info from SNM agent services */
		if ((stat = get_auth(reqp)) != 0) {
			verify_err(reqp);
			break;
		}

		/* Validate the request and check results */
		if ((stat = verify_validate(reqp)) != 0)
			if (stat == ADM_ERR_AMSL_AUTHWEAK) {
				reqp->dispatch_type =
				    (u_int)AMSL_DISPATCH_WEAKAUTH;
				stat = 0;
			} else
				verify_err(reqp);

		/* If errors, break out of loop now */
		if (stat != 0)
			break;


	} while (stat != stat);				/* End of do while */

	/* End of verification.  Complete request struct initialization. */
	(void) verify_finish(reqp);

	/* Write the verification log message depending upon status */
	(void) verify_logmsg(reqp, stat);

	/*
	 * Check if we got an error validating the request.
	 * If so, deny the request, free structures, and return FALSE.  
	 * If not, continue to the dispatch routine by returning TRUE.
	 */

	retval = B_TRUE;
	if (stat != 0) {
		retval = B_FALSE;
		(void) unlink_req(reqp);
		free_req(reqp);
	}

	/* Return boolean */
	return (retval);
}

#undef PROG_NAME

/*
 * -------------------------------------------------------------------
 *  amsl_local_verify - Framework class agent local request verify procedure
 *	Accepts pointer to the request structure already initialized
 *	with values from the local dispatch call list arguments.
 *	Returns a zero status if verified; otherwise, returns a non-zero
 *	status code and an error message in the error structure in the
 *	request structure.
 * -------------------------------------------------------------------
 */

#define	PROG_NAME	"amsl_local_verify"

int
amsl_local_verify(
	struct amsl_req *reqp)	/* Pointer to amsl request structure */
{
	int    stat;		/* Local status code */

	/*
	 * Fill in some default request information for a local request:
	 *	Use local request tag (good until MT time)
	 *	Use current Framework version number
	 *	Use action request for performing an invocation
	 *	Flag this request as a local request
	 */

	reqp->request_tag = AMSL_LOCAL_REQUEST_TAG;
	reqp->request_version = (int)ADM_VERSION;
	reqp->request_type = ADM_PERFORM_REQUEST;
	reqp->request_flags = AMSL_LOCAL_REQUEST;
	reqp->dispatch_type = AMSL_DISPATCH_INVOKE;

	/* Link the request structure onto control structure list */
	(void) link_req(reqp);

	/* Initialize for local user authentication before verification */
	(void) set_local_auth(reqp);

	/* Validate the request */
	stat = verify_validate(reqp);

	/* End of verification.  Complete request struct initialization. */
	(void) verify_finish(reqp);

	/* Fill in a couple more request fields now that defaults set */
	reqp->client_host = strdup(reqp->agent_host);
	reqp->client_domain = strdup(reqp->agent_domain);

	/* Write the verification log message depending upon status */
	(void) verify_logmsg(reqp, stat);

	/* Return with status */
	return (stat);
}

#undef PROG_NAME

/*
 * -------------------------------------------------------------------
 *  verify_validate - Validate the request from the request structure.
 *	Accepts a pointer to the amsl request structure to be validated.
 *	Returns a zero status if verified; otherwise returns a non-zero
 *	error status code and puts the error message in the formatted
 *	error structure in the request structure.
 * -------------------------------------------------------------------
 */

#define	PROG_NAME	"verify_validate"

static int
verify_validate(
	struct amsl_req *reqp)	/* Pointer to amsl request structure */
{
	int    stat;		/* Local error status code */

	/*
	 * Validate the request via checking for appropriate valid
	 * values in the request structure.  The request structure
	 * fields were filled in either from a remote request
	 * via the get_system_parms routine or from a local dispatch
	 * request via its call list arguments.  We use a switch
	 * and functionalize the validation routines so that they
	 * can be used for more than one version number.
	 */

	switch (reqp->request_version) {

	case 1:				/* Version 1 */
		stat = verify_vers_1(reqp);
		break;

	default:			/* Bad version */
		stat = amsl_err(reqp, ADM_ERR_BADVERSION,
		    reqp->request_version);
		break;
	}					/* End of switch */

	/* Return status */
	return (stat);
}

#undef PROG_NAME

/*
 * -------------------------------------------------------------------
 *  verify_finish - Complete filling in the request structure.
 *	Accepts a pointer to the amsl request structure to be completed.
 *	Returns no status.
 * -------------------------------------------------------------------
 */

#define	PROG_NAME	"verify_finish"

static void
verify_finish(
	struct amsl_req *reqp)	/* Pointer to amsl request structure */
{
	char   errbuf[ADM_MAXERRMSG];	/* Local error text buffer */
	char   ridbuf[ADM_MAXRIDLEN+1]; /* Local request id string buffer */
	u_int  utlen;		/* Local length variable */
	int    tlen;		/* Local length variable */

	/* Set up diag standard categories & header for this request */
	if (reqp != (struct amsl_req *)NULL) {
		(void) adm_reqID_rid2str(*(reqp->reqIDp), ridbuf,
		    (ADM_MAXRIDLEN+1), &utlen);
		reqp->diag_stdcats = adm_diag_stdcats(reqp->class_name,
		    reqp->class_version, reqp->method_name,
		    reqp->agent_host, reqp->agent_domain, ridbuf);
		reqp->diag_reqcats = adm_diag_catcats(3, reqp->diag_stdcats,
		    ADM_CAT_REQUEST, reqp->client_diag_categories);
		reqp->diag_errcats = adm_diag_catcats(3, reqp->diag_stdcats,
		    ADM_CAT_ERROR, reqp->client_diag_categories);
		reqp->diag_dbgcats = adm_diag_catcats(3, reqp->diag_stdcats,
		    ADM_CAT_DEBUG, reqp->client_diag_categories);
		reqp->diag_infocats = adm_diag_catcats(3, reqp->diag_stdcats,
		    ADM_CAT_INFO, reqp->client_diag_categories);

		/* Set up logging standard header for this request */
		(void) adm_log_info(ADM_ALL_LOGS, reqp->class_name,
		    reqp->class_version, reqp->method_name,
		    reqp->agent_host, reqp->agent_domain,
		    reqp->client_host, *(reqp->reqIDp));
	}

	/* If OM caching is on, see if time to log cache statistics */
	if (amsl_ctlp->om_cache != (adm_cache *)NULL) {
		amsl_ctlp->om_cache_count++;
		if (amsl_ctlp->om_cache_count >= AMSL_OM_CACHE_COUNT) {
			amsl_ctlp->om_cache_count = 0;
			errbuf[0] = '\0';
			tlen = ADM_MAXERRMSG;
			adm_cache_stats(amsl_ctlp->om_cache, errbuf, &tlen);
			(void) amsl_log3((u_int)0, errbuf);
		}
	}

	/* Return */
	return;
}

#undef PROG_NAME

/*
 * -------------------------------------------------------------------
 *  verify_logmsg - Write verification log message depending on status.
 *	Accepts a pointer to the amsl request structure and the 
 *	validation status code.  If status is zero, write verified
 *	log message, else write denied log message.
 *	Returns no status.
 * -------------------------------------------------------------------
 */

#define	PROG_NAME	"verify_logmsg"

static void
verify_logmsg(
	struct amsl_req *reqp,	/* Pointer to amsl request structure */
	int    stat)		/* Verify validation status code */
{

	/* Check results from verification */
	if (stat == 0) {			/* Request accepted */

		/* Write verified debug message */
	ADM_DBG("d", ("Verify: validated request; disp=%d, type=%d, class=%s %s, method=%s",
		    reqp->dispatch_type, reqp->request_type, reqp->class_name,
		    (reqp->class_version != NULL?reqp->class_version:""),
		    reqp->method_name));

	} else {				/* Request denied */

		/* Log the request error and denial messages */
		if (reqp != (struct amsl_req *)NULL) {
			if (reqp->errp != (Adm_error *)NULL)
				if (reqp->errp->message != (char *)NULL)
					(void) amsl_log1(reqp->diag_errcats,
					    reqp->errp->message);
			(void) amsl_log2(reqp, ADM_ERR_AMSL_DENIED,
			    reqp->request_type, reqp->class_name,
			    reqp->class_version, reqp->method_name);
		}

		/* Write denied debug message */
	ADM_DBG("d", ("Verify: denied request; type=%d, class=%s %s, method=%s",
		    reqp->request_type,
		    (reqp->class_name != NULL?reqp->class_name:"(nil)"),
		    (reqp->class_version != NULL?reqp->class_version:""),
		    (reqp->method_name != NULL?reqp->method_name:"(nil)")));

	}

	/* Return */
	return;
}

#undef PROG_NAME

/*
 * -------------------------------------------------------------------
 *  verify_vers_1 - Validate version 1 request protocol.
 *	Accepts a pointer to the amsl request structure to be validated.
 *	Returns a zero status if verified; otherwise returns a non-zero
 *	error status code and puts the error message in the formatted
 *	error structure in the request structure.
 * -------------------------------------------------------------------
 */

#define	PROG_NAME	"verify_vers_1"

static int
verify_vers_1(
	struct amsl_req *reqp)	/* Pointer to amsl request structure */
{
	char   pathbuf[MAXPATHLEN]; /* Local method pathname buffer */
	char   sysbuf[SYS_NMLN+1];  /* Local system info buffer */
	char  *tp;		/* Local string pointer */
	int    stat;		/* Local status code */

	/* Use do while statement so can exit early via break statements */
	stat = 0;
	do {

		/* verify request type */
		if (reqp->request_type != ADM_PERFORM_REQUEST) {
			stat = amsl_err(reqp, ADM_ERR_BADREQTYPE,
			    reqp->request_type);
			break;
		}

		/* If target host name ommitted, use local host name */
		if (reqp->agent_host == (char *)NULL) {
			sysbuf[0] = '\0';
			(void) sysinfo((int)SI_HOSTNAME, sysbuf,
			    (long)(SYS_NMLN+1));
			sysbuf[SYS_NMLN] = '\0';
			reqp->agent_host = strdup(sysbuf);
		}

		/* If target domain name ommitted, use local domain name */
		if (reqp->agent_domain == (char *)NULL) {
			sysbuf[0] = '\0';
			(void) sysinfo((int)SI_SRPC_DOMAIN, sysbuf,
			    (long)(SYS_NMLN+1));
			sysbuf[SYS_NMLN] = '\0';
			reqp->agent_domain = strdup(sysbuf);
		}

		/* verify class name */
		if (reqp->class_name == (char *)NULL) {
			stat = amsl_err(reqp, ADM_ERR_BADCLASS);
			break;
		}

		/* verify method name */
		if (reqp->method_name == (char *)NULL) {
			stat = amsl_err(reqp, ADM_ERR_BADMETHOD);
			break;
		}

		/* validate class and method can be executed */
		if ((stat = adm_find_method(reqp->class_name,
					reqp->class_version,
					reqp->method_name,
					pathbuf,
					MAXPATHLEN)) != 0) {
			/* NOTE: Error text returned in path buffer */
			stat = amsl_err1(reqp, stat, pathbuf);
			break;
		}
		if ((stat = check_auth(reqp)) != 0) {
			break;
		}

		/* Save method path and file names in request structure */
		tp = strrchr(pathbuf, '/');
		if (tp != (char *)NULL)
			*tp = '\0';
		reqp->method_pathname = strdup(pathbuf);
		if (tp != (char *)NULL) {
			*tp = '/';
			tp++;
			reqp->method_filename = strdup(tp);
		}
		if (reqp->method_pathname == (char *)NULL) {
			stat = amsl_err(reqp, ADM_ERR_NOMEMORY, PROG_NAME, 2);
			break;
		}
		if (reqp->method_filename == (char *)NULL) {
			stat = amsl_err(reqp, ADM_ERR_NOMEMORY, PROG_NAME, 3);
			break;
		}

		/* Retrieve class method I18n text domain names */
		if ((stat = adm_find_domain(reqp->class_name, pathbuf,
		    MAXPATHLEN)) != 0) {
			stat = amsl_err1(reqp, stat, pathbuf);
			break;
		}
		reqp->method_text_domains = strdup(pathbuf);
		if (reqp->method_text_domains == (char *)NULL) {
			stat = amsl_err(reqp, ADM_ERR_NOMEMORY, PROG_NAME, 4);
			break;
		}

	} while (stat != stat);				/* End of do while */

	/* Return status */
	return (stat);
}

#undef PROG_NAME

/*
 * ------------------------------------------------------------------
 * verify_err - class agent verify remote error handling routine
 *	Accepts a pointer to the request structure which contains
 *	a pointer to the formatted error structure.
 *	Calls verify_err2 to return the error.
 * -------------------------------------------------------------------
 */

#define	PROG_NAME	"verify_err"

static void
verify_err(
	struct amsl_req *reqp)	/* Pointer to amsl request structure */
{
	Adm_error *ep;		/* Local pointer to error structure */

	/* Send error code and message */
	ep = reqp->errp;
	if (ep != (struct Adm_error *)NULL)
		verify_err2(ep->code, ep->message);

	/* Return without status */
	return;
}

#undef PROG_NAME

/*
 * -------------------------------------------------------------------
 *  verify_err2 - class agent verify remote error handling routine
 *	This routine accepts a status code and error message string.
 *	It returns an error message on the SNM agent services response
 *	to the request RPC.  It uses the SNM error handling routine
 *	to get the Admin Framework error code and error message back to
 *	the AMCL.
 * -------------------------------------------------------------------
 */

#define	PROG_NAME	"verify_err2"

static void
verify_err2(
	int stat,		/* Error status code */
	char *msgp)		/* Error message string */
{
	Netmgt_error error;	/* NETMGT error structure */
	u_int  len;		/* Temporary length variable */

	ADM_DBG("e", ("Verify: Error %d: %s", stat, msgp));

	/* Set up Netmgt_error structure for returning error */
	error.service_error = NETMGT_FATAL;
	error.agent_error = stat;
	adm_err_snm2str(ADM_FAILURE, ADM_ERR_SYSTEM, ADM_FAILCLEAN, msgp,
	    &error.message, &len);

	/* Send back the error */
	netmgt_send_error(&error);

	return;
}

#undef PROG_NAME
