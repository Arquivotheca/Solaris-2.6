#if !defined(lint) && !defined(NOID)
  static char sccsid[] = "@(#)client.cc	1.49 15 May 1991 SMI";
#endif

/**************************************************************************
 *  File:	lib/libnetmgt/client.cc
 *
 *  Author:	Lynn Monsanto, Sun Microsystems Inc.
 *
 *  SCCSID:     @(#)client.cc  1.49  91/05/15
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
 *  Comments:	RPC client allocation routines
 *
 **************************************************************************
 */

/*LINTLIBRARY*/

#include "netmgt/netmgt_hdrs.h"
#include "netmgt/netmgt.h"
#include "libnetmgt/libnetmgt.h"

#define NETMGT_DEFAULT_INTERVAL	30 // default reporting interval

/* --------------------------------------------------------------------
 *  Netmgt_message::newClient - creates a new RPC client handle
 *	returns TRUE if successful; otherwise FALSE
 * ---------------------------------------------------------------------
 */
bool_t
NetmgtClient::newClient (struct in_addr new_addr,
			 u_long new_proto,
			 u_long new_prog,
			 u_long new_vers,
			 int new_flavor,
			 struct timeval new_timeout)
     // server IP address
     // transport protocol
     // RPC program number
     // RPC program version
     // authentication flavor
     // request timeout
{
  
  // allocate client
  this->client = (CLIENT *) NULL;
  return this->getClient (new_addr, new_proto, new_prog, new_vers, new_flavor, 
			  new_timeout);
}

/* --------------------------------------------------------------------
 *  NetmgtClient::destroyClient - destroy an RPC client 
 *	no return value
 * --------------------------------------------------------------------
 */
void 
NetmgtClient::destroyClient (void)
{
  NETMGT_PRN (("client: NetmgtClient::destroyClient\n"));

  if (this->client) 
    {
      if (this->client->cl_auth)
	{
          (void) auth_destroy (this->client->cl_auth);
	}
      (void) clnt_destroy (this->client);
      (void) close (this->sock);
      this->client = (CLIENT *) NULL;
    }
  return;
}

/* --------------------------------------------------------------------
 *  NetmgtMessage::getClient - create (or reuses) an RPC client handle
 *	returns TRUE if successful; otherwise returns FALSE
 * --------------------------------------------------------------------
 */
bool_t
NetmgtMessage::getClient (struct in_addr addr, 
			  u_long proto,
			  u_long prog,
			  u_long vers,
			  u_int flavor,
			  struct timeval timeout)
     // server IP address
     // transport protocol
     // RPC program number
     // RPC program version
     // authentication flavor
     // request timeout
{
  // ask this client for a handle
  return this->myClient.getClient (addr, 
				   proto, 
				   prog, 
				   vers, 
				   flavor, 
				   timeout) ;
}

/* ---------------------------------------------------------------------
 *  NetmgtClient::getClient - create (or reuses) an RPC client handle
 *	returns TRUE if successful; otherwise returns FALSE
 * ---------------------------------------------------------------------
 */
bool_t
NetmgtClient::getClient (struct in_addr get_addr,
			 u_long get_proto,
			 u_long get_prog,	
			 u_long get_vers,
			 int get_flavor,
			 struct timeval get_timeout)
     // server IP address
     // transport protocol
     // RPC program number
     // RPC program version
     // authentication flavor
     // request timeout
{
  
  NETMGT_PRN (("client: Netmgt_message::getClient for RPC %d at %s\n",
	       prog, inet_ntoa (getaddr))) ;

  // reset internal error code
  _netmgtStatus.clearStatus () ;

  // check if cached handle can be used. If not, free it
  
  if (this->client)
    {
      if ((memcmp ((char *) & (this->addr),
		   (char *) & get_addr,
		   sizeof (get_addr)) == 0) &&
	  (this->proto == get_proto) &&
	  (this->prog == get_prog) &&
	  (this->vers == get_vers) &&
	  (this->flavor == get_flavor) &&
	  (memcmp ((char *) & (this->timeout),
		   (char *) & get_timeout,
		   sizeof (get_timeout)) == 0))
	{
	  NETMGT_PRN (("client: using cached client handle:\n"));
	  NETMGT_PRN (("\taddr == %s, proto == %d, prog == %d, vers == %d\n",
		       inet_ntoa (this->addr),
		       this->proto,
		       this->prog,
		       this->vers));
	  return TRUE;
	}
      this->destroyClient ();
    }
  
  // need to create a new handle
  
  this->addr = get_addr;
  this->proto = get_proto;
  this->prog = get_prog;
  this->vers = get_vers;
  this->flavor = get_flavor;
  this->timeout = get_timeout;
  
  struct sockaddr_in name ;	// socket name
  name.sin_family = AF_INET;
  name.sin_port = (u_short) INADDR_ANY;
  (void) memcpy ((caddr_t) & name.sin_addr, 
		 (caddr_t) & get_addr,
		 sizeof (struct in_addr));
  this->sock = RPC_ANYSOCK;
  
  NETMGT_PRN (("client: creating new client handle:\n"));
  NETMGT_PRN (("\taddr == %s, proto == %d, prog == %d, vers == %d\n",
	       inet_ntoa (this->addr),
	       this->proto,
	       this->prog,
	       this->vers));
  
  struct hostent *hp;		// host table pointer
  char host [NETMGT_NAMESIZ];	// host name
  switch (this->proto)
    {
    case IPPROTO_UDP:
#ifdef _SVR4_
      // get host name
      hp = gethostbyaddr ((char *) & get_addr,
			  sizeof (struct in_addr),
			  AF_INET) ;
      if (!hp)
	{
	  NETMGT_PRN (("client: gethostbyaddr failed\n"));
	  _netmgtStatus.setStatus (NETMGT_UNKNOWNADDRESS, 0, NULL);
	  return FALSE;
	}
      (void) strncpy (host, hp->h_name, sizeof (host) - 1);

      this->client = clnt_create (host, 
				  this->prog, 
				  this->vers,
				  "udp");
#else
      this->client = clntudp_create (&name, 
				     this->prog, 
				     this->vers,
				     this->timeout, 
				     &this->sock);
#endif _SVR4_
      break;

    case IPPROTO_TCP:
#ifdef _SVR4_
      // get hostname
      hp = gethostbyaddr ((char *) & get_addr,
			  sizeof (struct in_addr),
			  AF_INET) ;
      if (!hp)
	{
	  NETMGT_PRN (("client: gethostbyaddr failed\n"));
	  _netmgtStatus.setStatus (NETMGT_UNKNOWNADDRESS, 0, NULL);
	  return FALSE;
	}
      (void) strncpy (host, hp->h_name, sizeof (host) - 1);

      this->client = clnt_create (host, 
				  this->prog, 
				  this->vers,
				  "tcp");
#else
      this->client = clnttcp_create (&name, 
				     this->prog, 
				     this->vers,
				     &this->sock, 
				     (u_int) 0, 
				     (u_int) 0);
#endif _SVR4_
      break;

    default:
      this->client = (CLIENT *) NULL;
      NETMGT_PRN (("client: unknown transport protocol: %d\n", this->proto));
      _netmgtStatus.setStatus (NETMGT_BADPROTO, 0, NULL);
      return FALSE;
    }
  
  if (!this->client)
    {
      char message [NETMGT_ERRORSIZ]; // error string buffer
      
      if (netmgt_debug)
	clnt_pcreateerror ("client");
      
      (void) sprintf (message, 
		      "%s = %d, %s = %d",
		      dgettext (NETMGT_TEXT_DOMAIN, "program"),
		      this->prog,
		      dgettext (NETMGT_TEXT_DOMAIN, "version"),
		      this->vers);
      if (rpc_createerr.cf_error.re_status == RPC_TIMEDOUT)
	_netmgtStatus.setStatus (NETMGT_RPCTIMEDOUT, 0, message);
      else
	_netmgtStatus.setStatus (NETMGT_CANTCREATECLIENT, 
				 0,
				 clnt_spcreateerror (message));
      return FALSE;
    }
  
  // create client authentication credentials
  char message [32]; 		// error message buffer
  switch (flavor)
    {
    case AUTH_NONE:
      this->client->cl_auth = authnone_create ();
      return TRUE;

    case AUTH_DES:
      if (!this->authdesCreate (&name, (u_int) NETMGT_AUTH_LIFETIME))
	{
	  (void) this->destroyClient ();
	  return FALSE;
	}
      return TRUE;

    case AUTH_SHORT:
    case AUTH_UNIX:
#ifdef _SVR4_
      this->client->cl_auth = authsys_create_default ();
#else
      this->client->cl_auth = authunix_create_default ();
#endif _SVR4_
      return TRUE;

    default:
      (void) sprintf (message, "flavor = %d", flavor);
      _netmgtStatus.setStatus (NETMGT_BADAUTHFLAVOR, 0, message);
      return FALSE;
    }
  /*NOTREACHED*/
}

/* ----------------------------------------------------------------------
 *  NetmgtServiceMsg::sendReport - send a report message to a rendezvous
 *	returns TRUE if successful; otherwise returns FALSE
 * ----------------------------------------------------------------------
 */
bool_t
NetmgtServiceMsg::sendReport (NetmgtPerformer *aPerformer,
			      NetmgtRequestInfo *requestInfo)
     // agent performer pointer
     // request information pointer
{
  
  // if the report argument length is greater than the maximum
  // UDP - transport RPC buffer size, destroy the UDP - transport
  // client handle and allocate a TCP - transport client handle
  int send_proto;			// current protocol
  send_proto = (this->length > NETMGT_MAXARGSIZ) ? IPPROTO_TCP : IPPROTO_UDP;
  if (!this->getClient (requestInfo->rendez_addr,
			send_proto,
			requestInfo->rendez_prog,
			requestInfo->rendez_vers,
			NETMGT_NO_SECURITY,
			aPerformer->getTimeout ()))
    {
      // if we can't get a client handle and the rendezvous isn't
      // a transient RPC program, cache the report
      if (this->rendez_prog < NETMGT_TRANSIENT)      
	return aPerformer->cacheReport ();
      return FALSE;
    }

  // set report interval if the requestor told the agent to 
  // select a report interval
  if (requestInfo->interval.tv_sec == 0 && requestInfo->interval.tv_usec == 0)
    {
      static bool_t firstTime = TRUE;
      static struct timeval currTime = { 0, 0 };
      static struct timeval lastTime = { 0, 0 };

      // if this is the first report, just save the current time
      if (firstTime)
	{
	  firstTime = FALSE;
	  if (gettimeofday (&lastTime, (struct timezone *) NULL ) == -1)
	    {
	      Netmgt_error error;
	      error.service_error = NETMGT_GETTIMEOFDAY;
	      error.agent_error = 0;
	      error.message = strerror (errno);
	      return FALSE;
	    }
	  this->interval.tv_sec = NETMGT_DEFAULT_INTERVAL;
	  this->interval.tv_usec = 0;
	}
      // if we've already sent a report, set the report interval
      // from the last time we sent a report
      else
	{
	  // get the current time
	  if (gettimeofday (&currTime, (struct timezone *) NULL ) == -1)
	    {
	      Netmgt_error error;
	      error.service_error = NETMGT_GETTIMEOFDAY;
	      error.agent_error = 0;
	      error.message = strerror (errno);
	      return FALSE;
	    }
	  this->interval.tv_sec = currTime.tv_sec - lastTime.tv_sec;
	  this->interval.tv_usec = currTime.tv_usec - lastTime.tv_usec;
	  if (this->interval.tv_usec < 0)
	    {
	      this->interval.tv_sec -= 1;
	      this->interval.tv_usec *= -1;
	    }
	  lastTime.tv_sec = currTime.tv_sec;
	  lastTime.tv_usec = currTime.tv_usec;
	}
    }
  
  // cache report if doing deferred sending 
  if (requestInfo->flags & NETMGT_DO_DEFERRED)
    return aPerformer->cacheReport ();
  
  // if there are cached reports, cache this report and attempt
  // to send all cached reports
  if (aPerformer->areCachedReports ())
    {
      if (!aPerformer->cacheReport ())
	return FALSE;
      return aPerformer->resendReports ();
    }
  
  // send the report message 
  enum clnt_stat clnt_stat;	// clnt_call return status
  struct rpc_err rpc_err;	// RPC error status
  bool_t callSucceeded = FALSE;	// whether the client call succeeded
  for (int calls = 0; calls < NETMGT_MAXRESEND; calls++)
    {
      // make the client call
      clnt_stat = clnt_call (this->myClient.getHandle (),
			     NETMGT_SERVICE_PROC,
			     (xdrproc_t) _netmgt_serialMsg,
			     (caddr_t) this,
			     (xdrproc_t) xdr_void,
			     (caddr_t) NULL,
			     aPerformer->getTimeout ());
      
      // did the call succeed ?
      if (clnt_stat == RPC_SUCCESS)
	{
	  callSucceeded = TRUE;
	  break;
	}

      // if the call timed out, resend the report.  
      if (netmgt_debug)
	clnt_perror (this->myClient.getHandle (), "client: clnt_call");
      CLNT_GETERR (this->myClient.getHandle (), &rpc_err);
      if (rpc_err.re_status != RPC_TIMEDOUT)
	break;
    }

  // if the call failed, cache the report if we're not sending 
  // to a transient RPC program
  if (!callSucceeded)
    {
      _netmgtStatus.setStatus (NETMGT_RPCFAILED,
			       0,
			       clnt_sperror (this->myClient.getHandle (),
					    dgettext (NETMGT_TEXT_DOMAIN,
						      "Can't send report")));
      if (this->rendez_prog < NETMGT_TRANSIENT) 
	{     
	  if (!aPerformer->cacheReport ())
	    {
	      NETMGT_PRN (("client: can't cache report: %s: exiting...\n",
			   netmgt_sperror ()));
	      exit (1);
	    }
	  return TRUE;
	}
      NETMGT_PRN (("client: can't send report: %s: exiting...\n",
		   netmgt_sperror ()));
  // Return error so we can try to send a fatal error message
	return FALSE;
    }
      
  // remember if this is the last message sent
  if (this->flags & NETMGT_LAST)
    aPerformer->setState (NETMGT_SENT_LAST);

  NETMGT_PRN (("client: clnt_call succeeded\n"));
  return TRUE;
}
