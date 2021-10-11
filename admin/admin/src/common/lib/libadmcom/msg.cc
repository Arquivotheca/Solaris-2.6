#if !defined(lint) && !defined(NOID)
  static char sccsid[] = "@(#)msg.cc	1.35 5/5/91 SMI";
#endif

/**************************************************************************
 *  File:	lib/libnetmgt/myServiceMsg.c
 *
 *  Author:	Lynn Monsanto, Sun Microsystems Inc.
 *
 *  SCCSID:     @(#)msg.cc  1.35  91/05/05
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
 *  Comments:   message XDR encoding/decoding routines
 *
 **************************************************************************
 */

/*LINTLIBRARY*/

#include "netmgt/netmgt_hdrs.h"
#include "netmgt/netmgt.h"
#include "libnetmgt/libnetmgt.h"

/* ----------------------------------------------------------------------
 *  _netmgt_serialMsg - C wrapper to serialize message 
 *	returns TRUE if successful; otherwise returns FALSE
 * ----------------------------------------------------------------------
 */
bool_t
_netmgt_serialMsg (XDR *xdr, NetmgtServiceMsg *aServiceMsg)
               			// transport handle 
                      		// netmgt message 
{
  NETMGT_PRN (("msg: _netmgt_serialMsg\n")) ;
  
  return aServiceMsg->serialize (xdr);
}

/* ----------------------------------------------------------------------
 *  NetmgtServiceMsg::serialize - XDR encode and serialize message
 *	returns TRUE if successful; otherwise returns FALSE
 * ----------------------------------------------------------------------
 */
bool_t
NetmgtServiceMsg::serialize (XDR *xdr)
               			// transport handle 
{
  char *manager ;		// manager IP address pointer 
  char *agent ;			// agent IP address pointer 
  char *rendez ;		// rendezvous IP address pointer 
  u_int size ;			// IP address size 
  char *serializesystem ;	// system name string 
  char *serializegroup ;      	// object group string 
  char *serializekey ;	      	// object instance string 

  NETMGT_PRN (("msg: NetmgtServiceMsg::serialize\n")) ;

  assert (xdr != (XDR *) NULL) ;

  // reset internal error code 
  _netmgtStatus.clearStatus ();

#ifdef DEBUG
  NETMGT_PRN (("msg: sending message ...\n")) ;
  NETMGT_PRN ((" request_time: %d sec. %d microsec.\n",
	       this->request_time.tv_sec, this->request_time.tv_usec)) ;
  NETMGT_PRN (("  report_time: %d sec. %d microsec.\n",
	       this->report_time.tv_sec, this->report_time.tv_usec)) ;
  NETMGT_PRN (("   delta_time: %d sec. %d microsec.\n",
	       this->delta_time.tv_sec, this->delta_time.tv_usec)) ;
  NETMGT_PRN (("       handle: %u\n", this->handle)) ;
  NETMGT_PRN (("         type: %u\n", this->type)) ;
  NETMGT_PRN (("       status: %u\n", this->status)) ;
  NETMGT_PRN (("        flags: %u\n", this->flags)) ;
  NETMGT_PRN (("     priority: %u\n", this->priority)) ;
  NETMGT_PRN ((" manager_addr: %s\n", inet_ntoa (this->manager_addr))) ;
  NETMGT_PRN (("   agent_addr: %s\n", inet_ntoa (this->agent_addr))) ;
  NETMGT_PRN (("   agent_prog: %u\n", this->agent_prog)) ;
  NETMGT_PRN (("   agent_vers: %u\n", this->agent_vers)) ;
  NETMGT_PRN (("  rendez_addr: %s\n", inet_ntoa (this->rendez_addr))) ;
  NETMGT_PRN (("  rendez_prog: %u\n", this->rendez_prog)) ;
  NETMGT_PRN (("  rendez_vers: %u\n", this->rendez_vers)) ;
  NETMGT_PRN (("        proto: %u\n", this->proto)) ;
  NETMGT_PRN (("      timeout: %d sec. %d microsec.\n",
	       this->timeout.tv_sec, this->timeout.tv_usec)) ;
  NETMGT_PRN (("        count: %u\n", this->count)) ;
  NETMGT_PRN (("     interval: %d sec. %d microsec.\n",
	       this->interval.tv_sec, this->interval.tv_usec)) ;
  NETMGT_PRN (("       system: %s\n", this->system)) ;
  NETMGT_PRN (("        group: %s\n", this->group)) ;
  NETMGT_PRN (("          key: %s\n", this->key)) ;
  NETMGT_PRN (("       length: %u\n", this->length)) ;
#endif DEBUG

  // serialize the message header 
  manager = (char *) &this->manager_addr.s_addr ;
  agent = (char *) &this->agent_addr.s_addr ;
  rendez = (char *) &this->rendez_addr.s_addr ;
  size = sizeof (struct in_addr) ;
  serializesystem = this->system ;
  serializegroup = this->group ;
  serializekey = this->key ;

  if (!xdr_long (xdr, &this->request_time.tv_sec) ||
      !xdr_long (xdr, &this->request_time.tv_usec) ||
      !xdr_long (xdr, &this->report_time.tv_sec) ||
      !xdr_long (xdr, &this->report_time.tv_usec) ||
      !xdr_long (xdr, &this->delta_time.tv_sec) ||
      !xdr_long (xdr, &this->delta_time.tv_usec) ||
      !xdr_u_int (xdr, &this->handle) ||
      !xdr_u_int (xdr, &this->type) ||
      !xdr_u_int (xdr, (u_int *) & this->status) ||
      !xdr_u_int (xdr, &this->flags) ||
      !xdr_u_int (xdr, &this->priority) ||
      !xdr_bytes (xdr, &manager, &size, size) ||
      !xdr_bytes (xdr, &agent, &size, size) ||
      !xdr_u_long (xdr, &this->agent_prog) ||
      !xdr_u_long (xdr, &this->agent_vers) ||
      !xdr_bytes (xdr, &rendez, &size, size) ||
      !xdr_u_long (xdr, &this->rendez_prog) ||
      !xdr_u_long (xdr, &this->rendez_vers) ||
      !xdr_u_long (xdr, &this->proto) ||
      !xdr_long (xdr, &this->timeout.tv_sec) ||
      !xdr_long (xdr, &this->timeout.tv_usec) ||
      !xdr_u_int (xdr, &this->count) ||
      !xdr_long (xdr, &this->interval.tv_sec) ||
      !xdr_long (xdr, &this->interval.tv_usec) ||
      !xdr_string (xdr, &serializesystem, NETMGT_NAMESIZ) ||
      !xdr_string (xdr, &serializegroup, NETMGT_NAMESIZ) ||
      !xdr_string (xdr, &serializekey, NETMGT_NAMESIZ) ||
      !xdr_u_int (xdr, &this->length))
    {
      _netmgtStatus.setStatus (NETMGT_ENCODEHEADER, 0, NULL);
      return FALSE;
    }

  // serial the message argument list 
  if (!this->myArglist.serialize (xdr, this))
    {
      _netmgtStatus.setStatus (NETMGT_ENCODEARGLIST, 0, NULL);
      return FALSE;
    }
  return TRUE;
}

/* ------------------------------------------------------------------------
 *  _netmgt_deserialMsg - wrapper to deserialize message 
 *	returns TRUE if successful; otherwise returns FALSE
 * ------------------------------------------------------------------------
 */
bool_t
_netmgt_deserialMsg (XDR *xdr, NetmgtServiceMsg *aServiceMsg)
     				// transport handle 
                     		// netmgt message 
{
  NETMGT_PRN (("msg: _netmgt_deserialMsg\n"));

  return aServiceMsg->deserialize (xdr);
}

/* ------------------------------------------------------------------------
 *  NetmgtServiceMsg::deserialize - XDR deserialize and decode message 
 *	returns TRUE if successful; otherwise returns FALSE
 * ------------------------------------------------------------------------
 */
bool_t
NetmgtServiceMsg::deserialize (XDR *xdr)
     				// transport handle 
{
  char *manager;		// manager IP address pointer 
  char *agent;			// agent IP address pointer 
  char *rendez;			// rendezvous IP address pointer 
  u_int size;			// IP address size 
  char *desersystem;		// system string 
  char *desergroup;	       	// object group string 
  char *deserkey;	       	// object instance string 
  
  NETMGT_PRN (("msg: NetmgtServiceMsg::deserialize\n"));
  
  assert (xdr != (XDR *) NULL);
  
  // deserial the message header 
  manager = (char *) &this->manager_addr.s_addr;
  agent = (char *) &this->agent_addr.s_addr;
  rendez = (char *) &this->rendez_addr.s_addr;
  size = sizeof (struct in_addr);
  desersystem = this->system;
  desergroup = this->group;
  deserkey = this->key;
  
  if (!xdr_long (xdr, &this->request_time.tv_sec) ||
      !xdr_long (xdr, &this->request_time.tv_usec) ||
      !xdr_long (xdr, &this->report_time.tv_sec) ||
      !xdr_long (xdr, &this->report_time.tv_usec) ||
      !xdr_long (xdr, &this->delta_time.tv_sec) ||
      !xdr_long (xdr, &this->delta_time.tv_usec) ||
      !xdr_u_int (xdr, &this->handle) ||
      !xdr_u_int (xdr, &this->type) ||
      !xdr_u_int (xdr, (u_int *) & this->status) ||
      !xdr_u_int (xdr, &this->flags) ||
      !xdr_u_int (xdr, &this->priority) ||
      !xdr_bytes (xdr, &manager, &size, size) ||
      !xdr_bytes (xdr, &agent, &size, size) ||
      !xdr_u_long (xdr, &this->agent_prog) ||
      !xdr_u_long (xdr, &this->agent_vers) ||
      !xdr_bytes (xdr, &rendez, &size, size) ||
      !xdr_u_long (xdr, &this->rendez_prog) ||
      !xdr_u_long (xdr, &this->rendez_vers) ||
      !xdr_u_long (xdr, &this->proto) ||
      !xdr_long (xdr, &this->timeout.tv_sec) ||
      !xdr_long (xdr, &this->timeout.tv_usec) ||
      !xdr_u_int (xdr, &this->count) ||
      !xdr_long (xdr, &this->interval.tv_sec) ||
      !xdr_long (xdr, &this->interval.tv_usec) ||
      !xdr_string (xdr, &desersystem, NETMGT_NAMESIZ) ||
      !xdr_string (xdr, &desergroup, NETMGT_NAMESIZ) ||
      !xdr_string (xdr, &deserkey, NETMGT_NAMESIZ) ||
      !xdr_u_int (xdr, &this->length))
    {
      _netmgtStatus.setStatus (NETMGT_DECODEHEADER, 0, NULL);
      return FALSE;
    }

#ifdef DEBUG
  NETMGT_PRN (("msg: received message ...\n"));
  NETMGT_PRN ((" request_time: %d sec. %d microsec.\n",
	       this->request_time.tv_sec, this->request_time.tv_usec));
  NETMGT_PRN (("  report_time: %d sec. %d microsec.\n",
	       this->report_time.tv_sec, this->report_time.tv_usec));
  NETMGT_PRN (("   delta_time: %d sec. %d microsec.\n",
	       this->delta_time.tv_sec, this->delta_time.tv_usec));
  NETMGT_PRN (("       handle: %u\n", this->handle));
  NETMGT_PRN (("         type: %u\n", this->type));
  NETMGT_PRN (("       status: %u\n", this->status));
  NETMGT_PRN (("        flags: %u\n", this->flags));
  NETMGT_PRN (("     priority: %u\n", this->priority));
  NETMGT_PRN ((" manager_addr: %s\n", inet_ntoa (this->manager_addr)));
  NETMGT_PRN (("   agent_addr: %s\n", inet_ntoa (this->agent_addr)));
  NETMGT_PRN (("   agent_prog: %u\n", this->agent_prog));
  NETMGT_PRN (("   agent_vers: %u\n", this->agent_vers));
  NETMGT_PRN (("  rendez_addr: %s\n", inet_ntoa (this->rendez_addr)));
  NETMGT_PRN (("  rendez_prog: %u\n", this->rendez_prog));
  NETMGT_PRN (("  rendez_vers: %u\n", this->rendez_vers));
  NETMGT_PRN (("        proto: %u\n", this->proto));
  NETMGT_PRN (("      timeout: %d sec. %d microsec.\n",
	       this->timeout.tv_sec, this->timeout.tv_usec));
  NETMGT_PRN (("        count: %u\n", this->count));
  NETMGT_PRN (("     interval: %d sec. %d microsec.\n",
	       this->interval.tv_sec, this->interval.tv_usec));
  NETMGT_PRN (("       desersystem: %s\n", this->system));
  NETMGT_PRN (("        desergroup: %s\n", this->group));
  NETMGT_PRN (("          deserkey: %s\n", this->key));
  NETMGT_PRN (("       length: %u\n", this->length));
#endif DEBUG

  // deserial the message argument list 
  if (!this->myArglist.deserialize (xdr, this))
    {
      _netmgtStatus.setStatus (NETMGT_DECODEARGLIST, 0, NULL);
      return FALSE;
    }
  return TRUE;
}



