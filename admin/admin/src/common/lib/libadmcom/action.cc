#if !defined(lint) && !defined(NOID)
  static char sccsid[] = "@(#)action.cc	1.28 5/5/91 SMI";
#endif

/**************************************************************************
 *  File:	lib/libnetmgt/action.cc
 *
 *  Author:	Lynn Monsanto, Sun Microsystems Inc.
 *
 *  SCCSID:     @(#)action.cc  1.28  91/05/05
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
 *  Comments:	create, delete, action request sending function
 *
 **************************************************************************
 */

/*LINTLIBRARY*/

#include "netmgt/netmgt_hdrs.h"
#include "netmgt/netmgt.h"
#include "libnetmgt/libnetmgt.h"
#if !defined(lint) && !defined(NOID)
#include "patchlevel.h"
#endif

/* ------------------------------------------------------------------------
 *   _netmgt_request_action - C wrapper to send action request message
 *	returns request timestamp if successful; otherwise NULL
 * ------------------------------------------------------------------------
 */
struct timeval *
_netmgt_request_action (char *agent_host,
			u_long agent_prog,
			u_long agent_vers,	
			char *rendez_host,
			u_long rendez_prog,
			u_long rendez_vers,	
			struct timeval timeout,
			u_int flags,
			struct in_addr *agent_addr)	
     // agent name
     // agent RPC program
     // agent RPC version
     // rendezvous name
     // rendezvous RPC program
     // rendezvous RPC version
     // RPC timeout
     // request flags
     // location at which to return agent address
{

  NETMGT_PRN (("action: _netmgt_request_action\n"));
  
  assert (aNetmgtManager != (NetmgtManager *) NULL);
  return aNetmgtManager->myServiceMsg->sendActionRequest (agent_host,
							  agent_prog,
							  agent_vers,	
							  rendez_host,
							  rendez_prog,
							  rendez_vers,	
							  timeout,
							  flags,
							  agent_addr);
}


/* ------------------------------------------------------------------------
 *   NetmgtServiceMsg::sendActionRequest - send action request message
 *	returns request timestamp if successful; otherwise NULL
 * ------------------------------------------------------------------------
 */
struct timeval *
NetmgtServiceMsg::sendActionRequest (char  *action_agent_host,
				     u_long action_agent_prog,
				     u_long action_agent_vers,	
				     char  *action_rendez_host,
				     u_long action_rendez_prog,
				     u_long action_rendez_vers,	
				     struct timeval action_timeout,
				     u_int action_flags,
				     struct in_addr *action_agent_addr)	
     // agent IP address
     // agent RPC program
     // agent RPC version
     // rendezvous IP address
     // rendezvous RPC program
     // rendezvous RPC version
     // RPC timeout
     // request flags
     // location at which to return agent address
{

  NETMGT_PRN (("action: NetmgtServiceMsg::sendActionRequest\n"));

  // clear error buffer 
  _netmgtStatus.clearStatus ();

  u_long addr ;                        // IP address (for inet_addr) 
  struct hostent *hp ;                 // host table entry 
  struct in_addr action_rendez_addr ;  // rendezvous IP address 

  // get agent IP address 
  addr = inet_addr (action_agent_host);
  if (addr != -1)
    action_agent_addr->s_addr = addr;
  else
    {
      hp = gethostbyname (action_agent_host) ;
      if (!hp)
        {
          NETMGT_PRN (("send: unknown agent: %s\n", action_agent_host));
          _netmgtStatus.setStatus (NETMGT_UNKNOWNHOST, 0, action_agent_host);
          return (struct timeval *)NULL;
        }
      (void) memcpy ((caddr_t) action_agent_addr, (caddr_t) hp->h_addr,
                     hp->h_length);
    }
      
  // get rendezvous IP address 
  (void) memset ((caddr_t) & action_rendez_addr, 0, sizeof (struct in_addr));
  if (action_rendez_host)
    {
      addr = inet_addr (action_rendez_host);
      if (addr != -1)
        action_rendez_addr.s_addr = addr;
      else
        {
          hp = gethostbyname (action_rendez_host);
          if (!hp)
            {
              NETMGT_PRN (("send: unknown rendezvous: %s\n", action_rendez_host));
              _netmgtStatus.setStatus (NETMGT_UNKNOWNHOST, 0, action_rendez_host);
              return (struct timeval *)NULL;
            }
          (void) memcpy ((caddr_t) & action_rendez_addr, (caddr_t) hp->h_addr,
                         hp->h_length);
        }
    }

  // setup (and possibly send) request message to agent
  struct timeval action_interval;
  action_interval.tv_sec = 0L;
  action_interval.tv_usec = 0L;
  return this->setupRequest (NETMGT_ACTION_REQUEST,
			     *action_agent_addr, 
			     action_agent_prog, 
			     action_agent_vers,
			     action_rendez_addr, 
			     action_rendez_prog, 
			     action_rendez_vers,
			     0, 
			     action_interval, 
			     action_timeout, 
			     action_flags);
}

/* -------------------------------------------------------------------
 *  NetmgtServiceMsg::sendActionReport - send data report message to rendezvous
 *      returns TRUE if successful; otherwise returns FALSE
 * -------------------------------------------------------------------
 */
bool_t
NetmgtServiceMsg::sendActionReport (NetmgtPerformer *aPerformer,
				  struct timeval action_delta_time, 
				  Netmgt_stat action_status, 
				  u_int action_flags)
     // agent performer pointer
     // report delta time
     // report status
     // report flags
{
  NETMGT_PRN (("response: NetmgtServiceMsg::sendActionReport\n")) ;

  // reset internal error code 
  _netmgtStatus.clearStatus ();

  // get request information from agent
  NetmgtRequestInfo requestInfo ;   	// request information buffer
  aPerformer->getRequestInfo (&requestInfo);

  // fill out report message header from request information
  this->request2report (&requestInfo) ;

  // fill out the rest of the header
  if (gettimeofday (&this->report_time, (struct timezone *) NULL ) == -1)
    {
      if (netmgt_debug)
	perror ("response: gettimeofday");
      _netmgtStatus.setStatus (NETMGT_GETTIMEOFDAY, 0, strerror (errno));
      return FALSE;
    }
  this->delta_time = action_delta_time;
  this->type = NETMGT_ACTION_REPORT;
  this->status = action_status;
  this->flags = action_flags;
  this->agent_addr = aPerformer->getLocalAddr ();
  this->agent_prog = aPerformer->getProgram ();
  this->agent_vers = aPerformer->getVersion ();
  if (this->myArglist.getOffset () == 0)
    this->length = 0;
  else
    this->length = u_int (this->myArglist.getOffset () 
			  + sizeof (NETMGT_END_TAG)
			  + sizeof (NETMGT_ENDOFARGS) + 1);

  // send the report
  return this->sendReport (aPerformer, &requestInfo);
}
