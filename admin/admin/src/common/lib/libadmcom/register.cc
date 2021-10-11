#if !defined(lint) && !defined(NOID)
  static char sccsid[] = "@(#)register.cc	1.38 5/5/91 SMI";
#endif

/**************************************************************************
 *  File:	lib/libnetmgt/rendez.cc
 *
 *  Author:	Lynn Monsanto, Sun Microsystems Inc.
 *
 *  SCCSID:     @(#)register.cc  1.38  91/05/05
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
 *  Comments:	event report rendezvous (un)registration routines
 *
 **************************************************************************
 */

/*LINTLIBRARY*/

#include "netmgt/netmgt_hdrs.h"
#include "netmgt/netmgt.h"
#include "libnetmgt/libnetmgt.h"

/* ------------------------------------------------------------------------
 *  netmgt_register_rendez - C wrapper to register event rendezvous
 *	returns TRUE if successful; otherwise returns FALSE
 * ------------------------------------------------------------------------
 */
bool_t
netmgt_register_rendez (char *event_dispatcher_host, 
			char *rendez_host, 
			u_long rendez_prog, 
			u_long rendez_vers, 
			u_long agent_prog, 
			u_int event_priority, 
			struct timeval timeout)
{
  NETMGT_PRN (("rendez: netmgt_register_rendez\n")) ;

  // create and inititialize a manager object if necessary 
  if (!aNetmgtManager)
    {
      if (!_netmgt_init_rpc_manager ())
	return FALSE;
    }

  // set managed object instance information
  if (!aNetmgtManager->setObjectInstance ((char *) NULL, 
					  (char *) NULL, 
					  (char *) NULL))
    {
      (void) cfree ((caddr_t) aNetmgtManager);
      return FALSE ;
    }

  // create a rendezvous object if necessary
  if (!aNetmgtRendez)
    {
      aNetmgtRendez = (NetmgtRendez *) calloc (1, sizeof (NetmgtRendez));
      if (!aNetmgtRendez)
	{
	  if (netmgt_debug)
	    perror ("register: calloc failed\n");
	  _netmgtStatus.setStatus (NETMGT_MALLOC, 0, strerror (errno));
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
	    perror("register: calloc");
	  _netmgtStatus.setStatus (NETMGT_MALLOC, 0, strerror (errno));
	  return FALSE;
	}

      // remember who's referencing this request
      aNetmgtRendez->myRequest->setMyEntity (aNetmgtRendez);
    }
  
  return aNetmgtRendez->controlRendez (NETMGT_CREATE_ACTION, 
				       event_dispatcher_host, 
				       rendez_host, 
				       rendez_prog, 
				       rendez_vers, 
				       agent_prog, 
				       event_priority, 
				       timeout);
}

/* -------------------------------------------------------------------------
 *  netmgt_unregister_rendez - C wrapper to unregister event rendezvous
 *	returns TRUE if successful; otherwise returns FALSE
 * -------------------------------------------------------------------------
 */
bool_t
netmgt_unregister_rendez (char *event_dispatcher_host, 
			  char *rendez_host, 
			  u_long rendez_prog, 
			  u_long rendez_vers, 
			  u_long agent_prog, 
			  u_int event_priority, 
			  struct timeval timeout)
{
  NETMGT_PRN (("rendez: netmgt_unregister_rendez\n")) ;

  // create a rendezvous object if necessary
  if (!aNetmgtRendez)
    {
      aNetmgtRendez = (NetmgtRendez *) calloc (1, sizeof (NetmgtRendez));
      if (!aNetmgtRendez)
	{
	  if (netmgt_debug)
	    perror ("register: calloc failed\n");
	  _netmgtStatus.setStatus (NETMGT_MALLOC, 0, strerror (errno));
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
	    perror("register: calloc");
	  _netmgtStatus.setStatus (NETMGT_MALLOC, 0, strerror (errno));
	  return FALSE;
	}

      // remember who's referencing this request
      aNetmgtRendez->myRequest->setMyEntity (aNetmgtRendez);
    }
  
  return aNetmgtRendez->controlRendez (NETMGT_DELETE_ACTION, 
				       event_dispatcher_host, 
				       rendez_host, 
				       rendez_prog, 
				       rendez_vers, 
				       agent_prog, 
				       event_priority, 
				       timeout);
}

/* -----------------------------------------------------------------------
 *  NetmgtRendez::controlRendez - register/unregister event rendezvous
 *	returns TRUE if successful; otherwise returns FALSE
 * -----------------------------------------------------------------------
 */
bool_t
NetmgtRendez::controlRendez (u_int flag, 
			     char *event_dispatcher_host, 
			     char *rendez_host, 
			     u_long rendez_prog, 
			     u_long rendez_vers, 
			     u_long agent_prog, 
			     u_int event_priority, 
			     struct timeval control_timeout)
{
  struct in_addr agent_addr;

  // reset internal error buffer 
  _netmgtStatus.clearStatus ();

  // verify input 
  if (!event_dispatcher_host)
    {
      NETMGT_PRN (("rendez: no event dispatcher host name\n"));
      _netmgtStatus.setStatus (NETMGT_NOAGENTHOSTNAME, 0, NULL);
      return FALSE;
    }
  if (!rendez_host)
    {
      NETMGT_PRN (("rendez: no event rendezvous host name\n"));
      _netmgtStatus.setStatus (NETMGT_NORENDEZHOSTNAME, 0, NULL);
      return FALSE;
    }

  // get event dispatcher IP address 
  u_long addr ;			 // IP address (for inet_addr) 
  struct hostent *hp ;		 // host table entry 

  // get event rendezvous IP address 
  struct in_addr rendez_addr ;	 // event rendezvous IP address
 
  addr = inet_addr (rendez_host);
  if (addr != -1)
    rendez_addr.s_addr = addr;
  else
    {
      hp = gethostbyname (rendez_host);
      if (!hp)
	{
	  NETMGT_PRN (("request: unknown host: %s\n", rendez_host));
	  _netmgtStatus.setStatus (NETMGT_UNKNOWNHOST, 0, NULL);
	  return FALSE;
	}
      (void) memcpy ((caddr_t) & rendez_addr, 
		     (caddr_t) hp->h_addr, 
		     hp->h_length);
    }

  // build the register rendezvous argument list 
  Netmgt_arg option ;		// request option argument
  NetmgtGeneric aGeneric ;	// generic argument

  // reset message argument list pointer
  this->myServiceMsg->myArglist.resetPtr ();

  (void) strcpy (option.name, NETMGT_RENDEZ_ADDR);
  option.type = NETMGT_INADDR;
  option.length = sizeof (struct in_addr);
  option.value = (caddr_t) & rendez_addr.s_addr;
  if (!aGeneric.putOption (&option, this->myServiceMsg))
    return FALSE;

  (void) strcpy (option.name, NETMGT_RENDEZ_PROG);
  option.type = NETMGT_U_LONG;
  option.length = sizeof (u_long);
  option.value = (caddr_t) & rendez_prog;
  if (!aGeneric.putOption (&option, this->myServiceMsg))
    return FALSE;

  (void) strcpy (option.name, NETMGT_RENDEZ_VERS);
  option.type = NETMGT_U_LONG;
  option.length = sizeof (u_long);
  option.value = (caddr_t) & rendez_vers;
  if (!aGeneric.putOption (&option, this->myServiceMsg))
    return FALSE;

  (void) strcpy (option.name, NETMGT_AGENT_PROG);
  option.type = NETMGT_U_LONG;
  option.length = sizeof (u_long);
  option.value = (caddr_t) & agent_prog;
  if (!aGeneric.putOption (&option, this->myServiceMsg))
    return FALSE;

  (void) strcpy (option.name, NETMGT_PRIORITY);
  option.type = NETMGT_U_LONG;
  option.length = sizeof (u_long);
  option.value = (caddr_t) & event_priority;
  if (!aGeneric.putOption (&option, this->myServiceMsg))
    return FALSE;

  // create a message object and send request
  if (!this->myServiceMsg->sendActionRequest (event_dispatcher_host, 
					      NETMGT_EVENT_PROG,
					      NETMGT_EVENT_VERS,
					      (char *) NULL,
					      (u_long) 0,
					      (u_long) 0,
					      control_timeout, 
					      flag,
					      &agent_addr))
    {
      if (flag == NETMGT_CREATE_ACTION)
	_netmgtStatus.setStatus (NETMGT_CREATERENDEZ, 0, NULL);
      else
	_netmgtStatus.setStatus (NETMGT_DELETERENDEZ, 0, NULL);
      return FALSE;
    }

  // destroy rendezvous object if unregistering rendezvous
  if (flag == NETMGT_DELETE_ACTION)
    {
      (void) cfree ((caddr_t) aNetmgtRendez);
      aNetmgtRendez = (NetmgtRendez *) NULL;
    }
  return TRUE;
}
