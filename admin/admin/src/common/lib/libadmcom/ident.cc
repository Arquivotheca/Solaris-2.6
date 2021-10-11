#if !defined(lint) && !defined(NOID)
  static char sccsid[] = "@(#)ident.cc	1.38 5/6/91 SMI";
#endif

/**************************************************************************
 *  File:	lib/libnetmgt/ident.cc
 *
 *  Author:	Lynn Monsanto, Sun Microsystems Inc.
 *
 *  SCCSID:     @(#)ident.cc  1.38  91/05/06
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
 *  Comments:	agent identification routines
 *
 **************************************************************************
 */

/*LINTLIBRARY*/

#include "netmgt/netmgt_hdrs.h"
#include "netmgt/netmgt.h"
#include "libnetmgt/libnetmgt.h"

Netmgt_agent_ID netmgt_agent_ID ; // remove !!!

/* ---------------------------------------------------------------------
 *  netmgt_request_agent_ID - C wrapper to request agent identification
 *	returns pointer to identification buffer if successful;
 *	otherwise returns NULL
 * ---------------------------------------------------------------------
 */
Netmgt_agent_ID *
netmgt_request_agent_ID (char *agent_host, 
			 u_long agent_prog, 
			 u_long agent_vers,
			 struct timeval timeout)
{
  NETMGT_PRN (("ident: netmgt_request_agent_ID\n")) ;
  
  // create a control message requesting an agent ID and
  // send it to the agent
  NetmgtControlMsg *aControlMsg ;
  aControlMsg = (NetmgtControlMsg *) calloc (1, sizeof (NetmgtControlMsg));
  if (!aControlMsg)
    {
      if (netmgt_debug)
	perror ("ident: calloc failed");
      _netmgtStatus.setStatus (NETMGT_MALLOC, 0, strerror (errno));
      return (Netmgt_agent_ID *)FALSE;
    }
  Netmgt_agent_ID *retval = aControlMsg->requestAgentID (agent_host, 
							 agent_prog, 
							 agent_vers,
							 timeout) ;
  (void) cfree ((caddr_t) aControlMsg);
  return retval;
}

/* ---------------------------------------------------------------------
 *  NetmgtControlMsg::requestAgentID - request agent identification
 *	returns pointer to identification buffer if successful;
 *	otherwise returns NULL
 * ---------------------------------------------------------------------
 */
Netmgt_agent_ID *
NetmgtControlMsg::requestAgentID (char *agent_host, 
				  u_long req_agent_prog, 
				  u_long req_agent_vers, 
				  struct timeval timeout)
     // agent hostname
     // agent RPC program 
     // agent RPC version
     // RPC timeout
{
  u_long addr ;			// IP address (for inet_addr) 
  struct hostent *hp ;		// host table entry 
  struct in_addr req_agent_addr ;	// agent IP address 
  enum clnt_stat clnt_stat ;	// clnt_call return status 
  struct rpc_err rpc_err ;	// RPC error status 
  u_int retries ;		// number clnt_call retries 

  NETMGT_PRN (("ident: NetmgtControlMsg::requestAgentID\n")) ;

  // reset internal error code 
  _netmgtStatus.clearStatus ();

  // get agent IP address 
  addr = inet_addr (agent_host);
  if (addr != -1)
    req_agent_addr.s_addr = addr;
  else
    {
      hp = gethostbyname (agent_host) ;
      if (!hp)
	{
	  NETMGT_PRN (("ident: unknown agent: %s\n", agent_host));
	  _netmgtStatus.setStatus (NETMGT_UNKNOWNHOST, 0, NULL);
	  return (Netmgt_agent_ID *) NULL;
	}
      (void) memcpy ((caddr_t) & req_agent_addr, 
		     (caddr_t) hp->h_addr, 
		     hp->h_length);
    }

  // get agent client handle 
  if (!this->myClient.newClient (req_agent_addr, 
				 (u_long) IPPROTO_UDP, 
				 req_agent_prog, 
				 req_agent_vers, 
				 NETMGT_NO_SECURITY,
				 timeout))
      return (Netmgt_agent_ID *) NULL;

  // send the request message 
  retries = 1;
  do
  {
    clnt_stat = clnt_call (this->myClient.getHandle(), 
			   NETMGT_AGENT_ID_PROC, 
			   (xdrproc_t) xdr_void,
			   (caddr_t) NULL, 
			   (xdrproc_t) _netmgt_xdrAgentId, 
			   (caddr_t) &netmgt_agent_ID, 
			   timeout);

    if (clnt_stat != RPC_SUCCESS)
      {
	if (netmgt_debug)
	  clnt_perror (this->myClient.getHandle(), "ident: clnt_call");

	CLNT_GETERR (this->myClient.getHandle(), &rpc_err);
	if (rpc_err.re_status != RPC_TIMEDOUT)
	  {
	    if (rpc_err.re_status == RPC_TIMEDOUT)
	      _netmgtStatus.setStatus (NETMGT_RPCTIMEDOUT, 
				       0, 
				       dgettext (NETMGT_TEXT_DOMAIN,
						 "Can't send request"));
	    else
	      _netmgtStatus.setStatus (NETMGT_RPCFAILED, 
				       0, 
				       clnt_sperror(this->myClient.getHandle(), 
						    dgettext (NETMGT_TEXT_DOMAIN,
							      "Can't send request")));
	    this->myClient.destroyClient ();
	    return (Netmgt_agent_ID *) NULL;
	  }

	// resend the request NETMGT_MAXRESEND times before giving up 
	retries++;
      }
    else
      {
	this->myClient.destroyClient ();
	return &netmgt_agent_ID;
      }

  } while (clnt_stat != RPC_SUCCESS && retries < NETMGT_MAXRESEND) ;

  // probably timed out 
  if (rpc_err.re_status == RPC_TIMEDOUT)
    _netmgtStatus.setStatus (NETMGT_RPCTIMEDOUT, 
			     0, 
			     dgettext (NETMGT_TEXT_DOMAIN,
				       "Can't send report"));
  else
    _netmgtStatus.setStatus (NETMGT_RPCFAILED, 
			     0, 
			     clnt_sperror(this->myClient.getHandle(),	
					  dgettext (NETMGT_TEXT_DOMAIN,
						    "Can't send report")));
  this->myClient.destroyClient ();
  return (Netmgt_agent_ID *) NULL;
}

/* -----------------------------------------------------------------
 *  NetmgtDispatcher::getAgentID - send agent identification
 *	no return value
 * -----------------------------------------------------------------
 */
void
NetmgtDispatcher::getAgentID (void)
{
  struct stat statb;		// stat buffer 
  int fd [2];			// pipe descriptors 
  int nread;			// # characters read from pipe 
  char message [NETMGT_ERRORSIZ] ; // error message buffer

  NETMGT_PRN (("ident: NetmgtDispatcher::getAgentID\n"));

  // get agent name and serial number 
  (void) strcpy (netmgt_agent_ID.name, this->name);
  netmgt_agent_ID.serial = this->serial;

  // check whether `arch' command exists 
  if (stat ("/bin/arch", &statb) == -1)
    {
      (void) strcpy (message, dgettext (NETMGT_TEXT_DOMAIN, "/bin/arch: "));
      (void) strcat (message, strerror (errno));
      _netmgtStatus.setStatus (NETMGT_FATAL, 0, message);
      return;
    }

  // create a pipe for reading `arch' output 
  if (pipe (fd) == -1)
    {
      (void) strcpy (message, dgettext (NETMGT_TEXT_DOMAIN, 
					"pipe(2) failed: "));
      (void) strcat (message, strerror (errno));
      _netmgtStatus.setStatus (NETMGT_FATAL, 0, message);
      return;
    }

  // fork a subprocess to exec `arch' command 
  switch (vfork ())
    {
    case -1:			// error 
      _netmgtStatus.setStatus (NETMGT_FORK, 0, strerror (errno));
      return ;

    case 0:			// child 
      (void) close (fd[0]);
      (void) close (1);
      (void) dup (fd[1]);
      (void) execl ("/bin/arch", "arch", 0);
      _exit (1);

    default:			// parent 
      (void) close (fd[1]);
      nread = read (fd[0], netmgt_agent_ID.arch, 
		    sizeof (netmgt_agent_ID.arch));
      if (nread <= 0)
	{
	  (void) strcpy (message, dgettext (NETMGT_TEXT_DOMAIN,
					    "Can't read from pipe: "));
	  (void) strcat (message, strerror (errno));
	  _netmgtStatus.setStatus (NETMGT_FATAL, 0, message);
	}
      (void) close (fd[0]);
      return;
    }
  /*NOTREACHED*/
}

/* ------------------------------------------------------------------
 *  _netmgt_xdrAgentId - (de)serialize agent identification buffer
 *      returns TRUE if successful; otherwise returns FALSE
 * ------------------------------------------------------------------
 */
bool_t
_netmgt_xdrAgentId (XDR *xdr, Netmgt_agent_ID *ident)
              			// transport handle 
                            	// agent identification buffer 
{
  char *name;			// agent name pointer 
  char *arch;			// agent architecture pointer 

  NETMGT_PRN (("ident: _netmgt_xdrAgentId\n"));

  assert (xdr != (XDR *) NULL);
  assert (ident != (Netmgt_agent_ID *) NULL);

#ifdef DEBUG
  if (xdr->x_op == XDR_ENCODE)
    {
      NETMGT_PRN (("ident: sending identification message...\n"));
      NETMGT_PRN (("ident:   name: %s\n", ident->name));
      NETMGT_PRN (("ident: serial: %u\n", ident->serial));
      NETMGT_PRN (("ident:   arch: %s\n", ident->arch));
    }
#endif DEBUG

  // (de)serialize agent name 
  name = ident->name;
  if (!xdr_string (xdr, &name, sizeof (ident->name)))
    {
      NETMGT_PRN (("ident: xdr_string failed\n"));
      return FALSE;
    }

  // (de)serialize agent serial number 
  if (!xdr_u_int (xdr, (u_int *) & ident->serial))
    {
      NETMGT_PRN (("ident: xdr_int failed\n"));
      return FALSE;
    }

  // (de)serialize agent architecture 
  arch = ident->arch;
  if (!xdr_string (xdr, &arch, sizeof (ident->arch)))
    {
      NETMGT_PRN (("ident: xdr_string failed\n"));
      return FALSE;
    }

#ifdef DEBUG
  if (xdr->x_op == XDR_DECODE)
    {
      NETMGT_PRN (("ident: received ident message ...\n"));
      NETMGT_PRN (("ident:   name: %s\n", ident->name));
      NETMGT_PRN (("ident: serial: %u\n", ident->serial));
      NETMGT_PRN (("ident:   arch: %s\n", ident->arch));
    }
#endif DEBUG

  return TRUE;
}
