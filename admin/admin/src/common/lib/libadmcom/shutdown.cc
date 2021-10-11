#if !defined(lint) && !defined(NOID)
  static char sccsid[] = "@(#)shutdown.cc	1.33 5/5/91 SMI";
#endif

/**************************************************************************
 *  File:	lib/libnetmgt/shutdown.cc
 *
 *  Author:	Lynn Monsanto, Sun Microsystems Inc.
 *
 *  SCCSID:     @(#)shutdown.cc  1.33  91/05/05
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
 *  Comments:	agent and rendezvous shutdown functions
 *
 **************************************************************************
 */

/*LINTLIBRARY*/

#include "netmgt/netmgt_hdrs.h"
#include "netmgt/netmgt.h"
#include "libnetmgt/libnetmgt.h"

/* --------------------------------------------------------------------
 *  netmgt_shutdown_agent - C wrapper for agent shutdown function
 *	no return
 * --------------------------------------------------------------------
 */
void
#ifdef _SVR4_
netmgt_shutdown_agent (int sig)
#else
netmgt_shutdown_agent (int sig, int code, struct sigcontext *scp, char *addr)
#endif _SVR4_
{

  NETMGT_PRN (("shutdown: netmgt_shutdown_agent\n"));
  if (aNetmgtDispatcher)
#ifdef _SVR4_
    aNetmgtDispatcher->myDestructor (sig);
#else
    aNetmgtDispatcher->myDestructor (sig, code, scp, addr);
#endif _SVR4_

  // for compatibility with SNM 1.0
  else
    exit (0);

}

/* --------------------------------------------------------------------
 *  NetmgtDispatcher::myDestructor - destroy agent object and exit
 *	no return
 * --------------------------------------------------------------------
 */
void
#ifdef _SVR4_
NetmgtDispatcher::myDestructor (int sig)
#else
NetmgtDispatcher::myDestructor (int sig, 
				int code, 
				struct sigcontext *scp, 
				char *addr)
#endif _SVR4_
{

  NETMGT_PRN (("shutdown: NetmgtDispatcher::myDestructor\n"));

  thr_kill(tid, SIGTERM);
  
  // unregister agent RPC with portmapper if not started by inetd
  if (! (this->flags & NETMGT_STARTED_BY_INETD))
#ifdef _SVR4_
    (void) rpcb_unset (this->program, this->version, (struct netconfig *)NULL);
#else
    (void) pmap_unset (this->program, this->version);
#endif _SVR4_

  NETMGT_PRN (("shutdown: agent [%d] exiting ...\n, getpid ()"));
  exit (0);
}

/* --------------------------------------------------------------------
 *  _netmgt_shutdownRendez - destroy rendez object and exit
 *	no return
 * --------------------------------------------------------------------
 */
void
#ifdef _SVR4_
_netmgt_shutdownRendez (int sig)
#else
_netmgt_shutdownRendez (int sig, int code, struct sigcontext *scp, char *addr)
#endif _SVR4_
{

  NETMGT_PRN (("shutdown: _netmgt_shutdownRendez\n"));
  assert (aNetmgtRendez != (NetmgtRendez *) NULL);
#ifdef _SVR4_
  aNetmgtRendez->myDestructor (sig);
#else
  aNetmgtRendez->myDestructor (sig, code, scp, addr);
#endif _SVR4_
}

/* --------------------------------------------------------------------
 *  NetmgtRendez::myDestructor - destroy agent object and exit
 *	no return
 * --------------------------------------------------------------------
 */
void
#ifdef _SVR4_
NetmgtRendez::myDestructor (int sig)
#else 
NetmgtRendez::myDestructor (int sig, 
			    int code, 
			    struct sigcontext *scp, 
			    char *addr)
#endif _SVR4_
{

  NETMGT_PRN (("shutdown: NetmgtRendez::myDestructor\n"));

  // unregister rendezvous RPC with portmapper if not started by inetd
  if (! (this->flags & NETMGT_STARTED_BY_INETD))
#ifdef _SVR4_
    (void) rpcb_unset (this->program, this->version, (struct netconfig *)NULL);
#else
    (void) pmap_unset (this->program, this->version);
#endif _SVR4_

  NETMGT_PRN (("shutdown: rendezvous [%d] exiting ...\n", getpid ()));
  exit (0);
}
