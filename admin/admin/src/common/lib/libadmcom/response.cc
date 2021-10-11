#if !defined(lint) && !defined(NOID)
  static char sccsid[] = "@(#)response.cc	1.42 5/5/91 SMI";
#endif

/**************************************************************************
 *  File:	lib/libnetmgt/response.cc
 *
 *  Author:	Lynn Monsanto, Sun Microsystems Inc.
 *
 *  SCCSID:     @(#)response.cc  1.42  91/05/05
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
 *  Comments:	performance report sending and receiving routines
 *
 **************************************************************************
 */

/*LINTLIBRARY*/

#include "netmgt/netmgt_hdrs.h"
#include "netmgt/netmgt.h"
#include "libnetmgt/libnetmgt.h"

/* ---------------------------------------------------------------------
 *  netmgt_build_report - build report arglist
 *	returns TRUE if successful; otherwise returns FALSE
 * ---------------------------------------------------------------------
 */
bool_t
netmgt_build_report (Netmgt_data *data, bool_t *eventOccurred)
                       		// data report 
                            	// whether an event occurred 
{

  // get request information from agent
  NetmgtRequestInfo requestInfo ;   // request information buffer
  aNetmgtPerformer->getRequestInfo (&requestInfo);

  // build report arglist based upon request type 
  switch (requestInfo.type)
    {
    case NETMGT_ACTION_REQUEST:
    case NETMGT_DATA_REQUEST:
      return aNetmgtPerformer->buildDataReport (data, eventOccurred);
      
    case NETMGT_EVENT_REQUEST:
      return aNetmgtPerformer->buildEventReport (data, eventOccurred);
      
    case NETMGT_TRAP_REQUEST:
      *eventOccurred = 0;
      return aNetmgtPerformer->buildTrapReport (data);
      
    default:
      _netmgtStatus.setStatus (NETMGT_UNKNOWNREQUEST, (u_int) 0, NULL);
      return FALSE;
    }
  /*NOTREACHED*/  

}

/* ---------------------------------------------------------------------
 *  NetmgtPerformer::buildDataReport - build data report arglist
 *	returns TRUE if successful; otherwise returns FALSE
 * ---------------------------------------------------------------------
 */
bool_t
NetmgtPerformer::buildDataReport (Netmgt_data *data, int *alarm)
                       		// data report 
                            	// whether an event occurred 
{

  NETMGT_PRN (("response: NetmgtPerformer::buildDataReport\n"));

  // reset internal error buffer 
  _netmgtStatus.clearStatus ();

  // clear alarm flag 
  *alarm = FALSE;

  // verify input 
  if (!data)
    {
      NETMGT_PRN (("report.c: no data report argument\n"));
      _netmgtStatus.setStatus (NETMGT_NOPERFBUF, 0, NULL);
      return FALSE;
    }
  if (!data->name)
    {
      NETMGT_PRN (("report.c: no attribute name\n"));
      _netmgtStatus.setStatus (NETMGT_NOARGNAME, 0, NULL);
      return FALSE;
    }
  if (strlen (data->name) >= NETMGT_NAMESIZ)
    {
      NETMGT_PRN (("report.c: attribute name length >= %d\n",
		   NETMGT_NAMESIZ));
      _netmgtStatus.setStatus (NETMGT_NAME2BIG, 0, NULL);
      return FALSE;
    }
  if (!data->value)
    {
      NETMGT_PRN (("report.c: no attribute value pointer\n"));
      _netmgtStatus.setStatus (NETMGT_NOARGVALUE, 0, NULL);
      return FALSE;
    }
  
  NetmgtGeneric aGeneric ;	// generic argument
  assert (this->myServiceMsg != (NetmgtServiceMsg *) NULL);
  return aGeneric.putData (data, this->myServiceMsg);
}

/* -------------------------------------------------------------------
 *  NetmgtServiceMsg::sendDataReport - send data report message to rendezvous
 *      returns TRUE if successful; otherwise returns FALSE
 * -------------------------------------------------------------------
 */
bool_t
NetmgtServiceMsg::sendDataReport (NetmgtPerformer *aPerformer,
				  struct timeval adelta_time, 
				  Netmgt_stat astatus, 
				  u_int aflags)
     // agent performer pointer
     // report delta time
     // report status
     // report flags
{
  NETMGT_PRN (("response: NetmgtServiceMsg::sendDataReport\n")) ;

  // reset internal error code 
  _netmgtStatus.clearStatus ();

  // get request information from agent
  NetmgtRequestInfo requestInfo ;   	// request information buffer
  aPerformer->getRequestInfo (&requestInfo);

  // fill out report message header from request information
  this->request2report (&requestInfo) ;

  // fill out the the rest of the header
  if (gettimeofday (&this->report_time, (struct timezone *)NULL ) == -1)
    {
      if (netmgt_debug)
	perror ("response: gettimeofday");
      _netmgtStatus.setStatus (NETMGT_GETTIMEOFDAY, 0, strerror (errno));
      return FALSE;
    }
  this->delta_time = adelta_time;
  this->type = NETMGT_DATA_REPORT;
  this->status = astatus;
  this->flags = aflags;
  this->agent_addr = aPerformer->getLocalAddr ();
  this->agent_prog = aPerformer->getProgram ();
  this->agent_vers = aPerformer->getVersion ();
  if (this->myArglist.getOffset () == 0)
    this->length = 0;
  else
    this->length = u_int (this->myArglist.getOffset () 
			  + sizeof (NETMGT_END_TAG)
			  + sizeof (NETMGT_ENDOFARGS) + 1);

  // send the report
  return this->sendReport (aPerformer, &requestInfo);
}


