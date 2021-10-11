#if !defined(lint) && !defined(NOID)
  static char sccsid[] = "@(#)tli-callback.cc	1.12 6/12/91 SMI";
#endif

/**************************************************************************
 *  File:	lib/libnetmgt/tli-callback.cc
 *
 *  Author:	Lynn Monsanto, Sun Microsystems Inc.
 *
 *  SCCSID:     @(#)tli-callback.cc  1.12  91/06/12
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
 *  Comments:	streams RPC callback routines
 *
 **************************************************************************
 */

/*LINTLIBRARY*/

#ifdef _SVR4_

#include "netmgt/netmgt_hdrs.h"
#include "netmgt/netmgt.h"
#include "libnetmgt/libnetmgt.h"
#include <sys/stropts.h>

//global data 
static void (*_netmgt_TliCallback) (u_int type, char *system, char *group, 
				    char *key, u_int count, 
				    struct timeval interval, u_int flags) ;

// static functions 
static void _netmgt_dispatchTliCallback (struct svc_req *rqst, SVCXPRT *xprt) ;

static u_long _netmgt_getTliTransient (u_long version, 
				       struct netconfig *netconfig,
				       struct netbuf *address) ;

struct netconfig *_rpc_getconfip (char *nettype) ;

/* --------------------------------------------------------------------
 *  NetmgtRendez::registerTliCallback - this function: 
 *	allocates a transient RPC program number
 *	creates streams RPC transport handle(s) for *udpFdp and/or *tcpFdp
 *	registers the transient RPC program number with the portmapper
 *
 *	returns the transient program number and sets *udpFdp and/or
 *	*tcpFdp if successful; otherwise, returns (u_long)0.
 * --------------------------------------------------------------------
 */
u_long 
NetmgtRendez::registerTliCallback (void (*callbck) (u_int type, char *system, 
						     char *group, char *key, 
						     u_int count, 
						     struct timeval interval, 
						     u_int flags), 
				   int *udpFdp, 
				   int *tcpFdp, 
				   u_long vers, 
				   u_long registerproto)
     // callback function
     // UDP stream file descriptor pointer
     // TCP stream file descriptor pointer
     // RPC version
     // RPC transport protocol
{

  NETMGT_PRN (("callback: NetmgtRendez::registerTliCallback\n")) ;

  // reset internal error code 
  _netmgtStatus.clearStatus ();

  // verify input 
  if (!callbck)
    {
      NETMGT_PRN (("callback: no callback function\n"));
      _netmgtStatus.setStatus (NETMGT_NODISPATCH, 0, NULL);
      return 0;
    }
  if ((registerproto & IPPROTO_UDP) && udpFdp == (int *) NULL)
    {
      NETMGT_PRN (("callback: null UDP/IP socket pointer\n"));
      _netmgtStatus.setStatus (NETMGT_NULLSOCKET, 0, NULL);
      return 0;
    }
  if ((registerproto & IPPROTO_TCP) && tcpFdp == (int *) NULL)
    {
      NETMGT_PRN (("callback: null TCP/IP socket pointer\n"));
      _netmgtStatus.setStatus (NETMGT_NULLSOCKET, 0, NULL);
      return 0;
    }

  // save the callback function address 
  _netmgt_TliCallback = callbck;

  struct netconfig *udpNetconfig; // UDP/IP transport information
  struct netconfig *tcpNetconfig; // TCP/IP transport information
  SVCXPRT *udpXprt ;		  // UDP/IP server transport handle 
  SVCXPRT *tcpXprt ;		  // TCP/IP server transport handle 

  // get transport information
  switch (registerproto)
    {
    case IPPROTO_UDP:
      udpNetconfig = this->myGetnetconfigent ("udp");
      if (!udpNetconfig)
	{
	  NETMGT_PRN (("callback: udp: unknown transport\n"));
	  _netmgtStatus.setStatus (NETMGT_BADPROTO, 
				   0, 
				   "udp: unknown transport");
	  return 0;
	}

      udpXprt = svc_tli_create (RPC_ANYFD, 
				udpNetconfig, 
				(struct t_bind *) NULL,
				0,
				0);
      if (!udpXprt)
	{
	  NETMGT_PRN (("callback: svc_tli_create failed\n"));
	  _netmgtStatus.setStatus (NETMGT_SVCUDPCREATE, 0, NULL);
	  return 0;
	}

      // get UDP transport file descriptor
      *udpFdp = udpXprt->xp_fd;

      break;

    case IPPROTO_TCP:
      tcpNetconfig = this->myGetnetconfigent ("tcp");
      if (!tcpNetconfig)
	{
	  NETMGT_PRN (("callback: tcp: unknown transport\n"));
	  _netmgtStatus.setStatus (NETMGT_BADPROTO, 
				   0, 
				   "tcp: unknown transport");
	  return 0;
	}
      tcpXprt = svc_tli_create (RPC_ANYFD, 
				tcpNetconfig, 
				(struct t_bind *) NULL,
				0,
				0);
      if (!tcpXprt)
	{
	  NETMGT_PRN (("callback: svc_tli_create failed\n"));
	  _netmgtStatus.setStatus (NETMGT_SVCTCPCREATE, 0, NULL);
	  return 0;
	}

      // get TCP transport file descriptor
      *tcpFdp = tcpXprt->xp_fd;

      break;

    case IPPROTO_UDP|IPPROTO_TCP:
      udpNetconfig = this->myGetnetconfigent ("udp");
      if (!udpNetconfig)
	{
	  NETMGT_PRN (("callback: udp: unknown transport\n"));
	  _netmgtStatus.setStatus (NETMGT_BADPROTO, 
				   0, 
				   "tcp: unknown transport");
	  return 0;
	}
      udpXprt = svc_tli_create (RPC_ANYFD, 
				udpNetconfig, 
				(struct t_bind *) NULL,
				0,
				0);
      if (!udpXprt)
	{
	  NETMGT_PRN (("callback: svc_tli_create failed\n"));
	  _netmgtStatus.setStatus (NETMGT_SVCTCPCREATE, 0, NULL);
	  return 0;
	}

      // get UDP transport file descriptor
      *udpFdp = udpXprt->xp_fd;

      tcpNetconfig = this->myGetnetconfigent ("tcp");
      if (!tcpNetconfig)
	{
	  NETMGT_PRN (("callback: tcp: unknown transport\n"));
	  _netmgtStatus.setStatus (NETMGT_BADPROTO, 
				   0, 
				   "tcp: unknown transport");
	  return 0;
	}
      tcpXprt = svc_tli_create (RPC_ANYFD, 
				tcpNetconfig, 
				(struct t_bind *) NULL,
				0,
				0);
      if (!tcpXprt)
	{
	  NETMGT_PRN (("callback: svc_tli_create failed\n"));
	  _netmgtStatus.setStatus (NETMGT_SVCTCPCREATE, 0, NULL);
	  return 0;
	}

      // get TCP transport file descriptor
      *tcpFdp = tcpXprt->xp_fd;

      break;

    default:
	{
	  NETMGT_PRN (("callback: unknown transport\n"));
	  _netmgtStatus.setStatus (NETMGT_SVCUDPCREATE, 
				   0, 
				   "unknown transport");
	  return 0;
	}
    }

  // get a transient RPC program number 
  u_long prog ;			  // transient program number 

  // register the transient program number with the portmapper 
  switch (registerproto)
    {
    case IPPROTO_UDP:

      prog = _netmgt_getTliTransient (vers, udpNetconfig, &udpXprt->xp_ltaddr); 
      if (!prog)
	{
	  NETMGT_PRN (("callback: rpcb_set failed for udp tranport\n"));
	  return 0;
	}

      if (!svc_reg (udpXprt, 
		    prog, 
		    vers, 
		    (SIG_PFV2) _netmgt_dispatchTliCallback, 
		    (struct netconfig *)NULL))
	{
	  NETMGT_PRN (("callback: can't register transient rpc\n"));
          _netmgtStatus.setStatus (NETMGT_SVCREGISTER, 0, NULL);
	  return 0;
	}
      return prog;
  
    case IPPROTO_TCP:
      prog = _netmgt_getTliTransient (vers, tcpNetconfig, &tcpXprt->xp_ltaddr); 
      if (!prog)
	{
	  NETMGT_PRN (("callback: rpcb_set failed for tcp transport\n"));
	  return 0;
	}

      if (!svc_reg (tcpXprt, 
		    prog, 
		    vers, 
	       	    (SIG_PFV2) _netmgt_dispatchTliCallback, 
		    (struct netconfig *)NULL))
	{
	  NETMGT_PRN (("callback: can't register transient rpc\n"));
          _netmgtStatus.setStatus (NETMGT_SVCREGISTER, 0, NULL);
	  return 0;
	}
      return prog;

    case IPPROTO_UDP|IPPROTO_TCP:
      prog = _netmgt_getTliTransient (vers, udpNetconfig, &udpXprt->xp_ltaddr); 
      if (!prog)
	{
	  NETMGT_PRN (("callback: rpcb_set failed for udp transport\n"));
          _netmgtStatus.setStatus (NETMGT_SVCREGISTER, 0, NULL);
	  return 0;
	}

      if (!svc_reg (udpXprt, 
		    prog, 
		    vers, 
                    (SIG_PFV2) _netmgt_dispatchTliCallback,
		    (struct netconfig *)NULL))
	{
	  NETMGT_PRN (("callback: can't register transient rpc\n"));
          _netmgtStatus.setStatus (NETMGT_SVCREGISTER, 0, NULL);
	  return 0;
	}

      if (! rpcb_set (prog, vers, tcpNetconfig, &tcpXprt->xp_ltaddr))
	{
	  NETMGT_PRN (("callback: rpcb_set for tcp transport failed\n"));
          _netmgtStatus.setStatus (NETMGT_SVCREGISTER, 0, NULL);
	  return 0;
	}

      if (!svc_reg (tcpXprt, 
		    prog, 
		    vers, 
                    (SIG_PFV2) _netmgt_dispatchTliCallback,
		    (struct netconfig *)NULL))
	{
	  NETMGT_PRN (("callback: can't register transient rpc\n"));
          _netmgtStatus.setStatus (NETMGT_SVCREGISTER, 0, NULL);
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
 *  _netmgt_getTliTransient - get a transient RPC program number
 *	returns transient RPC number if successful; otherwise returns 0
 * ---------------------------------------------------------------------
 */
static u_long
_netmgt_getTliTransient (u_long version, 
			 struct netconfig *netconfig,
			 struct netbuf *address)
     // RPC version number
     // network information
     // transport endpoing address
{

  NETMGT_PRN (("callback: _netmgt_getTliTransient\n"));

  // reset internal error code 
  _netmgtStatus.clearStatus ();

  static u_long program = NETMGT_TRANSIENT; // transient RPC program number 
  pid_t client_pid;

         
        // try to get the number where it's likely to be free
        client_pid = getpid();
        program = program + (u_long) client_pid;

  
  while (! rpcb_set (program, version, netconfig, address))
    {

      // return an error if we couldn't bind to an RPC program number
      // for any other reason than the program number is already in use
//      struct netbuf svcaddr ;	// RPC service address
//      if (! rpcb_getaddr (program, version, netconfig, & svcaddr, "localhost"))
//	return 0;
        program = program + (u_long) client_pid;
      continue;
    }
  return program ;
}

/* -----------------------------------------------------------------------
 *  _netmgt_dispatchTliCallback - dispatch callback function
 *	no return value
 * -----------------------------------------------------------------------
 */
static void
_netmgt_dispatchTliCallback (struct svc_req *rqst, SVCXPRT *xprt)
                          	// server request structure 
                            	// server transport handle 
{

  NETMGT_PRN (("callback: _netmgt_dispatchTliCallback\n"));

  assert (aNetmgtRendez != (NetmgtRendez *) NULL);
  aNetmgtRendez->dispatchTliCallback (rqst, xprt);
  return;
}

/* -----------------------------------------------------------------------
 *  NetmgtRendez::dispatchTliCallback - dispatch callback function
 *	no return value
 * -----------------------------------------------------------------------
 */
void
NetmgtRendez::dispatchTliCallback (struct svc_req *rqst, SVCXPRT *xprt)
{
  NetmgtServiceHeader header ;	// request message header

  NETMGT_PRN (("callback: NetmgtRendez::dispatchTliCallback\n"));

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
      (*_netmgt_TliCallback) (header.type,
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

/* -----------------------------------------------------------------------
 *  NetmgtEntity::myGetnetconfigent - get netconfig database entry
 *	returns netconfig database entry if successful; otherwise NULL
 * -----------------------------------------------------------------------
 */
struct netconfig *
NetmgtEntity::myGetnetconfigent (char *nettype)
				// netconfig database entry name
{
  char *netid;			// netconfig database entry name
  static char *netid_tcp;	// TCP/IP netconfig database entry name
  static char *netid_udp;	// UDP/IP netconfig database entry name
  
  if (!netid_udp && !netid_tcp) 
    {
      struct netconfig *nconf;	// netconfig database entry pointer
      void *confighandle;	// netconfig database entry handle
      
      if (!(confighandle = setnetconfig()))
	return (NULL);
      while (nconf = getnetconfig(confighandle)) 
	{
	  if (strcmp(nconf->nc_protofmly, NC_INET) == 0) 
	    {
	      if (strcmp(nconf->nc_proto, NC_TCP) == 0)
		netid_tcp = strdup(nconf->nc_netid);
	      else if (strcmp(nconf->nc_proto, NC_UDP) == 0)
		netid_udp = strdup(nconf->nc_netid);
	    }
	}
      endnetconfig(confighandle);
    }
  if (strcmp(nettype, "udp") == 0)
    netid = netid_udp;
  else if (strcmp(nettype, "tcp") == 0)
    netid = netid_tcp;
  else
    return ((struct netconfig *)NULL);
  if ((netid == NULL) || (netid[0] == NULL))
    return ((struct netconfig *)NULL);
  return (getnetconfigent(netid));
}
#endif _SVR4_
