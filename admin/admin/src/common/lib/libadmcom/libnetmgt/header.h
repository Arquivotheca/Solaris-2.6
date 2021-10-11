
/**************************************************************************
 *  File:	include/libnetmgt/header.h
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
 *  SCCSID:	@(#)header.h  1.27  90/12/05
 *
 *  Comments:   message header information
 *
 **************************************************************************
 */
#ifndef _header_h
#define _header_h

// message header information
typedef struct
{
  struct timeval request_time ;	// request timestamp 
  struct timeval report_time ;	// report timestamp 
  struct timeval delta_time ;	// report delta time 
  u_int handle ;		// request handle 
  u_int type ;			// message type 
  Netmgt_stat status ;		// status code 
  u_int flags ;			// flags 
  u_int priority ;		// event priority 
  struct in_addr manager_addr ;	// manager IP address 
  struct in_addr agent_addr ;	// agent IP address 
  u_long agent_prog ;		// agent RPC program number 
  u_long agent_vers ;		// agent RPC version number 
  struct in_addr rendez_addr ;	// rendezvous IP address 
  u_long rendez_prog ;		// rendezvous RPC program number 
  u_long rendez_vers ;		// rendezvous RPC version number 
  u_long proto ;		// RPC transport protocol 
  struct timeval timeout ;	// RPC timeout 
  u_int count ;			// report count 
  struct timeval interval ;	// report interval 
  char system [NETMGT_NAMESIZ] ; // system name 
  char group [NETMGT_NAMESIZ] ;	 // object group/table name 
  char key [NETMGT_NAMESIZ];	 // optional table key
  u_int length ;		 // message data length 
}	NetmgtServiceHeader ;

#endif  _header_h
