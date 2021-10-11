
/**************************************************************************
 *  File:	include/libnetmgt/request.h
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
 *  SCCSID:	@(#)request.h  1.36  91/08/01
 *
 *  Comments:	management request class
 *
 **************************************************************************
 */

#ifndef _request_h
#define _request_h

// foreward declaration
class NetmgtEntity;

// request information 

typedef struct 
{
  NetmgtEntity *myEntity ;	 // entity referencing this request
#ifdef _SVR4_
  pid_t pid ;			 // process ID handling request
#else
  int pid ;			 // process ID handling request
#endif _SVR4_
  struct timeval request_time ;  // request timestamp 
  u_int type ;			 // request type
  u_int handle ;                 // request handle 
  u_int sequence ;		 // request sequence number
  u_int flags ;			 // request flags
  u_int priority ;		 // request priority
  bool_t requested_flavor ;	 // whether authentication flavor requested
  int flavor ;			 // authentication flavor
#ifdef _SVR4_
  uid_t uid ;			 // requestor's user ID
  gid_t gid ;			 // requestor's group ID
  int gidlen ;			 // requestor's group ID list length
  gid_t gidlist [NGRPS] ;	 // requestor's group ID list
#else
  int uid ;			 // requestor's user ID
  int gid ;			 // requestor's group ID
  int gidlen ;			 // requestor's group ID list length
  int gidlist [NGRPS] ;		 // requestor's group ID list
#endif _SVR4_
  char netname [NETMGT_NAMESIZ] ; // client netname
  struct in_addr manager_addr ;  // manager IP address 
  struct in_addr rendez_addr ;	 // rendezvous IP address 
  u_long rendez_prog ;		 // rendezvous RPC program number 
  u_long rendez_vers ;		 // rendezvous RPC version number
  u_long proto;			 // transport protocol
  struct timeval interval ;	 // reporting interval
  u_int count ;			 // reporting count
  char system [NETMGT_NAMESIZ] ; // system name
  char group [NETMGT_NAMESIZ] ;  // group name
  char key [NETMGT_NAMESIZ] ;    // key name
  u_int length ;		 // argument list length
  struct timeval last_verified ; // time last verified 
}	NetmgtRequestInfo ;

// forward declaration
class NetmgtServiceMsg ;

// management request class ***********************************************

class NetmgtRequest: public NetmgtObject
{

  // public instantiation functions
public:

  // initialize request instance
  bool_t myConstructor (char *system,
			char *group,
			char *host,
			u_int prog,
			u_int vers,
			u_int priority) ;


  // public access function
public:

  // was an authentication flavor requested ?
  bool_t flavorRequested (void) { return this->info.requested_flavor; }

  // get authentication flavor, etc.
#ifdef _SVR4_
  int getAuthFlavor (uid_t *uid, 
		     gid_t *gid, 
		     int *numgids, 
		     gid_t gidlist [], 
		     u_int maxnetnamelen,
		     char netname []) ;
#else
  int getAuthFlavor (int *uid, 
		     int *gid, 
		     int *numgids, 
		     int gidlist [], 
		     u_int maxnetnamelen,
		     char netname []) ;
#endif _SVR4_

#ifdef _SVR4_
  pid_t getChildPid (u_int handle) ;
#else
  int getChildPid (u_int handle) ;
#endif _SVR4_

  // get group
  char *getGroup (void) { return this->info.group; }

  // get key
  char *getKey (void) { return this->info.key; }

  // get request data length
  u_int getLength (void) { return this->info.length; }

  // get entity referencing this request
  NetmgtEntity *getMyEntity (void) { return this->info.myEntity; }
 
  // get request priority
  u_int getPriority (void) { return this->info.priority; }

  // get request handle
  u_int getRequestHandle (void) { return this->info.handle; }

  // get request information
  void getRequestInfo (NetmgtRequestInfo *info) ;

  // get request timestamp
  struct timeval *getRequestTime (void) { return &this->info.request_time; }

  // get request sequence
  u_int getSequence (void) { return this->info.sequence; }

  // get system
  char *getSystem (void) { return this->info.system; }

  // get request type
  u_int getType (void) { return this->info.type; }

  // which authentication flavor should be used for request ?
  int whichAuthFlavor (void) { return this->info.flavor; }


  // public update function
public:

  // save request message information 
  void saveRequestInfo (NetmgtRequestInfo *info) ;

  // set authentication flavor
  bool_t setAuthFlavor (int flavor) 
    { 
      this->info.requested_flavor = TRUE;
      this->info.flavor = flavor;
      return TRUE; 
    }

  // set group
  void setGroup (char *group) 
    { 
      (void) strncpy (this->info.group, group, sizeof (this->info.group));
    }

  // set request handle
  void setRequestHandle (u_int handle) { this->info.handle = handle; }

  // set key
  void setKey (char *key) 
    { 
      (void) strncpy (this->info.key, key, sizeof (this->info.key));
    }

  // set time request was last verified
  bool_t setLastVerified (void) ;

  // set entity referencing this request
  void setMyEntity (NetmgtEntity *anEntity) { this->info.myEntity = anEntity; }

  // set request priority
  void setPriority (u_int priority) { this->info.priority = priority; }

  // set request time
  bool_t setRequestTime (void) ;

  // increment request sequence number
  void setSequence (void) 
    { 
      if (++this->info.sequence == 0xffffffff)
	this->info.sequence = 1;
    }

  // set system
  void setSystem (char *system) 
    { 
      (void) strncpy (this->info.system, system, sizeof (this->info.system));
    }


  // private member variables
private:

    NetmgtRequestInfo info ;	// request information

} ;

#endif _request_h
