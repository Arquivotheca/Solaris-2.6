#if !defined(lint) && !defined(NOID)
  static char sccsid[] = "@(#)cache.cc	1.42 5/5/91 SMI";
#endif

/**************************************************************************
 *  File:	lib/libnetmgt/cache.cc
 *
 *  Author:	Lynn Monsanto, Sun Microsystems Inc.
 *
 *  SCCSID:     @(#)cache.cc  1.42  91/05/05
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
 *  Comments:	agent request caching routines
 *
 **************************************************************************
 */

/*LINTLIBRARY*/

#include "netmgt/netmgt_hdrs.h"
#include "netmgt/netmgt.h"
#include "libnetmgt/libnetmgt.h"

/* -------------------------------------------------------------------
 *  NetmgtDispatcher::cacheRequest - cache request message
 *	no return value
 * -------------------------------------------------------------------
 */
void
#ifdef _SVR4_
NetmgtDispatcher::cacheRequest (pid_t childPid)
#else
NetmgtDispatcher::cacheRequest (int childPid)
#endif _SVR4_
                    		// agent dispatcher PID 
{

  NETMGT_PRN (("cache: NetmgtDispatcher::cacheRequest %d\n", childPid)) ;

  // append request message to agent request queue - terminate
  // the request if this fails 
  if (!this->appendRequest (childPid))
    {
      NETMGT_PRN (("cache: can't append to request queue ; killing %d\n", 
		   childPid));
      if (kill (childPid, SIGTERM) == -1)
	if (netmgt_debug)
	  perror ("cache: kill failed");
      return;
    }

  // get request message header
  NetmgtServiceHeader header;		// request message header
  if (!this->myServiceMsg->getHeader (&header))
    {
      if (kill (childPid, SIGTERM) == -1)
	if (netmgt_debug)
	  perror ("cache: kill failed");
      return;
    }

  // create a control message and tell the activity daemon to
  // cache the request in its activity logfile
  if ((header.type == NETMGT_DATA_REQUEST ||
       header.type == NETMGT_EVENT_REQUEST) &&
      (header.count == 0 || header.count > 1))
    {
      NetmgtControlMsg *aControlMsg ;
      aControlMsg = (NetmgtControlMsg *) calloc (1, sizeof (NetmgtControlMsg));
      if (!aControlMsg)
	{
	  if (netmgt_debug)
	    perror ("cache: calloc failed");
	  _netmgtStatus.setStatus (NETMGT_MALLOC, 0, strerror (errno));
	  if (kill (childPid, SIGTERM) == -1)
	    if (netmgt_debug)
	      perror ("cache: kill failed");
	  return;
	}

      if (!aControlMsg->cacheActivity (this))
	{
	  // kill the request is the request fails
	  NETMGT_PRN (("cache: activity daemon won't cache request: "));
	  NETMGT_PRN (("killing %d\n", childPid));
	  if (kill (childPid, SIGTERM) == -1)
	    if (netmgt_debug)
	      perror ("cache: kill failed");
	  (void) cfree ((caddr_t) aControlMsg);
	  return;
	}
      (void) cfree ((caddr_t) aControlMsg);
    }

  return;
}

/* ----------------------------------------------------------------------
 *  NetmgtDispatcher::appendRequest - append request to request queue
 *	returns TRUE if successful; otherwise returns FALSE
 * ----------------------------------------------------------------------
 */
bool_t
#ifdef _SVR4_
NetmgtDispatcher::appendRequest (pid_t childPid) 
#else
NetmgtDispatcher::appendRequest (int childPid)
#endif _SVR4_ 
				// agent performer PID
{

  NETMGT_PRN (("cache: NetmgtDispatcher::appendRequest\n"));

  // reset internal error buffer 
  _netmgtStatus.clearStatus ();

  // allocate and initialize request queue node data 
  NetmgtRequest *request;	// request queue data pointer 

  request = (NetmgtRequest *) calloc (1, sizeof (NetmgtRequest));
  if (!request)
    {
      if (netmgt_debug)
	perror ("cache: calloc failed");
      _netmgtStatus.setStatus (NETMGT_MALLOC, 0, strerror (errno));
      return FALSE;
    }

  // copy request information into temporary buffer so we can
  // initialize a couple fields
  NetmgtRequestInfo requestInfo ; 
  this->getRequestInfo (&requestInfo);

  // save performer subprocess pid
  requestInfo.pid = childPid;

  // initialize activity verification time
  if (gettimeofday (&requestInfo.last_verified, (struct timezone *)NULL) == -1)
    {
      if (netmgt_debug)
	perror ("cache: gettimeofday");
      _netmgtStatus.setStatus (NETMGT_GETTIMEOFDAY, 0, strerror (errno));
      (void) cfree ((caddr_t) request);
      return FALSE;
    }

  // tell request object to save request information
  request->saveRequestInfo (&requestInfo);  

  // append new request information to agent request queue 
  if (!this->myRequestQueue.append ((caddr_t) request, FALSE))
    {
      (void) cfree ((caddr_t) request);
      return FALSE;
    }

  // log request to request file if the request restarting
  // is specified and the agent is already initialized 
  if (requestInfo.flags & NETMGT_RESTART &&
      this->state != NETMGT_INITIALIZING)
    {
      if (!this->logRequest ())
	{
#ifdef notdef
	  (void) cfree ((caddr_t) request);
#endif notdef
	  return FALSE;
	}
    }
  return TRUE;
}

/* -----------------------------------------------------------------------
 *  NetmgtDispatcher::logRequest - append request message to request file
 * 	returns TRUE if successful; otherwise returns FALSE
 * -----------------------------------------------------------------------
 */
bool_t
NetmgtDispatcher::logRequest (void)
{

  NETMGT_PRN (("cache: NetmgtDispatcher::logRequest\n"));

  // reset internal error buffer 
  _netmgtStatus.clearStatus ();

  // get request information from request message header
  NetmgtServiceHeader msgInfo;
  if (!this->myServiceMsg->getHeader (&msgInfo))
    return FALSE;

  // open and lock netmgt request cache file --- display warning
  // but don't fail if can't open request file for appending 
#ifdef _SVR4_
  mode_t oumask;		// previous umask 
  oumask = umask ((mode_t)0);
#else
  int oumask;			// previous umask 
  oumask = umask (0);
#endif _SVR4_

  FILE *file;			// request cache file stream 
  file = fopen (this->request_log, "a+");
  (void) umask (oumask);
  if (!file)
    {
      NETMGT_PRN (("cache: can't fopen %s\n", this->request_log));
      _netmgtStatus.setStatus (NETMGT_NOREQUESTLOG, 0, NULL);
      return FALSE;
    }

  if (!this->lockFile (fileno (file)))
    {
      (void) fclose (file);
      _netmgtStatus.setStatus (NETMGT_NOREQUESTLOG, 0, NULL);
      return FALSE;
    }

  // write request message header 
  int nwrite;			// #items written 
  nwrite = fwrite ((char *) & msgInfo, 
		   1, 
		   sizeof (msgInfo), 
		   file);
  if (nwrite != sizeof (msgInfo))
    {
      NETMGT_PRN (("cache: fwrite failed to %s\n", this->request_log));
      (void) this->unlockFile (fileno (file));
      (void) fclose (file);
      _netmgtStatus.setStatus (NETMGT_NOREQUESTLOG, 0, NULL);
      return FALSE;
    }

  // write request argument list 
  if (this->myServiceMsg->isData ())
    {
      this->myServiceMsg->myArglist.resetPtr ();
      nwrite = fwrite (this->myServiceMsg->myArglist.getPtr (), 
		       1,
		       (int) msgInfo.length, 
		       file);
      if (nwrite != msgInfo.length)
	{
	  NETMGT_PRN (("cache: fwrite failed to %s\n", this->request_log));
	  (void) this->unlockFile (fileno (file));
	  (void) fclose (file);
	  _netmgtStatus.setStatus (NETMGT_NOREQUESTLOG, 0, NULL);
	  return FALSE;
	}
    }

  // close and unlock the request log 
  (void) this->unlockFile (fileno (file));
  (void) fclose (file);
  NETMGT_PRN (("cache: logged request to %s\n", this->request_log));

  return TRUE;
}

/* ------------------------------------------------------------------
 *  NetmgtDispatcher::logActivity - log activity in activity logfile
 *      returns TRUE if successful; otherwise returns FALSE
 * ------------------------------------------------------------------
 */
bool_t
NetmgtDispatcher::logActivity (NetmgtControlMsg *aControlMsg)
				// activity control message
{

  NETMGT_PRN (("cache: NetmgtDispatcher::logActivity\n"));

  assert (this->activity_log != (char *) NULL);

  // clear error status
  _netmgtStatus.clearStatus ();

  // open and lock activity log file 
#ifdef _SVR4_
  mode_t oumask;		// previous umask 
  oumask = umask ((mode_t)0);
#else
  int oumask;			// previous umask 
  oumask = umask (0);
#endif _SVR4_
  FILE *logfile;		// activity logfile stream 

  logfile = fopen (this->activity_log, "a+");
  (void) umask (oumask);
  if (!logfile)
    {
      NETMGT_PRN (("cache: can't open %s\n", this->activity_log));
      _netmgtStatus.setStatus (NETMGT_INSERTACTIVITY, 0, NULL);
      return FALSE;
    }

  // get control message data
  Netmgt_activity activity ; 	// activity record
  if (!aControlMsg->control2activity (&activity))
    {
      (void) fclose (logfile);
      _netmgtStatus.setStatus (NETMGT_INSERTACTIVITY, 0, NULL);
      return FALSE;
    }

  // lock file for updating 
  if (!this->lockFile (fileno (logfile)))
    {
      (void) fclose (logfile);
      _netmgtStatus.setStatus (NETMGT_INSERTACTIVITY, 0, NULL);
      return FALSE;
    }

  // check whether entry is already in the logfile 
  if (this->inLogfile (logfile, aControlMsg))
    {
      (void) this->unlockFile (fileno (logfile));
      (void) fclose (logfile);
      return TRUE;
    }

  // append activity record to logfile 
  int nwrite;			// #items written 

  (void) fseek (logfile, 0L, 2);
  nwrite = fwrite ((caddr_t) & activity, 
		   1, 
		   sizeof (Netmgt_activity),
		   logfile);
  if (nwrite != sizeof (Netmgt_activity))
    {
      NETMGT_PRN (("cache: can't write to %s\n", this->activity_log));
      (void) this->unlockFile (fileno (logfile));
      (void) fclose (logfile);
      _netmgtStatus.setStatus (NETMGT_INSERTACTIVITY, 0, NULL);
      return FALSE;
    }

  (void) this->unlockFile (fileno (logfile));
  (void) fclose (logfile);
  return TRUE;
}

/* ----------------------------------------------------------------------
 *  NetmgtDispatcher::deleteActivity - delete activity logfile entry
 *	returns TRUE if successful; otherwise returns TRUE
 * ----------------------------------------------------------------------
 */
bool_t
NetmgtDispatcher::deleteActivity (NetmgtControlMsg *aControlMsg)
				// activity control message
{

  NETMGT_PRN (("cache: NetmgtDispatcher::deleteActivity\n"));

  assert (this->activity_log != (char *) NULL);

  // clear error status
  _netmgtStatus.clearStatus ();

  // open two activity logfile streams 
#ifdef _SVR4_
  mode_t oumask;		// previous umask 
  oumask = umask ((mode_t)0);
#else
  int oumask;			// previous umask 
  oumask = umask (0);
#endif _SVR4_

  FILE *front;			// leading logfile stream 
  front = fopen (this->activity_log, "a+");
  if (!front)
    {
      NETMGT_PRN (("cache: can't open %s\n", this->activity_log));
      _netmgtStatus.setStatus (NETMGT_DELETEACTIVITY, 0, NULL);
      return FALSE;
    }
  FILE *rear;			// trailing logfile stream 
  rear = fopen (this->activity_log, "a+");
  if (!rear)
    {
      NETMGT_PRN (("cache: can't open %s\n", this->activity_log));
      (void) fclose (front);
      _netmgtStatus.setStatus (NETMGT_DELETEACTIVITY, 0, NULL);
      return FALSE;
    }
  (void) umask (oumask);

  // lock logfile and block signals while updating 
  if (!this->lockFile (fileno (front)))
    {
      (void) fclose (front);
      (void) fclose (rear);
      _netmgtStatus.setStatus (NETMGT_DELETEACTIVITY, 0, NULL);
      return FALSE;
    }
  sigset_t osigmask;		// previous signal mask 
  osigmask = this->blockSignals ();

  // read activity logfile entries from `leading stream' and
  // write active logfile entries to `trailing stream' 
  Netmgt_activity record;       // activity logfile record 
  int nread;			// #items read 
  int nwrite;			// #items written 
  off_t length;			// new activity logfile length 

  rewind (front);
  rewind (rear);
  while (TRUE)
    {

      // read logfile record from `leading stream' 
      nread = fread ((char *) &record, 1, sizeof (record), front);
      if (nread == 0)
	break;
      if (nread != sizeof (record))
	{
	  NETMGT_PRN (("cache: %s corrupted\n", 
		       this->activity_log));
	  (void) this->unlockFile (fileno (front));
	  (void) fclose (front);
	  (void) fclose (rear);
	  this->unblockSignals (osigmask);
	  _netmgtStatus.setStatus (NETMGT_DELETEACTIVITY, 0, NULL);
	  return FALSE;
	}

      // skip logfile record to delete 
      if (memcmp ((caddr_t) & record.request_time,
		  (caddr_t) aControlMsg->getRequestTime (),
		  sizeof (struct timeval)) == 0 &&
	  memcmp ((caddr_t) & record.manager_addr,
		  (caddr_t) aControlMsg->getManagerAddr (),
		  sizeof (struct in_addr)) == 0)
	continue;

      // write active record to `trailing stream' 
      nwrite = fwrite ((caddr_t) & record, 1, sizeof (record), rear);
      if (nwrite != sizeof (record))
	{
	  NETMGT_PRN (("cache: can't write to %s\n", 
		       this->activity_log));
	  (void) this->unlockFile (fileno (front));
	  (void) fclose (front);
	  (void) fclose (rear);
	  this->unblockSignals (osigmask);
	  _netmgtStatus.setStatus (NETMGT_DELETEACTIVITY, 0, NULL);
	  return FALSE;
	}
    }

  // truncate activity logfile to new length 
  length = ftell (rear);
  if (ftruncate (fileno (rear), length) == -1)
    {
      NETMGT_PRN (("cache: can't truncate %s to %d : %s\n",
		   this->activity_log, length, strerror (errno)));
      (void) this->unlockFile (fileno (front));
      (void) fclose (front);
      (void) fclose (rear);
      this->unblockSignals (osigmask);
      _netmgtStatus.setStatus (NETMGT_DELETEACTIVITY, 0, NULL);
      return FALSE;
    }

  (void) this->unlockFile (fileno (front));
  (void) fclose (front);
  (void) fclose (rear);
  this->unblockSignals (osigmask);

  return TRUE;
}

/* ---------------------------------------------------------------------
 *  NetmgtDispatcher::inLogfile - check whether activity is in logfile
 *	returns TRUE if in logfile; otherwise returns NULL
 * ---------------------------------------------------------------------
 */
bool_t
NetmgtDispatcher::inLogfile (FILE *logfile, NetmgtControlMsg *aControlMsg)
                   		// logfile stream 
				// activity control message
{

  NETMGT_PRN (("cache: NetmgtDispatcher::inLogfile\n"));

  (void) fseek (logfile, 0L, 0);

  Netmgt_activity record;	// activity logfile record 
  int nread;			// #items read 
  while (TRUE)
    {
      nread = fread ((char *) &record, 1, sizeof (record), logfile);
      if (nread != sizeof (record))
	return FALSE;

      if (memcmp ((caddr_t) & record.request_time, 
		  (caddr_t) aControlMsg->getRequestTime (),
		  sizeof (struct timeval)) == 0 &&
	  memcmp ((caddr_t) & record.request_time,
		  (caddr_t) aControlMsg->getManagerAddr (),
		  sizeof (struct in_addr)) == 0)
	  return TRUE;
    }
}

/* ----------------------------------------------------------------------
 *  NetmgtControlMsg::control2activity - copy data to activity record
 *	returns TRUE if successful; otherwise returns FALSE
 * ----------------------------------------------------------------------
 */
bool_t
NetmgtControlMsg::control2activity (Netmgt_activity *activity)
{

  NETMGT_PRN (("cache: NetmgtControlMsg::control2activity\n"));

  assert (activity != (Netmgt_activity *) NULL);
  activity->type = this->type;
  activity->request_time = this->request_time;
  activity->handle = this->handle;
  activity->manager_addr = this->manager_addr;
  activity->agent_addr = this->agent_addr;
  activity->agent_prog = this->agent_prog;
  activity->agent_vers = this->agent_vers;
  return TRUE;
}
