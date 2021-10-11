#if !defined(lint) && !defined(NOID)
  static char sccsid[] = "@(#)verify.cc	1.42 5/5/91 SMI";
#endif

/**************************************************************************
 *  File:	lib/libnetmgt/verify.cc
 *
 *  Author:	Lynn Monsanto, Sun Microsystems Inc.
 *
 *  SCCSID:     @(#)verify.cc  1.42  91/05/05
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
 *  Comments:	activity verification routines
 *
 **************************************************************************
 */

/*LINTLIBRARY*/

#include "netmgt/netmgt_hdrs.h"
#include "netmgt/netmgt.h"
#include "libnetmgt/libnetmgt.h"
#include <signal.h>

/* ------------------------------------------------------------------------
 *  NetmgtDispatcher::verifyActivity - verify agent is performing activity
 *	returns TRUE if successful; otherwise returns FALSE
 * ------------------------------------------------------------------------
 */
bool_t
NetmgtDispatcher::verifyActivity (NetmgtControlMsg *aControlMsg)
     // a control message pointer
{

  NETMGT_PRN (("verify: NetmgtDispatcher::verifyActivity\n")) ;

  // clear error status
  _netmgtStatus.clearStatus ();

  // block signals while in critical section 
  sigset_t osigmask = this->blockSignals () ;

  // find request to verify 
  NetmgtQueueNode *currNode ;	// current requst queue node 
  NetmgtRequest *aRequest ;	// request queue data 
  NetmgtRequestInfo requestInfo ; // request information
  bool_t found ;		// whether request was found 
#ifdef _SVR4_ 
  pid_t pid;			// process ID to kill 
#else
  int pid;			// process ID to kill
#endif _SVR4_ 

  found = FALSE ;
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
	  pid = requestInfo.pid;
	  found = TRUE;
	  break;
	}
      currNode = currNode->getNext ();
    }

  // restore signal mask 
  this->unblockSignals (osigmask);

  // did we find the target request ? 
  if (!found)
    {
      NETMGT_PRN (("verify: can't find request\n"));
      _netmgtStatus.setStatus (NETMGT_UNKNOWNREQUEST, 0, NULL);
      return FALSE;
    }

  // verify that the process handling the request is still alive 
  if (kill (pid, 0) == -1 && errno == ESRCH)
    {
      NETMGT_PRN (("verify: PID %d not alive\n", pid));

      // delete the request from the agent request queue 
      (void) this->cleanRequest (requestInfo.pid);
      _netmgtStatus.setStatus (NETMGT_UNKNOWNREQUEST, 0, NULL);
      return FALSE;
    }

  // set verification timestamp 
  if (!aRequest->setLastVerified ())
    {

      // terminate request if we can't get a verification timestamp
      if (kill (pid, SIGTERM) == -1)
	{
	  if (netmgt_debug)
	    perror ("verify: kill failed");
	  _netmgtStatus.setStatus (NETMGT_KILLREQUEST, 0, NULL);
	  return FALSE;
	}
      _netmgtStatus.setStatus (NETMGT_GETTIMEOFDAY, 0, NULL);
      return FALSE;
    }

  return TRUE;
}

/* ---------------------------------------------------------------------
 *  NetmgtRequest::setLastVerified - set time request was verified
 *	returns TRUE if successful; otherwise returns FALSE
 * ---------------------------------------------------------------------
 */
bool_t
NetmgtRequest::setLastVerified (void)
{

  if (gettimeofday (&this->info.last_verified, (struct timezone *)NULL ) == -1)
    {
      if (netmgt_debug)
	perror ("verify: gettimeofday");
      return FALSE;
    }
  NETMGT_PRN (("verify: setting last verified: %d sec. %d usec\n",
	       this->info.last_verified.tv_sec, 
	       this->info.last_verified.tv_usec));
  return TRUE;
}

/* ---------------------------------------------------------------------
 *  NetmgtDispatcher::setItimer - set verification interval timer
 *	returns TRUE if successful; otherwise returns FALSE
 * ---------------------------------------------------------------------
 */
bool_t
NetmgtDispatcher::setItimer (void)
{

  NETMGT_PRN (("verify: NetmgtDispatcher::_setItimer\n"));

  // declare request verification signal handler 
#ifdef _SVR4_
  struct sigaction vec;		// signal vector 
  struct sigaction ovec;	// previous signal vector 
#else
  struct sigvec vec;		// signal vector 
  struct sigvec ovec;		// previous signal vector 
#endif

#ifdef _SVR4_
  vec.sa_handler = (SIG_PFV) _netmgt_killUnverified;
  (void) sigemptyset (&vec.sa_mask);
  vec.sa_flags = 0;
  if (sigaction (SIGALRM, &vec, &ovec) == -1)
    {
      if (netmgt_debug)
	perror ("verify: sigaction");
      return FALSE;
    }
#else
  vec.sv_handler = (SIG_PFV) _netmgt_killUnverified;
  vec.sv_mask = 0;
  vec.sv_flags = 0;
  if (sigvec (SIGALRM, &vec, &ovec) == -1)
    {
      if (netmgt_debug)
	perror ("verify: sigvec");
      return FALSE;
    }
#endif

  // start interval timer 
  struct itimerval value;	// interval timer value 
  value.it_interval.tv_sec = NETMGT_VERIFY_TIME;
  value.it_interval.tv_usec = 0;
  value.it_value.tv_sec = NETMGT_VERIFY_TIME;
  value.it_value.tv_usec = 0;
  if (setitimer (ITIMER_REAL, &value, (struct itimerval *) NULL) == -1)
    {
      if (netmgt_debug)
	perror ("verify: setitimer");
      return FALSE;
    }

  return TRUE;
}

/* ---------------------------------------------------------------------
 *  NetmgtDispatcher::clearItimer - clear verification interval timer
 *	returns TRUE if successful; otherwise returns FALSE
 * ---------------------------------------------------------------------
 */
bool_t
NetmgtDispatcher::clearItimer (void)
{
  struct itimerval value;	// interval timer value 

  NETMGT_PRN (("verify: NetmgtDispatcher::clearItimer\n"));

  // stop interval timer 
  value.it_interval.tv_sec = 0;
  value.it_interval.tv_usec = 0;
  value.it_value.tv_sec = 0;
  value.it_value.tv_usec = 0;
  if (setitimer (ITIMER_REAL, &value, (struct itimerval *) NULL) == -1)
    {
      if (netmgt_debug)
	perror ("verify: setitimer");
      return FALSE;
    }

  return TRUE;
}

/* -------------------------------------------------------------------------
 *  _netmgt_killUnverified - C wrapper for NetmgtDispatcher::killUnverified
 *	no return value
 * -------------------------------------------------------------------------
 */
void
#ifdef _SVR4_
_netmgt_killUnverified (int sig)
#else
_netmgt_killUnverified (int sig,
			int code, 
			struct sigcontext *scp, 
			char *addr)
#endif _SVR4_
     // signal caught 
     // additional parameters 
     // signal context 
     // additional address information 
{

  NETMGT_PRN (("verify: _netmgt_killUnverified\n"));

  assert (aNetmgtDispatcher != (NetmgtDispatcher *) NULL);
#ifdef _SVR4_
  aNetmgtDispatcher->killUnverified (sig);
  return;
#else
  return aNetmgtDispatcher->killUnverified (sig, code, scp, addr);
#endif _SVR4_
}

/* -----------------------------------------------------------------------
 *  NetmgtDispatcher::killUnverified - terminate activity if not verified
 *	no return value
 * -----------------------------------------------------------------------
 */
void
#ifdef _SVR4_
NetmgtDispatcher::killUnverified (int sig)
#else
NetmgtDispatcher::killUnverified (int sig, 
				  int code, 
				  struct sigcontext *scp, 
				  char *addr)
#endif _SVR4_
     // signal caught 
     // additional parameters 
     // signal context 
     // additional address information 
{

  NETMGT_PRN (("verify: NetmgtDispatcher::killUnverified\n"));

  // block signals while in critical section 
  sigset_t osigmask = this->blockSignals ();

  // get current time 
  struct timeval currTime;
  if (gettimeofday (&currTime, (struct timezone *)NULL ) == -1)
    {
      if (netmgt_debug)
	perror ("verify: gettimeofday");
      return;
    }

  // find request to verify 
  bool_t found = FALSE;		// whether unverified request found
  NetmgtRequest *aRequest;	// request queue data 
  NetmgtRequestInfo requestInfo ; // request information

  NetmgtQueueNode *currNode = this->myRequestQueue.getHead ();
  while (currNode)
    {

      // get request information pointer
      assert (currNode->isData ());
      aRequest = (NetmgtRequest *) currNode->getData ();

      // get request information
      aRequest->getRequestInfo (&requestInfo) ;

      // mark request for deletion if it hasn't been
      // verified within the last NETMGT_UNVERIFIED_TIME
      // seconds 
      NETMGT_PRN (("verify: current time: %d sec. %d usec\n",
		   currTime.tv_sec, 
		   currTime.tv_usec));
      NETMGT_PRN (("verify: checking last verified: %d sec. %d usec\n",
		   requestInfo.last_verified.tv_sec, 
		   requestInfo.last_verified.tv_usec));

      if (currTime.tv_sec
	  - requestInfo.last_verified.tv_sec > NETMGT_UNVERIFIED_TIME)
	{
	  currNode->markStale ();
	  found = TRUE;
	}

      currNode = currNode->getNext ();
    }

  // return if there are no unverified requests 
  if (!found)
    {
      // restore signal mask 
      this->unblockSignals (osigmask);
      return;
    }

  // delete unverified requests 
  do
  {
    // block signals while in critical section 
    osigmask = this->blockSignals ();

    found = FALSE;
    currNode = this->myRequestQueue.getHead ();
    while (currNode)
      {

	// get request data 
	assert (currNode->isData ());
	aRequest = (NetmgtRequest *) currNode->getData ();

	// terminate request marked for deletion 
	if (currNode->isStale ())
	  {
	    found = TRUE;
	    break;
	  }

	currNode = currNode->getNext ();
      }

    // restore signal mask 
    this->unblockSignals (osigmask);

    if (found)
      {
	NETMGT_PRN (("verify: killing PID %d\n", requestInfo.pid));
	if (kill (requestInfo.pid, SIGTERM) == -1 &&
	    netmgt_debug)
	  perror ("verify: kill");
      }

    // clean up request 
    if (found)
      (void) this->cleanRequest (requestInfo.pid);

  } while (found) ;

  return;
}

