#if !defined(lint) && !defined(NOID)
  static char sccsid[] = "@(#)performer.cc	1.17 5/5/91 SMI";
#endif

/**************************************************************************
 *  File:	lib/libnetmgt/performer.cc
 *
 *  Author:	Lynn Monsanto, Sun Microsystems Inc.
 *
 *  SCCSID:     @(#)performer.cc  1.17  91/05/05
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
 *  Comments:	agent performer process initialization/shutdown functions
 *
 **************************************************************************
 */

/*LINTLIBRARY*/

#include "netmgt/netmgt_hdrs.h"
#include "netmgt/netmgt.h"
#include "libnetmgt/libnetmgt.h"

/* --------------------------------------------------------------------
 *  NetmgtPerformer::myConstructor - initialize agent performer process
 *	returns TRUE if successful; otherwise returns FALSE
 * --------------------------------------------------------------------
 */
bool_t
NetmgtPerformer::myConstructor (NetmgtDispatcher *aDispatcher)
{

  NETMGT_PRN (("performer: NetmgtPerformer::myConstructor\n"));

  // copy parent's data members inherited from NetmgtEntity
  this->myServiceMsg = aDispatcher->myServiceMsg;
  this->readSecurity = aDispatcher->readSecurity;
  this->rdwrSecurity = aDispatcher->rdwrSecurity;
  this->myOptionsQueue = aDispatcher->myOptionsQueue;
  this->myRequest = aDispatcher->myRequest;

  // copy parent's data members inherited from NetmgtAgent
  this->name = aDispatcher->name;
  this->serial = aDispatcher->serial;
  this->local_addr = aDispatcher->local_addr;
  this->program = aDispatcher->program;
  this->version = aDispatcher->version;
  this->proto = aDispatcher->proto;
  this->flags = aDispatcher->flags;
  this->state = aDispatcher->state;
  this->timeout = aDispatcher->timeout;
  this->mySetargQueue  = aDispatcher->mySetargQueue ;
  this->myThreshQueue = aDispatcher->myThreshQueue;

  // initialize deferred report queue
  if (!this->myReportQueue.myConstructor (NETMGT_MAXREPORTS))
    return FALSE;

  return TRUE;
}

/* --------------------------------------------------------------------
 *  NetmgtPerformer::myDestructor - clean up and exit performer subprocess
 *      no return
 * --------------------------------------------------------------------
 */
void
NetmgtPerformer::myDestructor (void)
{
  NETMGT_PRN (("performer: NetmgtPerformer::myDestructor\n"));

  // get request information
  NetmgtRequestInfo request ;
  this->getRequestInfo (&request);

  // send a message to the rendezvous if no events occurred 
  // during the last sampling cycle 
  if (request.type == NETMGT_EVENT_REQUEST &&
      this->state != NETMGT_SENT_LAST)
    {
      NETMGT_PRN (("performer: sending NETMGT_NO_EVENTS message\n"));

      // set receive message priority to high so it will be copied
      // to the send message - we want this message to go to
      // all rendezvous 
      this->myRequest->setPriority (NETMGT_HIGH_PRIORITY);

      struct timeval timestamp ;	// event report timestamp 
      if (gettimeofday (&timestamp, (struct timezone *)NULL ) == -1)
	{
	  if (netmgt_debug)
	    perror ("request: gettimeofday");
	  timestamp.tv_sec = (long) 0;
	  timestamp.tv_usec = (long) 0;
	}      

      // create message object and send event report
      (void) this->myServiceMsg->sendEventReport (this,
					 timestamp, 
					 NETMGT_SUCCESS, 
					 NETMGT_LAST | NETMGT_NO_EVENTS);
    }

  // attempt to send any non-deferred and undelivered reports or 
  // event reports before exiting 
  if (! (request.flags & NETMGT_DO_DEFERRED) &&
      ! this->myReportQueue.isEmpty ())
    {

      for (int i = 0; i < NETMGT_MAXRESEND; i++)
	{
	  (void) sleep ((u_int) request.interval.tv_sec);

	  // attempt to send the cached reports/event reports 
	  if (this->resendReports ())
	    exit (0);
	}
      if (i == NETMGT_MAXRESEND)
	exit (1);
    }
  exit (0);
  /*NOTREACHED*/
}

