#if !defined(lint) && !defined(NOID)
  static char sccsid[] = "@(#)report.cc	1.44 8/1/91 SMI";
#endif

/**************************************************************************
 *  File:       lib/libnetmgt/report.cc
 *
 *  Author:     Lynn Monsanto, Sun Microsystems Inc.
 *
 *  SCCSID:     @(#)report.cc  1.44  91/08/01
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
 *  Comments:   report receiving functions
 *
 **************************************************************************
 */

/*LINTLIBRARY*/

#include "netmgt/netmgt_hdrs.h"
#include "netmgt/netmgt.h"
#include "libnetmgt/libnetmgt.h"

/* ----------------------------------------------------------------------
 *  _netmgt_receiveReport - C wrapper for receiving report message
 *	no return value
 *  This function is called by the RPC library when a rendezvous
 *  receives a report message
 *-----------------------------------------------------------------------
 */
void
_netmgt_receiveReport (struct svc_req *rqst, register SVCXPRT *xprt)
				// server report 
                             	// server transport handle 
{
  NETMGT_PRN (("report: _netmgt_receiveReport\n")) ;

  assert (aNetmgtRendez != (NetmgtRendez *) NULL);
  aNetmgtRendez->receiveReport (rqst, xprt) ;
} 

/* ---------------------------------------------------------------------
 *  NetmgtRendez::receiveReport - receive report message from transport,
 *	dispatch report handler and send confirmation to agent
 *	no return value
 *----------------------------------------------------------------------
 */
void
NetmgtRendez::receiveReport (struct svc_req *rqst, register SVCXPRT *xprt)
                           	// server report 
                             	// server transport handle 
{

  NETMGT_PRN (("report: NetmgtRendez::receiveReport\n")) ;

  assert (rqst != (struct svc_req *) NULL) ;
  assert (xprt != (SVCXPRT *) NULL) ;

  // reset error buffer 
  _netmgtStatus.clearStatus ();

  switch (rqst->rq_proc)
    {
    case NETMGT_PING_PROC:	// ping 

      // just send a reply 
      (void) svc_sendreply (xprt, (xdrproc_t) xdr_void, (caddr_t) NULL);
      return;

    case NETMGT_SERVICE_PROC:	// management report 

      // get report message from the RPC transport
      if (!svc_getargs (xprt, (xdrproc_t) _netmgt_deserialMsg,
			(caddr_t) this->myServiceMsg))
	{
	  NETMGT_PRN (("report: can't deserialize message\n"));
	  (void) svc_sendreply (xprt, 
				(xdrproc_t) _netmgt_xdrStatus,
				(char *) &netmgt_error);
	  return;
	}

      // dispatch the report 
      this->dispatchReport (rqst, xprt);

      // send confirmation to agent
      (void) svc_sendreply (xprt, 
			    (xdrproc_t) _netmgt_xdrStatus, 
			    (char *) &netmgt_error);

      return;

    default:			// error 
      svcerr_noproc (xprt);
      return;
    }
}

/* -------------------------------------------------------------------
 *  NetmgtRendez::dispatchReport - dispatch report message
 *	no return value
 * -------------------------------------------------------------------
 */
void
NetmgtRendez::dispatchReport (struct svc_req *rqst, 
			      register SVCXPRT *xprt)
     // server request 
     // server transport handle 
{

  NETMGT_PRN (("report: NetmgtRendez::dispatchReport\n"));

  assert (rqst != (struct svc_req *) NULL);
  assert (xprt != (SVCXPRT *) NULL);

  // get message header 
  NetmgtServiceHeader header;
  if (!this->myServiceMsg->getHeader (&header))
    return;

  // verify the report type - note rendezvous can receive
  // configuration action, create, and delete requests
  if (header.type != NETMGT_ACTION_REQUEST &&
      header.type != NETMGT_CREATE_REQUEST &&
      header.type != NETMGT_DATA_REPORT &&
      header.type != NETMGT_DELETE_REQUEST &&
      header.type != NETMGT_EVENT_REPORT &&
      header.type != NETMGT_TRAP_REPORT &&
      header.type != NETMGT_ERROR_REPORT && 
      header.type != NETMGT_SET_REPORT) 
    {
      NETMGT_PRN (("report: unknown report type: %d\n", header.type));
      _netmgtStatus.setStatus (NETMGT_UNKNOWNREQUEST, 0, NULL);
      return;
    }

  // verify the requestor's credentials 
#ifdef _SVR4_
  uid_t uid;			// user ID
  gid_t gid;			// group ID (ignored)
  int gidlen;			// group ID length (ignored)
  gid_t gidlist [NGRPS];	// group ID list (ignored)
#else
  int uid;			// user ID
  int gid;			// group ID (ignored)
  int gidlen;			// group ID length (ignored)
  int gidlist [NGRPS];		// group ID list (ignored)
#endif _SVR4_
  char netname [NETMGT_NAMESIZ] ; // netname (ignored)

  if (!this->verifyCredentials (rqst,
				xprt, 
				&uid,
				&gid,
				&gidlen,
				gidlist,
				netname))
    return;

  // verify the requestor's authorization
  if (this->readSecurity > NETMGT_NO_SECURITY ||
      this->rdwrSecurity > NETMGT_NO_SECURITY)
    {
      if (!this->verifyAuthorization (header.type, uid))
	return;
    }

  // clear request options queues 
  (void) this->myOptionsQueue.myDestructor ();

  // if this is a request message instead of a report, initialize 
  // request queue  --- rendezvous objects use this function to also 
  // dispatch configuration requests (e.g., to register an event 
  // rendezvous) 
  if ((header.type == NETMGT_ACTION_REQUEST ||
       header.type == NETMGT_CREATE_REQUEST ||
       header.type == NETMGT_DELETE_REQUEST) &&
      header.length > 0)
    {
      if (!this->initRequestQueues ())
	return;
    }
  // if this is an error or set report, get error report 
  // arguments from arglist and set management status
  if (header.type == NETMGT_ERROR_REPORT ||
      header.type == NETMGT_SET_REPORT)
    {
      NetmgtGeneric aGeneric ;  		// generic argument
      if (!aGeneric.getError (this->myServiceMsg))
	return;
    }

  // reset arglist pointer
  this->myServiceMsg->myArglist.resetPtr ();

  // dispatch the rendezvous application's report handler 
  if (this->dispatch)
    (*this->dispatch) (header.type,
		       header.system,
		       header.group,
		       header.key,
		       header.count,
		       header.interval,
		       header.flags);

  return;
}

