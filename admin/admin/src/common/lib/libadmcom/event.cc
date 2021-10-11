#if !defined(lint) && !defined(NOID)
  static char sccsid[] = "@(#)event.cc	1.38 5/5/91 SMI";
#endif

/**************************************************************************
 *  File:	lib/libnetmgt/event.cc
 *
 *  Author:	Lynn Monsanto, Sun Microsystems Inc.
 *
 *  SCCSID:     @(#)event.cc  1.38  91/05/05
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
 *  Comments:	event report routines
 *
 **************************************************************************
 */

/*LINTLIBRARY*/

#include "netmgt/netmgt_hdrs.h"
#include "netmgt/netmgt.h"
#include "libnetmgt/libnetmgt.h"

/* ----------------------------------------------------------------------
 *  NetmgtServiceMsg::sendEventReport - send event report message 
 *      returns TRUE if successful; otherwise returns FALSE
 * ----------------------------------------------------------------------
 */
bool_t
NetmgtServiceMsg::sendEventReport (NetmgtPerformer *aPerformer,
				   struct timeval send_delta_time, 
				   Netmgt_stat send_status, 
				   u_int send_flags)
     // agent performer pointer
     // report delta time
     // report status code 
     // report flags 
{

  NETMGT_PRN (("event: NetmgtServiceMsg::sendEventReport\n")) ;

  assert (aPerformer != (NetmgtPerformer *) NULL) ;

  // reset internal error code 
  _netmgtStatus.clearStatus ();

  // don't send an event report unless an event occurred or the
  // NETMGT_NO_EVENTS flag is set - the NETMGT_NO_EVENT flag is
  // used to send a message to the rendezvous indicating the
  // request completed but no events occurred 
  if (aPerformer->getState () != NETMGT_EVENT_OCCURRED &&
      !(send_flags & NETMGT_NO_EVENTS))
    {
      // reset the report message arglist and event state 
      this->clearEvent (aPerformer);
      return TRUE;
    }

  // reset agent state to 'dispatched' 
  aPerformer->setState (NETMGT_DISPATCHED);

  // get request information from agent 
  NetmgtRequestInfo requestInfo ;   // request information buffer
  aPerformer->getRequestInfo (&requestInfo);

  // fill out report message header from request information
  this->request2report (&requestInfo) ;

  // fill out the rest of the report message header
  if (gettimeofday (&this->report_time, (struct timezone *) NULL ) == -1)
    {
      if (netmgt_debug)
	perror ("event: gettimeofday");
      _netmgtStatus.setStatus (NETMGT_GETTIMEOFDAY, 0, strerror (errno));
      return FALSE;
    }
  this->delta_time = send_delta_time;
  this->type = NETMGT_EVENT_REPORT;
  this->status = send_status;
  if (requestInfo.flags & NETMGT_SEND_ONCE)
    this->flags = send_flags | NETMGT_LAST;
  else
    this->flags = send_flags;

  this->agent_addr = aPerformer->getLocalAddr ();
  this->agent_prog = aPerformer->getProgram ();
  this->agent_vers = aPerformer->getVersion ();
  if (this->myArglist.getOffset () == 0)
    this->length = 0;
  else
    this->length = u_int (this->myArglist.getOffset ()
			  + sizeof (NETMGT_END_TAG)
			  + sizeof (NETMGT_ENDOFARGS) + 1);

  // return an error if the event report message argument list buffer
  // is greater than the maximum UDP-transport RPC buffer size
  if (this->length > NETMGT_MAXARGSIZ)
    {
      NETMGT_PRN (("event: arglist too large: %d > %d\n",
		   this->length, NETMGT_MAXARGSIZ));
      _netmgtStatus.setStatus (NETMGT_MSG2BIG, 0, NULL);
      return FALSE;
    }

  // send the report message
  this->sendReport(aPerformer, &requestInfo);

  // remember if this is the last message sent 
  if (this->flags & NETMGT_LAST)
    aPerformer->setState (NETMGT_SENT_LAST);

  // shutdown if only sending one event report 
  if (requestInfo.flags & NETMGT_SEND_ONCE)
    aPerformer->myDestructor ();

  // reset the report message arglist and event state 
  this->clearEvent (aPerformer);

  return TRUE;
}



