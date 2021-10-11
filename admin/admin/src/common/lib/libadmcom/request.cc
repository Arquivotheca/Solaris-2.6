#if !defined(lint) && !defined(NOID)
  static char sccsid[] = "@(#)request.cc	1.56 8/1/91 SMI";
#endif

/**************************************************************************
 *  File:	lib/libnetmgt/request.cc
 *
 *  Author:	Lynn Monsanto, Sun Microsystems Inc.
 *
 *  SCCSID:     @(#)request.cc  1.42  90/10/27
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
 *  Comments:	agent request dispatch functions
 *
 **************************************************************************
 */

/*LINTLIBRARY*/

#include "netmgt/netmgt_hdrs.h"
#include "netmgt/netmgt.h"
#include "libnetmgt/libnetmgt.h"
#include <signal.h>

/* ----------------------------------------------------------------------
 *  _netmgt_receiveRequest - C wrapper for receiving request message
 *	dispatch request handler, and send confirmation to manager
 *	no return value
 *  This function is called by the RPC library when an agent
 *  process receives a request message
 *----------------------------------------------------------------------
 */
void
_netmgt_receiveRequest (struct svc_req *rqst, register SVCXPRT *xprt)
                           	// server request 
                             	// server transport handle 
{
  NETMGT_PRN (("request: _netmgt_receiveRequest\n")) ;
  assert (aNetmgtDispatcher != (NetmgtDispatcher *) NULL);

  mutex_lock(& aNetmgtDispatcher->update_queues_mutex);

  // call the agent's request dispatch function
  aNetmgtDispatcher->receiveRequest (rqst, xprt) ;

  mutex_unlock(& aNetmgtDispatcher->update_queues_mutex);
} 

/* ---------------------------------------------------------------------------
 *  NetmgtDispatcher::receiveRequest - receive request message from transport,
 *	dispatch request handler, and send confirmation to manager
 *	no return value
 *----------------------------------------------------------------------------
 */

static int _netmgt_get_id(char *name);
static bool_t _netmgt_xdr_id_string(XDR *xdrs, char **strp);

void
NetmgtDispatcher::receiveRequest (struct svc_req *rqst, register SVCXPRT *xprt)
                           	// server request 
                             	// server transport handle 
{

  char recname[64], *nameptr;
  int id;

  // clear idle termination alarm for the dispatcher, if any

  cond_signal(&idle_cond_var);
  
 

  

  NETMGT_PRN (("request: NetmgtDispatcher::receiveRequest\n")) ;

  assert (rqst != (struct svc_req *) NULL) ;
  assert (xprt != (SVCXPRT *) NULL) ;

  // reset error buffer 
  _netmgtStatus.clearStatus ();

  // dispatch the request
  switch (rqst->rq_proc)
    {
    case NETMGT_PING_PROC:	// ping 

      // just send a reply 
      (void) svc_sendreply (xprt, (xdrproc_t) xdr_void, (caddr_t) NULL);
      return;

    case NETMGT_GET_ID_PROC:		// get id
	nameptr = recname;
	if (!svc_getargs (xprt,  (xdrproc_t)_netmgt_xdr_id_string,
			  (caddr_t) &nameptr))
		id = 0;
        else
		id = _netmgt_get_id(recname);
	(void) svc_sendreply (xprt, (xdrproc_t) xdr_int,  (char *) &id);
	return;

    case NETMGT_AGENT_ID_PROC:	// agent identification 

      // get agent identification 
      this->getAgentID ();

      // send agent identification buffer 
      (void) svc_sendreply (xprt, 
			    (xdrproc_t) _netmgt_xdrAgentId, 
			    (char *) &netmgt_agent_ID);
      return;


    case NETMGT_CONTROL_PROC:	// activity control 

      {
	// allocate a control message
	NetmgtControlMsg *aControlMsg ;
	aControlMsg 
	  = (NetmgtControlMsg *) calloc (1, sizeof (NetmgtControlMsg));
	if (!aControlMsg)
	  {
	    if (netmgt_debug)
	      perror ("request: calloc failed");
	    _netmgtStatus.setStatus (NETMGT_MALLOC, 0, strerror (errno));
	    (void) svc_sendreply (xprt, 
				  (xdrproc_t) _netmgt_xdrStatus,
				  (char *) &netmgt_error);
	    return;
	  }

	// get message from transport 
	if (!svc_getargs (xprt, 
			  (xdrproc_t) _netmgt_xdrControl,
			  (caddr_t) aControlMsg))
	{
	  (void) svc_sendreply (xprt, 
				(xdrproc_t) _netmgt_xdrStatus,
				(char *) &netmgt_error);
	  return;
	}
	
	// dispatch activity control request 
	
	switch (aControlMsg->getType ())
	  {
	  case NETMGT_CACHE_REQUEST:
	    (void) this->logActivity (aControlMsg);
	    break;
	  case NETMGT_UNCACHE_REQUEST:
	    (void) this->deleteActivity (aControlMsg);
	    break;
	  case NETMGT_KILL_REQUEST:
	    (void) this->terminateRequest (rqst, xprt, aControlMsg);
	    break;
	  case NETMGT_DEFERRED_REQUEST:
	    (void) this->dispatchDeferred (aControlMsg);
	    break;
	  case NETMGT_VERIFY_REQUEST:
	    (void) this->verifyActivity (aControlMsg);
	    break;
	  default:
	    NETMGT_PRN (("request: unknown activity flag: %d\n",
			 aControlMsg->getType ()));
	    _netmgtStatus.setStatus (NETMGT_UNKNOWNREQUEST, 0, NULL);
	  }

	// free control message
	(void) cfree ((caddr_t) aControlMsg);
	
	// send request confirmation 
	(void) svc_sendreply (xprt, 
			      (xdrproc_t) _netmgt_xdrStatus, 
			      (char *) &netmgt_error);
	
	
	return;
      }

    case NETMGT_SERVICE_PROC:	// management service message
      
      // get request message from the RPC transport
      if (!svc_getargs (xprt, (xdrproc_t) _netmgt_deserialMsg,
			(caddr_t) this->myServiceMsg))
	{
	  NETMGT_PRN (("request: can't deserialize message\n"));
	  (void) svc_sendreply (xprt, 
				(xdrproc_t) _netmgt_xdrStatus,
				(char *) &netmgt_error);
	  return;
	}

      // get message header information 
      NetmgtServiceHeader header;
      if (!this->myServiceMsg->getHeader (&header))
	return;

      // If the requestor wants us to send reports to
      // the request source, save the IP address of the
      // interface from which the request was sent - we
      // need to do this if ip_forwarding is disabled 
#ifdef notdef
      if (svc_getcaller (xprt)->sin_addr.s_addr &&
	  memcmp ((caddr_t) & header.manager_addr,
		  (caddr_t) & header.rendez_addr,
		  sizeof (struct in_addr)) == 0)
	this->myServiceMsg->setRendezAddr (svc_getcaller (xprt)->sin_addr) ;
#endif notdef

      // dispatch the request 
      this->dispatchRequest (rqst, xprt);

      // send confirmation to manager
      NETMGT_PRN (("request: sending confirmation to manager\n"));
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
 *  NetmgtDispatcher::dispatchRequest - dispatch request message
 *	no return value
 * -------------------------------------------------------------------
 */
void
NetmgtDispatcher::dispatchRequest (struct svc_req *rqst, 
				   register SVCXPRT *xprt)
     // server request 
     // server transport handle 
{

  NETMGT_PRN (("request: NetmgtDispatcher::dispatchRequest\n"));

  assert (rqst != (struct svc_req *) NULL);
  assert (xprt != (SVCXPRT *) NULL);

  // set agent state to "verifying request" 
  this->state = NETMGT_VERIFYING;

  // get request message information from request message 
  NetmgtServiceHeader msgInfo;
  if (!this->myServiceMsg->getHeader (&msgInfo))
    return;

  // verify the request type 
  if (msgInfo.type != NETMGT_ACTION_REQUEST &&
      msgInfo.type != NETMGT_CREATE_REQUEST &&
      msgInfo.type != NETMGT_DELETE_REQUEST &&
      msgInfo.type != NETMGT_DATA_REQUEST &&
      msgInfo.type != NETMGT_EVENT_REQUEST &&
      msgInfo.type != NETMGT_SET_REQUEST)
    {
      NETMGT_PRN (("request: unknown request type: %d\n",
		   msgInfo.type));
      _netmgtStatus.setStatus (NETMGT_UNKNOWNREQUEST, 0, NULL);
      return;
    }

  NetmgtRequestInfo requestInfo ; // request information buffer
  (void) memset ((caddr_t) & requestInfo, 0, sizeof (requestInfo));

  // verify the requestor's credentials 
  if (!this->verifyCredentials (rqst,
				xprt, 
				& requestInfo.uid,
				& requestInfo.gid,
				& requestInfo.gidlen,
				requestInfo.gidlist,
				requestInfo.netname))
	return;

  // verify the requestor's authorization
  if (this->readSecurity > NETMGT_NO_SECURITY ||
      this->rdwrSecurity > NETMGT_NO_SECURITY)
    {
      if (!this->verifyAuthorization (msgInfo.type, requestInfo.uid))
	return;
    }

 
  // save request message information in agent's request 
  // information buffer
  requestInfo.myEntity = this;
  requestInfo.pid = getpid ();
  requestInfo.request_time = msgInfo.request_time;
  requestInfo.type = msgInfo.type;
  requestInfo.handle = msgInfo.handle;
  requestInfo.sequence = this->myRequest->getSequence ();
  requestInfo.flags = msgInfo.flags;
  requestInfo.priority = msgInfo.priority;
  (void) memcpy ((caddr_t) & requestInfo.manager_addr,
		 (caddr_t) & msgInfo.manager_addr,
		 sizeof (struct in_addr));
  (void) memcpy ((caddr_t) & requestInfo.rendez_addr,
		 (caddr_t) & msgInfo.rendez_addr,
		 sizeof (struct in_addr));
  requestInfo.rendez_prog = msgInfo.rendez_prog;
  requestInfo.rendez_vers = msgInfo.rendez_vers;
  requestInfo.proto = msgInfo.proto;
  requestInfo.interval = msgInfo.interval;
  requestInfo.count = msgInfo.count;
  (void) strncpy (requestInfo.system,
		  msgInfo.system,
		  sizeof (requestInfo.system));
  (void) strncpy (requestInfo.group,
		  msgInfo.group,
		  sizeof (requestInfo.group));
  (void) strncpy (requestInfo.key,
		  msgInfo.key,
		  sizeof (requestInfo.key));
  requestInfo.length = msgInfo.length;
  requestInfo.last_verified.tv_sec = 0L;
  requestInfo.last_verified.tv_usec = 0L;
  requestInfo.flavor = rqst->rq_cred.oa_flavor;
  this->saveRequestInfo (&requestInfo);

  // clear request argument queues
  
  (void) this->myOptionsQueue.myDestructor ();
  (void) this->mySetargQueue.myDestructor ();
  (void) this->myThreshQueue.myDestructor ();

  // if there are request arguments, initialize the argument queues
  if (msgInfo.length > 0)
    {
      if (!this->initRequestQueues ())
	{
	    return;
	}
      
    }

  // get a rendezvous client handle for sending report or
  // event report messages 
  this->timeout = msgInfo.timeout;

  bool_t gotClntHandle = FALSE; // whether got rendezvous client handle 

  if ((msgInfo.type == NETMGT_ACTION_REQUEST ||
       msgInfo.type == NETMGT_DATA_REQUEST ||
       msgInfo.type == NETMGT_EVENT_REQUEST) &&
      msgInfo.rendez_addr.s_addr)
    {
      NETMGT_PRN (("request: getting rendezvous client for %s\n",
		   inet_ntoa (msgInfo.rendez_addr)));
      if (!myServiceMsg->myClient.newClient (msgInfo.rendez_addr,
					     msgInfo.proto, 
					     msgInfo.rendez_prog,
					     msgInfo.rendez_vers,
					     NETMGT_NO_SECURITY,
					     this->timeout))
	{
	  char message [NETMGT_NAMESIZ];
	  (void) sprintf (message, "program = %u", msgInfo.rendez_prog);
	  _netmgtStatus.setStatus (NETMGT_CANTCREATECLIENT, 
				   0, 
				   clnt_spcreateerror (message));
	  return;
	}
      gotClntHandle = TRUE;
    }

  // increment request sequence number
  this->myRequest->setSequence ();

  // call the agent application's request verification function
  // i.e., the "verify" routine declared by "netmgt_init_rpc_agent"
  if (! (*this->verify) (msgInfo.type,
			 msgInfo.system,
			 msgInfo.group,
			 msgInfo.key,
			 msgInfo.count,
			 msgInfo.interval,
			 msgInfo.flags))
    {
      if (gotClntHandle)
	{
	  // destroy rendezvous client handle 
	  (void) myServiceMsg->myClient.destroyClient ();
	}
      
      return;
    }

  // set agent state to "dispatched request" 
  this->state = NETMGT_DISPATCHED;
  
  // fork a subprocess to handle the management request 
  if (! (this->flags & NETMGT_DONT_FORK))
    {
#ifdef _SVR4_
      pid_t pid ;		// child PID
#else
      int pid ;			// child PID
#endif _SVR4_
      switch (pid = fork ())
	{

	case -1:		// fork failed 

	  if (netmgt_debug)
	    perror ("request: fork failed");
	  _netmgtStatus.setStatus (NETMGT_FORK, 0, strerror (errno));
	  return;

	default:		// parent process 
	  if (gotClntHandle)
	    {
	      // destroy rendezvous client handle - its only
	      // used by the child process
	      (void) myServiceMsg->myClient.destroyClient ();
	    }

	  // cache the request 
	  this->cacheRequest (pid);

	  // return to get another request from a manager
	  return;

	case 0:		// child process
	 mutex_unlock(& update_queues_mutex);
	 // Now kill the thread for SIGCHLD 
	 int thr_status;
	  
	 this->amsl_dispatcher = TRUE;
	 thr_status=thr_kill(tid, SIGHUP);
	  if (thr_status != 0) perror("\n bad thread kill");
	 if((thr_status=thr_join(tid,NULL,NULL)) != 0)
	   perror("\n bad thread join");

	  sigset_t set;
	  sigemptyset(&set);
	  sigaddset(&set, SIGCHLD);
	  sigaddset(&set,SIGHUP);
	  sigaddset(&set,SIGINT);
	  sigaddset(&set,SIGQUIT);
	  sigaddset(&set,SIGTERM);

	  thr_sigsetmask(SIG_UNBLOCK, &set, NULL);

	  
	  
	 
	 
	  // fall though --- we're now in the child process's context
	  break;
	}
    }

  // turn off verification interval timer 
  if (!this->clearItimer ())
    return;

  // create a child object and call its initialization function
  aNetmgtPerformer = (NetmgtPerformer *) calloc (1, sizeof (NetmgtPerformer));
  if (!aNetmgtPerformer)
    {
      NETMGT_PRN (("request: can't create performer object: %s\n",
		   strerror (errno)));
      _netmgtStatus.setStatus (NETMGT_FORK, 0, strerror (errno));
      return;
    }
  if (!aNetmgtPerformer->myConstructor (this))
    {
      (void) cfree ((caddr_t) aNetmgtPerformer);
      return;
    }

  // reset signals 
#ifdef _SVR4_
  struct sigaction vec;		// signal vector 
  struct sigaction ovec;	// old signal vector 
#else
  struct sigvec vec;		// signal vector 
  struct sigvec ovec;		// old signal vector 
#endif

#ifdef _SVR4_
  vec.sa_handler = (SIG_PFV) NULL;
  (void) sigemptyset (&vec.sa_mask);
  vec.sa_flags = 0;
  (void) sigaction (SIGINT, &vec, &ovec);
  (void) sigaction (SIGQUIT, &vec, &ovec);
  (void) sigaction (SIGTERM, &vec, &ovec);
  (void) sigaction (SIGCHLD, &vec, &ovec);
#else
  vec.sv_handler = (SIG_PFV) NULL;
  vec.sv_mask = 0;
  vec.sv_flags = 0;
  (void) sigvec (SIGINT, &vec, &ovec);
  (void) sigvec (SIGQUIT, &vec, &ovec);
  (void) sigvec (SIGTERM, &vec, &ovec);
  (void) sigvec (SIGCHLD, &vec, &ovec);
#endif

  // declare handler for sending deferred reports 
#ifdef _SVR4_
  vec.sa_handler = (SIG_PFV) _netmgt_sendDeferred;
  (void) (&vec.sa_mask);
  vec.sa_flags = 0;
  (void) sigaction (SIGUSR1, &vec, &ovec);
#else
  vec.sv_handler = (SIG_PFV) _netmgt_sendDeferred;
  vec.sv_mask = 0;
  vec.sv_flags = 0;
  (void) sigvec (SIGUSR1, &vec, &ovec);
#endif

  // if this is an error or set report, get error report 
  // arguments from arglist and set management status
  if (msgInfo.type == NETMGT_ERROR_REPORT ||
      msgInfo.type == NETMGT_SET_REPORT)
    {
      NetmgtGeneric aGeneric ; 		// generic argument
      if (!aGeneric.getError (aNetmgtPerformer->myServiceMsg))
	return;
    }

  // call the agent application's request dispatch function
  // i.e., the "dispatch" routine declared by "netmgt_init_rpc_agent"
  if (this->dispatch)
    (*this->dispatch) (msgInfo.type,
		       msgInfo.system,
		       msgInfo.group,
		       msgInfo.key,
		       msgInfo.count,
		       msgInfo.interval,
		       msgInfo.flags);

  // shutdown child process if forked 
  if (!(this->flags & NETMGT_DONT_FORK))
    aNetmgtPerformer->myDestructor ();

  return;
}

// ID fetching routines 

static
bool_t _netmgt_xdr_id_string(XDR *xdrs, char **strp)
{
	return xdr_string(xdrs, strp, 64);
}

static char netmgt_id_dir[] = "/etc/snm.id";

static
int
_netmgt_get_id(char *name)
{
   char *s, *ss, c;
    char *filename;
    FILE *file;
    int slash_count;
    struct stat buf;
    int dir_exists = FALSE;

    // Check to see if it already exists 
    if (stat(netmgt_id_dir, &buf)) {
	if (errno != ENOENT) {
		return 0;
        }
     } else {
	if ((buf.st_mode & S_IFDIR) == 0) {
		unlink(netmgt_id_dir);
	} else {
		dir_exists = TRUE;
	}
    }     

    /* If not, try and create it */
    if (!dir_exists) {
	if (mkdir(netmgt_id_dir, 0777)) {
	    return 0;
   	 }
    }

    /* Fix up user name */
    /* Count slashes and backslashes */
    s = name;
    slash_count = 0;
    while (c = *s++) {
	if ((c == '/')||(c == '\\'))
		slash_count++;
    }

    /* Build magic name */
    filename = (char *)malloc(slash_count + strlen(name) + 
		strlen(netmgt_id_dir) + 2);
    s = filename;

    ss = netmgt_id_dir;
    while (c = *ss++)
	*s++ = c;
    *s++ = '/';

    ss = name;
    while (c = *ss++) {
	if (c == '/') {
		*s++ = '\\';
		*s++ = '0';
	} else if (c == '\\') {	
		*s++ = '\\';
		*s++ = '\\';
	} else {
		*s++ = c;
	}
    }
    *s++ = '\0';

    if (stat(filename, &buf) == 0) {
		utimes(filename, (struct timeval *)NULL);
		free(filename);
		return buf.st_ino;
    }

    if (errno != ENOENT) {
		free(filename);
		return 0;
    }

    /* File doesn't exist, create it and try again */
    file = fopen(filename, "w");
    if (file == NULL) {
		free(filename);
		return 0;
    }
    fclose(file);
    free(filename);
    return _netmgt_get_id(name);
}

/* -----------------------------------------------------------------------
 *  _netmgt_set_request_handle - set request handle
 *	returns TRUE
 * -----------------------------------------------------------------------
 */
bool_t
_netmgt_set_request_handle (u_int handle)
                   		// request handle 
{
  NETMGT_PRN (("message: _netmgt_set_request_handle: %d\n", handle));

  assert (aNetmgtManager != (NetmgtManager *) NULL);
  return aNetmgtManager->setRequestHandle (handle);
}

/* -----------------------------------------------------------------------
 *  _netmgt_get_request_handle - get request handle
 *	returns request handle
 * -----------------------------------------------------------------------
 */
u_int
_netmgt_get_request_handle (void)
{
  NETMGT_PRN (("message: _netmgt_get_request_handle\n"));

  assert (aNetmgtDispatcher != (NetmgtDispatcher *) NULL);
  return aNetmgtDispatcher->myRequest->getRequestHandle ();
}

/* -----------------------------------------------------------------------
 *  _netmgt_get_request_sequence - get request sequence number
 *	returns request sequence number
 * -----------------------------------------------------------------------
 */
u_int
_netmgt_get_request_sequence (void)
{
  NETMGT_PRN (("message: _netmgt_get_request_sequence\n"));

  assert (aNetmgtDispatcher != (NetmgtDispatcher *) NULL);
  return aNetmgtDispatcher->myRequest->getSequence ();
}

/* -----------------------------------------------------------------------
 *  _netmgt_get_report_handle - get report handle
 *	returns report handle
 * -----------------------------------------------------------------------
 */
u_int
_netmgt_get_report_handle (void)
{
  NETMGT_PRN (("message: _netmgt_get_report_handle\n"));

  assert (aNetmgtRendez != (NetmgtRendez *) NULL);
  return aNetmgtRendez->getReportHandle ();
}

/* --------------------------------------------------------------------
 *  _netmgt_lock_file - lock file for exclusive access
 *	returns TRUE if successful; otherwise returns FALSE
 * --------------------------------------------------------------------
 */
bool_t
_netmgt_lock_file (int fd)
				// file descriptor to lock 
{

  NETMGT_PRN (("request: _netmgt_lock_file\n")) ;

  NetmgtObject object;
  return object.lockFile (fd);
}

/* --------------------------------------------------------------------
 *  _netmgt_unlock_file - unlock file
 *	returns TRUE if successful; otherwise returns FALSE
 * --------------------------------------------------------------------
 */
bool_t
_netmgt_unlock_file (int fd)
				// file descriptor to unlock 
{

  NETMGT_PRN (("request: _netmgt_unlock_file\n"));

  NetmgtObject object;
  return object.unlockFile (fd);
}
 

