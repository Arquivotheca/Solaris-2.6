
/*
 *  Copyright (c) 1990 Sun Microsystems, Inc.  All Rights Reserved.
 */

/*
 ****************************************************************************
 *
 *	This file contains the private definitions for use within the "apm"
 *	command.  This command is the administrative framework command line
 *	interface to performing administrative methods.  This file contains
 *	definitions for:
 *
 *		o Command Line Options
 *
 ****************************************************************************
 */

#ifndef _apm_impl_h
#define _apm_impl_h

#pragma	ident	"@(#)apm_impl.h	1.12	92/01/28 SMI"

/*
 * Command Line Options and Mnemonics.
 */

#define ASH_ARGUMENTS_O		"-a"		/* Method arguments */
#define ASH_ARGUMENTS_L		"-arguments"
#define ASH_CLASS_O		"-c"		/* Class */
#define ASH_CLASS_L		"-class"
#define ASH_DOMAIN_O		"-d"		/* Domain */
#define ASH_DOMAIN_L		"-domain"
#define ASH_AUTHFLAVOR_O	"-f"		/* Authentication flavor */
#define ASH_AUTHFLAVOR_L	"-auth_flavor"
#define ASH_CLIENTGROUP_O	"-g"		/* Client's preferred group */
#define ASH_CLIENTGROUP_L	"-client_group"
#define ASH_HOST_O		"-h"		/* Host */
#define ASH_HOST_L		"-host"
#define ASH_PING_DELAY_O	"-i"		/* Initial delay before ping */
#define ASH_PING_DELAY_L	"-ping_delay"
#define ASH_PING_CNT_O		"-k"		/* # of retries for a ping */
#define ASH_PING_CNT_L		"-ping_cnt"
#define ASH_LOCAL_O		"-l"		/* Local method dispatch? */
#define ASH_LOCAL_L		"-local_dispatch"
#define ASH_METHOD_O		"-m"		/* Method */
#define ASH_METHOD_L		"-method"
#define ASH_NONEGO_O		"-n"		/* Do not allow auth nego */
#define ASH_NONEGO_L		"-no_nego"
#define ASH_PERMITNEGO_O	"-p"		/* Permit auth nego */
#define ASH_PERMITNEGO_L	"-permit_nego"
#define ASH_AGENT_O		"-r"		/* Method server RPC # */
#define ASH_AGENT_L		"-agent"
#define ASH_AUTHTYPE_O		"-t"		/* Authentication type */
#define ASH_AUTHTYPE_L		"-auth_type"
#define ASH_UNFMT_O		"-u"		/* Unformatted input to method */
#define ASH_UNFMT_L		"-unfmt"
#define ASH_PING_TIMEOUT_O	"-w"		/* Timeout for a ping ack */
#define ASH_PING_TIMEOUT_L	"-ping_timeout"
#define ASH_ACK_TIMEOUT_O	"-x"		/* Initial request timeout */
#define ASH_ACK_TIMEOUT_L	"-ack_timeout"
#define ASH_REP_TIMEOUT_O	"-y"		/* Method results timeout */
#define ASH_REP_TIMEOUT_L	"-rep_timeout"
#define ASH_DEBUG_O		"-D"		/* AMSL debug level (test mode) */
#define ASH_DEBUG_L		"-DEBUG"

#endif /* !_apm_impl_h */

