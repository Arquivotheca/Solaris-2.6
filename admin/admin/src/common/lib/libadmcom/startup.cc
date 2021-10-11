#if !defined(lint) && !defined(NOID)
  static char sccsid[] = "@(#)startup.cc	1.31 5/5/91 SMI";
#endif

/**************************************************************************
 *  File:	lib/libnetmgt/startup.cc
 *
 *  Author:	Lynn Monsanto, Sun Microsystems Inc.
 *
 *  SCCSID:     @(#)startup.cc  1.31  91/05/05
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
 *  Comments:	RPC agent starup method
 *
 **************************************************************************
 */

/*LINTLIBRARY*/

#include "netmgt/netmgt_hdrs.h"
#include "netmgt/netmgt.h"
#include "libnetmgt/libnetmgt.h"

/* --------------------------------------------------------------------
 *  netmgt_start_agent - C wrapper to start RPC agent server
 *	no return value
 * --------------------------------------------------------------------
 */
void
netmgt_start_agent (void)
{
  NETMGT_PRN (("startup: netmgt_start_agent\n"));

    // tell agent to start up RPC server
  if (aNetmgtDispatcher)
    aNetmgtDispatcher->startupServer ();
  
  // for compatibiltiy with Release 1.0
  else
    svc_run ();

  // no return
}

/* --------------------------------------------------------------------
 *  NetmgtDispatcher::startupServer - startup agent RPC server
 *	no return value
 * --------------------------------------------------------------------
 */
void
NetmgtDispatcher::startupServer (void)
{
  NETMGT_PRN (("startupServer: NetmgtDispatcher::startupServer\n"));

  // enter RPC server select loop 
  svc_run ();

  // should never return 
  NETMGT_PRN (("startupServer: svc_run returned abnormally\n"));
  (void) exit (1);
}

/* --------------------------------------------------------------------
 *  _netmgt_start_rendez - C wrapper to start RPC rendezvous server
 *	no return value
 * --------------------------------------------------------------------
 */
void
_netmgt_start_rendez (void)
{
  NETMGT_PRN (("startup: _netmgt_start_rendez\n"));
  assert (aNetmgtRendez != (NetmgtRendez *) NULL);
  
  // tell rendez to start up RPC server
  aNetmgtRendez->startupServer ();

  // no return
  /*NOTREACHED*/
}

/* --------------------------------------------------------------------
 *  NetmgtRendez::startupServer - startup rendezvous RPC server
 *	no return value
 * --------------------------------------------------------------------
 */
void
NetmgtRendez::startupServer (void)
{
  NETMGT_PRN (("startupServer: NetmgtRendez::startupServer\n"));

  // enter RPC server select loop 
  svc_run ();

  // should never return 
  NETMGT_PRN (("startupServer: svc_run returned abnormally\n"));
  (void) exit (1);
}
