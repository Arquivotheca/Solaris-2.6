
/*
 *  Copyright (c) 1990 Sun Microsystems, Inc.  All Rights Reserved.
 */

/*
 *******************************************************************************
 *
 *	This file contains the exported definitions for the adm_perf_method()
 *	routine.  The file contains definitions for:
 *
 *		o Framework control option types.
 *		o Default framework control values.
 *		o Framework control flags.
 *		o Exit status/cleanliness codes for methods.
 *		o Miscellaneous constants.
 *		o Exported interface definitions.
 *
 *******************************************************************************
 */

#ifndef _adm_amcl_h
#define _adm_amcl_h

#pragma	ident	"@(#)adm_amcl.h	1.16	95/05/04 SMI"

#include <stddef.h>
#include <sys/types.h>

/*
 *----------------------------------------------------------------------
 * Framework control options.
 *----------------------------------------------------------------------
 */

#define ADM_ENDOPTS	   0	/* End of framework control options */
#define ADM_CLASS	   1	/* Name of class for the invoked method */
#define ADM_CLASS_VERS	   2	/* Class version number */
#define ADM_HOST	   3	/* Host on which to perform method */
#define ADM_DOMAIN	   4	/* Domain in which to perform request */
#define ADM_ACK_TIMEOUT	   5	/* Timeout for waiting for initial request ack */
#define ADM_REP_TIMEOUT	   6	/* Timeout for waiting for agent report */
#define ADM_AGENT	   7	/* Class agent program and version # */
#define ADM_CATEGORIES	   8	/* Additional categories for tracing message */
#define ADM_PINGS	   9	/* # of ping retries before giving up on agent */
#define ADM_PING_TIMEOUT  10	/* Timeout to wait for ping acknowledgement */
#define ADM_PING_DELAY	  11	/* Delay before beginning to ping request */
#define ADM_AUTH_TYPE	  12	/* Request authentication type */
#define ADM_AUTH_FLAVOR	  13	/* Request authentication flavor */
#define ADM_CLIENT_GROUP  14	/* Client's preferred group */
#define ADM_ALLOW_AUTH_NEGO 15  /* Allow authentication negotiation? */
#define ADM_LOCAL_DISPATCH  16  /* Invoke method locally w/o using RPC? */

/*
 *----------------------------------------------------------------------
 * Default framework control values.
 *----------------------------------------------------------------------
 */

#define ADM_ACK_TSECS	(long)  30    /* Seconds to wait for request ack */
#define ADM_ACK_TUSECS	(long)   0    /* Micro-secs to wait for request ack */
#define ADM_REP_TSECS	(long)   0    /* Wait forever for a requested method */
#define ADM_REP_TUSECS	(long)   0    /* to complete */
#define ADM_PING_TSECS	(long)  20    /* Seconds to wait for ping ack */
#define ADM_PING_TUSECS	(long)   0    /* Micro-secs to wait for ping ack */
#define ADM_PING_CNT	(u_int)  2    /* # of ping retries before giving up */
#define ADM_DELAY_TSECS (long)  30    /* Secs. and micro-secs to delay */
#define ADM_DELAY_TUSECS (long)  0    /* before attempting to ping a request */
#define ADM_AMSL_DEBUG		 0    /* AMSL debug level (test mode only) */
#define ADM_AMSL_PID	   B_FALSE    /* Should AMSL print method PID? */

#define ADM_RETRY_FLAVOR	AUTH_UNIX	/* Auth flavor for retry when */
						/* RPC fails to generate cred */
#define ADM_DEFAULT_AUTH_TYPE	ADM_AUTH_UNSPECIFIED /* Default authentication */
#define ADM_DEFAULT_AUTH_FLAVOR	AUTH_UNIX	     /* type and flavor */
#define ADM_DEFAULT_NEGO	B_TRUE		/* Default negotiation flag */

#define ADM_DEFAULT_FLAGS	(long) 0	/* Default FW control flags */

/*
 *--------------------------------------------------------------------
 * Framework control flags.
 *--------------------------------------------------------------------
 */

#define ADM_LOCAL_REQUEST_FLAG (u_long) 0x01	/* Local dispatch mode? */

/*
 *--------------------------------------------------------------------
 * Administrative method exit status codes/cleanliness codes.
 *--------------------------------------------------------------------
 */

#define ADM_FAILCLEAN   (u_int) 1       /* Method failure - no data altered */
#define ADM_FAILDIRTY   (u_int) 2       /* Method failure - data may be altered */

/*
 *--------------------------------------------------------------------
 * Miscellaneous constants.
 *--------------------------------------------------------------------
 */

#define ADM_LOCALHOST	    NULL	/* Local host identifier */
#define ADM_DEFAULT_VERS    "2.1"	/* 2.1 is initial Solstice version */
#define ADM_NOCATS	    NULL	/* No extra tracing categories specified */

/*
 * Multiplicative factor (in hundredths) for determining the maximum
 * delay between ping attempts during a method invocation.  Maximum delay
 * is the initial delay multiplied by this factor.
 */

#define ADM_MAX_PING_DELAY_FACTOR 1000	

/*
 *--------------------------------------------------------------------
 * Exported interface definitions.
 *--------------------------------------------------------------------
 */

#ifdef __cplusplus
extern "C" int	adm_perf_method(char *, Adm_data *, Adm_data **,
				Adm_error **, ...);
#else
extern	int	adm_perf_method(char *, Adm_data *, Adm_data **,
				Adm_error **, ...);
#endif

#endif /* !_adm_amcl_h */

