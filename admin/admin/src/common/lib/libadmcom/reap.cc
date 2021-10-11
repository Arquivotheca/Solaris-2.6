#if !defined(lint) && !defined(NOID)
  static char sccsid[] = "@(#)reap.cc	1.44 5/5/91 SMI";
#endif

/**************************************************************************
 *  File:	lib/libnetmgt/reap.cc
 *
 *  Author:	Lynn Monsanto, Sun Microsystems Inc.
 *
 *  SCCSID:     @(#)reap.cc  1.44  91/05/05
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
 *  Comments:	routines to reap agent child process
 *
 **************************************************************************
 */

/*LINTLIBRARY*/

#include "netmgt/netmgt_hdrs.h"
#include "netmgt/netmgt.h"
#include "libnetmgt/libnetmgt.h"

/*****************/
#include <stdio.h>
 
#include <sys/signal.h>
#include <thread.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/time.h>
 

void * NetmgtDispatcher::idleTimer(NetmgtDispatcher * This)
{
    timestruc_t timeOut;
    int error_status;
    
    
    /* This routine already has the mutex from threadReapRequest thread */
 
    timeOut.tv_sec = time(NULL) + This->idletime;
    timeOut.tv_nsec = 0;
    mutex_lock(&This->update_queues_mutex);
    error_status = cond_timedwait(&This->idle_cond_var, &This->update_queues_mutex, &timeOut);
    if ((error_status == ETIME) && (This->myRequestQueue.getLength () == 0 ))
      exit(0);
    mutex_unlock(&This->update_queues_mutex);
    
    return(NULL);
}


/* ---------------------------------------------------------------
 *  _netmgt_get_child_sequence - get sequence number of reaped
 *	request
 *	returns request sequence number
 * ---------------------------------------------------------------
 */
u_int
_netmgt_get_child_sequence (void)
{

  NETMGT_PRN (("agent: _netmgt_get_child_sequence\n"));
  assert (aNetmgtDispatcher != (NetmgtDispatcher *) NULL);
  return aNetmgtDispatcher->getChildSequence ();
}


// This is declared as a static function ...
void * NetmgtDispatcher::threadReapRequest (NetmgtDispatcher * This)
{


#ifdef _SVR4_
  pid_t childPid ;		// child's pid 
  int stat_loc ;		// child status location
  caddr_t rusage ;		// fake resource usage
#else
  int childPid ;		// child's pid 
  union wait status ;		// child status 
  struct rusage rusage ;	// child resource usage 
#endif

 sigset_t set;
  int ret_signal;

  
  
  NETMGT_PRN (("reap: NetmgtDispatcher::reapRequest\n")) ;

  sigemptyset(&set);
  sigaddset(&set, SIGCHLD);
  sigaddset(&set,SIGHUP);
  sigaddset(&set,SIGINT);
  sigaddset(&set,SIGQUIT);
  sigaddset(&set,SIGTERM);
  

  
  // reap all deceased child processes 
 for (;;)
   {
       ret_signal = sigwait(&set);
       switch ( ret_signal){
	 case SIGCHLD:
	   mutex_lock(& This->update_queues_mutex);
	   rusage = (caddr_t)NULL;
	   while ((childPid = waitpid((pid_t)-1, & stat_loc, WNOHANG)) > 0){
		     NETMGT_PRN (("reap: reaping child PID %d; from thread \n", 
			 childPid));

		     // clean up request 
		     (void) This->cleanRequest (childPid);

		     // call agent-specific SIGCHLD handler if one is declared 
		     if (This->reap_child)
#ifdef _SVR4_
		       (*This->reap_child) (SIGCHLD, 
			   childPid, 
			   & stat_loc,
			   rusage);
#else
		       (*This->reap_child) (SIGCHLD, 
			   code, 
			   scp, 
			   addr, 
			   childPid, 
			   & status,
			   & rusage);
#endif _SVR4_



		 };

           // if there are no more active requests, the agent was
           // started by inetd and the NETMGT_DONT_EXIT flag is not set
           // set idletime seconds for alarm and termination.
           // idletime == 0 signifies run forever


	    if (This->myRequestQueue.getLength () == 0 &&
	       This->flags & NETMGT_STARTED_BY_INETD && 
	       !(This->flags & NETMGT_DONT_EXIT))
	       {
		   NETMGT_PRN (("reap: no active requests, setting idle alarm ...\n"));
      
		   if(This->idletime > 0){
		       if((thr_create(NULL, NULL, (void * (*)(void *)) NetmgtDispatcher::idleTimer, This, THR_DETACHED, NULL)) != 0)
			 perror("error creating idle timer thread:");
		   }
		   



	       }
	   
	   mutex_unlock( & This->update_queues_mutex);
	       break;
	 case SIGHUP:
	   if (This->amsl_dispatcher)
	     thr_exit(NULL);
	   break;
	   

	 case SIGINT:
	   
	 case SIGQUIT:
	   
	 case SIGTERM:
	   if (This->shutdown)
	     (*This->shutdown)(ret_signal);
	   else
	     netmgt_shutdown_agent(ret_signal);
	   
	   break;
	 default:
	   break;
       }
   }

  return(NULL);
  
}

  

  
  

/* --------------------------------------------------------------------
 *  NetmgtDispatcher::cleanRequest - cleanup netmgt request
 *	returns TRUE if successful; otherwise returns FALSE
 * --------------------------------------------------------------------
 */
bool_t
#ifdef _SVR4_
NetmgtDispatcher::cleanRequest (pid_t childPid)
#else
NetmgtDispatcher::cleanRequest (int childPid)
#endif _SVR4_
                   		// child pid to clean 
{

  NETMGT_PRN (("reap: NetmgtDispatcher::cleanRequest (%d)\n", childPid));
  assert (aNetmgtDispatcher != (NetmgtDispatcher *) NULL);

  // find request to delete 
  NetmgtQueueNode *currNode;	// current request queue node 
  NetmgtRequest *request;	// request queue data 
  NetmgtRequestInfo requestInfo ; // request information
  bool_t found;			// whether request was found 

  found = FALSE;
  currNode = this->myRequestQueue.getHead ();
  while (currNode)
    {

      // get request data 
      assert (currNode->isData ());
      request = (NetmgtRequest *) currNode->getData ();

      // get request information
      request->getRequestInfo (&requestInfo);

      // look for child PID match 
      if (requestInfo.pid == childPid)
	{
	  // remember request handle
	  this->child_sequence = requestInfo.sequence;

	  found = TRUE;
	  break;
	}

      currNode = currNode->getNext ();
    }
  if (!found)
    return FALSE;

  // delete request from the request logfile 
  if (requestInfo.flags & NETMGT_RESTART)
    (void) this->deleteRequest (request);

  // create a control message and send it to the activity daemon
  // if the activity was cached with the activity daemon
  if ((requestInfo.type == NETMGT_DATA_REQUEST ||
       requestInfo.type == NETMGT_EVENT_REQUEST) &&
      (requestInfo.count == 0 || requestInfo.count > 1))
    {
      NetmgtControlMsg *aControlMsg ;
      aControlMsg = (NetmgtControlMsg *) calloc (1, sizeof (NetmgtControlMsg));
      if (!aControlMsg)
	{
	  if (netmgt_debug)
	    perror ("reap: calloc failed");
	  _netmgtStatus.setStatus (NETMGT_MALLOC, 0, strerror (errno));
	  return FALSE;
	}
      (void) aControlMsg->uncacheActivity (this, request);
      (void) cfree ((caddr_t) aControlMsg);
    }

  // free the request queue node - this will also free the queue node data
  return this->myRequestQueue.remove (currNode);

}

/* --------------------------------------------------------------------
 *  NetmgtDispatcher::deleteRequest - delete request logfile entry
 *	returns TRUE if successful; otherwise return FALSE
 * --------------------------------------------------------------------
 */
bool_t
NetmgtDispatcher::deleteRequest (NetmgtRequest *aRequest)
                             	// request node to delete 
{

  NETMGT_PRN (("reap: NetmgtDispatcher::deleteRequest\n"));

  assert (aRequest != (NetmgtRequest *) NULL);

  // open two request logfile streams 
  FILE *front;			// leading logfile stream 
  FILE *rear;			// trailing logfile stream 

#ifdef _SVR4_
  mode_t oumask = umask ((mode_t)0);
#else
  int oumask = umask (0);
#endif _SVR4_

  front = fopen (this->request_log, "a+");
  if (!front)
    {
      NETMGT_PRN (("reap: can't open %s\n", this->request_log));
      return FALSE;
    }
  rear = fopen (this->request_log, "a+");
  if (!rear)
    {
      NETMGT_PRN (("reap: can't open %s\n", this->request_log));
      (void) fclose (front);
      return FALSE;
    }
  (void) umask (oumask);

  // lock logfile and block signals while updating 
  if (!this->lockFile (fileno (front)))
    {
      (void) fclose (front);
      (void) fclose (rear);
      return FALSE;
    }
  sigset_t osigmask = this->blockSignals ();

  // read request logfile entries from `leading stream' and
  // write active logfile entries to `trailing stream' 
  NetmgtServiceHeader msgInfo ;	  // request message header information
  NetmgtBuffer arglist ;	  // cached message argument list
  NetmgtRequestInfo requestInfo ; // request information
  int nread;			  // #items read 
  int nwrite;			  // #items written 

  rewind (front);
  rewind (rear);
  while (TRUE)
    {

      // read message header information from `leading stream' 
      nread = fread ((char *) & msgInfo, 
		     1, 
		     sizeof (msgInfo), 
		     front);
      if (nread == 0)
	break;
      if (nread != sizeof (msgInfo))
	{
	  NETMGT_PRN (("reap: %s corrupted\n", 
		       this->request_log));
	  (void) this->unlockFile (fileno (front));
	  (void) fclose (front);
	  (void) fclose (rear);
	  this->unblockSignals (osigmask);
	  return FALSE;
	}


      // (re)allocate argument list buffer if necessary 
      arglist.setBuffer ((caddr_t) NULL, (u_int) 0, (off_t) 0);
      if (!arglist.alloc (msgInfo.length, NETMGT_MINARGSIZ))
	{
	  (void) this->unlockFile (fileno (front));
	  (void) fclose (front);
	  (void) fclose (rear);
	  this->unblockSignals (osigmask);
	  return FALSE;
	}

      // read request arglist from `leading stream' 
      arglist.resetPtr ();
      nread = fread (arglist.getPtr (), 
		     1, 
		     (int) msgInfo.length, 
		     front);

      if (nread != msgInfo.length)
	{
	  NETMGT_PRN (("reap: %s corrupted\n", 
		       this->request_log));
	  (void) this->unlockFile (fileno (front));
	  (void) fclose (front);
	  (void) fclose (rear);
	  (void) cfree ((caddr_t) arglist.getBase ());
	  this->unblockSignals (osigmask);
	  return FALSE;
	}

      // get request message information
      aRequest->getRequestInfo (&requestInfo);

      // skip logfile record to delete 
      if (memcmp ((caddr_t) & msgInfo.request_time,
		  (caddr_t) & requestInfo.request_time,
		  sizeof (struct timeval)) == 0 &&
	  memcmp ((caddr_t) & msgInfo.manager_addr,
		  (caddr_t) & requestInfo.manager_addr,
		  sizeof (struct in_addr)) == 0)
	continue;

      // write request message header information to `trailing stream' 
      nwrite = fwrite ((char *) &msgInfo, 
		       1, 
		       sizeof (msgInfo), 
		       rear);
      if (nwrite != sizeof (msgInfo))
	{
	  NETMGT_PRN (("reap: can't write to %s\n", 
		       this->request_log));
	  (void) this->unlockFile (fileno (front));
	  (void) fclose (front);
	  (void) fclose (rear);
	  (void) cfree ((caddr_t) arglist.getBase ());
	  this->unblockSignals (osigmask);
	  return FALSE;
	}

      // write request message arglist to `trailing stream' 
      arglist.resetPtr ();
      nwrite = fwrite (arglist.getPtr (), 
		       1, 
		       (int) msgInfo.length, 
		       rear);
      if (nwrite != msgInfo.length)
	{
	  NETMGT_PRN (("reap: can't write to %s\n", 
		       this->request_log));
	  (void) this->unlockFile (fileno (front));
	  (void) fclose (front);
	  (void) fclose (rear);
	  (void) cfree ((caddr_t) arglist.getBase ());
	  this->unblockSignals (osigmask);
	  return FALSE;
	}
    }

  // free temporary arglist buffer
  (void) cfree ((caddr_t) arglist.getBase ());

  // truncate request logfile to new length 
  off_t length;			// new request logfile length 

  length = ftell (rear);
  if (ftruncate (fileno (rear), length) == -1)
    {
      NETMGT_PRN (("reap: can't truncate %s to %d bytes: %s\n",
		   this->request_log, length, strerror (errno)));
      (void) this->unlockFile (fileno (front));
      (void) fclose (front);
      (void) fclose (rear);
      this->unblockSignals (osigmask);
      return FALSE;
    }

  (void) this->unlockFile (fileno (front));
  (void) fclose (front);
  (void) fclose (rear);
  this->unblockSignals (osigmask);

  return TRUE;
}
