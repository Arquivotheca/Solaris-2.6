#if !defined(lint) && !defined(NOID)
  static char sccsid[] = "@(#)rendez.cc	1.26 5/5/91 SMI";
#endif

/**************************************************************************
 *  File:	lib/libnetmgt/rendez.cc
 *
 *  Author:	Lynn Monsanto, Sun Microsystems Inc.
 *
 *  SCCSID:     @(#)rendez.cc  1.26  91/05/05
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
 *  Comments:	rendezvous initialization functions
 *
 **************************************************************************
 */

/*LINTLIBRARY*/

#include "netmgt/netmgt_hdrs.h"
#include "netmgt/netmgt.h"
#include "libnetmgt/libnetmgt.h"
#include <signal.h>

/* -------------------------------------------------------------------
 *  _netmgt_init_rpc_rendez - C wrapper to initialize rendezvous
 *	returns TRUE if successful; otherwise returns FALSE
 * -------------------------------------------------------------------
 */
bool_t
_netmgt_init_rpc_rendez (char *name, 
			 u_int serial, 
			 u_long program, 
			 u_long version, 
			 u_long proto, 
			 struct timeval timeout, 
			 u_int flags, 
			 void (*dispatch) (u_int type, char *system, 
					   char *group, char *key, 
					   u_int count, 
					   struct timeval interval, 
					   u_int flags),
#ifdef _SVR4_ 
			 void (*shutdown) (int sig))
#else
			 void (*shutdown) (int sig, int code, 
					   struct sigcontext *scp, 
					   char *addr))
#endif
{
  
  // create a rendezvous instance
  aNetmgtRendez = (NetmgtRendez *) calloc (1, sizeof (NetmgtRendez));
  if (!aNetmgtRendez)
    {
      if (netmgt_debug)
	perror ("callback: calloc failed\n");
      _netmgtStatus.setStatus (NETMGT_MALLOC, 0, strerror (errno));
      return FALSE;
    }
  
  // initialize rendezvous instance
  if (!aNetmgtRendez->myConstructor (name,
				     serial,
				     program,
				     version,
				     proto,
				     timeout,
				     flags,
				     dispatch,
				     shutdown))
    {
      (void) cfree ((caddr_t) aNetmgtRendez);
      aNetmgtRendez = (NetmgtRendez *) NULL;
      return FALSE;
    }
  
  return TRUE;
}


/* -------------------------------------------------------------------
 *  NetmgtRendez::myConstructor - rendezvous initialization method
 *	returns TRUE if successful; otherwise returns FALSE
 * -------------------------------------------------------------------
 */
bool_t 
NetmgtRendez::myConstructor (char *myname, 
			     u_int myserial, 
			     u_long myprogram, 
			     u_long myversion, 
			     u_long myproto, 
			     struct timeval mytimeout, 
			     u_int myflags, 
			     void (*mydispatch) (u_int type, char *system, 
					       char *group, char *key, 
					       u_int count, 
					       struct timeval interval, 
					       u_int flags),
#ifdef _SVR4_ 
			     void (*myshutdown) (int sig))
#else
			     void (*myshutdown) (int sig, int code, 
					       struct sigcontext *scp, 
					       char *addr))
#endif _SVR4_
{

  int fd;

  NETMGT_PRN (("NetmgtRendez::myConstructor\n"));
 
  // set global rendezvous pointer
  aNetmgtRendez = this;

  // initialize member variables
  if (!myname)
    {
      NETMGT_PRN (("rendez: no rendez name\n"));
      _netmgtStatus.setStatus (NETMGT_NOAGENTNAME, 0, NULL);
      return FALSE;
    }
  this->name = strdup (myname) ;
  if (!this->name)
    {
      if (netmgt_debug)
	perror("rendez: calloc");
      _netmgtStatus.setStatus (NETMGT_MALLOC, 0, strerror (errno));
      return FALSE;
    }

  struct sockaddr_in localname ; //  local IP name 
  (void) get_myaddress (&localname);
  (void) memcpy ((caddr_t) & this->local_addr, 
		 (caddr_t) & localname.sin_addr.s_addr, 
		 sizeof (struct in_addr));
  this->serial = myserial;
  this->program = myprogram;
  this->version = myversion;
  this->proto = myproto;
  this->timeout = mytimeout;

  // set rendezvous security levels
  if (!this->setSecurityLevels (myname))
    return FALSE;

  // create a message object for sending and receiving messages
  this->myServiceMsg 
    = (NetmgtServiceMsg *) calloc (1, sizeof (NetmgtServiceMsg));
  if (!this->myServiceMsg)
    {
      if (netmgt_debug)
	perror("rendez: calloc");
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
	perror("rendez: calloc");
      _netmgtStatus.setStatus (NETMGT_MALLOC, 0, strerror (errno));
      return FALSE;
    }

  // remember who's referencing this request
  this->myRequest->setMyEntity (this);

  // initialize report queue
  if (!this->myReportQueue.myConstructor (NETMGT_MAXREPORTS))
    return FALSE;

  this->flags = myflags;
  this->dispatch = mydispatch;

  // register rendezvous RPC service
#ifdef _SVR4_
  if (!this->registerTliService ())
#else
  if (!this->registerService ())
#endif _SVR4_
    return FALSE ;

  // declare shutdown handler 
#ifdef _SVR4_
  struct sigaction vec;		//  signal vector 
  struct sigaction ovec;	//  old signal vector 
#else
  struct sigvec vec;		//  signal vector 
  struct sigvec ovec;		//  old signal vector 
#endif

#ifdef _SVR4_
  if (myshutdown)
    vec.sa_handler = (SIG_PFV) myshutdown;
  else
    vec.sa_handler = (SIG_PFV) _netmgt_shutdownRendez;
  (void) sigemptyset (&vec.sa_mask);
  vec.sa_flags = 0;
  (void) sigaction (SIGINT, &vec, &ovec);
  (void) sigaction (SIGQUIT, &vec, &ovec);
  (void) sigaction (SIGTERM, &vec, &ovec);
#else
  if (myshutdown)
    vec.sv_handler = (SIG_PFV) myshutdown;
  else
    vec.sv_handler = (SIG_PFV) _netmgt_shutdownRendez;
  vec.sv_mask = 0;
  vec.sv_flags = 0;
  (void) sigvec (SIGINT, &vec, &ovec);
  (void) sigvec (SIGQUIT, &vec, &ovec);
  (void) sigvec (SIGTERM, &vec, &ovec);
#endif

  // run as a detached process if not starting rendez with inetd
  // and not debugging 
  if (! (this->flags & NETMGT_STARTED_BY_INETD) && netmgt_debug == 0)
    {
      switch (fork ())
	{
	case -1:		//  error 
	  if (netmgt_debug)
	    perror ("rendez: fork");
	  exit (errno);

	case 0:			//  child 
	  (void) close (0);
	  (void) close (1);
	  (void) close (2);
	  (void) open ("/dev/null", O_WRONLY);
	  (void) dup2 (0, 1);
	  (void) dup2 (1, 2);
	  fd = open ("/dev/tty", O_RDONLY);
	  if (fd >= 0)
	    {
#ifdef _SVR4_
	      (void) setsid ();
#else
	      (void) ioctl (fd, TIOCNOTTY, (char *) 0);
#endif _SVR4_
	      (void) close (fd);
	    }
	  break;

	default:		//  parent 
	  exit (0);
	}
    }

  return TRUE;
}

/* ---------------------------------------------------------------------
 *  _netmgt_get_priority - get event priority
 *	returns event priority
 * ---------------------------------------------------------------------
 */
u_int
_netmgt_get_priority (void)
{

  if (!aNetmgtRendez)
    return (u_int) 0;

  return aNetmgtRendez->myServiceMsg->getPriority ();
}

