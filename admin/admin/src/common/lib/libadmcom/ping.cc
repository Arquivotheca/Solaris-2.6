#if !defined(lint) && !defined(NOID)
  static char sccsid[] = "@(#)ping.cc	1.35 5/5/91 SMI";
#endif

/**************************************************************************
 *  File:	lib/libnetmgt/ping.cc
 *
 *  Author:	Lynn Monsanto, Sun Microsystems Inc.
 *
 *  SCCSID:     @(#)ping.cc  1.35  91/05/05
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
 *  Comments:	ping agent routine
 *
 **************************************************************************
 */


/*LINTLIBRARY*/

#include "netmgt/netmgt_hdrs.h"
#include "netmgt/netmgt.h"
#include "libnetmgt/libnetmgt.h"

/* ------------------------------------------------------------------
 *   _netmgt_pingEntity - C wrapper to ping RPC management entity
 *	returns TRUE if successful; otherwise returns FALSE
 * ------------------------------------------------------------------
 */
bool_t
_netmgt_pingEntity (struct in_addr agent_addr, 
		    u_long agent_prog, 
		    u_long agent_vers,
		    struct timeval timeout)
     // agent IP address
     // agent RPC program
     // agent RPC version
     // RPC timeout
{
  NETMGT_PRN (("ping: netmgt_pingEntity\n"));

  // create a message object and ping entity
  NetmgtServiceMsg aServiceMsg ;
  return aServiceMsg.pingEntity (agent_addr, agent_prog, agent_vers, timeout) ;
}

/* ------------------------------------------------------------------
 *   NetmgtServiceMsg::pingEntity - ping RPC agent
 *	returns TRUE if successful; otherwise returns FALSE
 * ------------------------------------------------------------------
 */
bool_t
NetmgtServiceMsg::pingEntity (struct in_addr ping_agent_addr, 
			      u_long ping_agent_prog, 
			      u_long ping_agent_vers, 
			      struct timeval ping_timeout)
     // agent IP address
     // agent RPC program
     // agent RPC version
     // RPC timeout
{
  
  NETMGT_PRN (("ping: NetmgtServiceMsg::pingEntity: to RPC program %d at %s\n",
	       ping_agent_prog, inet_ntoa (ping_agent_addr))) ;
  
  // reset internal error code 
  _netmgtStatus.clearStatus ();
  
  // get agent client handle 
  if (!this->myClient.newClient (ping_agent_addr, 
				 (u_long) IPPROTO_UDP,
				 ping_agent_prog, 
				 ping_agent_vers, 
				 NETMGT_NO_SECURITY,
				 ping_timeout))
    return FALSE;
  
  // send the ping message 
  enum clnt_stat clnt_stat ;	// clnt_call return status 
  struct rpc_err rpc_err ;	// RPC error status 

  clnt_stat = clnt_call (this->myClient.getHandle(), 
			 NETMGT_PING_PROC, 
			 (xdrproc_t) xdr_void, 
			 (caddr_t) NULL,
			 (xdrproc_t) xdr_void, 
			 (caddr_t) NULL, 
			 ping_timeout);
  
  // return ping results 
  if (clnt_stat != RPC_SUCCESS)
    {
      if (netmgt_debug)
	clnt_perror (this->myClient.getHandle(), "ping: clnt_call failed");
      CLNT_GETERR (this->myClient.getHandle(), &rpc_err);
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
  this->myClient.destroyClient ();
  return TRUE;
}
