
/**************************************************************************
 *  File:	include/libnetmgt/activity.h
 *
 *  Author:	Lynn Monsanto, Sun Microsystems Inc.
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
 *  SCCSID:	@(#)activity.h  1.3  91/02/12
 *
 *  Comments:	request activity definitions
 *
 **************************************************************************
 */

#ifndef _activity_h
#define _activity_h


/* activity logfile keyword */
#define NETMGT_ACTIVITY_LOG    "activity-log"

/* activity control message */
typedef struct
{
  u_int type ;                  /* message type */
  struct timeval request_time ; /* request timestamp */
  u_int handle ;                /* request handle */
  struct in_addr manager_addr ; /* manager IP address */
  struct in_addr agent_addr ;   /* agent IP address */
  u_long agent_prog ;           /* agent RPC program number */
  u_long agent_vers ;           /* agent RPC version number */
}      Netmgt_activity ;

#endif  _activity_h
