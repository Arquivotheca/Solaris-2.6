
/**************************************************************************
 *  File:	include/libnetmgt/control.h
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
 *  SCCSID:	@(#)control.h  1.32  91/02/11
 *
 *  Comments:	control message class
 *
 **************************************************************************
 */

#ifndef	_control_h
#define _control_h

// control message types 
typedef enum
{
  NETMGT_CACHE_REQUEST = 1,
  NETMGT_UNCACHE_REQUEST = 2,
  NETMGT_KILL_REQUEST = 3,
  NETMGT_VERIFY_REQUEST = 4,
  NETMGT_DEFERRED_REQUEST = 5
}  NetmgtControlMsgType ;


// forward declaration
class NetmgtRequest ;


//  control message class
class NetmgtControlMsg: public NetmgtMessage
{

  // public access functions 
public:

  // get manager IP address pointer
  struct in_addr *getManagerAddr (void) { return &this->manager_addr; }

  // get request timestamp pointer
  struct timeval *getRequestTime (void) { return &this->request_time; }

  // get message type
  NetmgtControlMsgType getType (void)  { return this->type; }


  // public service functions 
public:

  // cache activity 
  bool_t cacheActivity (NetmgtDispatcher *aDispatcher) ;

  // check whether activity is being performed
  bool_t checkActivity (Netmgt_activity *activity, struct timeval timeout) ;

  // copy control message data to activity log record
  bool_t control2activity (Netmgt_activity *activity) ;

  // send agent ID request to agent
  Netmgt_agent_ID *requestAgentID (char *agent_host, 
				   u_long agent_prog, 
				   u_long agent_vers, 
				   struct timeval timeout) ;

  // send deferred reports request to agent
  bool_t requestDeferred (char *agent_name, 
			  u_long agent_prog, 
			  u_long agent_vers,
			  char *manager_name, 
			  struct timeval request_time, 
			  struct timeval timeout) ;

  // send kill request to agent
  bool_t sendKillRequest (char *agent_name, 
			  u_long agent_prog, 
			  u_long agent_vers, 
			  char *manager_name, 
			  struct timeval request_time,
			  struct timeval timeout) ;

  // uncache activity
  bool_t uncacheActivity (NetmgtDispatcher *aDispatcher, 
			  NetmgtRequest *aRequest) ;

  // (de)serialize activity control message
  bool_t xdrControl (XDR *xdr) ;


  // private member variables
private:

  NetmgtControlMsgType type; // message type
  struct timeval request_time ;	 // request timestamp 
  u_int handle ;		 // request handle 
  struct in_addr manager_addr ;	 // manager IP address 
  struct in_addr agent_addr ;	 // agent IP address 
  u_long agent_prog ;		 // agent RPC program number 
  u_long agent_vers ;		 // agent RPC version number 

} ;

#endif _control_h
