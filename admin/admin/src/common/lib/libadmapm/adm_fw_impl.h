
/*
 *  Copyright (c) 1990 Sun Microsystems, Inc.  All Rights Reserved.
 */

/*
 *******************************************************************************
 *
 *	This file contains the general definitions used internally within
 *	the administrative framework.  The file contains:
 *
 *		o AMCL/AMSL internal protocol version.
 *		o Internal system arguments for use in framework protocol
 *		  (also used for environment variable names).
 *		o Misc. constants related to system arguments.
 *		o Formatted output pipe control values.
 *		o Framework/Object Manager message localization domain names.
 *		o General framework global variable references.
 *		o Debugging macro definitions.
 *		o General framework routine references.
 *
 *******************************************************************************
 */

#ifndef _adm_fw_impl_h
#define _adm_fw_impl_h

#pragma	ident	"@(#)adm_fw_impl.h	1.27	94/06/07 SMI"

#include <stdio.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/times.h>

/*
 *----------------------------------------------------------------------
 * AMCL/AMSL internal protocol version number.
 *----------------------------------------------------------------------
 */

#define ADM_VERSION		1
#define ADM_LOW_VERSION		1
#define ADM_HIGH_VERSION	1

/*
 *----------------------------------------------------------------------
 * Internal system argument names (and environment variable names).
 *----------------------------------------------------------------------
 */

#define ADM_VERSION_NAME     "ADM_FW_VERSION"	/* Internal FW protocol version */
#define ADM_HIGH_VERSION_NAME "ADM_HIGH_VERSION" /* Highest FW protocol version */
						 /* a client or agent can speak */
#define ADM_LOW_VERSION_NAME  "ADM_LOW_VERSION" /* Lowest FW protocol version */
						/* a client or agent can speak */

#define ADM_FLAGS_NAME	     "ADM_FLAGS"	/* FW control flags */
#define ADM_LANG_NAME	     "ADM_LANG"		/* Client's language preference */

#define ADM_REQUESTID_NAME   "ADM_REQUESTID"	/* Method request ID */
#define ADM_CLASS_NAME	     "ADM_CLASS"	/* Class of requested method */
#define ADM_CLASS_VERS_NAME  "ADM_CLASS_VERS"	/* Class version name */
#define ADM_METHOD_NAME	     "ADM_METHOD"	/* Name of requested method */
#define ADM_HOST_NAME	     "ADM_HOST"		/* Target of method request */
#define ADM_DOMAIN_NAME	     "ADM_DOMAIN"	/* Domain to perform request in */
#define ADM_CATEGORIES_NAME   "ADM_CATEGORIES"	/* Additional categories for */
						/* tracing message */
#define ADM_AUTH_FLAVORS_NAME "ADM_AUTH_FLAVORS"/* List of acceptable authen. */
						/* flavors from agent */

#define ADM_TIMEOUT_PARMS_NAME "ADM_TIMEOUT_PARMS" /* Request timeout parms. */

#define ADM_CLIENT_HOST_NAME	"ADM_CLIENT_HOST"	/* Client's hostname */
#define ADM_CLIENT_DOMAIN_NAME	"ADM_CLIENT_DOMAIN"	/* Client SecureRPC */
							/* domain */
#define ADM_CLIENT_GROUP_NAME	"ADM_CLIENT_GROUP"	/* Client's preferred */
							/* group */

#define ADM_UNFMT_NAME	    "NETMGT_OPTSTRING"  /* Unformatted data */
#define ADM_UNFERR_NAME	    "ADM_UNFERR"	/* Unformatted error info */

#define ADM_FENCE_NAME	    "ADM_FENCE"		/* Divider between system and */
						/* method arguments */

/* The following are used as environment variables only. */

#define ADM_CLIENT_ID_NAME  "ADM_CLIENT_ID"	/* Client's identity */
#define ADM_STDCATS_NAME    "ADM_STDCATS"	/* Standard category list */
#define ADM_STDFMT_NAME     "ADM_STDFMT"	/* fd for passing fmt'd results */
#define ADM_INPUTARGS_NAME  "ADM_INPUTARGS"	/* Fm'd input args for method */
#define ADM_LPATH_NAME	    "ADM_LPATH"		/* Default method msg file path */
#define ADM_TEXT_DOMAINS_NAME "ADM_TEXT_DOMAINS"/* Class/method i18n domains */

/*
 *----------------------------------------------------------------------
 * Misc. constants related to system arguments.
 *----------------------------------------------------------------------
 */

					/* ADM_TIMEOUT_PARMS format */
#define ADM_TIMEOUTS_FMT "TTL=%ld PTO=%ld PCNT=%u PDLY=%ld"

/*
 *----------------------------------------------------------------------
 * Constants used to control formatted output pipe.
 *----------------------------------------------------------------------
 */

#define ADM_NOFMT	-1		/* No known file desc. for formatted */
					/* output. */
#define ADM_FMTMODE	"a"		/* Open mode for fmt'd output file ptr */

#define ADM_ARGMARKER	"+"		/* Char. to signal fmt'd data */
#define ADM_ROWMARKER	"@"		/* Char. to signal end-of-row */
#define ADM_ERRMARKER	"-"		/* Char. to signal fmt'd error */
#define ADM_CLEANMARKER	"="		/* Char. to signal cleanup status */
#define ADM_DIAGMARKER	"#"		/* Char. to signal diagnostic msg */

/*
 *----------------------------------------------------------------------
 * Framework/Object Manager message localization domain names.
 *   - full file name can be formed by appending ".mo"
 *     to the domain name.
 *----------------------------------------------------------------------
 */

#define ADM_MSG_PATH	"/usr/lib/locale"	/* Message locale dir */

/*
 *----------------------------------------------------------------------
 * References to general framework global variables.
 *----------------------------------------------------------------------
 */

extern boolean_t adm_inited;	  /* Has process (or method) been initialized? */
extern boolean_t adm_amsl_pid;	  /* Should AMSL print PID info (when in */
extern char 	*adm_class;	  /* Requested method class name (if method) */
extern char	*adm_client_id;	  /* Client identity from authentication */
extern char	*adm_debug_cats;  /* Active framework debugging categories */
extern char	*adm_domain;	  /* Domain of method request (if method) */
extern u_long	 adm_flags;	  /* Framework control flags */
extern char	 adm_host[MAXHOSTNAMELEN+1]; /* Name of current host */
extern char	*adm_lpath;	  /* Default method message file pathname */
extern char	*adm_method;	  /* Requested method name (if method) */
extern boolean_t adm_msgs_inited; /* Message files initialized? */
extern pid_t	 adm_pid;	  /* PID of running process */
extern u_int	 adm_ping_cnt;	  /* # of times to try to ping a class agent */
extern long	 adm_ping_delay;  /* Time delay before beginning pinging */
extern long	 adm_ping_timeout;/* Timeout to wait for a ping ack */
extern Adm_requestID	adm_reqID;/* Request ID (if method) */
extern long	 adm_rep_timeout; /* Max. time-to-live for a request */
extern char	*adm_stdcats;	  /* Standard tracing categories */
extern FILE	*adm_stdfmt;	  /* FILE ptr for formatted output (if method) */
extern char	*adm_text_domains;/* I18n text domains used by class/method */
extern u_int	adm_auth_init_type;   /* Default auth type and flavor to */
extern u_int	adm_auth_init_flavor; /* use on initial method req. attempt */

/*
 *----------------------------------------------------------------------
 * Debugging macro definitions.
 *----------------------------------------------------------------------
 */

#define ADM_DBG_NOCATS  NULL		/* No categories activated */
#define ADM_DBG_ALLCATS ""		/* All categories activated */

#define ADM_TMG_T	struct { u_long     wall_time;		\
				 struct tms cpu_time; }

#ifdef ADM_DEBUG

#define ADM_DBG(cats, msg)					\
	if ((adm_debug_cats != NULL) &&				\
	    ((*adm_debug_cats == NULL) ||			\
	     (strpbrk(adm_debug_cats, cats) != NULL))) {	\
			adm_fw_debug msg;			\
	}

#define ADM_TMG_START(ru)					\
	{ struct timeval adm_tv_zzz;				\
	  gettimeofday(&adm_tv_zzz, NULL);				\
	  ru.wall_time = (adm_tv_zzz.tv_sec * MICROSEC)		\
		       + adm_tv_zzz.tv_usec;			\
	  times(&ru.cpu_time);					\
	}

#define ADM_TMG_END(ru)						\
	{ struct tms adm_use_zzz;				\
	  struct timeval adm_tv_zzz;				\
	  times(&adm_use_zzz);					\
	  gettimeofday(&adm_tv_zzz, NULL);				\
	  adm_use_zzz.tms_utime  -= ru.cpu_time.tms_utime;	\
	  adm_use_zzz.tms_stime  -= ru.cpu_time.tms_stime;	\
	  adm_use_zzz.tms_cutime -= ru.cpu_time.tms_cutime;	\
	  adm_use_zzz.tms_cstime -= ru.cpu_time.tms_cstime;	\
	  ru.cpu_time = adm_use_zzz;				\
	  ru.wall_time = ((adm_tv_zzz.tv_sec * MICROSEC) +	\
		          adm_tv_zzz.tv_usec)			\
		       - ru.wall_time;				\
	}

#define ADM_USECS_PER_TICK    (1000000 / sysconf(_SC_CLK_TCK))

#define ADM_TMG_USER(ru)      (long)(ru.cpu_time.tms_utime * ADM_USECS_PER_TICK)

#define ADM_TMG_SYSTEM(ru)    (long)(ru.cpu_time.tms_stime * ADM_USECS_PER_TICK)

#define ADM_TMG_ELAPSED(ru)   (long)ru.wall_time

#else

#define ADM_DBG(cats, msg)
#define ADM_TMG_START(ru)
#define ADM_TMG_END(ru)
#define ADM_TMG_USER(ru)
#define ADM_TMG_SYSTEM(ru)

#endif

/*
 *----------------------------------------------------------------------
 * References to general framework routines.
 *----------------------------------------------------------------------
 */

#ifdef __cplusplus
extern "C" {
#endif

extern	void	adm_check_len(char *, u_int, int, boolean_t *, int *);
extern	void	adm_fw_debug(char *, ...);
extern	int	adm_env_init();
extern  void	adm_exit();
extern	int	adm_meth_init();
extern	int	adm_msgs_init();
extern	char   *adm_strtok(char **, char *);
extern	int	adm_make_tok(char **, char *, char *, char *, u_int);

#ifdef __cplusplus
}
#endif

/*
 *----------------------------------------------------------------------
 * Include files from other components.
 *----------------------------------------------------------------------
 */

#include "adm_args_impl.h"
#include "adm_reqID_impl.h"
#include "adm_err_impl.h"
#include "adm_amcl_impl.h"
#include "adm_diag_impl.h"
#include "adm_log_impl.h"
#include "adm_sec_impl.h"
#include "adm_lock_impl.h"
#include "adm_amsl_impl.h"
#include "adm_cache.h"

#endif /* !_adm_fw_impl_h */

