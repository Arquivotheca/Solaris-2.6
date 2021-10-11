#if !defined(lint) && !defined(NOID)
  static char sccsid[] = "@(#)activity.cc	1.40 5/5/91 SMI";
#endif

/**************************************************************************
 *  File:	lib/libnetmgt/activity.cc
 *
 *  Author:	Lynn Monsanto, Sun Microsystems Inc.
 *
 *  SCCSID:     @(#)activity.cc  1.40  91/05/05
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
 *  Comments:	activity caching routines
 *
 **************************************************************************
 */

/*LINTLIBRARY*/

#include "netmgt/netmgt_hdrs.h"
#include "netmgt/netmgt.h"
#include "libnetmgt/libnetmgt.h"

/* -----------------------------------------------------------------------
 *  NetmgtControlMsg::cacheActivity - send cache request message
 *	returns TRUE if successful; otherwise returns FALSE
 * -----------------------------------------------------------------------
 */
bool_t
NetmgtControlMsg::cacheActivity (NetmgtDispatcher *aDispatcher)
				// an agent dispatcher pointer
{

  NETMGT_PRN (("activity: NetmgtControlMsg::cacheActivity\n"));

  // reset internal error buffer 
  _netmgtStatus.clearStatus ();

  // get request message header
  NetmgtServiceHeader header;		// request message header
  if (!aDispatcher->myServiceMsg->getHeader (&header))
    return FALSE;
  
  // get a client handle 
  int sock = RPC_ANYSOCK;	// socket descriptor
  if (!this->myClient.newClient (header.manager_addr,
				 (u_long) IPPROTO_UDP,
				 NETMGT_ACTIVITY_PROG,
				 header.rendez_vers,
				 NETMGT_NO_SECURITY,
				 aDispatcher->getTimeout ()))
    return FALSE;
  
  // send the cache request message to the activity daemon
  this->type = NETMGT_CACHE_REQUEST;
  this->request_time = header.request_time;
  this->handle = header.handle;
  this->manager_addr = header.manager_addr;
  this->agent_addr = header.agent_addr;
  this->agent_prog = header.agent_prog;
  this->agent_vers = header.agent_vers;
  
  enum clnt_stat clnt_stat ;	// clnt_call return status 
  clnt_stat = clnt_call (this->myClient.getHandle(), 
			 NETMGT_CONTROL_PROC,
			 (xdrproc_t) _netmgt_xdrControl,
			 (caddr_t) this,
			 (xdrproc_t) _netmgt_xdrStatus, 
			 (caddr_t) &netmgt_error,
			 aDispatcher->getTimeout ());
  
  struct rpc_err rpc_err ;	// RPC return status 
  CLNT_GETERR (this->myClient.getHandle (), &rpc_err);
  if (clnt_stat != RPC_SUCCESS)
    {
      if (netmgt_debug)
	clnt_perror (this->myClient.getHandle (), "activity: clnt_call");
      if (rpc_err.re_status == RPC_TIMEDOUT)
	_netmgtStatus.setStatus (NETMGT_RPCTIMEDOUT, 
				 0, 
				 dgettext (NETMGT_TEXT_DOMAIN,
					   "Can't cache request"));
      else
	_netmgtStatus.setStatus (NETMGT_RPCFAILED, 
				 0, 
				 clnt_sperror (this->myClient.getHandle(),
					       dgettext (NETMGT_TEXT_DOMAIN,
							 "Can't cache request")));
      this->myClient.destroyClient ();
      return FALSE;
    }
  
  // destroy client
  this->myClient.destroyClient ();
  
  // was request successful ?
  if (_netmgtStatus.wasSuccess ())
    return TRUE;
  return FALSE;
}

/* ----------------------------------------------------------------------
 *  NetmgtControlMsg::uncacheActivity - send uncache request message
 *	request; returns TRUE if successful; otherwise returns FALSE
 * ----------------------------------------------------------------------
 */
bool_t
NetmgtControlMsg::uncacheActivity (NetmgtDispatcher *aDispatcher,
				   NetmgtRequest *aRequest)
     // an agent dispatcher pointer
     // a request pointer
{
  
  NETMGT_PRN (("activity: NetmgtControlMsg::uncacheActivity\n"));
  
  assert (aRequest != (NetmgtRequest *) NULL);
  
  // reset internal error buffer 
  _netmgtStatus.clearStatus ();
  
  // fork a subprocess to send the request message 
  switch (fork ())
    {
    case -1:			// error 
      if (netmgt_debug)
	perror ("activity: fork failed");
      _netmgtStatus.setStatus (NETMGT_FORK, 0, strerror (errno));
      return FALSE;
      
    default:			// parent 
      return TRUE;
      
    case 0:			// child 
      
      // fall through 
      break;
    }

  // get request info
  NetmgtRequestInfo requestInfo ;
  aRequest->getRequestInfo (&requestInfo);

  // get a client handle
  if (!this->myClient.newClient (requestInfo.manager_addr, 
				 (u_long) IPPROTO_UDP,
				 NETMGT_ACTIVITY_PROG, 
				 NETMGT_ACTIVITY_VERS, 
				 NETMGT_NO_SECURITY,
				 aDispatcher->getTimeout ()))
    exit (1);

  // send the cache message 
  struct sockaddr_in localname ; //  local IP name
 
  this->type = NETMGT_UNCACHE_REQUEST;
  this->request_time = requestInfo.request_time;
  this->handle = requestInfo.handle;
  this->manager_addr = requestInfo.manager_addr;
  (void) get_myaddress (&localname);
  (void) memcpy ((caddr_t) & this->agent_addr, 
		 (caddr_t) & localname.sin_addr.s_addr, 
		 sizeof (struct in_addr));
  this->agent_prog = aDispatcher->getProgram ();
  this->agent_vers = aDispatcher->getVersion ();

  enum clnt_stat clnt_stat ;	// clnt_call return status
  clnt_stat = clnt_call (this->myClient.getHandle(), 
			 NETMGT_CONTROL_PROC,
			 (xdrproc_t) _netmgt_xdrControl,
			 (caddr_t) this,
			 (xdrproc_t) _netmgt_xdrStatus, 
			 (caddr_t) &netmgt_error,
			 aDispatcher->getTimeout ());

  // destroy the client
  this->myClient.destroyClient ();
  if (clnt_stat != RPC_SUCCESS)
    {
      // don't set error buffer because child process will exit 
      if (netmgt_debug)
	clnt_perror (this->myClient.getHandle (), "activity: clnt_call");
      exit (1);
    }
  exit (0);

  /*NOTREACHED*/
}

/* ----------------------------------------------------------------------
 *  _netmgt_check_activity - check whether agent is performing request
 *	returns TRUE if agent is performing activity; otherwise FALSE
 * ----------------------------------------------------------------------
 */
bool_t _netmgt_check_activity (struct timeval request_time,
			       struct in_addr agent_addr,
			       u_long agent_prog,
			       u_long agent_vers,
			       struct timeval timeout)
{

  NETMGT_PRN (("activity: _netmgt_check_activity\n"));

  Netmgt_activity activity ;	// activity record
  (void) memset ((caddr_t) & activity, 0, sizeof (Netmgt_activity));
  activity.request_time = request_time;
  activity.agent_addr = agent_addr;
  activity.agent_prog = agent_prog;
  activity.agent_vers = agent_vers;

  NetmgtControlMsg control ;	// control message
  return control.checkActivity (& activity, timeout);
}

/* ----------------------------------------------------------------------
 *  NetmgtControlMsg::checkActivity - send activity verification message
 *	returns TRUE if agent is performing activity; otherwise FALSE
 * ----------------------------------------------------------------------
 */
bool_t
NetmgtControlMsg::checkActivity (Netmgt_activity *activity,
				 struct timeval timeout)
     // an activity record pointer
     // timeout
{

  NETMGT_PRN (("activity: NetmgtControlMsg::checkActivity\n"));

  // reset internal error code 
  _netmgtStatus.clearStatus ();

  // get a client handle
  if (!this->myClient.newClient (activity->agent_addr, 
				 (u_long) IPPROTO_UDP,
				 activity->agent_prog,
				 activity->agent_vers, 
				 NETMGT_NO_SECURITY,
				 timeout))
    return FALSE;

  // send the cache message 
  this->type = NETMGT_VERIFY_REQUEST;
  this->request_time = activity->request_time;
  this->handle = activity->handle;
  this->manager_addr = activity->manager_addr;
  this->agent_addr = activity->agent_addr;
  this->agent_prog = activity->agent_prog;
  this->agent_vers = activity->agent_vers;

  enum clnt_stat clnt_stat;	// clnt_call return status 
  struct rpc_err rpc_err;	// RPC error status 

  clnt_stat = clnt_call (this->myClient.getHandle (), 
			 NETMGT_CONTROL_PROC,
			 (xdrproc_t) _netmgt_xdrControl,
			 (caddr_t) this,
			 (xdrproc_t) _netmgt_xdrStatus, 
			 (caddr_t) &netmgt_error,
			 timeout);

  CLNT_GETERR (this->myClient.getHandle (), &rpc_err);
  if (clnt_stat != RPC_SUCCESS)
    {
      if (netmgt_debug)
	clnt_perror (this->myClient.getHandle (), "activity: clnt_call");
      if (rpc_err.re_status == RPC_TIMEDOUT)
	_netmgtStatus.setStatus (NETMGT_RPCTIMEDOUT, 
				 0, 
				 dgettext (NETMGT_TEXT_DOMAIN,
					   "Can't check activity"));
      else
	_netmgtStatus.setStatus (NETMGT_RPCFAILED, 
				 0, 
				 clnt_sperror (this->myClient.getHandle(),
					       dgettext (NETMGT_TEXT_DOMAIN,
							 "Can't check activity")));
      this->myClient.destroyClient ();
      return FALSE;
    }

  // destroy the client
  this->myClient.destroyClient ();

  // was request successful ?
  if (_netmgtStatus.wasSuccess ())
    return TRUE;
  return FALSE;
}

/* ------------------------------------------------------------------------
 *  _netmgt_xdrControl - wrapper to (de)serialize activity control message
 *	returns TRUE if successful; otherwise returns FALSE
 * ------------------------------------------------------------------------
 */
bool_t
_netmgt_xdrControl (XDR *xdr, NetmgtControlMsg *aControlMsg)
              			// transport handle 
                               	// activity control message
{
  NETMGT_PRN (("activity: _netmgt_xdrControl\n"));
  return aControlMsg->xdrControl (xdr) ;
}

/* ------------------------------------------------------------------------
 *  NetmgtControlMsg::xdrControl - (de)serialize activity control message
 *	returns TRUE if successful; otherwise returns FALSE
 * ------------------------------------------------------------------------
 */
bool_t
NetmgtControlMsg::xdrControl (XDR *xdr)
              			// transport handle 
{

  NETMGT_PRN (("activity: NetmgtControlMsg::xdrControl\n"));

  assert (xdr != (XDR *) NULL);

#ifdef DEBUG
  if (xdr->x_op == XDR_ENCODE)
    {
      NETMGT_PRN (("activity: sending activity control message\n"));
      NETMGT_PRN (("activity:         type: %u\n", this->type));
      NETMGT_PRN (("activity: request_time: %d sec. %d usec.\n",
		   this->request_time.tv_sec,
		   this->request_time.tv_usec));
      NETMGT_PRN (("activity:       handle: %u\n", this->handle));
      NETMGT_PRN (("activity: manager_addr: %s\n",
		   inet_ntoa (this->manager_addr)));
      NETMGT_PRN (("activity:   agent_addr: %s\n",
		   inet_ntoa (this->agent_addr)));
      NETMGT_PRN (("activity:   agent_prog: %u\n", this->agent_prog));
      NETMGT_PRN (("activity:   agent_vers: %u\n", this->agent_vers));
    }
#endif DEBUG

  // XDR (de)serialize message 
  char *manager;		// manager IP address pointer 
  char *agent;			// agent IP address pointer 
  u_int size;			// IP address size 

  manager = (char *) &this->manager_addr.s_addr;
  agent = (char *) &this->agent_addr.s_addr;
  size = sizeof (struct in_addr);
  if (!xdr_u_int (xdr, (u_int *) &this->type) ||
      !xdr_long (xdr, &this->request_time.tv_sec) ||
      !xdr_long (xdr, &this->request_time.tv_usec) ||
      !xdr_u_int (xdr, &this->handle) ||
      !xdr_bytes (xdr, &manager, &size, size) ||
      !xdr_bytes (xdr, &agent, &size, size) ||
      !xdr_u_long (xdr, &this->agent_prog) ||
      !xdr_u_long (xdr, &this->agent_vers))
    return FALSE;

#ifdef DEBUG
  if (xdr->x_op == XDR_DECODE)
    {
      NETMGT_PRN (("activity: received activity control message\n"));
      NETMGT_PRN (("activity:         type: %u\n", this->type));
      NETMGT_PRN (("activity: request_time: %d sec. %d usec.\n",
		   this->request_time.tv_sec,
		   this->request_time.tv_usec));
      NETMGT_PRN (("activity:       handle: %u\n", this->handle));
      NETMGT_PRN (("activity: manager_addr: %s\n",
		   inet_ntoa (this->manager_addr)));
      NETMGT_PRN (("activity:   agent_addr: %s\n",
		   inet_ntoa (this->agent_addr)));
      NETMGT_PRN (("activity:   agent_prog: %u\n", this->agent_prog));
      NETMGT_PRN (("activity:   agent_vers: %u\n", this->agent_vers));
    }
#endif DEBUG

  return TRUE;
}
