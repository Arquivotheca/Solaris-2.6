#if !defined(lint) && !defined(NOID)
  static char sccsid[] = "@(#)header.cc	1.33 5/5/91 SMI";
#endif

/**************************************************************************
 *  File:	lib/libnetmgt/header.cc
 *
 *  Author:	Lynn Monsanto, Sun Microsystems Inc.
 *
 *  SCCSID:     @(#)header.cc  1.33  91/05/05
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
 *  Comments:   fetch received message header information
 *
 **************************************************************************
 */

/*LINTLIBRARY*/

#include "netmgt/netmgt_hdrs.h"
#include "netmgt/netmgt.h"
#include "libnetmgt/libnetmgt.h"

/* ----------------------------------------------------------------------
 *  NetmgtServiceMsg::getHeader - get message header information
 *	returns TRUE if successful; otherwise returns FALSE 
 * ----------------------------------------------------------------------
 */
bool_t
NetmgtServiceMsg::getHeader (NetmgtServiceHeader *header)
{
  NETMGT_PRN (("header: NetmgtServiceMsg::getHeader\n"));

  // verify input 
  if (!header)
    {
      _netmgtStatus.setStatus (NETMGT_NOINFOBUF, 0, NULL);
      return FALSE;
    }

  // copy message header to buffer
  header->request_time = this->request_time;
  header->report_time = this->report_time;
  header->delta_time = this->delta_time;
  header->handle = this->handle ;
  header->type = this->type ;
  header->status = this->status ;
  header->flags = this->flags ;
  header->priority = this->priority ;
  header->manager_addr = this->manager_addr;
  header->agent_addr = this->agent_addr;
  header->agent_prog = this->agent_prog;
  header->agent_vers = this->agent_vers;
  header->rendez_addr = this->rendez_addr;
  header->rendez_prog = this->rendez_prog;
  header->rendez_vers = this->rendez_vers;
  header->proto = this->proto;
  header->timeout = this->timeout;
  header->count = this->count;
  header->interval = this->interval;
  (void) strncpy (header->system, this->system, sizeof (header->system));
  (void) strncpy (header->group, this->group, sizeof (header->group));
  (void) strncpy (header->key, this->key, sizeof (header->key));
  header->length = this->length;

  return TRUE;
}

/* ----------------------------------------------------------------------
 *  NetmgtServiceMsg::setHeader - set service message header 
 *	returns TRUE if successful; otherwise returns FALSE 
 * ----------------------------------------------------------------------
 */
bool_t
NetmgtServiceMsg::setHeader (NetmgtServiceHeader *header)
{
  NETMGT_PRN (("header: NetmgtServiceMsg::setHeader\n"));

  // verify input 
  if (!header)
    {
      _netmgtStatus.setStatus (NETMGT_NOINFOBUF, 0, NULL);
      return FALSE;
    }

  // copy message header to buffer
  this->request_time = header->request_time;
  this->report_time = header->report_time;
  this->delta_time = header->delta_time;
  this->handle = header->handle ;
  this->type = header->type ;
  this->status = header->status ;
  this->flags = header->flags ;
  this->priority = header->priority ;
  this->manager_addr = header->manager_addr;
  this->agent_addr = header->agent_addr;
  this->agent_prog = header->agent_prog;
  this->agent_vers = header->agent_vers;
  this->rendez_addr = header->rendez_addr;
  this->rendez_prog = header->rendez_prog;
  this->rendez_vers = header->rendez_vers;
  this->proto = header->proto;
  this->timeout = header->timeout;
  this->count = header->count;
  this->interval = header->interval;
  (void) strncpy (this->system, header->system, sizeof (this->system));
  (void) strncpy (this->group, header->group, sizeof (this->group));
  (void) strncpy (this->key, header->key, sizeof (this->key));
  this->length = header->length;

  return TRUE;
}




