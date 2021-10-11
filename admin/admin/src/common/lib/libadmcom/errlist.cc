#if !defined(lint) && !defined(NOID)
  static char sccsid[] = "@(#)errlist.cc	1.36 5/5/91 SMI";
#endif

/**************************************************************************
 *  File:	lib/libnetmgt/errlist.cc
 *
 *  Author:	Lynn Monsanto, Sun Microsystems Inc.
 *
 *  SCCSID:     @(#)errlist.cc  1.36  91/05/05
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
 *  Comments:	RPC agent error strings
 *
 **************************************************************************
 */

/*LINTLIBRARY*/

#include "netmgt/netmgt_hdrs.h"
#include "netmgt/netmgt.h"
#include "libnetmgt/libnetmgt.h"

static char *_netmgt_errlist[] = {
  "Success",			        // 0 

  // agent-defined errors 
  "Non-fatal error",			// 1 
  "Fatal error",			// 2 

  // RPC failures 
  "Remote procedure call failed",       // 3 
  "Remote procedure call timed out",    // 4 

  // RPC send request failures 
  "No manager host name",		// 5 
  "No agent host name",		        // 6  
  "No rendezvous host name",		// 7 
  "Unknown host name",		        // 8 
  "Unknown host address",		// 9 
  "Unknown transport protocol",	        // 10 
  "Can't create RPC client",		// 11 
  "No callback dispatch function",	// 12 
  "NULL callback socket pointer",	// 13 

  // message argument-handling failures 
  "Invalid reporting interval",         // 14 
  "Invalid reporting count",	        // 15 
  "Invalid RPC timeout",		// 16 
  "Invalid request timestamp",		// 17 
  "No system name",			// 18 
  "Invalid system name",		// 19 
  "No object group",			// 20 
  "Invalid object group",		// 21 
  "Invalid table key",			// 22 
  "No argument name",			// 23 
  "Unknown argument name",		// 24 
  "Argument name is too big",	        // 25 
  "No argument type",			// 26 
  "Invalid argument type",		// 27 
  "No argument length",			// 28 
  "No argument value",			// 29 
  "No relational operator",		// 30 
  "Invalid relational operator",	// 31 
  "No threshold value",			// 32 
  "No argument buffer",			// 33 
  "No threshold buffer",		// 34 
  "No performance buffer",		// 35 
  "No event report buffer",		// 36 
  "No error-report buffer",		// 37 
  "No message information buffer",	// 38 

  // message handling failures 
  "Invalid message type",	        // 39 
  "Can't encode message header",	// 40 
  "Can't decode message header",	// 41 
  "Can't encode argument list",	        // 42 
  "Can't decode argument list",	        // 43 
  "Message is too large",	        // 44 

  // RPC security failures 
  "Can't get user netname",	        // 45 
  "Can't get DES authentication handle", // 46 
  "Directory services not running",	// 47 
  "Security credentials too weak",	// 48 
  "Bad security credentials",		// 49 

  // confirmation errors 
  "Confirmation message too long",	// 50 

  // agent initialization failures 
  "No agent name",			// 51 
  "Invalid agent security level",	// 52 
  "Can't create UDP transport",	        // 53 
  "Can't create TCP transport",	        // 54 
  "Can't unregister RPC service",	// 55 
  "Can't register RPC service",	        // 56 

  // request dispatch failures 
  "Unknown request type",		// 57 
  "Can't cache request",                // 58 
  "Can't create request log file",      // 59 

  // rendezvous failures 
  "Can't register as an event rendezvous",   // 60 
  "Can't unregister as an event rendezvous", // 61 

  // activity control failures 
  "Can't find request to terminate",	// 62 
  "Can't terminate request",	        // 63 
  "Can't delete request",		// 64 
  "Can't insert activity in cache",	// 65 
  "Can't delete activity from cache",	// 66 
  "Can't find activity in cache",	// 67 

  // system failures 
  "Can't fork subprocess",		// 68 
  "Can't allocate memory",		// 69 
  "Can't get socket",		        // 70 
  "Can't bind socket",		        // 71 
  "Can't get socket name",	        // 72 
  "Can't get local host name",          // 73 
  "Can't read kernel memory",		// 74 
  "Can't get time of day",		// 76 

  // internal errors 
  "Internal error: NULL queue pointer",		     // 76 
  "Internal error: NULL queue node pointer",	     // 77 
  "Internal error: NULL queue node data pointer",    // 78 
  "Internal error: Would exceed queue length limit", // 79 
  "Internal error: Empty queue", 		     // 80 

  // additional Release 1.1 error messages
  "No manager - only a manager can call this function",        // 81
  "No agent - only an agent can call this function",           // 82
  "No rendezvous - only a rendezvous can call this function",  // 83
  "invalid RPC authentication flavor",			       // 84

  "Unknown argument type"		// 85
  
} ;

/* ---------------------------------------------------------------------
 *  netmgt_sperror - get error message
 * 	returns pointer to error string if successful; 
 *	otherwise returns (char *) NULL
 * ---------------------------------------------------------------------
 */
char *
netmgt_sperror (void)
{
  static char errorString [NETMGT_ERRORSIZ] ;  // error string buffer 

  // verify error code 
  if ((int) netmgt_error.service_error >= (int) NETMGT_NERRS)
    return dgettext (NETMGT_TEXT_DOMAIN, 
		     "unknown communication service error");

  // copy error code description string 
  (void) strncpy (errorString, 
		  dgettext (NETMGT_TEXT_DOMAIN, 
			    _netmgt_errlist [(int) netmgt_error.service_error]),
		  NETMGT_ERRORSIZ);

  // copy optional error message string 
  if (netmgt_error.message) 
    {
      (void) strcat (errorString, ": ");
      (void) strcat (errorString, dgettext (NETMGT_TEXT_DOMAIN, 
					    netmgt_error.message));
    }

  return errorString;
}
      

  
