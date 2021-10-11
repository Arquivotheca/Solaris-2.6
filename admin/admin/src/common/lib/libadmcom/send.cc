#if !defined(lint) && !defined(NOID)
  static char sccsid[] = "@(#)send.cc	1.45 5/5/91 SMI";
#endif

/**************************************************************************
 *  File:	lib/libnetmgt/send.cc
 *
 *  Author:	Lynn Monsanto, Sun Microsystems Inc.
 *
 *  SCCSID:     @(#)send.cc  1.45  91/05/05
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
 *  Comments:	request and report sending routines
 *
 **************************************************************************
 */

/*LINTLIBRARY*/

#include "netmgt/netmgt_hdrs.h"
#include "netmgt/netmgt.h"
#include "libnetmgt/libnetmgt.h"

// request flag
static u_int _netmgt_requestFlag ;

/* --------------------------------------------------------------------------
 *   NetmgtServiceMsg::sendGetRequest - send data/event request to agent
 *	returns request timestamp if successful; otherwise NULL
 * --------------------------------------------------------------------------
 */
struct timeval *
NetmgtServiceMsg::sendGetRequest (u_int request_type,
				  char *agent_host,	
				  u_long send_agent_prog,	
				  u_long send_agent_vers,
				  char *rendez_host,
				  u_long send_rendez_prog,
				  u_long send_rendez_vers,
				  u_int send_count,
				  struct timeval send_interval,
				  struct timeval send_timeout,
				  u_int send_flags)
     // request type    
     // agent hostname
     // agent RPC program
     // agent RPC version
     // rendez hostname
     // rendez RPC program
     // rendez RPC version
     // report count
     // report interval
     // RPC timeout
     // request flags
{

  NETMGT_PRN (("send: NetmgtServiceMsg::sendGetRequest\n")) ;

  // clear error buffer 
  _netmgtStatus.clearStatus ();

  u_long addr ;			// IP address (for inet_addr) 
  struct hostent *hp ;		// host table entry 
  struct in_addr send_agent_addr ;	// agent IP address 
  struct in_addr send_rendez_addr ;	// rendezvous IP address 

  // get agent IP address 
  addr = inet_addr (agent_host);
  if (addr != -1)
    send_agent_addr.s_addr = addr;
  else
    {
      hp = gethostbyname (agent_host) ;
      if (!hp)
	{
	  NETMGT_PRN (("send: unknown agent: %s\n", agent_host));
	  _netmgtStatus.setStatus (NETMGT_UNKNOWNHOST, 0, agent_host);
	  return (struct timeval *) NULL;
	}
      (void) memcpy ((caddr_t) & send_agent_addr, (caddr_t) hp->h_addr,
		     hp->h_length);
    }
      
  // get rendezvous IP address 
  (void) memset ((caddr_t) & send_rendez_addr, 0, sizeof (struct in_addr));
  if (rendez_host)
    {
      addr = inet_addr (rendez_host);
      if (addr != -1)
	send_rendez_addr.s_addr = addr;
      else
	{
	  hp = gethostbyname (rendez_host);
	  if (!hp)
	    {
	      NETMGT_PRN (("send: unknown rendezvous: %s\n", rendez_host));
	      _netmgtStatus.setStatus (NETMGT_UNKNOWNHOST, 0, rendez_host);
	      return (struct timeval *) NULL;
	    }
	  (void) memcpy ((caddr_t) & send_rendez_addr, (caddr_t) hp->h_addr,
			 hp->h_length);
	}
    }

  // setup (and possibly send) request message to agent
  return this->setupRequest (request_type,
			     send_agent_addr, 
			     send_agent_prog, 
			     send_agent_vers,
			     send_rendez_addr, 
			     send_rendez_prog, 
			     send_rendez_vers,
			     send_count, 
			     send_interval, 
			     send_timeout, 
			     send_flags);
}

/* -----------------------------------------------------------------------
 *   NetmgtServiceMsg::setupRequest - setup (and possibly send) request message 
 *	returns request timestamp if successful; otherwise NULL
 * -----------------------------------------------------------------------
 */
struct timeval *
NetmgtServiceMsg::setupRequest (u_int request_type,
				    struct in_addr setup_agent_addr,
				    u_long setup_agent_prog,
				    u_long setup_agent_vers,
				    struct in_addr setup_rendez_addr,
				    u_long setup_rendez_prog,
				    u_long setup_rendez_vers,
				    u_int setup_count,	
				    struct timeval setup_interval,
				    struct timeval setup_timeout,
				    u_int setup_flags)	
     // request type    
     // agent hostname
     // agent RPC program
     // agent RPC version
     // rendez hostname
     // rendez RPC program
     // rendez RPC version
     // report count
     // report interval
     // RPC timeout
     // request flags
{

  NETMGT_PRN (("send: NetmgtServiceMsg::setupRequest\n"));

  assert (setup_timeout.tv_sec >= (long) 0);
  assert (setup_timeout.tv_usec >= (long) 0);

  // clear error buffer 
  _netmgtStatus.clearStatus ();

  // fill in request message header 
  this->report_time.tv_sec = (long) 0;
  this->report_time.tv_usec = (long) 0;
  this->delta_time.tv_sec = (long) 0;
  this->delta_time.tv_usec = (long) 0;
  this->type = request_type;
  assert (aNetmgtManager != (NetmgtManager *) NULL);
  assert (aNetmgtManager->myRequest != (NetmgtRequest *) NULL);
  this->handle = aNetmgtManager->myRequest->getRequestHandle ();
  this->status = NETMGT_SUCCESS;
  this->flags = setup_flags;
  this->priority = (u_int) 0;

  struct sockaddr_in name ;	// local IP name 
  (void) get_myaddress (&name);

  (void) memcpy ((caddr_t) & this->manager_addr, 
		 (caddr_t) & name.sin_addr.s_addr, 
		 sizeof (struct in_addr));
  (void) memcpy ((caddr_t) & this->agent_addr,
		 (caddr_t) & setup_agent_addr, 
		 sizeof (struct in_addr));
  this->agent_prog = setup_agent_prog;
  this->agent_vers = setup_agent_vers;
  if (setup_rendez_addr.s_addr)
    (void) memcpy ((caddr_t) & this->rendez_addr,
		   (caddr_t) & setup_rendez_addr, 
		   sizeof (struct in_addr));
  this->rendez_prog = setup_rendez_prog;
  this->rendez_vers = setup_rendez_vers;
  this->proto = IPPROTO_UDP;
  this->timeout = setup_timeout;
  this->count = setup_count;
  this->interval = setup_interval;

  // get requst timestamp, system, group and key strings from 
  // manager's request cache
  (void) memcpy ((caddr_t) & this->request_time,
		 (caddr_t) aNetmgtManager->myRequest->getRequestTime (),
		 sizeof (struct timeval));
  (void) strncpy (this->system, 
		  aNetmgtManager->myRequest->getSystem (),
		  sizeof (this->system));
  (void) strncpy (this->group, 
		  aNetmgtManager->myRequest->getGroup (),
		  sizeof (this->group));
  (void) strncpy (this->key, 
		  aNetmgtManager->myRequest->getKey (),
		  sizeof (this->key));

  if (this->myArglist.getOffset () == 0)
    this->length = 0;
  else
    this->length = u_int (this->myArglist.getOffset () 
			  + sizeof (NETMGT_END_TAG)
			  + sizeof (NETMGT_ENDOFARGS) + 1);

  // if doing deferred client calling ... 
  if (_netmgt_requestFlag & NETMGT_DONT_CALL)
    {
      // indicate that we set up the request but didn't send it
      static struct timeval timestamp ;
      timestamp.tv_sec = (long) 0;
      timestamp.tv_usec = (long) 0;
      return &timestamp;
    }
      
  return this->callAgent (setup_agent_prog, setup_agent_addr, 
			  setup_agent_vers, setup_timeout);
}

/* -------------------------------------------------------------------
 *  NetmgtServiceMsg::callAgent - do client call to agent
 *	returns request timestamp if successful; otherwise NULL
 * -------------------------------------------------------------------
 */
struct timeval *
NetmgtServiceMsg::callAgent (u_long call_agent_prog, 
			     struct in_addr call_agent_addr,
			     u_long call_agent_vers,	
			     struct timeval call_timeout)
     // agent RPC program
     // agent IP address
     // agent RPC version
     // RPC timeout
{
  enum clnt_stat clnt_stat;	// clnt_call return status 
  struct rpc_err rpc_err;	// RPC return status 

  NETMGT_PRN (("send: NetmgtServiceMsg::callAgent\n"));

  // clear error buffer 
  _netmgtStatus.clearStatus ();

  // set request authentication flavor
  int flavor = this->myEntity->myRequest->whichAuthFlavor ();

  for (int attempts = 0; attempts < 2; attempts++)
    {
      if (!this->myClient.newClient (call_agent_addr, 
				     (u_long) IPPROTO_UDP,
				     call_agent_prog, 
				     call_agent_vers,
				     flavor, 
				     call_timeout))
	  return (struct timeval *) NULL;

      // send the request message 
      clnt_stat = clnt_call (this->myClient.getHandle (), 
			     NETMGT_SERVICE_PROC, 
			     (xdrproc_t) _netmgt_serialMsg,
			     (caddr_t) this, 
			     (xdrproc_t) _netmgt_xdrStatus,
			     (caddr_t) &netmgt_error, 
			     call_timeout);

      // RPC successful 
      if (clnt_stat == RPC_SUCCESS)
	{
	  this->myClient.destroyClient ();

	  // was request successful ?
	  if (_netmgtStatus.wasSuccess ())
	      return &this->request_time;
	  return (struct timeval *) NULL;
	}
      if (netmgt_debug)
	clnt_perror (this->myClient.getHandle(), "send: clnt_call");

      // resend the request using DES authentication if
      // we got a "too weak" authentication error and the
      // requestor didn't specify an authentication flavor
      CLNT_GETERR (this->myClient.getHandle (), &rpc_err);
      if (!aNetmgtManager->myRequest->flavorRequested () &&
	  flavor == AUTH_NONE && 
	  rpc_err.re_status == RPC_AUTHERROR &&
	  rpc_err.re_why == AUTH_TOOWEAK)
	{
	  flavor = AUTH_DES;
	  continue;
	}

      // RPC failure 
      if (rpc_err.re_status == RPC_TIMEDOUT)
	_netmgtStatus.setStatus (NETMGT_RPCTIMEDOUT, 
				 0, 
				 dgettext (NETMGT_TEXT_DOMAIN,
					   "Can't send request"));
      else
	_netmgtStatus.setStatus (NETMGT_RPCFAILED, 
				 0, 
				 clnt_sperror(this->myClient.getHandle (),
					      dgettext (NETMGT_TEXT_DOMAIN,
							"Can't send request")));
      this->myClient.destroyClient ();
      return (struct timeval *) NULL;
    }
  /*NOTREACHED*/
}

/* ------------------------------------------------------------------------
 *  netmgt_send_report - C wrapper for sending reports
 *	returns TRUE if successful; otherwise returns FALSE
 * ------------------------------------------------------------------------
 */
bool_t
netmgt_send_report (struct timeval delta_time, 
		    Netmgt_stat status, 
		    u_int flags)
				// report delta time
				// report status code
				// report flags
{

  NetmgtRequestInfo requestInfo ;   // request information buffer

  // get request information from agent
  assert (aNetmgtPerformer != (NetmgtPerformer *) NULL);
  aNetmgtPerformer->getRequestInfo (&requestInfo);

  switch (requestInfo.type)
    {
    case NETMGT_ACTION_REQUEST:
      return aNetmgtPerformer->myServiceMsg->sendActionReport (aNetmgtPerformer, 
							       delta_time, 
							       status, 
							       flags);
      
    case NETMGT_DATA_REQUEST:
      return aNetmgtPerformer->myServiceMsg->sendDataReport (aNetmgtPerformer, 
							     delta_time, 
							     status, 
							     flags);
      
    case NETMGT_EVENT_REQUEST:
      return aNetmgtPerformer->myServiceMsg->sendEventReport (aNetmgtPerformer, 
							      delta_time, 
							      status, 
							      flags);
      
    case NETMGT_TRAP_REQUEST:
      return aNetmgtPerformer->myServiceMsg->sendTrapReport (aNetmgtPerformer, 
							     delta_time, 
							     status, 
							     flags);
      
    default:
      _netmgtStatus.setStatus (NETMGT_UNKNOWNREQUEST, (u_int) 0, NULL);
    }
  /*NOTREACHED*/
}

/* ---------------------------------------------------------------------
 *  _netmgt_set_request_flag - set manager request flag
 *	returns TRUE if successful; otherwise returns FALSE
 * ---------------------------------------------------------------------
 */
bool_t
_netmgt_set_request_flag (u_int flag)
                 		// request flag 
{
  if (flag == (u_int) 0)
    _netmgt_requestFlag = 0;
  else
    _netmgt_requestFlag |= flag ;
  return TRUE ;
}

/* --------------------------------------------------------------------
 *  _netmgt_get_request_time - get request timestamp
 *	return timeval pointer
 * --------------------------------------------------------------------
 */
struct timeval *
_netmgt_get_request_time (void) 
{
  assert (aNetmgtManager != (NetmgtManager *) NULL);
  return aNetmgtManager->myRequest->getRequestTime ();
}

