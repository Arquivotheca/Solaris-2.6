/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 */

#ifndef	_adm_amsl_impl_h
#define	_adm_amsl_impl_h

#pragma	ident	"@(#)adm_amsl_impl.h	1.25	92/07/24 SMI"

/*
 * FILE:  adm_amsl_impl.h
 *
 *	Admin Framework class agent header file for internal definitions.
 *	Contains the declaration of two global pointer variables and their
 *	associated structure definitions required to pass information
 *	between the class agent verify, dispatch, reap, and shutdown
 *	procedures in the SNM agent program:
 *		amsl_ctlp -> amsl_ctl => class agent control structure
 *		amsl_reqp -> amsl_req => class agent request structure
 *
 *	NOTE: Global pointer variables are for the prototype based on
 *		SNM release 1.1.  When the MT version of SNM is used,
 *		these global variables must be replaced to support
 *		re-entrant class agent code.
 */

#include <sys/types.h>
#include <sys/time.h>
#include <netdb.h>
#include <poll.h>
#include <rpc/auth_sys.h>
#include "adm_cache.h"
#include "adm_log.h"
#include "adm_reqID.h"
#include "adm_args.h"
#include "adm_err.h"
#include "adm_auth.h"
#include "adm_auth_impl.h"

/* class agent request flags */
#define	AMSL_LOCAL_REQUEST	(u_int)1	/* Local call request */

/* class agent local request tag */
#define	AMSL_LOCAL_REQUEST_TAG	(u_int)1001	/* Local request tag */

/* class agent internal dispatch types; used for system data reports */
#define	AMSL_DISPATCH_INVOKE	(u_int)1	/* Invoke a method */
#define	AMSL_DISPATCH_VERSION	(u_int)2	/* Return versions */
#define	AMSL_DISPATCH_WEAKAUTH	(u_int)3	/* Return auth flavors */

/* class agent STDIN, STDOUT, STDERR, STDFMT pipe handling definitions */
#define	AMSL_NUM_PIPES		4	/* Number of pipes to a method */
#define	AMSL_STDIN_FD  		0	/* STDIN file descriptor number */
#define	AMSL_STDIN_READ		0	/* Pfd slot for STDIN read fd */
#define	AMSL_STDIN_WRITE	1	/* Pfd slot for STDIN write fd */
#define	AMSL_STDOUT_FD		1	/* STDOUT file descriptor number */
#define	AMSL_STDOUT_READ	2	/* Pfd slot for STDOUT read fd */
#define	AMSL_STDOUT_WRITE	3	/* Pfd slot for STDOUT write fd */
#define	AMSL_STDERR_FD		2	/* STDERR file descriptor number */
#define	AMSL_STDERR_READ	4	/* Pfd slot for STDERR read fd */
#define	AMSL_STDERR_WRITE	5	/* Pfd slot for STDERR write fd */
#define	AMSL_STDFMT_READ	6	/* Pfd slot for STDFMT read fd */
#define	AMSL_STDFMT_WRITE	7	/* Pfd slot for STDFMT write fd */

/* class agent options to set_pipes function */
#define	AMSL_CHILD		0	/* Set pipes for child process */
#define	AMSL_PARENT		1	/* Set pipes for parent process */

/* class agent shutdown wait times */
#define	AMSL_SHUTDOWN_GRACE_TIME	15	/* Graceful shutdown */
#define	AMSL_SHUTDOWN_FORCE_TIME	4	/* Forced shutdown */
#define	AMSL_SHUTDOWN_METHOD_TIME	2	/* Method quit shutdown */
#define	AMSL_SHUTDOWN_CHECK_TIME	2	/* Periodic check time */

/* class agent OW information */
#define	AMSL_OW_ETC_FILENAME	"/etc/OPENWINHOME" /* OW etc file name */
#define	AMSL_OW_BIN_FILENAME	"/bin/xnews"	/* OW server binary name */

/* class agent miscellaneous definitions */
#define	AMSL_FILE_NAMESIZE	255	/* log file name maximum size */
#define	AMSL_INPUTARGS_BUFFSIZE	8192	/* formatted input arg buff size */
#define	AMSL_WEAKAUTH_BUFFSIZE	255	/* Auth flavors name buff size */
#define	AMSL_OM_CACHE_COUNT	100	/* Log OM cache stats reset count */

/* class agent amsl control structure definition */
struct amsl_ctl {
	u_int  flags;			/* AMSL internal control flags */
	u_int  sys_auth_level;		/* System security level */
	u_int  sys_auth_type;		/* System auth minimum strength */
	u_int  sys_auth_flavor;		/* System auth flavor */
	time_t timeout;			/* RPC timeout (seconds) */
	u_int  startup_flags;		/* RPC startup flags */
	u_int  om_cache_count;		/* OM cache logging counter */
	adm_cache *om_cache;		/* Ptr to static OM cache struct */
	char  *class_locale;		/* Path to class L10n directory */
	Adm_logID *logIDp;		/* Ptr to diagnostic log file ID */
	mode_t log_mode;		/* Diagnostic log file mode flags */
	boolean_t log_and_flag;		/* Diag log file "and" flag */
	char  *log_pathname;		/* Ptr to diagnostic log pathname */
	char  *log_filter_cats;		/* Ptr to filter diag categories */
	char  *ow_cli_pathname;		/* Ptr to OW pathname from CLI */
	char  *ow_etc_pathname;		/* Ptr to OW pathname from /etc */
	char  *server_hostname;		/* Ptr to local server hostname */
	struct amsl_req *firstreq;	/* List of request structures */
};

/* amsl control flags definitions */
#define	AMSL_DEBUG_ON		1	/* Write debug msg's to terminal */
#define	AMSL_CONSLOG_ON		2	/* Write request msg's to syslogd */
#define	AMSL_DIAGLOG_ON		4	/* Log diag msg's to logfile */
#define	AMSL_SECURITY_OFF	8	/* No security checking for agent */
#define	AMSL_DEBUG_CHILD	16	/* Debugging method child proc */
#define	AMSL_CACHE_OFF		32	/* Admin OM caching turned off */

/* amsl security leval definitions */
#define	AMSL_AUTH_LEVEL_0	0	/* Level 0 = AUTH_NONE + ACLOFF */
#define	AMSL_AUTH_LEVEL_1	1	/* Level 1 = AUTH_UNIX + ACLON */
#define	AMSL_AUTH_LEVEL_2	2	/* Level 2 = AUTH_DES  + ACLON */

/* amsl control startup flags definitions */
#define	AMSL_DONT_EXIT	(u_int)(1 << 1)	/* Don't exit after request */
#define	AMSL_DBG_AGENT	(u_int)(1 << 0)	/* Don't fork agent dispatch */

/* amsl control diagnostic log file mode definition */
#define	AMSL_LOG_MODE	(mode_t)0664	/* Default log file mode */

/* amsl control diagnostic log file and flag definition */
#define	AMSL_LOG_AND	(boolean_t)B_FALSE /* Default log and flag */

/* class agent request control structure definition */
struct amsl_req {
	u_int  flags;			/* Request control flags */
	u_int  request_tag;		/* Request sequence tag from SNM */
	int    request_version;		/* Request protocol version num */
	u_int  request_type;		/* Request type */
	u_int  request_flags;		/* Request invocation flags */
	u_int  dispatch_type;		/* Type of dispatch report */
	u_int  exit_status;		/* Method runfile exit status */
	char  *class_name;		/* Name of class */
	char  *class_version;		/* Class version identifier */
	char  *method_name;		/* Name of method */
	char  *agent_host;		/* Name of target host */
	char  *agent_domain;		/* Name of target domain */
	char  *client_host;		/* Name of client host */
	char  *client_domain;		/* Name of client domain */
	char  *client_group_name;	/* Name of group from client */
	char  *client_diag_categories;	/* Client request diag categories */
	char  *client_timeout_parm;	/* Client max timeout for method */
	char  *client_lang_parm;	/* Client language preference */
	pid_t  method_pid;		/* Method process identifier */
	char  *method_pathname;		/* Path to method runfile dir */
	char  *method_filename;		/* Name of method runfile */
	char  *method_text_domains;	/* Names of I18n text domains */
	char  *diag_stdcats;		/* List of standard diag cats */
	char  *diag_reqcats;		/* List of request diag cats */
	char  *diag_errcats;		/* List of error diag cats */
	char  *diag_dbgcats;		/* List of debug diag cats */
	char  *diag_infocats;		/* List of system info diag cats */
	Adm_requestID *reqIDp;		/* Request identifier from AMCL */
	Adm_data *inp;			/* Linked list of input args */
	Adm_data *outp;			/* Linked list of output args */
	Adm_error *errp;		/* AMSL formatted error structure */
	struct amsl_auth *authp;	/* Request authorization struct */
	struct bufctl *inbuff;		/* Unformatted input buffer area */
	struct bufctl *outbuff;		/* Unformatted output buffer area */
	struct bufctl *errbuff;		/* Unformatted error buffer area */
	struct bufctl *fmtbuff;		/* Formatted data buffer area */
	struct amsl_req *nextreq;	/* Next request structure */
};

/* class agent request control and processing flag definitions */
#define	AMSL_REQ_NEWROW		1	/* Force new row of output args */

/* class agent request authentication & authorization structure defns */
struct amsl_auth {
	u_int  auth_flag;		/* Auth action flag */
	u_int  auth_fail;		/* Auth failure flag */
					/* From system wide security info:*/
	u_int  auth_sys_type;		/* Minimum auth strength */
	u_int  auth_sys_flavor;		/* Specific auth flavor */
					/* From client RPC security info: */
	u_int  auth_flavor;		/* Authentication flavor (client) */
	uid_t  auth_uid;		/* Auth user identity (client) */
	gid_t  auth_gid;		/* Auth group identity (client) */
	u_int  auth_gid_entries;	/* Number entries in group list */
	gid_t  auth_gid_list[NGRPS];	/* Alternate group ids (client) */
					/* From method security info: */
	u_int  auth_sid_flag;		/* Set identity flag (method) */
	uid_t  auth_sid_uid;		/* Set identity user uid */
	gid_t  auth_sid_gid;		/* Set identity group gid */
					/* From runtime authentication: */
	u_int  auth_type;		/* Required auth type */
	u_int  auth_flavor_entries;	/* Number entries in flavor list */
	u_int  auth_flavor_list[ADM_AUTH_MAXFLAVORS]; /* List of flavors */
	Adm_auth_cpn auth_cpn;		/* Common principal name */
};

/* class agent request auth structure action flag definitions */
#define	AMSL_AUTH_OFF		1	/* Auth checking off for request */
#define	AMSL_AUTH_GOT_IDS	2	/* Got client's identity */
#define	AMSL_AUTH_DEMOTED	4	/* Client identity demoted */
#define	AMSL_AUTH_ACL_OFF	8	/* ACL checking off for request */
#define	AMSL_AUTH_CHK_AUTH	16	/* Checked authentication type */
#define	AMSL_AUTH_CHK_ACL	32	/* Checked authorization access */
#define	AMSL_AUTH_CHK_SID	64	/* Checked set identity */
#define AMSL_AUTH_LOCAL_IDS	128	/* Local request; use proc's ids */
#define	AMSL_AUTH_ROOT_ID	256	/* Client is a root identity */

/* class agent request auth structure failure flag definitions */
#define	AMSL_AUTH_FAIL_OK	0	/* No failure; auth OK */
#define	AMSL_AUTH_FAIL_WEAK	1	/* Auth level too weak */
#define	AMSL_AUTH_FAIL_WRONG	2	/* Auth flavor not in method list */
#define	AMSL_AUTH_FAIL_ACL	4	/* Client not authorized */
#define	AMSL_AUTH_FAIL_NOUID	8	/* No local user id for client */
#define	AMSL_AUTH_FAIL_NOGRP	16	/* Client group not found */
#define	AMSL_AUTH_FAIL_BADGRP	32	/* Client user id not in group */
#define	AMSL_AUTH_FAIL_NOSETID	64	/* Method requires local user id*/

/* class agent method io buffer structure definitions */
#define	AMSL_BUFF_SIZE	1024		/* Size of buffer increment */
					/* !!! WARNING !!! Never make this */
					/* bigger than BUFSIZ in stdio */
#define	AMSL_STDIN_MAXWRITE	512	/* Maximum bytes per write to stdin */

struct bufctl {				/* Buffer control structure */
	u_int  size;			/* Length of buffer */
	char  *startp;			/* Address of start of buffer */
	u_int  left;			/* Length of free space in buffer */
	char  *currp;			/* Address of start of free space */
};

/* class agent global variable declarations */
extern struct amsl_ctl *amsl_ctlp;	/* Global pointer to amsl_ctl */

/* include other AMSL internal header files */
#include "adm_amsl_proto.h"

#endif /* !_adm_amsl_impl_h */
