
/**************************************************************************
 *  File:	include/libnetmgt/service.h
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
 *  SCCSID:	@(#)service.h  1.11  91/01/17
 *
 *  Comments:	service message class
 *
 **************************************************************************
 */

#ifndef	_service_h
#define _service_h

// package private message types
#define NETMGT_ACTION_REQUEST	(u_int)6  // action request
#define NETMGT_CREATE_REQUEST   (u_int)7  // create request
#define NETMGT_DELETE_REQUEST   (u_int)8  // delete request
#define NETMGT_ACTION_REPORT    (u_int)13 // action report

// package private message flags 
#define NETMGT_ATOMIC		(1 << 1) // atomic synchronization 
#define NETMGT_DONT_CALL	(1 << 3) // do deferred client call 
#define NETMGT_CREATE_ACTION    (1 << 7) // create action
#define NETMGT_DELETE_ACTION    (1 << 8) // delete action

// forward declarations
class NetmgtArglist ;
class NetmgtDispatcher ;
class NetmgtServiceMsg ;
class NetmgtPerformer ;


// service message class
class NetmgtServiceMsg: public NetmgtMessage
{

  // public access functions
public:

  // get agent RPC program number
  u_long getAgentProg (void)  { return this->agent_prog; }

  // get request handle
  bool_t getHandle (u_int *phandle)  { return *phandle = this->handle; }

  // get message header data
  bool_t getHeader (NetmgtServiceHeader *header) ;

  // get message flags
  u_int getFlags (void)  { return this->flags; }

  // get message length
  u_int getLength (void)  { return this->length; }

  // fill out rendezvous application's msginfo structure 
  void getMsginfo (Netmgt_msginfo *msginfo) ;

  // get entity referencing this message
  NetmgtEntity *getMyEntity (void) { return this->myEntity; }
 
  // get event priority
  u_int getPriority (void)  { return this->priority; }

  // get request timestamp
  struct timeval *getRequestTime (void) { return &this->request_time; }

  // get message type
  u_int getType (void)  { return this->type; }

  // is there message data ?
  bool_t isData (void)  { return this->length > 0; }

  // fill out report message from request information
  void request2report (NetmgtRequestInfo *requestInfo) ;


  // public update functions
public:

  // set rendezvous address 
  void setRendezAddr (struct in_addr addr)
    {
      (void) memcpy ((caddr_t) & rendez_addr, 
		     (caddr_t) & addr, 
		     sizeof (struct in_addr));
    }

  // set request handle
  bool_t setHandle (u_int set_handle) 
    { 
      this->handle = set_handle; 
      return TRUE;
    }

  // set request message instance information 
  bool_t setInstance (char *system, char *group, char *key) ;

  // set message length
  void setLength (u_int set_length) { this->length = set_length; }

  // set entity referencing this message
  void setMyEntity (NetmgtEntity *anEntity) { this->myEntity = anEntity; }

  // set priority
  void setPriority (u_int set_priority)  { this->priority = set_priority; }


  // public service functions
public:

  // send the setup but not-yet-sent request message to agent
  struct timeval *callAgent (u_long agent_prog, 
			     struct in_addr agent_addr, 
			     u_long agent_vers, 
			     struct timeval timeout) ;

  // clear event report argument from arglist
  void clearEvent (NetmgtPerformer *performer) ;

  // deserialize message
  bool_t deserialize (XDR *xdr) ;

  // ping entity
  bool_t pingEntity (struct in_addr agent_addr, 
		     u_long agent_prog, 
		     u_long agent_vers, 
		     struct timeval timeout) ;


  // send action report
  bool_t sendActionReport (NetmgtPerformer *aPerformer,
			   struct timeval delta_time, 
			   Netmgt_stat status, 
			   u_int flags) ;

  // send action message
  struct timeval *sendActionRequest (char *agent_name,
				     u_long agent_prog, 
				     u_long agent_vers, 
				     char *rendez_name,
				     u_long rendez_prog, 
				     u_long rendez_vers, 
				     struct timeval timeout, 
				     u_int flags,
				     struct in_addr *) ;

  // send data report
  bool_t sendDataReport (NetmgtPerformer *aPerformer,
			 struct timeval delta_time, 
			 Netmgt_stat status, 
			 u_int flags) ;

  // send error report
  bool_t sendErrorReport (NetmgtPerformer *aPerformer, Netmgt_error *error) ;

  // send event report
  bool_t sendEventReport (NetmgtPerformer *aPerformer,
			  struct timeval delta_time, 
			  Netmgt_stat status, 
			  u_int flags) ;

  // send data or event request to agent
  struct timeval *sendGetRequest (u_int request_type, 
				  char *agent_host, 
				  u_long agent_prog, 
				  u_long agent_vers, 
				  char *rendez_host, 
				  u_long rendez_prog, 
				  u_long rendez_vers, 
				  u_int count, 
				  struct timeval interval, 
				  struct timeval timeout, 
				  u_int flags) ;

  // send set request to agent
  bool_t sendSetRequest (char *agent_host, 
			 u_long agent_prog, 
			 u_long agent_vers, 
			 char *rendez_host,
			 u_long rendez_prog,
			 u_long rendez_vers,
			 struct timeval timeout, 
			 u_int flags) ;

  // send trap report
  bool_t sendTrapReport (NetmgtPerformer *aPerformer,
			 struct timeval delta_time, 
			 Netmgt_stat status, 
			 u_int flags) ;

  // serialize message
  bool_t serialize (XDR *xdr) ;

  // set message header
  bool_t setHeader (NetmgtServiceHeader *header) ;


  // public objects
public:
  
  // these objects have public access because they control
  // access to class members by themselves
  NetmgtArglist myArglist ;	 // argument list


   // private service functions
private:

  // setup and (possibly send) request message to agent
  struct timeval *setupRequest (u_int request_type, 
				struct in_addr agent_addr, 
				u_long agent_prog, 
				u_long agent_vers, 
				struct in_addr rendez_addr, 
				u_long rendez_prog, 
				u_long rendez_vers, 
				u_int count, 
				struct timeval interval, 
				struct timeval timeout, 
				u_int flags) ; 

  // send report message to rendezvous 
  bool_t sendReport (NetmgtPerformer *aPerformer, NetmgtRequestInfo *request);


// private variables
private:

  NetmgtEntity *myEntity ;	// entity referencing this message
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
  char key [NETMGT_NAMESIZ] ;	 // object instance name 
  u_int length ;		 // message data length

} ;

#endif _service_h
