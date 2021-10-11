#if !defined(lint) && !defined(NOID)
  static char sccsid[] = "@(#)restart.cc	1.42 5/5/91 SMI";
#endif

/**************************************************************************
 *  File:	lib/libnetmgt/restart.cc
 *
 *  Author:	Lynn Monsanto, Sun Microsystems Inc.
 *
 *  SCCSID:     @(#)restart.cc  1.42  91/05/05
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
 *  Comments:	restart request method
 *
 **************************************************************************
 */

/*LINTLIBRARY*/

#include "netmgt/netmgt_hdrs.h"
#include "netmgt/netmgt.h"
#include "libnetmgt/libnetmgt.h"
#include <signal.h>

/* ---------------------------------------------------------------------
 *  NetmgtDispatcher::restartRequests - restart any uncompleted requests
 *      returns TRUE if successful; otherwise returns FALSE
 * ---------------------------------------------------------------------
 */
bool_t
NetmgtDispatcher::restartRequests (void)
{

  NETMGT_PRN (("restart: NetmgtDispatcher::restartRequests\n")) ;

  assert (this != (NetmgtDispatcher *) NULL) ;

  // reset internal error code 
  _netmgtStatus.clearStatus (); 

  int nread ;			// number of items read 
  FILE *file ;			// request cache logfile 
#ifdef _SVR4_
  pid_t pid ;			// subprocess ID 
#else
  int pid ;			// subprocess ID 
#endif _SVR4_
  NetmgtServiceHeader msgInfo ; // request message header
  Netmgt_error error;		// error buffer
  char message [128];		// error message

  // open request logfile for reading 
  file = fopen (this->request_log, "a+") ;
  if (!file)
    {
      NETMGT_PRN (("restart: can't open %s\n", 
		   this->request_log));
      return TRUE;
    }

  (void) rewind (file);
  while (TRUE)
    {

      // read request message header
      nread = fread ((char *) & msgInfo, 
		     1, 
		     sizeof (msgInfo), 
		     file);
      if (nread == 0)
	break;
      if (nread != sizeof (msgInfo))
	{
	  NETMGT_PRN (("restart: %s corrupted\n", this->request_log));
	  (void) fclose (file);
	  return FALSE;
	}

      // set my service message from the logged request header information
      if (!this->myServiceMsg->setHeader (&msgInfo))
	{
	  (void) fclose (file);
	  return FALSE;
	}


      // allocate arglist buffer 
      this->myServiceMsg->myArglist.setBuffer ((caddr_t) NULL, 
					       (u_int) 0, 
					       (off_t) 0);
      if (!this->myServiceMsg->myArglist.alloc (msgInfo.length, 
						NETMGT_MINARGSIZ))
	{
	  (void) fclose (file);
	  return FALSE;
	}

      // allocate argument value buffer 
      this->myServiceMsg->myArglist.myValue1.setBuffer ((caddr_t) NULL, 
							(u_int) 0, 
							(off_t) 0);
      if (!this->myServiceMsg->myArglist.myValue1.alloc (msgInfo.length, 
							 NETMGT_MINARGSIZ))
	{
	  (void) fclose (file);
	  return FALSE;
	}

      // allocate threshold value buffer if this is an event request
      if (this->myServiceMsg->getType () == NETMGT_EVENT_REQUEST)
	{
	  this->myServiceMsg->myArglist.myValue2.setBuffer ((caddr_t) NULL, 
							    (u_int) 0, 
							    (off_t) 0);
	  if (!this->myServiceMsg->myArglist.myValue2.alloc (msgInfo.length, 
							     NETMGT_MINARGSIZ))
	    {
	      (void) fclose (file);
	      return FALSE;
	    }
	}

      // reset the current arglist offset 
      this->myServiceMsg->myArglist.resetPtr ();

      // read request arglist 
      if (msgInfo.length > 0)
	{
	  this->myServiceMsg->myArglist.resetPtr ();
	  nread = fread (this->myServiceMsg->myArglist.getPtr (), 
			 1,
			 (int) msgInfo.length, 
			 file);

	  if (nread != msgInfo.length)
	    {
	      NETMGT_PRN (("restart: %s is corrupted\n", 
			   this->request_log));
	      (void) fclose (file);
	      return FALSE;
	    }
	}

      // skip entry if it's not ours 
      if (msgInfo.agent_prog != this->program)
	continue;

      // save request message information in agent's request 
      // information buffer
      NetmgtRequestInfo requestInfo ;
      
      requestInfo.pid = getpid ();
      requestInfo.request_time = msgInfo.request_time;
      requestInfo.type = msgInfo.type;
      requestInfo.handle = msgInfo.handle;
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
      
      this->saveRequestInfo (&requestInfo);
      
      // clear request argument queues 
      (void) this->myOptionsQueue.myDestructor ();
      (void) this->mySetargQueue .myDestructor ();
      (void) this->myThreshQueue.myDestructor ();
      
      // if there are request arguments, initialize the argument queues
      if (msgInfo.length > 0)
	{
	  if (!this->initRequestQueues ())
	    continue;
	}
      
      // get a rendezvous client handle for sending report or
      // event report messages 
      this->timeout = msgInfo.timeout;
      
      bool_t gotClntHandle = FALSE; // whether got rendezvous client handle 
      
      if ((msgInfo.type == NETMGT_DATA_REQUEST ||
	   msgInfo.type == NETMGT_EVENT_REQUEST) &&
	  msgInfo.rendez_addr.s_addr)
	{
	  if (!myServiceMsg->myClient.newClient (msgInfo.rendez_addr,
						 msgInfo.proto, 
						 msgInfo.rendez_prog,
						 msgInfo.rendez_vers,
						 NETMGT_NO_SECURITY,
						 this->timeout))
	    {
	      _netmgtStatus.setStatus (NETMGT_CANTCREATECLIENT, 0, NULL);
	      continue;
	    }
	  gotClntHandle = TRUE;
	}
      
      // fork a subprocess to handle the management request 
      if (! (this->flags & NETMGT_DONT_FORK))
	{
	  switch (pid = fork ())
	    {
	    case -1:		// fork failed 
	      if (netmgt_debug)
		perror ("request: fork failed");
	      
	      // send a warning message that we couldn't restart
	      // the request
	      error.service_error = NETMGT_WARNING;
	      error.agent_error = 0;
	      (void) sprintf (message, 
			      "Can't create agent subprocess: %s",
			      strerror (errno));
	      error.message = message;
	      (void) netmgt_send_error (& error);
	      return FALSE;
	      
	    default:		// parent process 
	      if (gotClntHandle)
		{
		  // destroy rendezvous client handle - its only
		  // used by the child process
		  (void) myServiceMsg->myClient.destroyClient ();
		}
	      
	      // cache the request 
	      this->cacheRequest (pid);
	      
	      // get another request
	      continue;
	      
	    case 0:		// child process
	      
	      // fall though --- we're now in the child process's context
	      break;
	    }
	}
      
      // turn off verification interval timer 
      if (!this->clearItimer ())
	return FALSE;;
      
      // create a child object and call its initialization function
      aNetmgtPerformer 
	= (NetmgtPerformer *) calloc (1, sizeof (NetmgtPerformer));
      if (!aNetmgtPerformer)
	{
	  // send a warning message that we couldn't restart
	  // the request
	  error.service_error = NETMGT_WARNING;
	  error.agent_error = 0;
	  (void) sprintf (message, 
			  "Can't create agent subprocess: %s",
			  strerror (errno));
	  error.message = message;
	  (void) netmgt_send_error (& error);
	  return FALSE;
	}
      if (!aNetmgtPerformer->myConstructor (this))
	{
	  // send a warning message that we couldn't restart
	  // the request
	  error.service_error = NETMGT_WARNING;
	  error.agent_error = 0;
	  (void) sprintf (message, 
			  "Can't create agent subprocess: %s",
			  strerror (errno));
	  error.message = message;
	  (void) netmgt_send_error (& error);
	  (void) cfree ((caddr_t) aNetmgtPerformer);
	  return FALSE;
	}
      
      // reset signals 
#ifdef _SVR4_
      struct sigaction vec;		// signal vector 
      struct sigaction ovec;		// old signal vector 
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
      
      // declare handler for sending deferred reports 
      vec.sa_handler = (SIG_PFV) _netmgt_sendDeferred;
      (void) sigemptyset (&vec.sa_mask);
      vec.sa_flags = 0;
      (void) sigaction (SIGUSR1, &vec, &ovec);
#else      
      vec.sv_handler = (SIG_PFV) NULL;
      vec.sv_mask = 0;
      vec.sv_flags = 0;
      (void) sigvec (SIGINT, &vec, &ovec);
      (void) sigvec (SIGQUIT, &vec, &ovec);
      (void) sigvec (SIGTERM, &vec, &ovec);
      
      // declare handler for sending deferred reports 
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
	    return FALSE;;
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
      return TRUE;
    }

  (void) fclose (file);
  return TRUE;
}
