#if !defined(lint) && !defined(NOID)
  static char sccsid[] = "@(#)invoke.cc	1.36 5/5/91 SMI";
#endif

/**************************************************************************
 *  File:	lib/libnetmgt/invoke.cc
 *
 *  Author:	Lynn Monsanto, Sun Microsystems Inc.
 *
 *  SCCSID:     @(#)invoke.cc  1.36  91/05/05
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
 *  Comments:	management invocation routines
 *
 **************************************************************************
 */

/*LINTLIBRARY*/

#include "netmgt/netmgt_hdrs.h"
#include "netmgt/netmgt.h"
#include "libnetmgt/libnetmgt.h"

/* ---------------------------------------------------------------------
 *  netmgt_request_data - request agent performance statistics
 *	returns request timestamp if successful; otherwise NULL
 * ---------------------------------------------------------------------
 */
struct timeval *
netmgt_request_data (char *agent_host, 
		     u_long agent_prog, 
		     u_long agent_vers, 
		     char *rendez_host, 
		     u_long rendez_prog, 
		     u_long rendez_vers, 
		     u_int count, 
		     struct timeval interval, 
		     struct timeval timeout, 
		     u_int flags)
{
  char buf[NETMGT_NAMESIZ];	// error buffer 
  int signedCount;		// signed count value 

  NETMGT_PRN (("invoke: netmgt_request_data\n")) ;

  // clear error buffer 
  _netmgtStatus.clearStatus ();

  // verify input 
  if (!agent_host)
    {
      NETMGT_PRN (("invoke.c: no agent hostname\n"));
      _netmgtStatus.setStatus (NETMGT_NOAGENTHOSTNAME, 0, NULL);
      return (struct timeval *) NULL;
    }
  if (!rendez_host)
    {
      NETMGT_PRN (("invoke.c: no rendezvous hostname\n"));
      _netmgtStatus.setStatus (NETMGT_NORENDEZHOSTNAME, 0, NULL);
      return (struct timeval *) NULL;
    }
  if (timeout.tv_sec < 0 || timeout.tv_usec < 0)
    {
      NETMGT_PRN (("invoke.c: invalid RPC timeout: %d\n", timeout));
      (void) sprintf (buf, "%d seconds, %d microseconds", 
		      timeout.tv_sec, timeout.tv_usec);
      _netmgtStatus.setStatus (NETMGT_BADTIMEOUT, 0, buf);
      return (struct timeval *) NULL;
    }
  if (interval.tv_sec < 0 || interval.tv_usec < 0)
    {
      NETMGT_PRN (("invoke.c: invalid sampling interval: %d\n", interval));
      (void) sprintf (buf, "%d seconds, %d microseconds", 
		      interval.tv_sec, interval.tv_usec);
      _netmgtStatus.setStatus (NETMGT_BADINTERVAL, 0, buf);
      return (struct timeval *) NULL;
    }

  signedCount = (int) count;
  if (signedCount < 0)
    {
      NETMGT_PRN (("invoke: invalid sampling count: %d\n", signedCount));
      (void) sprintf (buf, "%d", signedCount);
      _netmgtStatus.setStatus (NETMGT_BADCOUNT, 0, buf);
      return (struct timeval *) NULL;
    }

  // create and inititialize a manager object if necessary 
  if (!aNetmgtManager)
    {
      aNetmgtManager = (NetmgtManager *) calloc (1, sizeof (NetmgtManager));
      if (!aNetmgtManager)
	{
	  _netmgtStatus.setStatus (NETMGT_MALLOC, 0, NULL);
	  return NULL;
	}
      if (!aNetmgtManager->myConstructor ())
	{
	  (void) cfree ((caddr_t) aNetmgtManager);
	  return FALSE ;
	}
    }

  // send the request to the agent
  return aNetmgtManager->sendGetRequest (NETMGT_DATA_REQUEST,
					 agent_host, 
					 agent_prog, 
					 agent_vers,
					 rendez_host, 
					 rendez_prog, 
					 rendez_vers,
					 count, 
					 interval, 
					 timeout, 
					 flags);
}
  
/* ---------------------------------------------------------------------
 *  netmgt_request_events - enable agent event reporting
 *	returns request timestamp if successful; otherwise NULL
 * ---------------------------------------------------------------------
 */
struct timeval *
netmgt_request_events (char *agent_host, 
		       u_long agent_prog, 
		       u_long agent_vers, 
		       char *rendez_host, 
		       u_long rendez_prog, 
		       u_long rendez_vers,
		       u_int count, 
		       struct timeval interval, 
		       struct timeval timeout, 
		       u_int flags)
{
  char buf[NETMGT_NAMESIZ];	// error buffer 
  int signedCount;		// signed count value 

  NETMGT_PRN (("event: netmgt_request_events\n"));

  // clear error buffer 
  _netmgtStatus.clearStatus ();

  // verify input 
  if (!agent_host)
    {
      NETMGT_PRN (("event.c: no agent hostname\n"));
      _netmgtStatus.setStatus (NETMGT_NOAGENTHOSTNAME, 0, NULL);
      return (struct timeval *) NULL;
    }
  if (!rendez_host)
    {
      NETMGT_PRN (("event.c: no rendezvous hostname\n"));
      _netmgtStatus.setStatus (NETMGT_NORENDEZHOSTNAME, 0, NULL);
      return (struct timeval *) NULL;
    }
  if (timeout.tv_sec < 0 || timeout.tv_usec < 0)
    {
      NETMGT_PRN (("event.c: invalid RPC timeout: %d.%d\n",
		   timeout.tv_sec, timeout.tv_usec));
      (void) sprintf (buf, "%d seconds, %d microseconds", 
		      timeout.tv_sec, timeout.tv_usec);
      _netmgtStatus.setStatus (NETMGT_BADTIMEOUT, 0, buf);
      return (struct timeval *) NULL;
    }
  if (interval.tv_sec < 0 || timeout.tv_usec < 0)
    {
      NETMGT_PRN (("event.c: invalid sampling interval: %d.%d\n",
		   interval.tv_sec, interval.tv_usec));
      (void) sprintf (buf, "%d seconds, %d microseconds", 
		      interval.tv_sec, interval.tv_usec);
      _netmgtStatus.setStatus (NETMGT_BADINTERVAL, 0, buf);
      return (struct timeval *) NULL;
    }

  signedCount = (int) count;
  if (signedCount < 0)
    {
      NETMGT_PRN (("invoke: invalid sampling count: %d\n", signedCount));
      (void) sprintf (buf, "%d", signedCount);
      _netmgtStatus.setStatus (NETMGT_BADCOUNT, 0, buf);
      return (struct timeval *) NULL;
    }

  // create and inititialize a manager object if necessary 
  if (!aNetmgtManager)
    {
      aNetmgtManager = (NetmgtManager *) calloc (1, sizeof (NetmgtManager));
      if (!aNetmgtManager)
	{
	  _netmgtStatus.setStatus (NETMGT_MALLOC, 0, NULL);
	  return NULL;
	}
      if (!aNetmgtManager->myConstructor ())
	{
	  (void) cfree ((caddr_t) aNetmgtManager);
	  return FALSE ;
	}
    }

  // send the request to the agent
  return aNetmgtManager->sendGetRequest (NETMGT_EVENT_REQUEST,
					 agent_host, 
					 agent_prog, 
					 agent_vers,
					 rendez_host, 
					 rendez_prog, 
					 rendez_vers,
					 count, 
					 interval, 
					 timeout, 
					 flags);
}

/* ------------------------------------------------------------------------
 *  netmgt_request_set - C wrapper to request agent to set attribute value
 *	returns TRUE if successful; otherwise returns FALSE
 * ------------------------------------------------------------------------
 */
bool_t
netmgt_request_set (char *agent_host, 
		    u_long agent_prog, 
		    u_long agent_vers, 
		    char *rendez_host,
		    u_long rendez_prog,
		    u_long rendez_vers,
		    struct timeval timeout, 
		    u_int flags)
{
  char buf [NETMGT_ERRORSIZ] ;	// error report buffer

  NETMGT_PRN (("invoke: netmgt_request_set\n"));
  
  // verify input 
  if (!agent_host)
    {
      NETMGT_PRN (("invoke.c: no agent hostname\n"));
      _netmgtStatus.setStatus (NETMGT_NOAGENTHOSTNAME, 0, NULL);
      return FALSE;
    }
  if (!rendez_host)
    {
      NETMGT_PRN (("invoke.c: no rendezvous hostname\n"));
      _netmgtStatus.setStatus (NETMGT_NORENDEZHOSTNAME, 0, NULL);
      return FALSE;
    }
  if (timeout.tv_sec < 0 || timeout.tv_usec < 0)
    {
      NETMGT_PRN (("invoke.c: invalid RPC timeout: %d\n", timeout));
      (void) sprintf (buf, "%d seconds, %d microseconds", 
		      timeout.tv_sec, timeout.tv_usec);
      _netmgtStatus.setStatus (NETMGT_BADTIMEOUT, 0, buf);
      return FALSE;
    }

  // send set request to agent
  return aNetmgtManager->sendSetRequest (agent_host, 
					 agent_prog, 
					 agent_vers,
					 rendez_host,
					 rendez_prog,
					 rendez_vers,
					 timeout, 
					 flags);
}

