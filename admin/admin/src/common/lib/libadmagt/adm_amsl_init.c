/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)adm_amsl_init.c	1.23	92/07/24 SMI"

/*
 * FILE:  init.c
 *
 *	Admin Framework class agent routines to initialize class agent
 *	structures.  Also includes routines to free global structures
 *	and clear their global pointer variables.
 */

#include <netdb.h>
#include <fcntl.h>
#include <malloc.h>
#include <unistd.h>
#include <signal.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <sys/systeminfo.h>
#include <rpc/types.h>
#include <rpc/auth.h>
#include "adm_fw.h"
#include "adm_fw_impl.h"
#include "adm_amsl.h"
#include "adm_amsl_impl.h"

/*
 * CONTROL STRUCTURE GLOBAL VARIABLE:
 *
 * This is the only global variable defined in the class agent.
 * It points to the agent's control structure, which contains information
 * that is valid thoughout the class agent for all requests.
 */

struct amsl_ctl *amsl_ctlp = (struct amsl_ctl *)NULL;

/* External signal blocking mask structure */
static sigset_t newmask;

/* Define internal function prototypes */
static void  free_str(char **);
static char *init_get_ow_path(char *);
static char *init_chk_ow_path(char *);

/*
 * -------------------------------------------------------------------
 *  init_amsl - Admin Framework class agent initialization routine
 *	Accepts following arguments:
 *		timeout  => timeval struct with RPC message timeout
 *		seclevel => system wide security level
 *		logfile  => log file pathname
 *		logcats  => log file filter categories
 *		owpath	 => OW pathname to use if OPENWINHOME not defined
 *		flags    => options flags, including
 *				AMSL_DEBUG_ON
 *				AMSL_CONSLOG_ON
 *				AMSL_DIAGLOG_ON
 *				AMSL_SECURITY_OFF
 *				AMSL_CACHE_OFF
 *	Returns status code 1 if unable to initialize.
 *	Sets global pointer for initialized amsl control structure.
 * -------------------------------------------------------------------
 */

int
init_amsl(
	time_t timeout,		/* RPC timeout value for class agent */
	u_int sec_level,	/* System wide security level */
	char *logfile,		/* Log file pathname */
	char *logcats,		/* Log file filter categories */
	char *owpath,		/* OW pathname */
	u_int flags)		/* options flags */
{
	struct amsl_ctl *cp;	/* Local pointer to amsl control struct */
	char *msgp;		/* Error format message pointer */
	char  pathbuf[MAXPATHLEN]; /* Locale pathname buffer */
	char  svrname[SYS_NMLN+1]; /* Buffer for server hostname */

	/* Initialize signal blocking structure for request link list */
	(void) sigemptyset(&newmask);
	(void) sigaddset(&newmask, SIGINT);
	(void) sigaddset(&newmask, SIGTERM);
	(void) sigaddset(&newmask, SIGCHLD);
	(void) sigaddset(&newmask, SIGUSR1);
	(void) sigaddset(&newmask, SIGUSR2);

	amsl_ctlp = (struct amsl_ctl *)malloc(sizeof (struct amsl_ctl));
	if (amsl_ctlp == NULL) {
		msgp = adm_err_msg(ADM_ERR_INIT_NOMEMORY);
		(void) printf("%s: %s\n", ADM_CLASS_AGENT_NAME, msgp);
		(void) amsl_err_syslog (ADM_ERR_INIT_NOMEMORY);
		return (1);
	}
	cp = amsl_ctlp;

	/* Initialize miscellaneous structure fields. */
	cp->flags = flags;
	cp->sys_auth_level = 0;
	cp->sys_auth_type = 0;
	cp->sys_auth_flavor = 0;
	cp->timeout = timeout;
	cp->startup_flags = 0;
	cp->om_cache_count = 0;
	cp->om_cache = &adm_om_cache;
	cp->class_locale = (char *)NULL;
	cp->logIDp = (Adm_logID *)NULL;
	cp->log_mode = AMSL_LOG_MODE;
	cp->log_and_flag = AMSL_LOG_AND;
	cp->log_pathname = (char *)NULL;
	cp->log_filter_cats = (char *)NULL;
	cp->ow_cli_pathname = (char *)NULL;
	cp->ow_etc_pathname = (char *)NULL;
	cp->server_hostname = (char *)NULL;
	cp->firstreq = (struct amsl_req *)NULL;

	/* Get server hostname for use in testing and error messages */
	svrname[0] = '\0';
	(void) sysinfo((int)SI_HOSTNAME, svrname, (long)(SYS_NMLN));
	svrname[SYS_NMLN] = '\0';
	cp->server_hostname = strdup(svrname);

	/* Set up system wide security level */
	switch (sec_level) {
		case AMSL_AUTH_LEVEL_0:			/* Level 0 */
			cp->sys_auth_level = 0;
			cp->sys_auth_type = ADM_AUTH_NONE;
			break;
		case AMSL_AUTH_LEVEL_1:			/* Level 1 */
			cp->sys_auth_level = 1;
			cp->sys_auth_type = ADM_AUTH_WEAK;
			cp->flags &= ~(AMSL_SECURITY_OFF);
			break;
		case AMSL_AUTH_LEVEL_2:			/* Level 2 */
			cp->sys_auth_level = 2;
			cp->sys_auth_type = ADM_AUTH_STRONG;
			cp->flags &= ~(AMSL_SECURITY_OFF);
			break;
		default:				/* Default level */
			cp->sys_auth_level = 1;
			cp->sys_auth_type = ADM_AUTH_WEAK;
			cp->flags &= ~(AMSL_SECURITY_OFF);
			break;
	}

	/* For now, auth flavor is unspecified at system wide level */
	cp->sys_auth_flavor = ADM_AUTH_UNSPECIFIED;

	/* Set up OM caching depending on CLI option */
	if (cp->flags & AMSL_CACHE_OFF) {
		adm_cache_off(cp->om_cache);
		cp->om_cache = (adm_cache *)NULL;
	} else
		adm_cache_on(cp->om_cache);

	/* Get pathname to class hierarchy class & method localization */
	if (adm_find_locale(pathbuf, MAXPATHLEN) != 0) {
		(void) printf("%s: %s\n", ADM_CLASS_AGENT_NAME, pathbuf);
		(void) amsl_err_syslog1(pathbuf);
		return (1);
	}
	cp->class_locale = strdup(pathbuf);
	if (cp->class_locale == (char *)NULL) {
		msgp = adm_err_msg(ADM_ERR_INIT_NOMEMORY);
		(void) printf("%s: %s\n", ADM_CLASS_AGENT_NAME, msgp);
		(void) amsl_err_syslog (ADM_ERR_INIT_NOMEMORY);
		return (1);
	}

	/* Set optional OW pathname from CLI and etc file */
	if (owpath != (char *)NULL) {
		cp->ow_cli_pathname = init_chk_ow_path(owpath);
		if (cp->ow_cli_pathname == (char *)NULL) {
			ADM_DBG("de", ("Init:   Unable to access OW pathname from CLI; path=",
			    owpath));
		}
	}
	cp->ow_etc_pathname = init_get_ow_path(AMSL_OW_ETC_FILENAME);
	if (cp->ow_etc_pathname == (char *)NULL) {
		ADM_DBG("de", ("Init:   Unable to access OW pathname from file; file=%s",
		    AMSL_OW_ETC_FILENAME));
	}

		
	/* Initialize for logging messages to local system admin log file */
	if ((cp->flags & AMSL_DIAGLOG_ON) && (logfile != NULL)) {

		/* Copy log file pathname and filter categories strings */
		cp->log_pathname = strdup(logfile);
		if (cp->log_pathname == (char *)NULL) {
			msgp = adm_err_msg(ADM_ERR_INIT_NOMEMORY);
			(void) printf("%s: %s\n", ADM_CLASS_AGENT_NAME, msgp);
			(void) amsl_err_syslog (ADM_ERR_INIT_NOMEMORY);
			return (1);
		}
		if (logcats == (char *)NULL)
			cp->log_filter_cats = adm_diag_catcats(2,
			    ADM_CAT_REQUEST, ADM_CAT_INFO);
		else
			cp->log_filter_cats = strdup(logcats);

		/* Make sure log file exists and is writable */
		if ((adm_log_start(cp->log_pathname, cp->log_mode,
		    ADM_CATLOG_OFF, cp->log_and_flag, cp->log_filter_cats,
		    &cp->logIDp)) != ADM_SUCCESS) {
			msgp = adm_err_msg(ADM_ERR_INIT_OPENLOG);
			(void) printf("%s: ", ADM_CLASS_AGENT_NAME);
			(void) printf(msgp, logfile);
			(void) printf("\n");
			(void) amsl_err_syslog (ADM_ERR_INIT_OPENLOG, logfile);
			return (1);
		}
	}
	else
		amsl_ctlp->flags &= ~(AMSL_DIAGLOG_ON);

	return (0);
}

/*
 * ----------------------------------------------------------------------
 * free_amsl - Deallocate amsl control structure
 *	Accepts address of variable containing pointer to amsl_ctl struct.
 *	No return status.
 * ----------------------------------------------------------------------
 */
void
free_amsl(
	struct amsl_ctl **ctlpp) /* Address of ptr to amsl_ctl struct */
{
	struct amsl_ctl *cp;	/* Temporary pointer to amsl_ctl struct */
	struct amsl_req *rp;	/* Temporary pointer to amsl_req struct */
	struct amsl_req *np;	/* Temporary pointer to amsl_req struct */

	cp = *ctlpp;
	if (cp == NULL)
		return;

	/* Loop through list of request structures; freeing each one */
	while ((rp = cp->firstreq) != (struct amsl_req *)NULL) {
		np = rp->nextreq;
		free_req(rp);
		cp->firstreq = np;
	}

	/* Close the log file if its open */
	if (cp->logIDp != (Adm_logID *)NULL) {
		(void) adm_log_end(cp->logIDp);
		cp->logIDp = (Adm_logID *)NULL;
	}

	/* Free character string fields */
	(void) free_str(&cp->class_locale);
	(void) free_str(&cp->log_pathname);
	(void) free_str(&cp->log_filter_cats);
	(void) free_str(&cp->ow_cli_pathname);
	(void) free_str(&cp->ow_etc_pathname);
	(void) free_str(&cp->server_hostname);

	/* Free up the amsl control structure and clear pointer argument */
	free(cp);
	*ctlpp = (struct amsl_ctl *)NULL;

	return;
}

/*
 * ----------------------------------------------------------------------
 * find_my_req - Find request structure for current process in the list.
 *	Returns address of the request structure if found; otherwise,
 *	returns a null pointer.
 *	!!!WARNING!!! Do not call this function for local request dispatch.
 * ----------------------------------------------------------------------
 */

struct amsl_req *
find_my_req(void)
{
	u_int  reqtag;		/* Request sequence tag */

	/* Ask SNM for current process's request sequence tag */
	reqtag = _netmgt_get_request_sequence();

	/* Now go look up the associated request structure */
	return (find_req(reqtag));
}

/*
 * ----------------------------------------------------------------------
 * find_req - Find a request structure in the list.
 *	Accepts request tag identifier.
 *	Returns address of the request structure if found; otherwise,
 *	returns a null pointer.
 * ----------------------------------------------------------------------
 */

struct amsl_req *
find_req(
	u_int  reqtag)		/* Request structure tag identifier */
{
	struct amsl_req *rp;	/* Temporary pointer to req struct */
	sigset_t oldmask;	/* Temporary signal mask save area */

	/*
	 * WARNING!
	 *
	 * Must protect this code sequence with mutex lock when threads
	 * become available.
	 */

	/* Block signals during linked list manipulation */
	(void) sigprocmask(SIG_SETMASK, &newmask, &oldmask);

	/* Find the request structure in list */
	rp = (struct amsl_req *)NULL;
	if (amsl_ctlp != (struct amsl_ctl *)NULL) {
		rp = amsl_ctlp->firstreq;
		while (rp != (struct amsl_req *)NULL) {
			if (rp->request_tag == reqtag)
				break;
			else
				rp = rp->nextreq;
		}					/* End of while */
	}

	/* Restore old signal mask */
	(void) sigprocmask(SIG_SETMASK, &oldmask, (sigset_t *)NULL);

	/* Return status */
	return (rp);
}

/*
 * ----------------------------------------------------------------------
 * link_req - Link a request structure into list.
 *	Accepts a pointer to a request structure to be linked.
 *	Returns zero if request structure is linked;
 *	otherwise, returns minus one.
 * ----------------------------------------------------------------------
 */

int
link_req(
	struct amsl_req *reqp)	/* Pointer to amsl request structure */
{
	sigset_t oldmask;	/* Temporary signal mask save area */

	/*
	 * WARNING!
	 *
	 * Must protect this code sequence with mutex lock when threads
	 * become available.
	 */

	/* Block signals during linked list manipulation */
	(void) sigprocmask(SIG_SETMASK, &newmask, &oldmask);

	/* Add new control structure to front of linked list */
	reqp->nextreq = amsl_ctlp->firstreq;
	amsl_ctlp->firstreq = reqp;

	/* Restore old signal mask */
	(void) sigprocmask(SIG_SETMASK, &oldmask, (sigset_t *)NULL);

	return (0);
}

/*
 * ----------------------------------------------------------------------
 * unlink_req - Unlink a request structure from list.
 *	Accepts a pointer to a request structure to be unlinked.
 *	Returns zero if request structure is found and unlinked;
 *	otherwise, returns minus one.
 * ----------------------------------------------------------------------
 */

int
unlink_req(
	struct amsl_req *reqp)	/* Pointer to amsl request structure */
{
	struct amsl_req **rpp;	/* Address of pointer to req struct */
	struct amsl_req *rp;	/* Temporary pointer to req struct */
	sigset_t oldmask;	/* Temporary signal mask save area */
	int    stat;		/* Return status code */

	/*
	 * WARNING!
	 *
	 * Must protect this code sequence with mutex lock when threads
	 * become available.
	 *
	 * This function can be called from the reap routine, which is
	 * a signal handler routine for the SIGCHLD signal.
	 */

	/* Block signals during linked list manipulation */
	(void) sigprocmask(SIG_SETMASK, &newmask, &oldmask);

	/* Find this request structure in list */
	stat = -1;
	rpp = &amsl_ctlp->firstreq;
	while ((rp = *rpp) != (struct amsl_req *)NULL) {
		if (rp == reqp) {
			*rpp = rp->nextreq;
			stat = 0;
			break;
		}
		else
			rpp = &rp->nextreq;
	}

	/* Restore old signal mask */
	(void) sigprocmask(SIG_SETMASK, &oldmask, (sigset_t *)NULL);

	/* Return status */
	return (stat);
}

/*
 * ----------------------------------------------------------------------
 * init_req - Allocate and initialize a request structure
 *	Accepts address of where to return request structure pointer,
 *	and optional local request ID and local error structure pointers.
 *	If omitted, request ID and error structures are allocated.
 *	Returns error status code if unable to allocate request structure;
 *	otherwise, returns zero and sets the request structure pointer
 *	variable.
 *
 *	NOTE:	Does NOT set request tag field.  Must be set by calling
 *		routine to either the SNM tag or the the local tag.
 * ----------------------------------------------------------------------
 */

int
init_req(
	struct amsl_req **reqpp, /* Address of ptr to amsl request struct */
	struct Adm_requestID *loc_reqidp, /* Ptr to existing reqID struct */
	struct Adm_error *loc_errp)	  /* Ptr to existing error struct */
{
	struct amsl_req *rp;	/* Temporary pointer to req struct */

	rp = (struct amsl_req *)malloc(sizeof (struct amsl_req));
	if (rp == (struct amsl_req *)NULL) {
		ADM_DBG("de", ("INIT:   Could not allocate amsl_req structure"));
		return (ADM_ERR_NOMEMORY);
	}

	/* Initialize integer fields in the structure */
	rp->flags = 0;
	rp->request_tag = 0;
	rp->request_type = 0;
	rp->request_flags = 0;
	rp->dispatch_type = 0;
	rp->exit_status = 0;
	rp->method_pid = (uid_t)0;

	/* Initialize character string fields in the structure */
	rp->class_name = (char *)NULL;
	rp->class_version = (char *)NULL;
	rp->method_name = (char *)NULL;
	rp->agent_host = (char *)NULL;
	rp->agent_domain = (char *)NULL;
	rp->client_host = (char *)NULL;
	rp->client_domain = (char *)NULL;
	rp->client_group_name = (char *)NULL;
	rp->client_diag_categories = (char *)NULL;
	rp->client_timeout_parm = (char *)NULL;
	rp->client_lang_parm = (char *)NULL;
	rp->method_pathname = (char *)NULL;
	rp->method_filename = (char *)NULL;
	rp->method_text_domains = (char *)NULL;
	rp->diag_stdcats = (char *)NULL;
	rp->diag_reqcats = (char *)NULL;
	rp->diag_errcats = (char *)NULL;
	rp->diag_dbgcats = (char *)NULL;
	rp->diag_infocats = (char *)NULL;

	/* Initialize pointers to subordinate structures */
	rp->reqIDp = (struct Adm_requestID *)NULL;
	rp->authp = (struct amsl_auth *)NULL;
	rp->inp = (Adm_data *)NULL;
	rp->outp = (Adm_data *)NULL;
	rp->errp = (struct Adm_error *)NULL;
	rp->inbuff = (struct bufctl *)NULL;
	rp->outbuff = (struct bufctl *)NULL;
	rp->errbuff = (struct bufctl *)NULL;
	rp->fmtbuff = (struct bufctl *)NULL;
	rp->nextreq = (struct amsl_req *)NULL;

	/* If we were passed a request id struct, use it.  Else get one */
	if (loc_reqidp != (struct Adm_requestID *)NULL)
		rp->reqIDp = loc_reqidp;
	else {
		if ((rp->reqIDp =
		    (Adm_requestID *)malloc(sizeof (struct Adm_requestID)))
		    == (Adm_requestID *)NULL) {
			ADM_DBG("de", ("Init:   Could not allocate reqid struct"));
			free_req(rp);
			return (ADM_ERR_NOMEMORY);
		}
		adm_reqID_blank(rp->reqIDp);
	}

	/* If we were passed an error structure, use it.  Else get one */
	if (loc_errp != (struct Adm_error *)NULL)
		rp->errp = loc_errp;
	else if ((rp->errp = adm_err_newh()) == (struct Adm_error *)NULL){
		ADM_DBG("de", ("Init:   Could not allocate error structure"));
		free_req(rp);
		return (ADM_ERR_NOMEMORY);
	}

	/* Allocate and initialize an authorization structure */
	if ((rp->authp = (struct amsl_auth *)malloc(sizeof (struct amsl_auth)))
	    == (struct amsl_auth *)NULL) {
		ADM_DBG("de", ("Init:   Could not allocate auth structure"));
		free_req(rp);
		return (ADM_ERR_NOMEMORY);
	}
	(void) memset(rp->authp, (int)0, (int)sizeof (struct amsl_auth));
	if (amsl_ctlp->flags & AMSL_SECURITY_OFF)
		rp->authp->auth_flag = (u_int)AMSL_AUTH_OFF;
	rp->authp->auth_sys_type = amsl_ctlp->sys_auth_type;
	rp->authp->auth_sys_flavor = amsl_ctlp->sys_auth_flavor;

	/* Return request structure pointer */
	*reqpp = rp;
	return (0);
}

/*
 * ----------------------------------------------------------------------
 * free_req - Deallocate memory associated with a request structure
 *	Accepts address of pointer variable containing pointer to struct.
 *	No status is returned.
 *
 *	NOTE:	If allocated and initialized with local request ID and
 *		local error structure, calling routine should clear the
 *		request struct pointers to these structures, otherwise
 *		this routine will deallocate those structures as well.
 *		Also the case for local argument list structures.
 * ----------------------------------------------------------------------
 */

void
free_req(
	struct amsl_req *reqp)	/* Pointer to amsl request structure */
{
	struct amsl_req *rp;	/* Temporary pointer to req struct */

	rp = reqp;
	if (rp == NULL)
		return;

	/* Free character string fields in the request structure */
	(void) free_str(&rp->class_name);
	(void) free_str(&rp->class_version);
	(void) free_str(&rp->method_name);
	(void) free_str(&rp->agent_host);
	(void) free_str(&rp->agent_domain);
	(void) free_str(&rp->client_host);
	(void) free_str(&rp->client_domain);
	(void) free_str(&rp->client_group_name);
	(void) free_str(&rp->client_diag_categories);
	(void) free_str(&rp->client_timeout_parm);
	(void) free_str(&rp->client_lang_parm);
	(void) free_str(&rp->method_pathname);
	(void) free_str(&rp->method_filename);
	(void) free_str(&rp->method_text_domains);
	(void) free_str(&rp->diag_stdcats);
	(void) free_str(&rp->diag_reqcats);
	(void) free_str(&rp->diag_errcats);
	(void) free_str(&rp->diag_dbgcats);
	(void) free_str(&rp->diag_infocats);

	/* Free linked list of input arguments in request structure */
	if (rp->inp != (struct Adm_data *)NULL) {
		(void)  adm_args_freeh(rp->inp);
		rp->inp = (struct Adm_data *)NULL;
	}

	/* Free linked list of output arguments in request structure */
	if (rp->outp != (struct Adm_data *)NULL) {
		(void) adm_args_freeh(rp->outp);
		rp->outp = (struct Adm_data *)NULL;
	}

	/* Free request ID structure */
	if (rp->reqIDp != (Adm_requestID *)NULL) {
		free(rp->reqIDp);
		rp->reqIDp = (Adm_requestID *)NULL;
	}

	/* Free formatted error structure */
	if (rp->errp != (struct Adm_error *)NULL) {
		adm_err_freeh(rp->errp);
		rp->errp = (struct Adm_error *)NULL;
	}

	/* Free authorization structure */
	if (rp->authp != (struct amsl_auth *)NULL) {
		free(rp->authp);
		rp->authp = (struct amsl_auth *)NULL;
	}

	/* Free unformatted input, output, and error buffers */
	(void) free_buffs(rp);

	/* Free the request structure itself */
	free(rp);

	return;
}

/*
 * ----------------------------------------------------------------------
 * free_str - Deallocate memory associated with a character string.
 *	Accepts address of pointer variable containing pointer to string.
 *	Frees the string and sets the pointer to NULL.
 *	No status is returned.
 * ----------------------------------------------------------------------
 */

static void
free_str(char **spp)		/* Address of pointer to string */
{

	if (*spp != NULL) {
		free(*spp);
		*spp = (char *)NULL;
	}
}

/*
 * -----------------------------------------------------------------------
 * init_get_ow_path - Routine to retrieve OW pathname from /etc file.
 *      Accepts name of OW /etc file from which to read the pathname.
 *	Checks for the existence of a well-known binary in the OW
 *	directory pointed to by the pathname.  If the binary exists,
 *	returns pointer to allocated OW pathname string.  If does not
 *	exist or an error along the way, returns a null pointer.
 * ----------------------------------------------------------------------
 */

static char *
init_get_ow_path(
	char *ow_filename)	/* Name of OW etc file */
{
	char  owbuff[MAXPATHLEN+1]; /* OW etc file buffer */
	int   owfd;		/* OW etc file descriptor */
	int   nbytes;		/* Number of bytes to be read */
	int   nread;		/* Number of bytes actually read */
 
	/* Open the OW etc file for reading */
	if ((owfd = open(ow_filename, O_RDONLY)) == -1)
		return ((char *)NULL);

	/* Read the etc file, ignoring interrupted reads */
	nbytes = MAXPATHLEN;
	nread = read(owfd, owbuff, nbytes);

	/* Close the etc file now, ignoring errors */
	(void) close(owfd);

	/* Process read errors and make pathname a string */
	if (nread == 0)
		return ((char *)NULL);
	if (nread < 0)
		return ((char *)NULL);
	if (owbuff[nread - 1] == '\n')
		nread--;
	owbuff[nread] = '\0';

	/* Check for existence of OW startup command */
	return (init_chk_ow_path(owbuff));
}

/*
 * -----------------------------------------------------------------------
 * init_chk_ow_path - Routine to check for a valid OW pathname.
 *      Accepts pathname to an installed OW directory.
 *	Checks for the existence of a well-known binary in the OW
 *	directory pointed to by the pathname.  If the binary exists,
 *	returns pointer to allocated OW pathname string.  If does not
 *	exist or an error along the way, returns a null pointer.
 * ----------------------------------------------------------------------
 */

static char *
init_chk_ow_path(
	char *ow_pathname)	/* Name of OW etc file */
{
	struct stat owstat;	/* File status structure */
	char  owbuff[MAXPATHLEN+1];	/* Buffer for pathname */
	char *owpn;		/* OW pathname for return */

	/* Create pathname to well-known OW binary */
	(void) strncpy(owbuff, ow_pathname, MAXPATHLEN);
	owbuff[MAXPATHLEN] = '\0';
	(void) strncat(owbuff, AMSL_OW_BIN_FILENAME, MAXPATHLEN);
	owbuff[MAXPATHLEN] = '\0';

	/* Check for existence of well-known OW binary */
	owpn = (char *)NULL;
	if ((stat(owbuff, &owstat) == 0))
		owpn = strdup(ow_pathname);
 
	/* Return with pointer */
	return (owpn);
}
