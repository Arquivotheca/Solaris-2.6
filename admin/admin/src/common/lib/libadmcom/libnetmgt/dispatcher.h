
/**************************************************************************
 *  File:	include/libnetmgt/dispatcher.h
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
 *  SCCSID:	@(#)dispatcher.h  1.21  91/07/08
 *
 *  Comments:	dispatcher agent class
 *
 **************************************************************************
 */

#ifndef _dispatcher_h
#define _dispatcher_h

#include <thread.h>

// dispatcher agent class 
class NetmgtDispatcher: public NetmgtAgent
{

  // friends 
  friend NetmgtPerformer::myConstructor (NetmgtDispatcher *aDispatcher) ;

  // public instantiation functions
public:

  // initialize performer instance
  bool_t myConstructor (char *name, 
			u_int serial, 
			u_long program, 
			u_long version, 
			u_long proto, 
			struct timeval timeout, 
			u_int reserved, 
			u_int flags, 
			boolean_t (*verify) (u_int type, char *system, char *group, 
					  char *key, u_int count, 
					  struct timeval interval, u_int flags), 
			void (*dispatch) (u_int type, char *system, char *group, 
					  char *key, u_int count, 
					  struct timeval interval, u_int flags), 
#ifdef _SVR4_
			void (*reap_child) (int sig, pid_t child_pid, 
					    int *stat_loc, caddr_t rusagep) ,
#else
			void (*reap_child) (int sig, int code, 
					    struct sigcontext *scp, char *addr, 
					    int child_pid, union wait *waitp, 
					    struct rusage *rusagep), 
#endif _SVR4_
#ifdef _SVR4_
			void (*shutdown) (int sig)) ;
#else
			void (*shutdown) (int sig, int code, 
					  struct sigcontext *scp, char *addr)) ;
#endif _SVR4_
  
  // destroy performer instance
#ifdef _SVR4_
  void myDestructor (int sig) ;
#else
  void myDestructor (int sig, int code, struct sigcontext *scp, char *addr) ;
#endif _SVR4_

  // start up RPC server
  void startupServer (void) ;


  // public access functions
public:

  // get child PID for a given request handle
#ifdef _SVR4_
  pid_t getChildPid (u_int handle) ;
#else
  int getChildPid (u_int handle) ;
#endif _SVR4_

  // get reaped request sequence number
  u_int getChildSequence (void) { return this->child_sequence; }

  // get request handle
  u_int getRequestHandle (void) ;


  // public service functions
public:
 
 // build trap report
  bool_t buildTrapReport (Netmgt_data *data) ;

  // delete request from activity logfile
  bool_t deleteActivity (NetmgtControlMsg *aControlMsg) ;

  // dispatch request
  void dispatchRequest (struct svc_req *rqst, register SVCXPRT *xprt) ;

  // kill unverified requests
#ifdef _SVR4_
  void killUnverified (int sig) ;
#else
  void killUnverified (int sig, int code, struct sigcontext *scp, char *addr) ;
#endif _SVR4_

  // log activity in activity logfile
  bool_t logActivity (NetmgtControlMsg *aControlMsg) ;

  // reap request
#ifdef _SVR4_
  void reapRequest (int sig) ;
#else
  void reapRequest (int sig, int code, struct sigcontext *scp, char *addr) ;
#endif _SVR4_

  // receive request
  void receiveRequest (struct svc_req *rqst, register SVCXPRT *xprt) ;

  // start trap reporting
  bool_t startTrap (char *system,
		    u_int agent_prog,
		    u_int agent_vers,
		    char *group,
		    char *host,
		    u_int prog,
		    u_int vers,
		    u_int priority,
		    struct timeval timeout);

  // verify performer is handling request
  bool_t verifyActivity (NetmgtControlMsg *aControlMsg) ;


// public update functions
public:

  // set agent state
  void setState (NetmgtAgentState set_state)  { this->state = set_state; }


  // private instantiation functions
private:

  // create and initialize related objects
  bool_t initializeObjects (void) ;


  // private service functions
private:

  // append request to request queue
#ifdef _SVR4_
  bool_t appendRequest (pid_t childPid) ;
#else
  bool_t appendRequest (int childPid) ;
#endif _SVR4_

  // append set request argument to argument queue
  bool_t appendSetarg (NetmgtSetarg *aSetarg) ;

  // append threshold argument to threshold queue
  bool_t appendThresh (Netmgt_thresh *thresh) ;

  // cache request 
#ifdef _SVR4_
  void cacheRequest (pid_t childPid) ;
#else
  void cacheRequest (int childPid) ;
#endif _SVR4_

  // clean up request
#ifdef _SVR4_
  bool_t cleanRequest (pid_t childPid) ;
#else
  bool_t cleanRequest (int childPid) ;
#endif _SVR4_

  // delete request from request logfile
  bool_t deleteRequest (NetmgtRequest *aRequest) ;

  // dispatch request for deferred reports
  bool_t dispatchDeferred (NetmgtControlMsg *aControlMsg) ;

  // get agent ID
  void getAgentID (void) ;

  // check whether activity is in request logfile
  bool_t inLogfile (FILE *logfile, NetmgtControlMsg *aControlMsg) ;

  // initialize request queues
  bool_t initRequestQueues (void) ;

  // log request in request logfile
  bool_t logRequest (void) ;

  // restart requests
  bool_t restartRequests (void) ;

  // set interval timer
  bool_t setItimer (void) ;

  // clear interval timer
  bool_t clearItimer (void) ;


  // terminate request
  bool_t terminateRequest (struct svc_req *rqst, 
			   register SVCXPRT *xprt,
			   NetmgtControlMsg *aControlMsg) ;

  // terminate all requests
  bool_t terminateAllRequests (struct svc_req *rqst, 
			       register SVCXPRT *xprt) ;

  // terminate request form one manager
  bool_t terminateMyRequests (struct svc_req *rqst, 
			      register SVCXPRT *xprt,
			      NetmgtControlMsg *aControlMsg) ;

  // terminate a single request
  bool_t terminateThisRequest (struct svc_req *rqst, 
			       register SVCXPRT *xprt,
			       NetmgtControlMsg *aControlMsg) ;


  // private objects
private:

  NetmgtQueue myRequestQueue ;	// request queue 


  // private member variables
private:

  char *activity_log ;		// activity logfile path 
  char *request_log ;		// request logfile path 
  u_int child_sequence ;	// reaped request sequence number


  // user-defined request dispatch function
  void (* dispatch) (u_int type, char *system, char *group, char *key, 
		     u_int count, struct timeval interval, u_int flag) ;

  // user-defined SIGCHLD handler
#ifdef _SVR4_
  void (* reap_child) (int sig, pid_t child_pid, int *stat_loc, 
 		       caddr_t usagep) ;
#else
  void (* reap_child) (int sig, int code, struct sigcontext *scp, char *addr,
		       int child_pid, union wait *pstatus, 
		       struct rusage *usagep) ;
#endif _SVR4_

  // user-defined SIGTERM, SIGQUIT, SIGINT handler
#ifdef _SVR4_
  void (* shutdown) (int sig) ;
#else
  void (* shutdown) (int sig, int code, struct sigcontext *scp, char *addr) ;
#endif _SVR4_
  
  // user-defined request verification function
  boolean_t (* verify) (u_int type, char *system, char *group, char *key, 
		     u_int count, struct timeval interval, u_int flags) ;
 public:

  mutex_t update_queues_mutex;
  cond_t idle_cond_var;
  thread_t tid; // Thread id for the thread waiting for child terminations
  static void * threadReapRequest(NetmgtDispatcher * This);
  static void * idleTimer(NetmgtDispatcher * This);
  bool_t amsl_dispatcher;



} ;

#endif !_dispatcher_h



