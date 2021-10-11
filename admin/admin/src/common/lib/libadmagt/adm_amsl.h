/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 */

#ifndef	_adm_amsl_h
#define	_adm_amsl_h

#pragma	ident	"@(#)adm_amsl.h	1.16	95/05/05 SMI"

/*
 * FILE:  adm_amsl.h
 *
 *	Admin Framework class agent header file exporting definitions
 *	needed to interact with the class agent and AMSL.
 */

#include <sys/types.h>

/* Admin class agent define constants */
#define	ADM_CLASS_AGENT_NAME	"sadmind"	/* Class agent name */
#define	ADM_CLASS_AGENT_PROG	100232		/* Agent RPC number */
#define	ADM_CLASS_AGENT_VERS	(u_long)10	/* Agent RPC version */
#define	ADM_CLASS_AGENT_SNUM	1L		/* Agent serial number */

/* Admin class agent request types */
#define	ADM_PERFORM_REQUEST	(u_int)6	/* Perform method */

/* Admin class agent security levels */
#define	ADM_MAXIMUM_SECURITY_LEVEL	2	/* Maximum security level */
#define	ADM_DEFAULT_SECURITY_LEVEL	1	/* Default security level */

/* Admin class agent default umask for method processes */
#define	ADM_DEFAULT_UMASK	022		/* Default umask */

/* Admin class agent default log file name */
#define	ADM_LOG_FILENAME	"/var/adm/admin.log"

/* Admin class agent default Open Windows pathname */
#define	ADM_OW_PATHNAME		"/usr/openwin"

/* class agent miscellaneous definitions */
#define	ADM_AMSL_TIMEOUT	30L	/* default RPC timeout (seconds) */
					/* for class agent verify */
#define ADM_AMSL_IDLETIME       900     /* default agent idle time (seconds) */
#define ADM_AMSL_MAXIDLETIME    10000000        /* maximum default agent idle 
					time (seconds) */

#endif /* !_adm_amsl_h */
