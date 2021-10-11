
/*
 *  Copyright (c) 1990 Sun Microsystems, Inc.  All Rights Reserved.
 */

/*
 *******************************************************************************
 *
 *	This file contains the definitions of global data used by the
 *	administrative framework.  It contains the definitions of:
 *
 *	    General Global Variables: (Info about current process/method)
 *
 *		adm_inited	 Has process been initialized?
 *
 *		adm_amsl_pid	 Boolean flag used to indicate if the AMSL
 *				 should print out method PID information
 *				 when invoked from test mode:
 *				   B_TRUE  => print PID information.
 *				   B_FALSE => do not print PID information.
 *		adm_auth_init_type   Default authentication type to use
 *				     on initial method request attempt.
 *		adm_auth_init_flavor Default authentication flavor to use
 *				     on initial method request attempt.
 *		adm_class	 Class name of method being run.
 *		adm_client_id	 Client identity from authentication.
 *		adm_debug_cats	 Active framework debugging categories.
 *				 NULL ptr. indicates that no categories are
 *				 active.  Ptr. to a null string indicates
 *				 that all categories are active.
 *		adm_domain	 Domain of method request.
 *		adm_flags	 Framework control flags.
 *		adm_host	 Name of host executing method.
 *		adm_method	 Name of method being run.
 *		adm_pid		 PID of running process.
 *		adm_ping_cnt	 Number of times to ping an agent before
 *				 assuming that it has crahsed.
 *		adm_ping_delay	 Delay at start of request before
 *				 beginning pinging activities.
 *		adm_ping_timeout Timeout to wait for an acknowledgement to
 *				 a ping request.
 *		adm_reqID	 ID of method request.
 *		adm_rep_timeout  Maximum time-to-live (in seconds) for a
 *				 request.
 *		adm_stdcats	 Standard tracing categories for all trace
 *				 messages associated with this invocation.
 *		adm_stdfmt	 Standard formatted output FILE pointer.
 *
 *	    Argument Handling Global Variables:
 *
 *		adm_input_args	 Pointer to administrative data structure
 *				 containing input arguments to currently
 *				 executing method.
 *
 *	    Request ID Handling Global Variables:
 *
 *		adm_nextIDcnt	 Counter to disambiguate request IDs.
 *
 *	    Internationalization Supporting Global Variables:
 *
 *		adm_lpath	 Default pathname for method message files.
 *		adm_msgs_inited	 Have framework/object manager message
 *				 localization files been initialized?
 *		adm_text_domains Internationalization text domains used
 *				 by the requested class method.
 *
 *******************************************************************************
 */

#ifndef _adm_fw_glob_c
#define _adm_fw_glob_c

#pragma	ident	"@(#)adm_fw_glob.c	1.12	92/02/28 SMI"

#include <netdb.h>
#include <sys/types.h>
#include "adm_fw.h"
#include "adm_fw_impl.h"

/*
 * General Globals
 */

boolean_t	adm_inited=B_FALSE; /* Has process (or method) been initialized? */
boolean_t	adm_amsl_pid=ADM_AMSL_PID;     /* Should AMSL print PID info? */
char 		*adm_class;	  /* Requested method class name (if method) */
char		*adm_client_id;	  /* Client identity from authentication */
char		*adm_debug_cats=NULL; /* Active framework debugging categories */
char		*adm_domain;	  /* Domain of method request (if method) */
u_long		adm_flags;	  /* Framework control flags */
char		adm_host[MAXHOSTNAMELEN+1]; /* Name of current host */
char		*adm_method;	  /* Requested method name (if method) */
pid_t		adm_pid;	  /* PID of running process */
u_int		adm_ping_cnt;	  /* # of pings before assuming server crash */
long		adm_ping_delay;   /* Seconds to delay before starting pinging */
long		adm_ping_timeout; /* Timeout to wait for ping ack */
Adm_requestID	adm_reqID;	  /* Request ID (if method) */
long		adm_rep_timeout;  /* Max. time-to-live for a request */
char		*adm_stdcats;	  /* Standard tracing categories */
FILE		*adm_stdfmt;	  /* FILE ptr for formatted output (if method) */

/*
 * Default authentication type and flavor to use on initial method 
 * request attempt.
 */

u_int		adm_auth_init_type = ADM_DEFAULT_AUTH_TYPE;
u_int		adm_auth_init_flavor = ADM_DEFAULT_AUTH_FLAVOR;

/*
 * Argument Handling Globals
 */

Adm_data *adm_input_args;	/* Input arguments to method. */

/*
 * Request ID Handling Globals
 */

u_long	adm_nextIDcnt;	/* Count to place in next generated req ID */

/*
 * Internationalization Supporting Globals
 */

char		*adm_lpath;	  	 /* Default method message file path */
boolean_t	adm_msgs_inited=B_FALSE; /* Message files initialized? */
char		*adm_text_domains;	 /* Class/method i18n domains */

#endif /* !_adm_fw_glob_c */

