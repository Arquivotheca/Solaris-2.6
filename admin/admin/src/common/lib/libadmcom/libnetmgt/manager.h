
/**************************************************************************
 *  File:	include/libnetmgt/manager.h
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
 *  SCCSID:	@(#)manager.h  1.22  91/01/14
 *
 *  Comments:   manager entity class
 *
 **************************************************************************
 */
#ifndef _manager_h
#define _manager_h

//  manager entity class 
class NetmgtManager: public NetmgtEntity
{

  // public instantiation functions
public:

  bool_t myConstructor (void) ;


  // public access functions
public:

  // get request arglist pointer --- this hack should go away
  NetmgtArglist *getRequestArglist (void) ;

  // get request message pointer --- this hack should also go away
  NetmgtServiceMsg *getRequestMsg (void) ;


  // public update functions
public:

  // set object instance for request
  bool_t setObjectInstance (char *system, char *group, char *key) ;

  // set request handle
  bool_t setRequestHandle (u_int handle) ;


  // public service functions
public:

  // send data or event request message to agent
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

} ;

#endif  _manager_h
