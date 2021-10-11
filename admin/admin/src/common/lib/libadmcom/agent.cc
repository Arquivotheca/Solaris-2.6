#if !defined(lint) && !defined(NOID)
  static char sccsid[] = "@(#)agent.cc	1.46 5/5/91 SMI";
#endif

/**************************************************************************
 *  File:	lib/libnetmgt/agent.cc
 *
 *  Author:	Lynn Monsanto, Sun Microsystems Inc.
 *
 *  SCCSID:     @(#)agent.cc  1.46  91/05/05
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
 *  Comments:   agent initialization function
 *
 **************************************************************************
 */

/*LINTLIBRARY*/

#include "netmgt/netmgt_hdrs.h"
#include "netmgt/netmgt.h"
#include "libnetmgt/libnetmgt.h"
#include <thread.h>

/* ------------------------------------------------------------------------
 *  netmgt_init_rpc_agent - create and initialize RPC-based agent
 *	returns TRUE if successful; otherwise returns FALSE
 *  This function is called once when the agent is started either
 *  from the shell command line or from inetd (8c)
 * ------------------------------------------------------------------------
 */
bool_t 
netmgt_init_rpc_agent (char *name, 
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
		       void (*reap_child) (int sig, pid_t child_pid, int *stat_loc, 
					   caddr_t rusagep), 
#else
		       void (*reap_child) (int sig, int code, 
					   struct sigcontext *scp, char *addr, 
					   int child_pid, union wait *waitp, 
					   struct rusage *rusagep), 
#endif _SVR4_
#ifdef _SVR4_
		       void (*shutdown) (int sig))
#else
		       void (*shutdown) (int sig, int code, 
					 struct sigcontext *scp, char *addr))
#endif _SVR4_
{
  NETMGT_PRN (("agent: netmgt_init_rpc_agent\n")) ;
  
//  reserved++;
  
  // create an agent dispatcher 
  aNetmgtDispatcher = (NetmgtDispatcher *) calloc (1, sizeof (NetmgtDispatcher));
  if (!aNetmgtDispatcher)
    {
      if (netmgt_debug)
	perror ("agent: new failed");
      _netmgtStatus.setStatus (NETMGT_MALLOC, 0, strerror (errno));
      return FALSE;
    }
  
  // initialize agent dispatcher instance
  if (!aNetmgtDispatcher->myConstructor (name, 
					 serial,
					 program,
					 version,
					 proto,
					 timeout,
					 reserved,
					 flags,
					 verify,
					 dispatch,
					 reap_child,
					 shutdown))
    {
      (void) cfree ((caddr_t) aNetmgtDispatcher);
      aNetmgtDispatcher = (NetmgtDispatcher *) NULL;
      return FALSE;
    }
  
  return TRUE;
}

static void dummyHandler(int)
{
    return;

}



/* -------------------------------------------------------------------
 *  NetmgtDispatcher::myConstructor - agent initialization method
 *	returns TRUE if successful; otherwise returns FALSE
 * -------------------------------------------------------------------
 */
bool_t 
NetmgtDispatcher::myConstructor (char *myname, 
				 u_int myserial, 
				 u_long myprogram, 
				 u_long myversion, 
				 u_long myproto, 
				 struct timeval mytimeout, 
				 u_int myreserved, 
				 u_int myflags, 
				 boolean_t (*myverify) (u_int type, char *system, 
						   char *group, 
						   char *key, u_int count, 
						   struct timeval interval, 
						   u_int flags), 
				 void (*mydispatch) (u_int type, char *system, 
						   char *group, 
						   char *key, u_int count, 
						   struct timeval interval, 
						   u_int flags), 
#ifdef _SVR4_
				 void (*myreap_child) (int sig, pid_t child_pid, 
						     int *stat_loc, 
						     caddr_t rusagep),
#else
				 void (*myreap_child) (int sig, int code, 
						     struct sigcontext *scp,  
						     char *addr, int child_pid, 
						     union wait *waitp, 
						     struct rusage *rusagep), 
#endif _SVR4_
#ifdef _SVR4_
				 void (*myshutdown) (int sig)) 
#else
				 void (*myshutdown) (int sig, int code, 
						   struct sigcontext *scp, 
						   char *addr))
#endif  _SVR4_
{

  NETMGT_PRN (("NetmgtDispatcher::myConstructor\n"));

//  reserved++;
 
  //  reset error buffer 
  _netmgtStatus.clearStatus ();

  // set global agent pointer
  aNetmgtDispatcher = this;

  // initialize mutex
 int status;
  
  amsl_dispatcher = FALSE;
  
  if ( (status = mutex_init(& update_queues_mutex, USYNC_THREAD, NULL)) != 0)
    return(FALSE);


  // Set up the condition varible used to kill idle adminds
  if ((status = cond_init(&idle_cond_var, USYNC_THREAD, NULL)) != 0)
      return(FALSE);
  

  

  // initialize member variables inherited from NetmgtEntity class ...
  // set agent security levels
  if (!this->setSecurityLevels (myname))
    return FALSE;

  // initialize request options queue
  if (!this->myOptionsQueue.myConstructor ((u_int) 0))
    return FALSE;

  // initialize member variables inherited from NetmgtAgent class ...
  if (!myname)
    {
      NETMGT_PRN (("agent: no agent name\n"));
      _netmgtStatus.setStatus (NETMGT_NOAGENTNAME, 0, NULL);
      return FALSE;
    }
  this->name = strdup (myname) ;
  if (!this->name)
    {
      if (netmgt_debug)
	perror("agent: calloc");
      _netmgtStatus.setStatus (NETMGT_MALLOC, 0, strerror (errno));
      return FALSE;
    }
  this->serial = myserial;

  struct sockaddr_in localname ;	//  local IP name  
  (void) get_myaddress (&localname);
  (void) memcpy ((caddr_t) & this->local_addr, 
		 (caddr_t) & localname.sin_addr.s_addr, 
		 sizeof (struct in_addr));

  this->program = myprogram;
  this->version = myversion;
  this->timeout = mytimeout;
  this->proto = myproto;
  this->flags = myflags;
  this->idletime =  myreserved;
  this->state = NETMGT_INITIALIZING;

  // initialize set request argument queue
  if (!this->mySetargQueue .myConstructor ((u_int) 0))
    return FALSE;

  // initialize threshold argument queue
  if (!this->myThreshQueue.myConstructor ((u_int) 0))
    return FALSE;

  
  // initialize NetmgtAgent member variables ...

  //  set activity logfile path 
  char *value ;			 //  configuration value pointer 

  value = this->getConfig (NETMGT_ACTIVITY_LOG);
  if (!value)
    {
      NETMGT_PRN (("agent: can't get activity logfile path\n"));
      value = NETMGT_DEFAULT_ACTIVITY_LOG;
    }
  this->activity_log = strdup (value);
  if (!this->activity_log)
    {
      if (netmgt_debug)
	perror("agent: calloc");
      _netmgtStatus.setStatus (NETMGT_MALLOC, 0, strerror (errno));
      return FALSE;
    }

  //  set request logfile path 
  value = this->getConfig (NETMGT_REQUEST_LOG);
  if (!value)
    {
      NETMGT_PRN (("agent: can't get request logfile path\n"));
      value = NETMGT_DEFAULT_REQUEST_LOG;
    }
  this->request_log = strdup (value);
  if (!this->request_log)
    {
      if (netmgt_debug)
	perror("agent: calloc");
      _netmgtStatus.setStatus (NETMGT_MALLOC, 0, strerror (errno));
      return FALSE;
    }

  // initialize request queue
  if (!this->myRequestQueue.myConstructor ((u_int) 0))
    return FALSE;

  this->verify = myverify;
  this->dispatch = mydispatch;
  this->reap_child = myreap_child;
  this->shutdown = myshutdown;
  


  // register RPC service
#ifdef _SVR4_
  if (!this->registerTliService ())
#else
  if (!this->registerService ())
#endif _SVR4_
    return FALSE ;

  //  start verification interval timer 
  if (!this->setItimer ())
    {
      (void) pmap_unset (this->program, this->version);
      return FALSE;
    }

  // create and initialize related objects
  if (!this->initializeObjects ())
    return FALSE;

  // run as a detached process if not starting agent with inetd
  // and not debugging 
  if (! (this->flags & NETMGT_STARTED_BY_INETD) && netmgt_debug == 0)
    {
      int fd;			//  file descriptor 

      switch (fork1 ())
	{
	case -1:		//  error 
	  if (netmgt_debug)
	    perror ("agent: fork");
	  exit (errno);

	case 0:			//  child 

	  (void) close (0);
	  (void) close (1);
	  (void) close (2);
	  (void) open ("/dev/null", O_WRONLY);
	  (void) dup2 (0, 1);
	  (void) dup2 (1, 2);

#ifdef _SVR4_
	  (void) setsid ();
#else
	  fd = open ("/dev/tty", O_RDONLY);
	  if (fd >= 0)
	    {
	      (void) ioctl (fd, TIOCNOTTY, (char *) 0);
	      (void) close (fd);
	    }
#endif _SVR4_

	  break;

	default:		//  parent 
	  exit (0);
	}
    }

  // Reset the default handler for SIGCHLD

  struct sigaction vec;
  struct sigaction ovec;

  vec.sa_handler = dummyHandler;
  sigemptyset(&vec.sa_mask);
  vec.sa_flags = 0;
  sigaction(SIGCHLD, &vec, &ovec);



  sigset_t set;
  sigemptyset(&set);
  sigaddset(&set, SIGCHLD);
  sigaddset(&set,SIGHUP);
  sigaddset(&set,SIGINT);
  sigaddset(&set,SIGQUIT);
  sigaddset(&set,SIGTERM);

  thr_sigsetmask(SIG_BLOCK, &set, NULL);
  
  
  
  if((status = thr_create(NULL, 65000, (void * (*)(void *)) NetmgtDispatcher::threadReapRequest , this, 0, (thread_t *) & tid)) != 0) {
      
      perror("error thread create");
      return(FALSE);
  }


  //  restart any uncompleted requests 
  (void) this->restartRequests ();

  return TRUE;
}

/* ----------------------------------------------------------------------
 *  NetmgtDispatcher::initializeObjects - initialize object references
 *	returns TRUE if successful; othersize returns FALSE
 * ----------------------------------------------------------------------
 */
bool_t
NetmgtDispatcher::initializeObjects (void)
{

  NETMGT_PRN (("agent: NetmgtDispatcher::initializeObjects\n"));

  // create a message object for sending and receiving messages
  this->myServiceMsg 
    = (NetmgtServiceMsg *) calloc (1, sizeof (NetmgtServiceMsg));
  if (!this->myServiceMsg)
    {
      if (netmgt_debug)
	perror("agent: calloc");
      _netmgtStatus.setStatus (NETMGT_MALLOC, 0, strerror (errno));
      return FALSE;
    }

  // remember who's referencing this message
  this->myServiceMsg->setMyEntity (this);

  // create a request object for holding request information
  this->myRequest = (NetmgtRequest *) calloc (1, sizeof (NetmgtRequest));
  if (!this->myRequest)
    {
      if (netmgt_debug)
	perror("agent: calloc");
      _netmgtStatus.setStatus (NETMGT_MALLOC, 0, strerror (errno));
      return FALSE;
    }

  // remember who's referencing this request
  this->myRequest->setMyEntity (this);

  return TRUE;
}


