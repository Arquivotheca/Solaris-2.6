#if !defined(lint) && !defined(NOID)
  static char sccsid[] = "@(#)term.cc	1.43 8/1/91 SMI";
#endif

/**************************************************************************
 *  File:	lib/libnetmgt/term.cc
 *
 *  Author:	Lynn Monsanto, Sun Microsystems Inc.
 *
 *  SCCSID:     @(#)term.cc  1.43  91/08/01
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
 *  Comments:	request termination routines
 *
 **************************************************************************
 */

/*LINTLIBRARY*/

#include "netmgt/netmgt_hdrs.h"
#include "netmgt/netmgt.h"
#include "libnetmgt/libnetmgt.h"

/* ---------------------------------------------------------------------
 *  _netmgt_get_child_pid - get child PID for a given sequence (C wrapper)
 *	returns child PID if successful; otherwise '-1'
 * ---------------------------------------------------------------------
 */
#ifdef _SVR4_
pid_t 
_netmgt_get_child_pid (u_int sequence)
#else
int 
_netmgt_get_child_pid (u_int sequence)
#endif _SVR4_
{
  assert (aNetmgtDispatcher != (NetmgtDispatcher *) NULL);
  return aNetmgtDispatcher->getChildPid (sequence);
}

/* --------------------------------------------------------------------
 *  NetmgtDispatcher::getChildPid - get child PID for a given sequence
 *	returns child PID if successful; otherwise '0'
 * --------------------------------------------------------------------
 */
#ifdef _SVR4_
pid_t 
NetmgtDispatcher::getChildPid (u_int sequence)
#else
int 
NetmgtDispatcher::getChildPid (u_int sequence)
#endif _SVR4_
{
  NETMGT_PRN (("term: NetmgtDispatcher::getChildPid\n"));

  // clear error status
  _netmgtStatus.clearStatus ();

  // block signals while in critical section 
  sigset_t osigmask = this->blockSignals ();

  // traverse the agent request list looking for a sequence match
  NetmgtQueueNode *currNode;	// current requst queue node 
  NetmgtRequest *aRequest;	// request queue data
  NetmgtRequestInfo requestInfo ; // request information

  currNode = this->myRequestQueue.getHead ();
  while (currNode)
    {
      
      // get request information pointer
      assert (currNode->isData ());
      aRequest = (NetmgtRequest *) currNode->getData ();
      
      // get request information
      aRequest->getRequestInfo (&requestInfo) ;
      
      // return the child PID if the request sequence matches
      if (requestInfo.sequence == sequence)
	{
	  this->unblockSignals (osigmask);
	  return requestInfo.pid;
	}

      currNode = currNode->getNext ();
    }

  this->unblockSignals (osigmask);
  return 0;
}

/* --------------------------------------------------------------------
 *  NetmgtDispatcher::terminateRequest - terminate management activity
 *	returns TRUE if successful; otherwise returns FALSE
 * --------------------------------------------------------------------
 */
bool_t
NetmgtDispatcher::terminateRequest (struct svc_req *rqst, 
				    register SVCXPRT *xprt,
				    NetmgtControlMsg *aControlMsg)
     // server request 
     // server transport handle 
     // control message
{
  NETMGT_PRN (("terminte: NetmgtDispatcher::terminateRequest\n")) ;
  
  // clear error status
  _netmgtStatus.clearStatus ();
  
  // terminate all agent requests if no source address is given 
  if (aControlMsg->getManagerAddr()->s_addr == 0)
    return this->terminateAllRequests (rqst, xprt);
  
  // terminate all requests from a specified source address
  // if no request timestamp is given 
  if (aControlMsg->getRequestTime()->tv_sec == 0 &&
      aControlMsg->getRequestTime()->tv_usec == 0)
    return this->terminateMyRequests (rqst, xprt, aControlMsg);
  
  // otherwise, terminate an activity if it has the specified
  // request timestamp
  return this->terminateThisRequest (rqst, xprt, aControlMsg);
}

/* -------------------------------------------------------------------
 *  NetmgtDispatcher::terminateThisRequest - delete agent activity
 *	returns TRUE if successful; otherwise returns FALSE
 * -------------------------------------------------------------------
 */
bool_t
NetmgtDispatcher::terminateThisRequest (struct svc_req *rqst, 
					register SVCXPRT *xprt,
					NetmgtControlMsg *aControlMsg)
{

  NETMGT_PRN (("term: NetmgtDispatcher::terminateThisRequest\n"));

  // clear error status
  _netmgtStatus.clearStatus ();

  // block signals while in critical section 
  sigset_t osigmask = this->blockSignals ();

  // find request to kill in agent request queue 
  NetmgtQueueNode *currNode;	// current requst queue node 
  NetmgtRequest *aRequest;	// request queue data
  NetmgtRequestInfo requestInfo ; // request information
  bool_t found;			// whether request was found 
#ifdef _SVR4_
  pid_t pid;			// process ID to kill 
#else
  int pid;			// process ID to kill
#endif _SVR4_ 

  found = FALSE;
  currNode = this->myRequestQueue.getHead ();
  while (currNode)
    {
      
      // get request information pointer
      assert (currNode->isData ());
      aRequest = (NetmgtRequest *) currNode->getData ();
      
      // get request information
      aRequest->getRequestInfo (&requestInfo) ;
      
      if (memcmp ((caddr_t) aControlMsg->getRequestTime (),
		  (caddr_t) & requestInfo.request_time,
		  sizeof (struct timeval)) == 0 &&
	  memcmp ((caddr_t) aControlMsg->getManagerAddr (),
		  (caddr_t) & requestInfo.manager_addr,
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
      NETMGT_PRN (("term: can't find request to kill\n"));
      _netmgtStatus.setStatus (NETMGT_NOSUCHREQUEST, 0, NULL);
      return FALSE;
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
  if (rqst->rq_cred.oa_flavor > AUTH_NONE)
    {
      if (!this->verifyCredentials (rqst,
				    xprt, 
				    &uid,
				    &gid,
				    &gidlen,
				    gidlist,
				    netname))
	return FALSE;
    }

  // verify the requestor's authorization
  if (this->readSecurity > NETMGT_NO_SECURITY ||
      this->rdwrSecurity > NETMGT_NO_SECURITY)
    {
      if (!this->verifyAuthorization (requestInfo.type, uid))
	return FALSE;
    }
  
  // send SIGTERM to process executing request 
  if (kill (pid, SIGTERM) == -1)
    {
      NETMGT_PRN (("term: can't kill PID %d: %s\n",
		   requestInfo.pid, strerror (errno)));
      _netmgtStatus.setStatus (NETMGT_KILLREQUEST, 0, NULL);
      return FALSE;
    }
  return TRUE;
}

/* ------------------------------------------------------------------------
 *  NetmgtDispatcher::terminateMyRequests - terminate all requests 
 *	from source address ---
 *	returns TRUE if successful; otherwise returns FALSE
 * ------------------------------------------------------------------------
 */
bool_t
NetmgtDispatcher::terminateMyRequests (struct svc_req *rqst, 
				       register SVCXPRT *xprt,
				       NetmgtControlMsg *aControlMsg)
{

  NETMGT_PRN (("term: NetmgtDispatcher::terminateMyRequests\n"));

  // clear error status
  _netmgtStatus.clearStatus ();

  // keep searching for request to kill in the agent request
  // until no more request to kill are found
  sigset_t osigmask;		// previous signal mask 
  NetmgtQueueNode *currNode;	// current requst queue node 
  NetmgtRequest *request;	// request queue data 
  NetmgtRequestInfo requestInfo ; // request information
  bool_t found;			// whether request was found 
  bool_t kill_failed;		// whether kill failed
#ifdef _SVR4_ 
  pid_t pid;			// process ID to kill 
#else
  int pid;			// process ID to kill
#endif _SVR4_ 

  kill_failed = FALSE;
  do
  {
    // block signals while in critical section 
    osigmask = this->blockSignals ();

    found = FALSE;
    for (currNode = this->myRequestQueue.getHead ();
	 currNode; 
	 currNode = currNode->getNext ())
      {

	// get request data 
	assert (currNode->isData ());
	request = (NetmgtRequest *) currNode->getData ();

	// get request information
	request->getRequestInfo (&requestInfo);

	if (memcmp ((caddr_t) & requestInfo.manager_addr,
		    (caddr_t) aControlMsg->getManagerAddr (),
		    sizeof (struct in_addr)) == 0)
	  {
	    found = TRUE;
	    pid = requestInfo.pid;
	    break;
	  }
      }

    // restore signal mask 
    this->unblockSignals (osigmask);

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
  if (rqst->rq_cred.oa_flavor > AUTH_NONE)
    {
      if (!this->verifyCredentials (rqst,
				    xprt, 
				    &uid,
				    &gid,
				    &gidlen,
				    gidlist,
				    netname))
	return FALSE;
    }

  // verify the requestor's authorization
  if (this->readSecurity > NETMGT_NO_SECURITY ||
      this->rdwrSecurity > NETMGT_NO_SECURITY)
    {
      if (!this->verifyAuthorization (requestInfo.type, uid))
	return FALSE;
    }
  
    // send SIGTERM to process executing request 
    if (found && kill (pid, SIGTERM) == -1)
      {
	NETMGT_PRN (("term: can't kill PID %d: %s\n",
		     pid, strerror (errno)));
	kill_failed = TRUE;;
      }

    // clean up request 
    if (found)
      (void) this->cleanRequest (pid);

  } while (found) ;

  if (kill_failed)
    {
      _netmgtStatus.setStatus (NETMGT_KILLREQUEST, 0, NULL);
      return FALSE;
    }
  return TRUE;
}

/* ------------------------------------------------------------------------
 *  NetmgtDispatcher::terminateAllRequests - terminate all requests
 *      returns TRUE if successful; otherwise FALSE
 * ------------------------------------------------------------------------
 */
bool_t
NetmgtDispatcher::terminateAllRequests (struct svc_req *rqst, 
					register SVCXPRT *xprt)
{

  NETMGT_PRN (("term: NetmgtDispatcher::terminateAllRequests\n"));

  // clear error status
  _netmgtStatus.clearStatus ();

  // keep searching for request to kill in the agent request
  // until no more request to kill are found 
  sigset_t osigmask;		// previous signal mask 
  NetmgtQueueNode *currNode;	// current requst queue node 
  NetmgtRequest *request;	// request queue data 
  NetmgtRequestInfo requestInfo ; // request information
  bool_t found;			// whether request was found 
  bool_t kill_failed;		// whether kill failed 
#ifdef _SVR4_ 
  pid_t pid;			// process ID to kill 
#else
  int pid;			// process ID to kill
#endif _SVR4_ 

  kill_failed = FALSE;
  do
  {
    // block signals while in critical section 
    osigmask = this->blockSignals ();

    found = FALSE;
    currNode = this->myRequestQueue.getHead ();
    if (currNode)
      {

	// get request data 
	assert (currNode->isData ());
	request = (NetmgtRequest *) currNode->getData ();
	found = TRUE;

	// get request information
	request->getRequestInfo (&requestInfo);

	pid = requestInfo.pid;
      }

    // restore signal mask 
    this->unblockSignals (osigmask);

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
    if (rqst->rq_cred.oa_flavor > AUTH_NONE)
      {
	if (!this->verifyCredentials (rqst,
				      xprt, 
				      &uid,
				      &gid,
				      &gidlen,
				      gidlist,
				      netname))
	  return FALSE;
      }
    
    // verify the requestor's authorization
    if (this->readSecurity > NETMGT_NO_SECURITY ||
	this->rdwrSecurity > NETMGT_NO_SECURITY)
      {
	if (!this->verifyAuthorization (requestInfo.type, uid))
	  return FALSE;
      }
  
    // send SIGTERM to process executing request 
    if (found && kill (pid, SIGTERM) == -1)
      {
	NETMGT_PRN (("term: can't kill PID %d: %s\n",
		     pid, strerror (errno)));
	kill_failed = TRUE;;
      }

  } while (found) ;

  // complain but don't return an error if we couldn't find
  // a request to kill
  if (!found)
    NETMGT_PRN (("term: can't find request to kill\n"));

  if (kill_failed)
    {
      _netmgtStatus.setStatus (NETMGT_KILLREQUEST, 0, NULL);
      return FALSE;
    }
  return TRUE;
}
