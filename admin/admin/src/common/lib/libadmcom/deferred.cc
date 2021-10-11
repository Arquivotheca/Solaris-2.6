#if !defined(lint) && !defined(NOID)
  static char sccsid[] = "@(#)deferred.cc	1.39 5/5/91 SMI";
#endif

/**************************************************************************
 *  File:	lib/libnetmgt/deferred.cc
 *
 *  Author:	Lynn Monsanto, Sun Microsystems Inc.
 *
 *  SCCSID:     @(#)deferred.cc  1.39  91/05/05
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
 *  Comments:	deferred data / event report sending routines
 *
 **************************************************************************
 */

/*LINTLIBRARY*/

#include "netmgt/netmgt_hdrs.h"
#include "netmgt/netmgt.h"
#include "libnetmgt/libnetmgt.h"

/* --------------------------------------------------------------------
 *  netmgt_request_deferred - C wrapper to request deferred reports
 *      returns TRUE if successful; otherwise returns FALSE
 * --------------------------------------------------------------------
 */
bool_t
netmgt_request_deferred (char *agent_name,  
			 u_long agent_prog,	
			 u_long agent_vers,	
			 char *manager_name,
			 struct timeval request_time,
			 struct timeval timeout)
     // agent hostname
     // agent RPC program
     // agent RPC version
     // manager hostname
     // request timestamp
     // RPC timeout
{

  NETMGT_PRN (("deferred: netmgt_request_deferred\n")) ;

  // create a control message requesting deferred reports and
  // send it to the agent
  NetmgtControlMsg *aControlMsg ;
  aControlMsg = (NetmgtControlMsg *) calloc (1, sizeof (NetmgtControlMsg));
  if (!aControlMsg)
    {
      if (netmgt_debug)
	perror ("deferred: calloc failed");
      _netmgtStatus.setStatus (NETMGT_MALLOC, 0, strerror (errno));
      return FALSE;
    }
  bool_t retval = aControlMsg->requestDeferred (agent_name, 
						agent_prog, 
						agent_vers,
						manager_name, 
						request_time,
						timeout) ;
  (void) cfree ((caddr_t) aControlMsg);
  return retval;
}

/* --------------------------------------------------------------------
 *  netmgt_request_deferred - request deferred reports
 *      returns TRUE if successful; otherwise returns FALSE
 * --------------------------------------------------------------------
 */
bool_t
NetmgtControlMsg::requestDeferred (char *agent_name,
				   u_long req_agent_prog,
				   u_long req_agent_vers, 
				   char *manager_name,
				   struct timeval req_request_time,
				   struct timeval timeout)	
     // agent hostname
     // agent RPC program
     // agent RPC version
     // manager hostname
     // request timestamp
     // RPC timeout
{

  NETMGT_PRN (("deferred: netmgt_request_deferred\n")) ;

  // reset internal error buffer 
  _netmgtStatus.clearStatus ();

  // verify input 
  if (!agent_name)
    {
      NETMGT_PRN (("deferred: no agent hostname\n"));
      _netmgtStatus.setStatus (NETMGT_NOAGENTHOSTNAME, 0, NULL);
      return FALSE;
    }
  if (!manager_name)
    {
      NETMGT_PRN (("deferred: no manager hostname\n"));
      _netmgtStatus.setStatus (NETMGT_NOMANAGERHOSTNAME, 0, NULL);
      return FALSE;
    }
  if (req_request_time.tv_sec < 0 || req_request_time.tv_usec < 0)
    {
      NETMGT_PRN (("deferred: invalid request timestamp: %d.%d\n",
		   req_request_time.tv_sec, req_request_time.tv_usec));
      _netmgtStatus.setStatus (NETMGT_BADTIMESTAMP, 0, NULL);
      return FALSE;
    }

  u_long addr;			// IP address (for inet_addr) 
  struct hostent *hp ;		// host table entry 
  struct in_addr req_agent_addr ;	// agent IP address 
  struct in_addr req_manager_addr ;	// manager IP address 

  // get agent IP address 
  addr = inet_addr (agent_name);
  if (addr != -1)
    req_agent_addr.s_addr = addr;
  else
    {
      hp = gethostbyname (agent_name);
      if (!hp)
	{
	  NETMGT_PRN (("deferred: unknown agent: %s\n", agent_name));
	  _netmgtStatus.setStatus (NETMGT_UNKNOWNHOST, 0, NULL);
	  return FALSE;
	}
      (void) memcpy ((caddr_t) & req_agent_addr, (caddr_t) hp->h_addr, 
		     hp->h_length);
    }

  // get manager IP address 
  addr = inet_addr (manager_name);
  if (addr != -1)
    manager_addr.s_addr = addr;
  else
    {
      hp = gethostbyname (manager_name);
      if (!hp)
	{
	  NETMGT_PRN (("deferred unknown host: %s\n", manager_name));
	  _netmgtStatus.setStatus (NETMGT_UNKNOWNHOST, 0, NULL);
	  return FALSE;
	}
      (void) memcpy ((caddr_t) & req_manager_addr, (caddr_t) hp->h_addr, 
		     hp->h_length);
    }

  // build deferred request buffer 
  this->type = NETMGT_DEFERRED_REQUEST;
  this->request_time = req_request_time;
  this->handle = (u_int) 0;
  (void) memcpy ((caddr_t) & this->manager_addr,
		 (caddr_t) & req_manager_addr, 
		 sizeof (struct in_addr));
  (void) memcpy ((caddr_t) & this->agent_addr,
		 (caddr_t) & req_agent_addr, 
		 sizeof (struct in_addr));
  this->agent_prog = req_agent_prog;
  this->agent_vers = req_agent_vers;

  enum clnt_stat clnt_stat ;	// clnt_call return status 
  struct rpc_err rpc_err ;	// RPC error status 
 
  // set request authentication flavor
  assert (aNetmgtManager != (NetmgtManager *) NULL);
  int flavor = aNetmgtManager->myRequest->whichAuthFlavor ();

  // send the request message 
  for (int attempts = 0; attempts < 2; attempts++)
    {

      // get agent client handle 
      if (!this->myClient.newClient (req_agent_addr, 
				     (u_long) IPPROTO_UDP,
				     req_agent_prog, 
				     req_agent_vers,
				     flavor, 
				     timeout))
	  return FALSE;

      clnt_stat = clnt_call (this->myClient.getHandle(), 
			     NETMGT_CONTROL_PROC,
			     (xdrproc_t) _netmgt_xdrControl,
			     (caddr_t) this,
			     (xdrproc_t) _netmgt_xdrStatus, 
			     (caddr_t) &netmgt_error, 
			     timeout);

      CLNT_GETERR (this->myClient.getHandle(), &rpc_err);

      // RPC successful 
      if (clnt_stat == RPC_SUCCESS)
	{
	  this->myClient.destroyClient ();

	  // was request successful ?
	  if (_netmgtStatus.wasSuccess ())
	    return TRUE;
	  return FALSE;
	}

      if (netmgt_debug)
	clnt_perror (this->myClient.getHandle(), "deferred: clnt_call");

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
				 clnt_sperror(this->myClient.getHandle(),
					      dgettext (NETMGT_TEXT_DOMAIN,
							"Can't send request")));
      this->myClient.destroyClient ();
      return FALSE;
    }
  /*NOTREACHED*/
}

/* -------------------------------------------------------------------------
 *  NetmgtDispatcher::dispatchDeferred - dispatch sending deferred reports
 *	returns TRUE if successful; otherwise returns TRUE
 * -------------------------------------------------------------------------
 */
bool_t
NetmgtDispatcher::dispatchDeferred (NetmgtControlMsg *aControlMsg)
				// control message pointer
{

  NETMGT_PRN (("deferred: NetmgtDispatcher::dispatchDeferred\n"));

  // block signals while in critical section 
  sigset_t osigmask = this->blockSignals ();

  // find request to kill in agent request queue 
  NetmgtQueueNode *currNode;	// current requst queue node 
  NetmgtRequest *request;	// request queue data 
  NetmgtRequestInfo requestInfo ; // request information
  bool_t found;			// whether request was found 
#ifdef _SVR4_
  pid_t pid;			// process ID to kill
#else 
  int pid;			// process ID to kill
#endif _SVR4_ 

  assert (this != (NetmgtDispatcher *) NULL);
  found = FALSE;
  currNode = this->myRequestQueue.getHead ();
  while (currNode)
    {

      // get request information pointer 
      assert (currNode->isData ());
      request = (NetmgtRequest *) currNode->getData ();

      // get request information
      request->getRequestInfo (&requestInfo) ;

      if (memcmp ((caddr_t) & requestInfo.request_time,
		  (caddr_t) aControlMsg->getRequestTime (),
		  sizeof (struct timeval)) == 0 &&
	  memcmp ((caddr_t) & requestInfo.manager_addr,
		  (caddr_t) aControlMsg->getManagerAddr (),
		  sizeof (struct in_addr)) == 0)
	{
	  found = TRUE;
	  pid = requestInfo.pid;
	  break;
	}
      currNode = currNode->getNext ();
    }

  // restore signal mask 
  this->unblockSignals (osigmask);

  if (!found)
    {
      NETMGT_PRN (("deferred: can't find request\n"));
      _netmgtStatus.setStatus (NETMGT_NOSUCHREQUEST, 0, NULL);
      return FALSE;
    }

  // send SIGUSR1 to process executing request 
  NETMGT_PRN (("deferred: sending SIGUSR1 to %d\n", pid));
  if (kill (pid, SIGUSR1) == -1)
    {
      NETMGT_PRN (("deferred: can't send SIGUSR1 to process ID %d: %s\n",
		   requestInfo.pid, strerror (errno)));
      _netmgtStatus.setStatus (NETMGT_KILLREQUEST, 0, NULL);
      return FALSE;
    }
  return TRUE;
}

/* ----------------------------------------------------------------------
 *  _netmgt_sendDeferred - wrapper for NetmgtDispatcher::sendDeferred
 *	no return value
 * ----------------------------------------------------------------------
 */
void 
#ifdef _SVR4_
_netmgt_sendDeferred (int sig)
#else 
_netmgt_sendDeferred (int sig, 
		      int code, 
		      struct sigcontext *scp, 
		      char *addr)
#endif _SVR4_
     // signal number caught 
     // additional signal detail 
     // saved process context 
     // additional address info 
{

  NETMGT_PRN (("deferred: NetmgtDispatcher::_netmgt_sendDeferred\n"));

#ifdef _SVR4_
  aNetmgtPerformer->sendDeferred (sig);
  return;
#else
  return aNetmgtPerformer->sendDeferred (sig, code, scp, addr);
#endif _SVR4_
}

/* ----------------------------------------------------------------------
 *  NetmgtPerformer::sendDeferred - send deferred reports to rendezvous
 *	no return value
 * ----------------------------------------------------------------------
 */
#ifdef _SVR4_
void NetmgtPerformer::sendDeferred (int sig)
#else
void NetmgtPerformer::sendDeferred (int sig, 
				    int code, 
				    struct sigcontext *scp, 
				    char *addr)
#endif _SVR4_
     // signal number caught 
     // additional signal detail 
     // saved process context 
     // additional address info 
{

  NETMGT_PRN (("deferred: NetmgtPerformer::sendDeferred\n"));

  (void) this->resendReports ();
  return;
}
