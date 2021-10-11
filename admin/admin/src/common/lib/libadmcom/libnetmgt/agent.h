
/**************************************************************************
 *  File:	include/libnetmgt/agent.h
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
 *  SCCSID:	@(#)agent.h  1.39  91/02/07
 *
 *  Comments:	agent entity class
 *
 **************************************************************************
 */

#ifndef _agent_h
#define _agent_h

// package private agent initialization flags 
#define NETMGT_STARTED_BY_INETD	(u_int) (1 << 2) 

// agent state 
typedef enum
{
  NETMGT_INITIALIZING = 1,
  NETMGT_VERIFYING = 2,
  NETMGT_DISPATCHED = 3,
  NETMGT_EVENT_OCCURRED = 4,
  NETMGT_ERROR_OCCURRED = 5,
  NETMGT_SENT_REPORT = 6,
  NETMGT_SENT_LAST = 7
}     NetmgtAgentState ;


// agent entity class 
class NetmgtAgent: public NetmgtEntity
{

  // public access functions
public:

  // fetch set request argument
  bool_t fetchSetval (Netmgt_setval *setval) ;

  // get local address
  struct in_addr getLocalAddr (void) { return this->local_addr; }

  // get RPC program number
  u_long getProgram (void) { return this->program; }

  // get agent state 
  NetmgtAgentState getState (void) { return this->state; }

  // get RPC timeout
  struct timeval getTimeout (void) { return this->timeout; }

  // get RPC version number
  u_long getVersion (void) { return this->version; }


// public update functions
public:

  // set agent state
  void setState (NetmgtAgentState set_state)  { this->state = set_state; }


  // protected server functions
protected:

  // register RPC service
  bool_t registerService (void) ;

  // register TLI RPC service
  bool_t registerTliService (void) ;


  // protected object containments
protected:

  NetmgtQueue mySetargQueue ;	// set argument queue 
  NetmgtQueue myThreshQueue ;	// threshold argument queue


  // protected variables
protected:

  char *name ;			// agent name string 
  u_int serial ;		// agent serial number 
  struct in_addr local_addr ;	// agent IP address 
  u_long program ;		// agent RPC program number 
  u_long version ;		// agent RPC version number 
  u_long proto ;		// agent RPC transport protocol 
  u_int flags ;			// agent initialization flags 
  u_int idletime ;              // agent idle time limit (seconds)
  NetmgtAgentState state ;	// agent state 
  struct timeval timeout ;	// report RPC timeout 

} ;

#endif _agent_h
