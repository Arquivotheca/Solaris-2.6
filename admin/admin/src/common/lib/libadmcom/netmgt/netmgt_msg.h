
/**************************************************************************
 *  File:	include/netmgt_msg.h
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
 *  SCCSID:	@(#)netmgt_msg.h  1.65  90/12/05
 *
 *  Comments:	SunNet Manager RPC message definitions
 *
 **************************************************************************
 */

#ifndef	_netmgt_msg_h
#define _netmgt_msg_h

/* message types */
#define NETMGT_DATA_REQUEST	(u_int)1  /* data request */
#define NETMGT_DATA_REPORT	(u_int)2  /* data report */
#define NETMGT_EVENT_REQUEST	(u_int)3  /* event request */
#define NETMGT_EVENT_REPORT	(u_int)4  /* event report */
#define NETMGT_ERROR_REPORT	(u_int)5  /* error report */
#define NETMGT_SET_REQUEST	(u_int)9  /* set request */
#define NETMGT_SET_REPORT	(u_int)10 /* set report */
#define NETMGT_TRAP_REQUEST	(u_int)11 /* trap request */
#define NETMGT_TRAP_REPORT	(u_int)12 /* trap report */

/* event report priority level */
#define	NETMGT_LOW_PRIORITY	(u_int)1 /* low priority */
#define	NETMGT_MEDIUM_PRIORITY	(u_int)2 /* medium priority */
#define	NETMGT_HIGH_PRIORITY	(u_int)3 /* high priority */

/* message flags */
#define NETMGT_RESTART		(1 << 2) /* restart uncompleted requests */
#define NETMGT_SEND_ONCE	(1 << 4) /* send only one event report */
#define NETMGT_DO_DEFERRED	(1 << 5) /* do deferred sending */
#define NETMGT_LAST		(1 << 6) /* last report */
#define NETMGT_NO_EVENTS	(1 << 9) /* no events occurred */

/* message information */
typedef struct
{
  struct in_addr manager_addr ;	/* manager IP address */
  struct timeval request_time ;	/* request timestamp */
  struct timeval report_time ;	/* report timestamp */
  struct timeval delta_time ;	/* delta time */
  struct in_addr agent_addr ;	/* agent IP address */
  u_long agent_prog ;		/* agent RPC program number */
  u_long agent_vers ;		/* agent RPC version number */
  Netmgt_stat status ;		/* status code */
}	Netmgt_msginfo ;

#endif /* !_netmgt_msg_h */




