#if !defined(lint) && !defined(NOID)
  static char sccsid[] = "@(#)callback.cc	1.41 5/5/91 SMI";
#endif

/**************************************************************************
 *  File:	lib/libnetmgt/callback.cc
 *
 *  Author:	Lynn Monsanto, Sun Microsystems Inc.
 *
 *  SCCSID:     @(#)callback.cc  1.41  91/05/05
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
 *  Comments:	RPC callback routines
 *
 **************************************************************************
 */

/*LINTLIBRARY*/

#include "netmgt/netmgt_hdrs.h"
#include "netmgt/netmgt.h"
#include "libnetmgt/libnetmgt.h"

#ifndef _SVR4_
// global data 
static void (*_netmgt_callback) (u_int type, char *system, char *group, 
				 char *key, u_int count, 
				 struct timeval interval, u_int flags) ;

// static functions 
static void _netmgt_dispatchCallback (struct svc_req *rqst, SVCXPRT *xprt) ;

static u_long _netmgt_getTransient (int *udpSockp, int *tcpSockp, 
				    u_long proto, u_long vers) ;
#endif _SVR4_

/* -------------------------------------------------------------------------
 *  netmgt_register_callback - C wrapper to NetmgtRendez::registerCallback
 *	returns RPC program number if successful; otherwise -1
 * -------------------------------------------------------------------------
 */
u_long 
netmgt_register_callback (void (*callbck) (u_int type, char *system, 
					    char *group, char *key, 
					    u_int count, 
					    struct timeval interval, 
					    u_int flags), 
			  int *udpFdp, 
			  int *tcpFdp, 
			  u_long vers, 
			  u_long proto)
     // callback function
     // UDP/IP socket or file descriptor
     // TCP/IP socket or file descriptor
     // RPC version number
     // transport protocol (either IPPROTO_UDP or IPPROTO_TCP)
{
  
  NETMGT_PRN (("callback: netmgt_register_callback\n"));

  // create a rendezvous object 
  aNetmgtRendez = (NetmgtRendez *) calloc (1, sizeof (NetmgtRendez));
  if (!aNetmgtRendez)
    {
      if (netmgt_debug)
	perror ("callback: calloc failed\n");
      _netmgtStatus.setStatus (NETMGT_MALLOC, 0, strerror (errno));
      return FALSE;
    }

  // allocate a rendezvous message
  aNetmgtRendez->myServiceMsg 
    = (NetmgtServiceMsg *) calloc (1, sizeof (NetmgtServiceMsg));
  if (!aNetmgtRendez->myServiceMsg)
    {
      if (netmgt_debug)
	perror ("callback: calloc failed\n");
      _netmgtStatus.setStatus (NETMGT_MALLOC, 0, strerror (errno));
      (void) cfree ((caddr_t) aNetmgtRendez);
      aNetmgtRendez = (NetmgtRendez *) NULL;
      return FALSE;
    }
    
  // remember who's referencing this message
  aNetmgtRendez->myServiceMsg->setMyEntity (aNetmgtRendez);

  // create a request object for holding request information
  aNetmgtRendez->myRequest
    = (NetmgtRequest *) calloc (1, sizeof (NetmgtRequest));
  if (!aNetmgtRendez->myRequest)
    {
      if (netmgt_debug)
	perror("callback: calloc");
      _netmgtStatus.setStatus (NETMGT_MALLOC, 0, strerror (errno));
      return FALSE;
    }

  // remember who's referencing this request
  aNetmgtRendez->myRequest->setMyEntity (aNetmgtRendez);

#ifdef _SVR4_
  return aNetmgtRendez->registerTliCallback (callbck,
					     udpFdp,
					     tcpFdp,
					     vers,
					     proto);
#else
  return aNetmgtRendez->registerCallback (callback,
					  udpFdp,
					  tcpFdp,
					  vers,
					  proto);
#endif _SVR4_
}

#ifndef _SVR4_
/* --------------------------------------------------------------------
 *  NetmgtRendez::registerCallback - this function: 
 *	allocates a transient RPC program number
 *	creates RPC transport handle(s) for *udpSockp and/or *tcpSockp
 *	registers the transient RPC program number with the portmapper
 *
 *	returns the transient program number and sets *udpSockp and/or
 *	*tcpSockp if successful; otherwise, returns (u_long)0.
 * --------------------------------------------------------------------
 */
u_long 
NetmgtRendez::registerCallback (void (*callbck) (u_int type, char *system, 
						  char *group, char *key, 
						  u_int count, 
						  struct timeval interval, 
						  u_int flags), 
				 int *udpSockp, 
				 int *tcpSockp, 
				 u_long vers, 
				 u_long proto)
     // callback function
     // UDP socket pointer
     // TCP socket pointer
     // RPC version
     // RPC transport protocol
{
  SVCXPRT *udpXprt ;		// UDP/IP server transport handle 
  SVCXPRT *tcpXprt ;		// TCP/IP server transport handle 
  u_long prog ;			// transient program number 

  NETMGT_PRN (("callback: NetmgtRendez::registerCallback\n")) ;

  // reset internal error code 
  _netmgtStatus.clearStatus ();

  // verify input 
  if (!callbck)
    {
      NETMGT_PRN (("callback: no callback function\n"));
      _netmgtStatus.setStatus (NETMGT_NODISPATCH, 0, NULL);
      return 0;
    }
  if ((proto & IPPROTO_UDP) && udpSockp == (int *) NULL)
    {
      NETMGT_PRN (("callback: null UDP/IP socket pointer\n"));
      _netmgtStatus.setStatus (NETMGT_NULLSOCKET, 0, NULL);
      return 0;
    }
  if ((proto & IPPROTO_TCP) && tcpSockp == (int *) NULL)
    {
      NETMGT_PRN (("callback: null TCP/IP socket pointer\n"));
      _netmgtStatus.setStatus (NETMGT_NULLSOCKET, 0, NULL);
      return 0;
    }

  // save the callback function address 
  _netmgt_callback = callbck;

  // get a transient RPC program number 
  prog = _netmgt_getTransient (udpSockp, tcpSockp, proto, vers);
  if (!prog)
    {
      NETMGT_PRN (("callback: can't get transient rpc\n"));
      return 0;
    }

  // get a transport handle 
  switch (proto)
    {
    case IPPROTO_UDP:
      udpXprt = svcudp_create (*udpSockp);
      if (!udpXprt)
	{
	  NETMGT_PRN (("callback: svcudp_create failed\n"));
	  _netmgtStatus.setStatus (NETMGT_SVCUDPCREATE, 0, NULL);
	  return 0;
	}
      break;

    case IPPROTO_TCP:
      tcpXprt = svctcp_create (*tcpSockp, (u_int) 0, (u_int) 0);
      if (!tcpXprt)
	{
	  NETMGT_PRN (("callback: svctcp_create failed\n"));
	  _netmgtStatus.setStatus (NETMGT_SVCTCPCREATE, 0, NULL);
	  return 0;
	}
      break;

    case IPPROTO_UDP|IPPROTO_TCP:
      udpXprt = svcudp_create (*udpSockp);
      if (!udpXprt)
	{
	  NETMGT_PRN (("callback: svcudp_create failed\n"));
	  _netmgtStatus.setStatus (NETMGT_SVCUDPCREATE, 0, NULL);
	  return 0;
	}

      tcpXprt = svctcp_create (*tcpSockp, (u_int) 0, (u_int) 0);
      if (!tcpXprt)
	{
	  NETMGT_PRN (("callback: svctcp_create failed\n"));
	  _netmgtStatus.setStatus (NETMGT_SVCTCPCREATE, 0, NULL);
	  (void) close (*udpSockp);
	  return 0;
	}
      break;

    default:
      NETMGT_PRN (("callback: unknown protocol: %d\n", proto));
      _netmgtStatus.setStatus (NETMGT_BADPROTO, 0, NULL);
      return 0;
    }

  // register the transient program number with the portmapper 
  switch (proto)
    {
    case IPPROTO_UDP:
      if (!svc_register (udpXprt, prog, vers, 
			 (SIG_PFV) _netmgt_dispatchCallback, IPPROTO_UDP))
	{
	  NETMGT_PRN (("callback: can't register transient rpc\n"));
          _netmgtStatus.setStatus (NETMGT_SVCREGISTER, 0, NULL);
	  (void) close (*udpSockp);
	  return 0;
	}
      return prog;
  
    case IPPROTO_TCP:
      if (!svc_register (tcpXprt, prog, vers, 
			 (SIG_PFV) _netmgt_dispatchCallback, IPPROTO_TCP))
	{
	  NETMGT_PRN (("callback: can't register transient rpc\n"));
          _netmgtStatus.setStatus (NETMGT_SVCREGISTER, 0, NULL);
	  (void) close (*tcpSockp);
	  return 0;
	}
      return prog;

    case IPPROTO_UDP|IPPROTO_TCP:
      if (!svc_register (udpXprt, prog, vers, 
			 (SIG_PFV) _netmgt_dispatchCallback, IPPROTO_UDP))
	{
	  NETMGT_PRN (("callback: can't register transient rpc\n"));
          _netmgtStatus.setStatus (NETMGT_SVCREGISTER, 0, NULL);
	  (void) close (*udpSockp);
	  (void) close (*tcpSockp);
	  return 0;
	}
      if (!svc_register (tcpXprt, prog, vers, 
			 (SIG_PFV) _netmgt_dispatchCallback, IPPROTO_TCP))
	{
	  NETMGT_PRN (("callback: can't register transient rpc\n"));
          _netmgtStatus.setStatus (NETMGT_SVCREGISTER, 0, NULL);
	  (void) close (*udpSockp);
	  (void) close (*tcpSockp);
	  return 0;
	}
      return prog;

    default:
      NETMGT_PRN (("callback: unknown protocol: %d\n", proto));
      _netmgtStatus.setStatus (NETMGT_BADPROTO, 0, NULL);
      return 0;
    }
  /*NOTREACHED*/
}

/* ---------------------------------------------------------------------
 *  _netmgt_getTransient - get a transient RPC program number
 *	returns transient RPC number if successful; otherwise returns 0
 * ---------------------------------------------------------------------
 */
static u_long
_netmgt_getTransient (int *udpSockp, int *tcpSockp, u_long proto, u_long vers)
     // UDP socket pointer
     // TCP socket pointer
     // RPC transport protocol
     // RPC version number
{
  static u_long prog;		// transient RPC program number 
  int length;			// address length 
  struct sockaddr_in udpName;	// UDP socket name 
  struct sockaddr_in tcpName;	// TCP socket name 

  NETMGT_PRN (("callback: _netmgt_getTransient\n"));

  // reset internal error code 
  _netmgtStatus.clearStatus ();

  // create socket(s) 
  switch (proto)
    {
    case IPPROTO_UDP:
      assert (udpSockp != (int *) NULL);

      *udpSockp = socket (AF_INET, SOCK_DGRAM, 0);
      if (*udpSockp == -1)
	{
	  if (netmgt_debug)
	    perror ("callcack: socket failed");
	  _netmgtStatus.setStatus (NETMGT_SOCKET, 0, strerror (errno));
	  return 0;
	}
      break;

    case IPPROTO_TCP:
      assert (tcpSockp != (int *) NULL);

      *tcpSockp = socket (AF_INET, SOCK_STREAM, 0);
      if (*tcpSockp == -1)
	{
	  if (netmgt_debug)
	    perror ("callcack: socket failed");
	  _netmgtStatus.setStatus (NETMGT_SOCKET, 0, strerror (errno));
	  return 0;
	}
      break;

    case IPPROTO_UDP|IPPROTO_TCP:
      assert (udpSockp != (int *) NULL);
      assert (tcpSockp != (int *) NULL);

      *udpSockp = socket (AF_INET, SOCK_DGRAM, 0);
      if (*udpSockp == -1)
	{
	  if (netmgt_debug)
	    perror ("callcack: socket failed");
	  _netmgtStatus.setStatus (NETMGT_SOCKET, 0, strerror (errno));
	  return 0;
	}
      *tcpSockp = socket (AF_INET, SOCK_STREAM, 0);
      if (*tcpSockp == -1)
	{
	  if (netmgt_debug)
	    perror ("callcack: socket failed");
	  _netmgtStatus.setStatus (NETMGT_SOCKET, 0, strerror (errno));
	  return 0;
	}
      break;

    default:
      NETMGT_PRN (("callback: unknown protocol: %d\n", proto));
      _netmgtStatus.setStatus (NETMGT_BADPROTO, 0, NULL);
      return 0;
    }

  // bind socket(s) 
  switch (proto)
    {
    case IPPROTO_UDP:
      udpName.sin_addr.s_addr = 0;
      udpName.sin_family = AF_INET;
      udpName.sin_port = 0;
      length = sizeof (udpName);

      if (bind (*udpSockp, (struct sockaddr *) & udpName, length) == -1)
	{
	  if (netmgt_debug)
	    perror ("callcack: bind failed");
	  _netmgtStatus.setStatus (NETMGT_BIND, 0, strerror (errno));
	  (void) close (*udpSockp);
	  return 0;
	}
      break;

    case IPPROTO_TCP:
      tcpName.sin_addr.s_addr = 0;
      tcpName.sin_family = AF_INET;
      tcpName.sin_port = 0;
      length = sizeof (tcpName);

      if (bind (*tcpSockp, (struct sockaddr *) & tcpName, length) == -1)
	{
	  if (netmgt_debug)
	    perror ("callcack: bind failed");
	  _netmgtStatus.setStatus (NETMGT_BIND, 0, strerror (errno));
	  (void) close (*tcpSockp);
	  return 0;
	}
      break;

    case IPPROTO_UDP|IPPROTO_TCP:
      udpName.sin_addr.s_addr = 0;
      udpName.sin_family = AF_INET;
      udpName.sin_port = 0;
      length = sizeof (udpName);

      if (bind (*udpSockp, (struct sockaddr *) & udpName, length) == -1)
	{
	  if (netmgt_debug)
	    perror ("callcack: bind failed");
	  _netmgtStatus.setStatus (NETMGT_BIND, 0, strerror (errno));
	  (void) close (*udpSockp);
	  (void) close (*tcpSockp);
	  return 0;
	}

      tcpName.sin_addr.s_addr = 0;
      tcpName.sin_family = AF_INET;
      tcpName.sin_port = 0;
      length = sizeof (tcpName);

      if (bind (*tcpSockp, (struct sockaddr *) & tcpName, length) == -1)
	{
	  if (netmgt_debug)
	    perror ("callcack: bind failed");
	  _netmgtStatus.setStatus (NETMGT_BIND, 0, strerror (errno));
	  (void) close (*udpSockp);
	  (void) close (*tcpSockp);
	  return 0;
	}
      break;

    default:
      NETMGT_PRN (("callback: unknown protocol: %d\n", proto));
      _netmgtStatus.setStatus (NETMGT_BADPROTO, 0, NULL);
      return 0;
    }

  // get local port(s) 
  switch (proto)
    {
    case IPPROTO_UDP:
      if (getsockname (*udpSockp, (struct sockaddr *) &udpName, &length) == -1)
	{
	  if (netmgt_debug)
	    perror ("callback: getsockname failed");
	  _netmgtStatus.setStatus (NETMGT_GETSOCKNAME, 0, strerror (errno));
	  (void) close (*udpSockp);
	  return 0;
	}
      break;

    case IPPROTO_TCP:
      if (getsockname (*tcpSockp, (struct sockaddr *) &tcpName, &length) == -1)
	{
	  if (netmgt_debug)
	    perror ("callback: getsockname failed");
	  _netmgtStatus.setStatus (NETMGT_GETSOCKNAME, 0, strerror (errno));
	  (void) close (*tcpSockp);
	  return 0;
	}
      break;

    case IPPROTO_UDP|IPPROTO_TCP:
      if (getsockname (*udpSockp, (struct sockaddr *) &udpName, &length) == -1)
	{
	  if (netmgt_debug)
	    perror ("callback: getsockname failed");
	  _netmgtStatus.setStatus (NETMGT_GETSOCKNAME, 0, strerror (errno));
	  (void) close (*udpSockp);
	  (void) close (*tcpSockp);
	  return 0;
	}
      if (getsockname (*tcpSockp, (struct sockaddr *) &tcpName, &length) == -1)
	{
	  if (netmgt_debug)
	    perror ("callback: getsockname failed");
	  _netmgtStatus.setStatus (NETMGT_GETSOCKNAME, 0, strerror (errno));
	  (void) close (*udpSockp);
	  (void) close (*tcpSockp);
	  return 0;
	}
      break;

    default:
      NETMGT_PRN (("callback: unknown protocol: %d\n", proto));
      _netmgtStatus.setStatus (NETMGT_BADPROTO, 0, NULL);
      return 0;
    }

  // get the first free transient RPC program number 
  prog = NETMGT_TRANSIENT;
  for (;;)			// keep trying 
    {
      switch (proto)
	{
	case IPPROTO_UDP:
	  if (pmap_set (prog, vers, IPPROTO_UDP, udpName.sin_port))
	    return prog;
	  break;

	case IPPROTO_TCP:
	  if (pmap_set (prog, vers, IPPROTO_TCP, tcpName.sin_port))
	    return prog;
	  break;

	case IPPROTO_UDP|IPPROTO_TCP:
	  if (!pmap_set (prog, vers, IPPROTO_UDP, udpName.sin_port))
	    break;
	  
	  if (!pmap_set (prog, vers, IPPROTO_TCP, tcpName.sin_port))
	    {
	      // we must have set the IPPROTO_UDP service 
	      (void) pmap_unset (prog, vers);
	      break;
	    }

	  return prog;

	default:
	  NETMGT_PRN (("callback: unknown protocol: %d\n", proto));
	  _netmgtStatus.setStatus (NETMGT_BADPROTO, 0, NULL);
	  return 0;
	}

      // try to pmap_set next program number 
      prog++;
    }
  /*NOTREACHED*/
}

/* -----------------------------------------------------------------------
 *  _netmgt_dispatchCallback - dispatch callback function
 *	no return value
 * -----------------------------------------------------------------------
 */
static void
_netmgt_dispatchCallback (struct svc_req *rqst, SVCXPRT *xprt)
                          	// server request structure 
                            	// server transport handle 
{

  NETMGT_PRN (("callback: _netmgt_dispatchCallback\n"));

  assert (aNetmgtRendez != (NetmgtRendez *) NULL);
  aNetmgtRendez->dispatchCallback (rqst, xprt);
  return;
}

/* -----------------------------------------------------------------------
 *  NetmgtRendez::dispatchCallback - dispatch callback function
 *	no return value
 * -----------------------------------------------------------------------
 */
void
NetmgtRendez::dispatchCallback (struct svc_req *rqst, SVCXPRT *xprt)
{
  NetmgtServiceHeader header ;	// request message header

  NETMGT_PRN (("callback: NetmgtRendez::dispatchCallback\n"));

  assert (rqst != (struct svc_req *) NULL);
  assert (xprt != (SVCXPRT *) NULL);

  // reset internal error code 
  _netmgtStatus.clearStatus ();

  // receive the event_report message from the RPC transport 
  switch (rqst->rq_proc)
    {
    case NETMGT_SERVICE_PROC:	// request procedure 

      // get report from the RPC transport
      assert (this->myServiceMsg != (NetmgtServiceMsg *) NULL);
      if (!svc_getargs (xprt, 
			(xdrproc_t) _netmgt_deserialMsg, 
			(char *) this->myServiceMsg))
	{
	  NETMGT_PRN (("callback: svc_getargs failed\n"));
	  (void) svc_sendreply (xprt, 
				(xdrproc_t)_netmgt_xdrStatus,
				(char *) &netmgt_error);
	  return;
	}

      // send a confirm message to sender that the message
      // was received correctly
      (void) svc_sendreply (xprt, 
			    (xdrproc_t)_netmgt_xdrStatus, 
			    (char *) &netmgt_error);

      // get report message header
      if (!this->myServiceMsg->getHeader (&header))
	return;
      
      // reset arglist pointer 
      this->myServiceMsg->myArglist.resetPtr ();

      // if this is an error or set report, get error report 
      // arguments from arglist and set management status
      if (header.type == NETMGT_ERROR_REPORT ||
	  header.type == NETMGT_SET_REPORT)
	{
	  NetmgtGeneric generic ; // generic argument
	  if (!generic.getError (this->myServiceMsg))
	      return;
	}

      // reset arglist pointer
      this->myServiceMsg->myArglist.resetPtr ();

      // dispatch callback routine 
      NETMGT_PRN (("callback: calling callback function\n"));
      (*_netmgt_callback) (header.type,
			   header.system,
			   header.group,
			   header.key,
			   header.count,
			   header.interval,
			   header.flags);
      return;

    case NULLPROC:		// ping 
      (void) svc_sendreply (xprt, (xdrproc_t) xdr_void, (char *) NULL);
      return;

    default:			// error 
      svcerr_noproc (xprt);
      return;
    }
}
#endif ! _SVR4_

/* -----------------------------------------------------------------------
 *  netmgt_unregister_callback - unregister callback function
 *	returns TRUE 
 * -----------------------------------------------------------------------
 */
bool_t
netmgt_unregister_callback (u_long prog, u_long vers)
                 		// RPC program number 
                 		// RPC version number 
{

  NETMGT_PRN (("callback: netmgt_unregister_callback\n"));
  return pmap_unset (prog, vers);
}
