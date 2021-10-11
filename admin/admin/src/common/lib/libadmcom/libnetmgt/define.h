
/**************************************************************************
 *  File:	include/libnetmgt/define.h
 *
 *  Author:	Lynn Monsanto, Sun Microsystems Inc.
 *
 *  Copyright (c) 1989 Sun Microsystems, Inc.  All Rights Reserved.
 *  Sun considers its source code as an unpublished, proprietary trade 
 *  secret, and it is available only under strict license provisions.  
 *  This copyright notice is placed here only to protect Sun in the event
 *  the source is deemed a published work.  Dissassembly, decompilation, 
 *  or other means of reducing the object code to human readable form is 
 *  prohibited by the license agreement under which this code is provided
 *  to the user or company in possession of this copy.
 * 
 *  RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the 
 *  Government is subject to restrictions as set forth in subparagraph 
 *  (c)(1)(ii) of the Rights in Technical Data and Computer Software 
 *  clause at DFARS 52.227-7013 and in similar clauses in the FAR and 
 *  NASA FAR Supplement.
 *
 *  SCCSID:	@(#)define.h  1.41  91/05/15
 *
 *  Comments:	package private definitions
 *
 **************************************************************************
 */

#ifndef	_define_h
#define _define_h

// define I18N text domain if the makefile didn't
#ifndef NETMGT_TEXT_DOMAIN
#define NETMGT_TEXT_DOMAIN "SYS_admcom"
#endif

// internal debugging macros 
#ifdef DEBUG
#define NETMGT_PRN(x)		if (netmgt_debug > 3) (void) printf x
#else DEBUG
#define NETMGT_PRN(x)
#endif DEBUG

// do deferred client call to agent 
#define NETMGT_DONT_CALL	(1 << 3) 
 
// special name for local host loopback IP address hostname
#define	NETMGT_LOCALHOST	"localhost"

// RPC procedure numbers 
#define NETMGT_PING_PROC	(u_long)0
#define NETMGT_SERVICE_PROC	(u_long)1
#define NETMGT_AGENT_ID_PROC	(u_long)3
#define NETMGT_CONTROL_PROC	(u_long)4
#define NETMGT_GET_ID_PROC	(u_long)5

// start transient RPC range 
#define NETMGT_TRANSIENT 	0x40000000 

// default DES authentication credential lifetime 
#define NETMGT_AUTH_LIFETIME 	60L

// internal request argument names 
#define NETMGT_RENDEZ_ADDR	"netmgt_rendez_addr"
#define NETMGT_RENDEZ_PROG	"netmgt_rendez_prog"
#define NETMGT_RENDEZ_VERS	"netmgt_rendez_vers"
#define NETMGT_AGENT_PROG	"netmgt_agent_prog"
#define NETMGT_PRIORITY		"netmgt_priority"

#define NETMGT_ERROR_CODE	"netmgt_error_code"
#define NETMGT_ERROR_INDEX	"netmgt_error_index"
#define NETMGT_ERROR_MESSAGE	"netmgt_error_message"

// administration file names 
#define NETMGT_CONFIG_FILE	"/etc/snm.conf"
#define NETMGT_ACTIVITY_LOG     "activity-log"
#define NETMGT_EVENT_LOG        "event-log"
#define NETMGT_MONITOR_LOG      "monitor-log"
#define NETMGT_REQUEST_LOG      "request-log"
#define NETMGT_DEFAULT_ACTIVITY_LOG	"/var/adm/snm/activity.log"
#define NETMGT_DEFAULT_EVENT_LOG	"/var/adm/snm/event.log"
#define NETMGT_DEFAULT_MONITOR_LOG	"/var/adm/snm/monitor.log"
#define NETMGT_DEFAULT_REQUEST_LOG	"/var/adm/snm/request.log"

#define NETMGT_MINARGSIZ	256	// minimum arglist size 
#define NETMGT_MAXARGSIZ	6144	// maximum UDP arglist size 
#define NETMGT_LONG_MAXARGSIZ	65536	// maximum string & octet arg size

// max request and report queue lengths 
#define NETMGT_MAXREQUESTS	32	// max request queue length 
#define NETMGT_MAXREPORTS	32	// max report queue length 
#define NETMGT_MAXRESEND 	2	// max resend attempts 
#define NETMGT_VERIFY_TIME	1800	// verification interval 
#define NETMGT_UNVERIFIED_TIME  5400    // agent will terminate request if
					// if hasn't been verified during
					// this interval 
#endif _define_h 
