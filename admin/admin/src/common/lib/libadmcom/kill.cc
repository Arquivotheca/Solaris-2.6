#if !defined(lint) && !defined(NOID)
  static char sccsid[] = "@(#)kill.cc	1.37 5/5/91 SMI";
#endif

/**************************************************************************
 *  File:	lib/libnetmgt/kill.c
 *
 *  Author:	Lynn Monsanto, Sun Microsystems Inc.
 *
 *  SCCSID:     @(#)kill.cc  1.37  91/05/05
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
 *  Comments:	kill agent activity routines
 *
 **************************************************************************
 */

/*LINTLIBRARY*/

#include "netmgt/netmgt_hdrs.h"
#include "netmgt/netmgt.h"
#include "libnetmgt/libnetmgt.h"

/* --------------------------------------------------------------------
 *  netmgt_kill_request - C wrapper to send kill request to agent
 *	returns TRUE if successful; otherwise returns FALSE
 * --------------------------------------------------------------------
 */
bool_t
netmgt_kill_request (char *agent_name, 
		     u_long agent_prog, 
		     u_long agent_vers, 
		     char *manager_name, 
		     struct timeval request_time, 
		     struct timeval timeout)

{
  NETMGT_PRN (("kill: netmgt_kill_request\n")) ;

  // create a control message requesting an agent to terminate
  // a request and send it to the agent
  NetmgtControlMsg *aControlMsg ;
  aControlMsg = (NetmgtControlMsg *) calloc (1, sizeof (NetmgtControlMsg));
  if (!aControlMsg)
    {
      if (netmgt_debug)
	perror ("kill: calloc failed");
      _netmgtStatus.setStatus (NETMGT_MALLOC, 0, strerror (errno));
      return FALSE;
    }
  bool_t retval = aControlMsg->sendKillRequest (agent_name, 
						agent_prog, 
						agent_vers,
						manager_name, 
						request_time, 
						timeout);
  (void) cfree ((caddr_t) aControlMsg);
  return retval;
}

/* --------------------------------------------------------------------
 *  NetmgtControlMsg::sendKillRequest - send kill request to agent
 *      returns TRUE if successful; otherwise returns FALSE
 * --------------------------------------------------------------------
 */
bool_t
NetmgtControlMsg::sendKillRequest (char *agent_name, 
				   u_long kill_agent_prog,
				   u_long kill_agent_vers, 
				   char *manager_name,
				   struct timeval kill_request_time, 
				   struct timeval timeout)
{
  u_long addr ;			// IP address (for inet_addr) 
  struct hostent *hp ;		// host table entry 
  struct in_addr kill_agent_addr ;	// agent IP address 
  struct in_addr kill_manager_addr ;	// request source IP address 
  enum clnt_stat clnt_stat;	// clnt_call return status 
  struct rpc_err rpc_err;	// RPC error status 

  NETMGT_PRN (("kill: NetmgtControlMsg::sendKillRequest\n"));

  // reset error buffer 
  _netmgtStatus.clearStatus ();

  // verify input 
  if (!agent_name)
    {
      NETMGT_PRN (("kill: no agent hostname\n"));
      _netmgtStatus.setStatus (NETMGT_NOAGENTHOSTNAME, 0, NULL);
      return FALSE;
    }
  if (!manager_name)
    {
      NETMGT_PRN (("kill: no manager hostname\n"));
      _netmgtStatus.setStatus (NETMGT_NOMANAGERHOSTNAME, 0, NULL);
      return FALSE;
    }
  if (kill_request_time.tv_sec < 0 || kill_request_time.tv_usec < 0)
    {
      NETMGT_PRN (("kill: invalid request timestamp: %d.%d\n",
		   kill_request_time.tv_sec, kill_request_time.tv_usec));
      _netmgtStatus.setStatus (NETMGT_BADTIMESTAMP, 0, NULL);
      return FALSE;
    }

  // get agent IP address 
  addr = inet_addr (agent_name);
  if (addr != -1)
    kill_agent_addr.s_addr = addr;
  else
    {
      hp = gethostbyname (agent_name);
      if (!hp)
	{
	  NETMGT_PRN (("kill: unknown agent: %s\n", agent_name));
	  _netmgtStatus.setStatus (NETMGT_UNKNOWNHOST, 0, agent_name);
	  return FALSE;
	}
      (void) memcpy ((caddr_t) & kill_agent_addr, (caddr_t) hp->h_addr, 
		     hp->h_length);
    }

  // get request source IP address 
  addr = inet_addr (manager_name);
  if (addr != -1)
    kill_manager_addr.s_addr = addr;
  else
    {
      hp = gethostbyname (manager_name);
      if (!hp)
	{
	  NETMGT_PRN (("kill unknown host: %s\n", manager_name));
	  _netmgtStatus.setStatus (NETMGT_UNKNOWNHOST, 0, manager_name);
	  return FALSE;
	}
      (void) memcpy ((caddr_t) & kill_manager_addr, 
		     (caddr_t) hp->h_addr,
		     hp->h_length);
    }

  // build kill request buffer 
  this->type = NETMGT_KILL_REQUEST;
  this->request_time = kill_request_time;
  this->handle = (u_int) 0;
  (void) memcpy ((caddr_t) & this->manager_addr,
		 (caddr_t) & kill_manager_addr, 
		 sizeof (struct in_addr));
  (void) memcpy ((caddr_t) & this->agent_addr,
		 (caddr_t) & kill_agent_addr, 
		 sizeof (struct in_addr));
  this->agent_prog = kill_agent_prog;
  this->agent_vers = kill_agent_vers;

  // set request authentication flavor
  assert (aNetmgtManager != (NetmgtManager *) NULL);
  int flavor = aNetmgtManager->myRequest->whichAuthFlavor ();

  // send the request message 
  for (int attempts = 0; attempts < 2; attempts++)
    {

      // get agent client handle 
      if (!this->myClient.newClient (kill_agent_addr, 
				     (u_long) IPPROTO_UDP,
				     kill_agent_prog, 
				     kill_agent_vers,
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
	clnt_perror (this->myClient.getHandle(), "kill: clnt_call");

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
