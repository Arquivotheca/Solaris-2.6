#if !defined(lint) && !defined(NOID)
  static char sccsid[] = "@(#)server.cc	1.13 5/5/91 SMI";
#endif

/**************************************************************************
 *  File:	lib/libnetmgt/server.cc
 *
 *  Author:	Lynn Monsanto, Sun Microsystems Inc.
 *
 *  SCCSID:     @(#)server.cc  1.13  91/05/05
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
 *  Comments:   RPC service initialization routines
 *
 **************************************************************************
 */

/*LINTLIBRARY*/

#include "netmgt/netmgt_hdrs.h"
#include "netmgt/netmgt.h"
#include "libnetmgt/libnetmgt.h"

#ifndef _SVR4_
/* ------------------------------------------------------------------------
 *  NetmgtAgent::registerService - register agent RPC service
 *	returns TRUE if successful; otherwise returns FALSE
 * ------------------------------------------------------------------------
 */
bool_t
NetmgtAgent::registerService (void)
{

  NETMGT_PRN (("server: NetmgtAgent::registerService\n"));

  //  unregister the server RPC with portmapper 
  (void) pmap_unset (this->program, this->version);

  // use any socket if not starting with inetd
  int sock ; 			// socket descriptor
  struct sockaddr sockname ;	// socket name 
  int socknamelen ;		// socket name length
 
  socknamelen = sizeof (struct sockaddr);
  if (getsockname (0, &sockname, &socknamelen) == -1 && errno == ENOTSOCK)
    {
      sock = RPC_ANYSOCK;
      this->flags &= ~NETMGT_STARTED_BY_INETD;
    }

  // otherwise use file descriptor 0 
  else
    {
      sock = 0;
      this->flags |= NETMGT_STARTED_BY_INETD;
    }

  // get server transport handle and register service 
  SVCXPRT *tcp_xprt ;		//  TCP server tranport handle 
  SVCXPRT *udp_xprt ;		//  UDP server tranport handle 

  switch (proto)
    {
    case IPPROTO_UDP | IPPROTO_TCP:
      if (flags & NETMGT_STARTED_BY_INETD)
	{
	  NETMGT_PRN (("server: only one transport protocol"));
	  NETMGT_PRN (("may be used if starting with inetd\n"));
	  _netmgtStatus.setStatus (NETMGT_BADPROTO, 0, NULL);
	  return FALSE;
	}
      //  fall through 

    case IPPROTO_UDP:
      udp_xprt = svcudp_create (sock);
      if (!udp_xprt)
	{
	  NETMGT_PRN (("server: can't create UDP transport\n"));
	  _netmgtStatus.setStatus (NETMGT_SVCUDPCREATE, 0, NULL);
	  return FALSE;
	}
      if (!svc_register (udp_xprt, 
			 this->program,
			 this->version,
			 (SIG_PFV) _netmgt_receiveRequest, 
			 IPPROTO_UDP))
	{
	  NETMGT_PRN (("server: can't register UDP server\n"));
	  _netmgtStatus.setStatus (NETMGT_SVCREGISTER, 0, NULL);
	  return FALSE;
	}
      if ((proto & IPPROTO_TCP) != IPPROTO_TCP)
	break;
      //  else fall through 

    case IPPROTO_TCP:
      tcp_xprt = svctcp_create (sock, 0, 0);
      if (!tcp_xprt)
	{
	  NETMGT_PRN (("server: can't create TCP transport\n"));
	  _netmgtStatus.setStatus (NETMGT_SVCTCPCREATE, 0, NULL);
	  return FALSE;
	}
      if (!svc_register (tcp_xprt, 
			 this->program,
			 this->version,
			 (SIG_PFV) _netmgt_receiveRequest, 
			 IPPROTO_TCP))
	{
	  NETMGT_PRN (("server: can't register server\n"));
	  _netmgtStatus.setStatus (NETMGT_SVCREGISTER, 0, NULL);
	  return FALSE;
	}
      break;

    default:			//  error 
      NETMGT_PRN (("server: unknown transport: %d\n", proto));
      _netmgtStatus.setStatus (NETMGT_BADPROTO, 0, NULL);
      return FALSE;
    }
  return TRUE;
}

/* ------------------------------------------------------------------------
 *  NetmgtRendez::registerService - register rendezvous RPC service
 *	returns TRUE if successful; otherwise returns FALSE
 * ------------------------------------------------------------------------
 */
bool_t
NetmgtRendez::registerService (void)
{

  NETMGT_PRN (("server: NetmgtRendez::registerService\n"));

  //  unregister the server RPC with portmapper 
  (void) pmap_unset (this->program, this->version);

  // use any socket if not starting with inetd
  int sock ; 			// socket descriptor
  struct sockaddr sockname ;	// socket name 
  int socknamelen ;		// socket name length
 
  socknamelen = sizeof (struct sockaddr);
  if (getsockname (0, &sockname, &socknamelen) == -1 && errno == ENOTSOCK)
    {
      sock = RPC_ANYSOCK;
      this->flags &= ~NETMGT_STARTED_BY_INETD;
    }

  // otherwise use file descriptor 0 
  else
    {
      sock = 0;
      this->flags |= NETMGT_STARTED_BY_INETD;
    }

  // get server transport handle and register service 
  SVCXPRT *tcp_xprt ;		//  TCP server tranport handle 
  SVCXPRT *udp_xprt ;		//  UDP server tranport handle 

  switch (proto)
    {
    case IPPROTO_UDP | IPPROTO_TCP:
      if (flags & NETMGT_STARTED_BY_INETD)
	{
	  NETMGT_PRN (("server: only one transport protocol"));
	  NETMGT_PRN (("may be used if starting with inetd\n"));
	  _netmgtStatus.setStatus (NETMGT_BADPROTO, 0, NULL);
	  return FALSE;
	}
      //  fall through 

    case IPPROTO_UDP:
      udp_xprt = svcudp_create (sock);
      if (!udp_xprt)
	{
	  NETMGT_PRN (("server: can't create UDP transport\n"));
	  _netmgtStatus.setStatus (NETMGT_SVCUDPCREATE, 0, NULL);
	  return FALSE;
	}
      if (!svc_register (udp_xprt, 
			 this->program,
			 this->version,
			 (SIG_PFV) _netmgt_receiveReport, 
			 IPPROTO_UDP))
	{
	  NETMGT_PRN (("server: can't register UDP server\n"));
	  _netmgtStatus.setStatus (NETMGT_SVCREGISTER, 0, NULL);
	  return FALSE;
	}
      if ((proto & IPPROTO_TCP) != IPPROTO_TCP)
	break;
      //  else fall through 

    case IPPROTO_TCP:
      tcp_xprt = svctcp_create (sock, 0, 0);
      if (!tcp_xprt)
	{
	  NETMGT_PRN (("server: can't create TCP transport\n"));
	  _netmgtStatus.setStatus (NETMGT_SVCTCPCREATE, 0, NULL);
	  return FALSE;
	}
      if (!svc_register (tcp_xprt, 
			 this->program,
			 this->version,
			 (SIG_PFV) _netmgt_receiveReport, 
			 IPPROTO_TCP))
	{
	  NETMGT_PRN (("server: can't register server\n"));
	  _netmgtStatus.setStatus (NETMGT_SVCREGISTER, 0, NULL);
	  return FALSE;
	}
      break;

    default:			//  error 
      NETMGT_PRN (("server: unknown transport: %d\n", proto));
      _netmgtStatus.setStatus (NETMGT_BADPROTO, 0, NULL);
      return FALSE;
    }
  return TRUE;
}
#endif ! _SVR4_
