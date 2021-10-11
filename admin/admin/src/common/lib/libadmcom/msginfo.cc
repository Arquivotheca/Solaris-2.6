#if !defined(lint) && !defined(NOID)
  static char sccsid[] = "@(#)msginfo.cc	1.34 5/5/91 SMI";
#endif

/**************************************************************************
 *  File:	lib/libnetmgt/msginfo.cc
 *
 *  Author:	Lynn Monsanto, Sun Microsystems Inc.
 *
 *  SCCSID:     @(#)msginfo.cc  1.34  91/05/05
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
 *  Comments:   message information functions
 *
 **************************************************************************
 */

/*LINTLIBRARY*/

#include "netmgt/netmgt_hdrs.h"
#include "netmgt/netmgt.h"
#include "libnetmgt/libnetmgt.h"

/* ----------------------------------------------------------------------
 *  netmgt_fetch_msginfo - C wrapper to fetch message information
 *	returns TRUE if successful; otherwise returns FALSE 
 * ----------------------------------------------------------------------
 */
bool_t
netmgt_fetch_msginfo (Netmgt_msginfo *msginfo)
                             	// message information pointer
{

  NETMGT_PRN (("msginfo: netmgt_fetch_msginfo\n"));

  // verify input 
  if (!msginfo)
    {
      _netmgtStatus.setStatus (NETMGT_NOINFOBUF, 0, NULL);
      return FALSE;
    }

  assert (aNetmgtRendez != (NetmgtRendez *) NULL);
  assert (aNetmgtRendez->myServiceMsg != (NetmgtServiceMsg*) NULL);
  aNetmgtRendez->myServiceMsg->getMsginfo (msginfo);
  return TRUE;
}

/* -------------------------------------------------------------------
 *  NetmgtServiceMsg::getMsginfo - get report message information
 *	no return value
 * -------------------------------------------------------------------
 */
void
NetmgtServiceMsg::getMsginfo (Netmgt_msginfo *msginfo)
                             	// message information buffer 
{

  NETMGT_PRN (("myServiceMsg: NetmgtServiceMsg::getMsginfo\n"));

  assert (msginfo != (Netmgt_msginfo *) NULL);

  // get report message information
  msginfo->manager_addr = this->manager_addr;
  msginfo->request_time = this->request_time;
  msginfo->report_time = this->report_time;
  msginfo->delta_time = this->delta_time;
  msginfo->agent_addr = this->agent_addr;
  msginfo->agent_prog = this->agent_prog;
  msginfo->agent_vers = this->agent_vers;
  msginfo->status = this->status;
}

/* --------------------------------------------------------------------
 *  NetmgtEntity::saveRequestInfo - save request message information
 *	no return value
 * --------------------------------------------------------------------
 */
void
NetmgtEntity::saveRequestInfo (NetmgtRequestInfo *info)
				// request information pointer
{

  NETMGT_PRN (("msginfo: NetmgtEntity::saveRequestInfo\n"));

  assert (info != (NetmgtRequestInfo *) NULL);
  this->myRequest->saveRequestInfo (info);
  return;
}

/* --------------------------------------------------------------------
 *  NetmgtRequest::saveRequestInfo - save request message information
 *	no return value
 * --------------------------------------------------------------------
 */
void
NetmgtRequest::saveRequestInfo (NetmgtRequestInfo *saveinfo)
				// request information pointer
{

  NETMGT_PRN (("msginfo: NetmgtRequest::saveRequestInfo\n"));

  assert (saveinfo != (NetmgtRequestInfo *) NULL);

  // save request message information in entity request information buffer
  (void) memcpy ((caddr_t) & this->info,
		 (caddr_t) saveinfo,
		 sizeof (NetmgtRequestInfo));
  return;
}

/* --------------------------------------------------------------------
 *  NetmgtEntity::getRequestInfo - get request information
 *	no return value
 * --------------------------------------------------------------------
 */
void
NetmgtEntity::getRequestInfo (NetmgtRequestInfo *info)
				// request information buffer
{

  NETMGT_PRN (("msginfo: NetmgtEntity::getRequestInfo\n"));

  assert (info != (NetmgtRequestInfo *) NULL);
  this->myRequest->getRequestInfo (info);
  return;
}

/* --------------------------------------------------------------------
 *  NetmgtRequest::getRequestInfo - get request information
 *	no return value
 * --------------------------------------------------------------------
 */
void
NetmgtRequest::getRequestInfo (NetmgtRequestInfo *getinfo)
				// request information buffer
{

  NETMGT_PRN (("msginfo: NetmgtRequest::getRequestInfo\n"));

  assert (getinfo != (NetmgtRequestInfo *) NULL);

  // get request message information from entity request information buffer
  (void) memcpy ((caddr_t) getinfo,
		 (caddr_t) & this->info,
		 sizeof (NetmgtRequestInfo));
  return;
}

/* -----------------------------------------------------------------------
 *  NetmgtServiceMsg::request2report - fill out report message header 
 *	no return value
 * -----------------------------------------------------------------------
 */
void
NetmgtServiceMsg::request2report (NetmgtRequestInfo *requestInfo)
				// request message information
{

  NETMGT_PRN (("msginfo: NetmgtRequest::request2report\n"));

  // get request message information from agent request information
  // buffer and copy it to the report message
  this->request_time = requestInfo->request_time;
  this->type = requestInfo->type;
  this->handle = requestInfo->handle;
  this->flags = requestInfo->flags;
  this->priority = requestInfo->priority;
  (void) memcpy ((caddr_t) & this->manager_addr,
		 (caddr_t) & requestInfo->manager_addr,
		 sizeof (struct in_addr));
  (void) memcpy ((caddr_t) & this->rendez_addr,
		 (caddr_t) & requestInfo->rendez_addr,
		 sizeof (struct in_addr));
  this->rendez_prog = requestInfo->rendez_prog;
  this->rendez_vers = requestInfo->rendez_vers;
  this->proto = requestInfo->proto;
  this->interval = requestInfo->interval;
  this->count = requestInfo->count;
  (void) strncpy (this->system,
		  requestInfo->system,
		  sizeof (this->system));
  (void) strncpy (this->group,
		  requestInfo->group,
		  sizeof (this->group));
  (void) strncpy (this->key,
		  requestInfo->key,
		  sizeof (this->key));
  return;
}

