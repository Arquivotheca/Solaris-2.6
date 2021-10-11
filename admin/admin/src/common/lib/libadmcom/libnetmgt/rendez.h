
/**************************************************************************
 *  File:	include/libnetmgt/rendez.h
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
 *  SCCSID:	@(#)rendez.h  1.34  91/02/07
 *
 *  Comments:   rendezvous entity class
 *
 **************************************************************************
 */
#ifndef _rendez_h
#define _rendez_h

//  rendezvous class 
class NetmgtRendez: public NetmgtEntity
{

  // public instantiation functions
public:
  
  // initialize rendezvous instance
  bool_t myConstructor (char *name, 
			u_int serial, 
			u_long program, 
			u_long version, 
			u_long proto, 
			struct timeval timeout, 
			u_int flags, 
			void (*dispatch) (u_int type, char *system, char *group, 
					  char *key, u_int count, 
					  struct timeval interval, u_int flags), 
#ifdef _SVR4_
			void (*shutdown) (int sig)) ;
#else
			void (*shutdown) (int sig, int code, 
					  struct sigcontext *scp, char *addr)) ;
#endif _SVR4_

  // destroy rendezvous instance
#ifdef _SVR4_
  void myDestructor (int sig) ;
#else
  void myDestructor (int sig, int code, struct sigcontext *scp, char *addr) ;
#endif _SVR4_

  // start up RPC server
  void startupServer (void) ;


  // public access functions
public:

  // get report handle
  u_int getReportHandle (void) ;


  // public services functions
public:
  
  // register/unregister rendezvous with event dispatcher
  bool_t controlRendez (u_int flag, 
			char *eventd_host, 
			char *rendez_host, 
			u_long rendez_prog, 
			u_long rendez_vers, 
			u_long agent_prog, 
			u_int event_priority, 
			struct timeval timeout);

  // TLI dispatch callback function
  void dispatchTliCallback (struct svc_req *rqst, SVCXPRT *xprt) ;
  
  // dispatch callback function
  void dispatchCallback (struct svc_req *rqst, SVCXPRT *xprt) ;

  // dispatch request
  void dispatchReport (struct svc_req *rqst, 
		       register SVCXPRT *xprt) ;

  // receive report message
  void receiveReport (struct svc_req *rqst, register SVCXPRT *xprt) ;

  // TLI register callback function
  u_long registerTliCallback (void (*callbck) (u_int type, char *system, 
					        char *group, char *key, 
					        u_int count, 
					        struct timeval interval, 
					        u_int flags), 
			   int *fdp1, 
			   int *fdp2, 
			   u_long vers, 
			   u_long proto) ;

  // register callback function
  u_long registerCallback (void (*callbck) (u_int type, char *system, 
					     char *group, char *key, 
					     u_int count, 
					     struct timeval interval, 
					     u_int flags), 
			   int *udpSockp, 
			   int *tcpSockp, 
			   u_long vers, 
			   u_long proto) ;


  // private update functions
private:

  // initialize request queues
  bool_t initRequestQueues (void) ;


// private server functions
private:

  // register TLI RPC service
  bool_t registerTliService (void) ;

  // register RPC service
  bool_t registerService (void) ;


  // private objects 
private:

  NetmgtQueue myReportQueue;	// report queue 


  // private members variables
private:

  char *name ;			// name string 
  u_int serial ;		// serial number 
  struct in_addr local_addr ;	// IP address 
  u_long program ;		// RPC program number 
  u_long version ;		// RPC version number 
  u_long proto ;		// RPC transport protocol 
  u_int flags ;			// initialization flags 
  struct timeval timeout ;	// RPC timeout 
  int sock ;			// rendezvous client socket

  // user-defined request dispatch function
  void (* dispatch) (u_int type, char *system, char *group, char *key, 
		     u_int count, struct timeval interval, u_int flag) ;

  // user-defined SIGTERM, SIGQUIT, SIGINT handler
#ifdef _SVR4_
  void (* shutdown) (int sig) ;
#else
  void (* shutdown) (int sig, int code, struct sigcontext *scp, char *addr) ;
#endif _SVR4_

} ;

#endif  _rendez_h
