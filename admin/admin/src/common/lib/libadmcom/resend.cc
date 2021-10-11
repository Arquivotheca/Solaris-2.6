#if !defined(lint) && !defined(NOID)
  static char sccsid[] = "@(#)resend.cc	1.44 5/5/91 SMI";
#endif

/**************************************************************************
 *  File:	lib/libnetmgt/resend.cc
 *
 *  Author:	Lynn Monsanto, Sun Microsystems Inc.
 *
 *  SCCSID:     @(#)resend.cc  1.44  91/05/05
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
 *  Comments:	agent data/event report resending routines
 *
 **************************************************************************
 */

/*LINTLIBRARY*/

#include "netmgt/netmgt_hdrs.h"
#include "netmgt/netmgt.h"
#include "libnetmgt/libnetmgt.h"

/* -------------------------------------------------------------------
 *  NetmgtPerformer::cacheReport - cache report message
 *	returns TRUE if successful; otherwise returns FALSE
 * -------------------------------------------------------------------
 */
bool_t
NetmgtPerformer::cacheReport (void)
{

  NETMGT_PRN (("resend: NetmgtPerformer::cacheReport\n")) ;

  // reset my service message's arglist read/write offset
  this->myServiceMsg->myArglist.resetPtr ();
      
  // allocate a service message to hold the cached report
  NetmgtServiceMsg *aServiceMsg ;
  aServiceMsg = (NetmgtServiceMsg *) calloc (1, sizeof (NetmgtServiceMsg)) ;
  if (!aServiceMsg)
    {
      // non-fatal error; keep handling the request
      if (netmgt_debug)
	perror ("resend: calloc failed");
      (void) cfree ((caddr_t) aServiceMsg);
      return TRUE;
    }

  // copy my service message header 
  (void) memcpy ((caddr_t) aServiceMsg, 
		 (caddr_t) this->myServiceMsg,
		 sizeof (NetmgtServiceMsg));

  // clear the cached message's arglist buffer
  aServiceMsg->myArglist.setBuffer ((caddr_t) NULL, (u_int) 0, (off_t) 0);
  aServiceMsg->myArglist.myValue1.setBuffer ((caddr_t) NULL, 
					     (u_int) 0, 
					     (off_t) 0);

  // copy my service message's arglist
  if (this->myServiceMsg->getLength () > 0)
    {
      // allocate arglist buffer 
      u_int length = this->myServiceMsg->getLength () 
	+ sizeof (NETMGT_END_TAG)
	+ sizeof (NETMGT_ENDOFARGS) + 1;
      if (!aServiceMsg->myArglist.alloc (length, NETMGT_MINARGSIZ))
	{
	  (void) cfree ((caddr_t) aServiceMsg);
	  return FALSE;
	}

      // allocate attribute value buffer 
      if (!aServiceMsg->myArglist.myValue1.alloc (length, 
						  NETMGT_MINARGSIZ))
	{
	  (void) cfree ((caddr_t) aServiceMsg->myArglist.getBase ());
	  (void) cfree ((caddr_t) aServiceMsg);
	  return FALSE;
	}
      
      (void) memcpy (aServiceMsg->myArglist.getBase (),
		     this->myServiceMsg->myArglist.getBase (),
		     length);

    }

  // append the cached report to my report queue 
  if (!this->appendReport (aServiceMsg))
    {
      // non-fatal error; keep handling the request 
      if (aServiceMsg->getLength () > 0)
	(void) cfree (aServiceMsg->myArglist.getBase ());
      (void) cfree ((caddr_t) aServiceMsg);
      return TRUE;
    }

  return TRUE;
}

/* -------------------------------------------------------------------------
 *  NetmgtPerformer::appendReport - append report message to deferred 
 *	report queue; returns TRUE if successful; otherwise FALSE
 * -------------------------------------------------------------------------
 */
bool_t
NetmgtPerformer::appendReport (NetmgtServiceMsg *aServiceMsg)
{
  NETMGT_PRN (("NetmgtPerformer::appendReport\n"));
  assert (aServiceMsg != (NetmgtServiceMsg *) NULL);
  return this->myReportQueue.append ((caddr_t) aServiceMsg, TRUE);
}

/* -------------------------------------------------------------------------
 *  NetmgtPerformer::resendReports - send cached data/event report messages
 *	returns TRUE if successful; otherwise returns FALSE
 * -------------------------------------------------------------------------
 */
bool_t
NetmgtPerformer::resendReports (void)
{

  NETMGT_PRN (("resend: NetmgtPerformer::resendReports\n"));

  // destroy performers message client so it will
  // create a new client
  this->myServiceMsg->myClient.destroyClient ();

  // block signals while in critical section 
  sigset_t osigmask = this->blockSignals ();

  // get request information
  NetmgtRequestInfo requestInfo ;   // request information buffer
  this->getRequestInfo (&requestInfo);

  NetmgtQueueNode *currNode;	// current report queue node
  enum clnt_stat clnt_stat;	// client call status

  // get cached report messages
  NetmgtServiceMsg *aServiceMsg; // a cached report message
  currNode = this->myReportQueue.getHead ();
  while (currNode)
    {

      // get report queue node data 
      assert (currNode->isData ());
      aServiceMsg = (NetmgtServiceMsg *) currNode->getData ();

      // resend the report if not marked for deletion 
      if (currNode->isStale ())
	{
	  currNode = currNode->getNext ();
	  continue;
	}

      // get a new rendezvous client handle
      if (!aServiceMsg->myClient.newClient (requestInfo.rendez_addr,
					    requestInfo.proto, 
					    requestInfo.rendez_prog,
					    requestInfo.rendez_vers,
					    NETMGT_NO_SECURITY,
					    this->timeout))
	{
	  this->unblockSignals (osigmask);
	  return FALSE;
	}

      // send the cached report message 
      NETMGT_PRN (("resend: sending cached report\n"));
      clnt_stat = clnt_call (aServiceMsg->myClient.getHandle (), 
			     NETMGT_SERVICE_PROC,
			     (xdrproc_t) _netmgt_serialMsg, 
			     (caddr_t) aServiceMsg,
			     (xdrproc_t) xdr_void, 
			     (caddr_t) NULL, 
			     this->getTimeout ());

      if (clnt_stat != RPC_SUCCESS)
	{
	  if (netmgt_debug)
	    clnt_perror (aServiceMsg->myClient.getHandle (), 
			 "resend: clnt_call");
	  currNode = currNode->getNext ();
	  aServiceMsg->myClient.destroyClient ();
	  continue;
	}
      aServiceMsg->myClient.destroyClient ();

      // mark request for deletion
      currNode->markStale ();

      // remember if this is the last event report 
      if (requestInfo.type == NETMGT_EVENT_REQUEST &&
	  aServiceMsg->getFlags () & NETMGT_LAST)
	this->setState (NETMGT_SENT_LAST);

      currNode = currNode->getNext ();
    }

  // unblock signals 
  this->unblockSignals (osigmask);

  // free the cached reports that are marked for deletion 
  (void) this->cleanReports ();

  return TRUE;
}

/* -------------------------------------------------------------------
 *  NetmgtPerformer::cleanReports - clean up agent report queue
 *	returns TRUE if successful; otherwise returns FALSE
 * -------------------------------------------------------------------
 */
bool_t
NetmgtPerformer::cleanReports (void)
{

  NETMGT_PRN (("resend: NetmgtPerformer::cleanReports\n"));

  // block signal while in critical section 
  sigset_t osigmask = this->blockSignals ();

  // keep searching agent report queue searching for cached
  // reports to delete until no more are found 
  NetmgtQueueNode *currNode;	// current report queue node 
  NetmgtServiceMsg *aServiceMsg; // report queue node data 
  bool_t found;			// whether found node to delete 
  bool_t ifSucceeded;		// whether delete succeeded 

  do
  {
    currNode = this->myReportQueue.getHead ();
    found = FALSE;
    while (currNode)
      {

	// get request data 
	assert (currNode->isData ());
	aServiceMsg = (NetmgtServiceMsg *) currNode->getData ();

	if (currNode->isStale ())
	  {
	    found = TRUE;
	    break;
	  }

	currNode = currNode->getNext ();
      }

    if (found)
      {
	// free the message arglist
	if (aServiceMsg->getLength () > 0)
	  {
	    (void) cfree (aServiceMsg->myArglist.getBase ());
	    (void) cfree (aServiceMsg->myArglist.myValue1.getBase ());
	    aServiceMsg->myArglist.setBuffer ((caddr_t) NULL, 
					      (u_int) 0, 
					      (off_t) 0);
	    aServiceMsg->myArglist.myValue1.setBuffer ((caddr_t) NULL, 
						       (u_int) 0, 
						       (off_t) 0);
	  }

	// free the report queue node 
	ifSucceeded = this->myReportQueue.remove (currNode);
      }

  } while (found) ;

  // unblock signals 
  this->unblockSignals (osigmask);

  return ifSucceeded;
}
