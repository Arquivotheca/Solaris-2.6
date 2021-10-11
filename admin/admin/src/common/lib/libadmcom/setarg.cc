#if !defined(lint) && !defined(NOID)
  static char sccsid[] = "@(#)setarg.cc	1.37 5/5/91 SMI";
#endif

/**************************************************************************
 *  File:	lib/libnetmgt/setarg.cc
 *
 *  Author:	Lynn Monsanto, Sun Microsystems Inc.
 *
 *  SCCSID:     @(#)setarg.cc  1.37  91/05/05
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
 *  Comments:	set request functions
 *
 **************************************************************************
 */

/*LINTLIBRARY*/

#include "netmgt/netmgt_hdrs.h"
#include "netmgt/netmgt.h"
#include "libnetmgt/libnetmgt.h"

/* ---------------------------------------------------------------------
 *  NetmgtServiceMsg::sendSetRequest - send set request to agent
 *	returns TRUE if successful; otherwise returns FALSE
 * ---------------------------------------------------------------------
 */
bool_t
NetmgtServiceMsg::sendSetRequest (char *agent_host, 
				  u_long send_agent_prog, 
				  u_long send_agent_vers, 
				  char *rendez_host, 
				  u_long send_rendez_prog, 
				  u_long send_rendez_vers,
       				  struct timeval send_timeout, 
				  u_int send_flags)
{
  u_long addr ;			// IP address (for inet_addr) 
  struct hostent *hp ;		// host table entry 
  struct in_addr send_agent_addr ;	// agent IP address 
  struct sockaddr_in name ;	// local IP name 

  NETMGT_PRN (("setarg: NetmgtServiceMsg::sendSetRequest\n"));

  // clear error buffer 
  _netmgtStatus.clearStatus ();

  // get agent IP address 
  addr = inet_addr (agent_host);
  if (addr != -1)
    send_agent_addr.s_addr = addr;
  else
    {
      hp = gethostbyname (agent_host) ;
      if (!hp)
	{
	  NETMGT_PRN (("setarg: unknown agent: %s\n", agent_host));
	  _netmgtStatus.setStatus (NETMGT_UNKNOWNHOST, 0, agent_host);
	  return FALSE;
	}
      (void) memcpy ((caddr_t) & send_agent_addr, (caddr_t) hp->h_addr,
		     hp->h_length);
    }

  // get rendezvous IP address 
  (void) memset ((caddr_t) & rendez_addr, 0, sizeof (struct in_addr));
  if (rendez_host)
    {
      addr = inet_addr (rendez_host);
      if (addr != -1)
	rendez_addr.s_addr = addr;
      else
	{
	  hp = gethostbyname (rendez_host);
	  if (!hp)
	    {
	      NETMGT_PRN (("setarg: unknown rendezvous: %s\n", rendez_host));
	      _netmgtStatus.setStatus (NETMGT_UNKNOWNHOST, 0, rendez_host);
	      return FALSE;
	    }
	  (void) memcpy ((caddr_t) & rendez_addr, 
			 (caddr_t) hp->h_addr,
			 hp->h_length);
	}
    }

  // build request message header 
  if (gettimeofday (&this->request_time, (struct timezone *)NULL ) == -1)
    {
      if (netmgt_debug)
	perror ("setarg: gettimeofday");
      _netmgtStatus.setStatus (NETMGT_GETTIMEOFDAY, 0, strerror (errno));
      return FALSE;
    }
  assert (aNetmgtManager != (NetmgtManager *) NULL);
  assert (aNetmgtManager->myRequest != (NetmgtRequest *) NULL);
  this->handle = aNetmgtManager->myRequest->getRequestHandle ();

  this->type = NETMGT_SET_REQUEST;
  this->priority = (u_int) 0;
  this->status = NETMGT_SUCCESS;
  (void) get_myaddress (&name);
  (void) memcpy ((caddr_t) & this->manager_addr, 
		 (caddr_t) & name.sin_addr.s_addr, sizeof (struct in_addr));
  (void) memcpy ((caddr_t) & this->agent_addr,
		 (caddr_t) & send_agent_addr, sizeof (struct in_addr));
  this->agent_prog = send_agent_prog;
  this->agent_vers = send_agent_vers;
  if (rendez_addr.s_addr)
    (void) memcpy ((caddr_t) & this->rendez_addr,
		   (caddr_t) & rendez_addr, 
		   sizeof (struct in_addr));
  this->rendez_prog = send_rendez_prog;
  this->rendez_vers = send_rendez_vers;
  this->proto = IPPROTO_UDP;
  this->timeout = send_timeout;
  this->count = (u_int) 0;
  this->flags = send_flags;

  (void) strncpy (this->system, 
		  aNetmgtManager->myRequest->getSystem (),
		  sizeof (this->system));
  (void) strncpy (this->group, 
		  aNetmgtManager->myRequest->getGroup (),
		  sizeof (this->group));
  (void) strncpy (this->key, 
		  aNetmgtManager->myRequest->getKey (),
		  sizeof (this->key));

  if (this->myArglist.getOffset () == 0)
    this->length = 0;
  else
    this->length =
      u_int (this->myArglist.getOffset () + sizeof (NETMGT_ENDOFARGS));

  // and send the request ...
  if (!this->callAgent (send_agent_prog, send_agent_addr, send_agent_vers, 
			send_timeout))
    return FALSE;
  return TRUE;
}

/* -------------------------------------------------------------------
 *  NetmgtSetarg::deserial - XDR decode and deserialize set argument
 *      returns TRUE is successful; otherwise returns FALSE
 * -------------------------------------------------------------------
 */
bool_t
NetmgtSetarg::deserial (XDR *xdr, NetmgtServiceMsg *aServiceMsg)
              			// xdr stream handle
				// message pointer
{
  char *pname;			// name pointer 

  NETMGT_PRN (("setarg: NetmgtSetarg::deserial\n"));

  assert (xdr != (XDR *) NULL);

  // decode group/table name string 
  pname = (char *) this->group;
  if (!xdr_string (xdr, &pname, NETMGT_NAMESIZ))
    {
      NETMGT_PRN (("setarg: can't decode group/table name\n"));
      xdr_destroy (xdr);
      return FALSE;
    }

  // decode key name string 
  pname = (char *) this->key;
  if (!xdr_string (xdr, &pname, NETMGT_NAMESIZ))
    {
      NETMGT_PRN (("setarg: can't decode key name\n"));
      xdr_destroy (xdr);
      return FALSE;
    }

  // decode attribute name string 
  pname = (char *) this->name;
  if (!xdr_string (xdr, &pname, NETMGT_NAMESIZ))
    {
      NETMGT_PRN (("setarg: can't decode attribute name\n"));
      xdr_destroy (xdr);
      return FALSE;
    }

  // decode argument type 
  if (!xdr_u_int (xdr, &this->type))
    {
      NETMGT_PRN (("setarg: can't decode argument type\n"));
      xdr_destroy (xdr);
      return FALSE;
    }

  // decode argument value length 
  if (!xdr_u_int (xdr, &this->length))
    {
      NETMGT_PRN (("setarg: can't decode value length\n"));
      xdr_destroy (xdr);
      return FALSE;
    }

  // decode argument value 
  if (this->length > 0)
    {
      aServiceMsg->myArglist.myValue1.resetPtr ();
      this->value = aServiceMsg->myArglist.myValue1.getPtr ();
      if (!aServiceMsg->myArglist.deserialValue (xdr, 
						 this->type, 
						 this->length, 
						 this->value))
	return FALSE;
    }

  return TRUE;
}

/* -------------------------------------------------------------------
 *  NetmgtSetarg::serial - XDR encode and serialize set argument
 *      returns TRUE is successful; otherwise returns FALSE
 * -------------------------------------------------------------------
 */
bool_t
NetmgtSetarg::serial (XDR *xdr, NetmgtServiceMsg *aServiceMsg)
              			// xdr stream handle 
				// message pointer
{
  NetmgtArgTag serialtag;      	// argument tag 
  char *pname;			// name pointer 

  NETMGT_PRN (("encode: NetmgtSetarg::serial\n"));

  assert (xdr != (XDR *) NULL);

  // encode argument tag 
  serialtag = NETMGT_SETARG_TAG;
  if (!xdr_u_int (xdr, (u_int *) &serialtag))
    {
      NETMGT_PRN (("encode: can't encode argument tag\n"));
      xdr_destroy (xdr);
      return FALSE;
    }

  // encode group/table name string 
  pname = (char *) this->group;
  if (!xdr_string (xdr, &pname, NETMGT_NAMESIZ))
    {
      NETMGT_PRN (("encode: can't encode group/table name\n"));
      xdr_destroy (xdr);
      return FALSE;
    }

  // encode key name string 
  pname = (char *) this->key;
  if (!xdr_string (xdr, &pname, NETMGT_NAMESIZ))
    {
      NETMGT_PRN (("encode: can't encode key name\n"));
      xdr_destroy (xdr);
      return FALSE;
    }

  // encode attribute name string 
  pname = (char *) this->name;
  if (!xdr_string (xdr, &pname, NETMGT_NAMESIZ))
    {
      NETMGT_PRN (("encode: can't encode attribute name\n"));
      xdr_destroy (xdr);
      return FALSE;
    }

  // encode argument type 
  if (!xdr_u_int (xdr, &this->type))
    {
      NETMGT_PRN (("encode: can't encode argument type\n"));
      xdr_destroy (xdr);
      return FALSE;
    }

  // encode argument value length 
  if (!xdr_u_int (xdr, &this->length))
    {
      NETMGT_PRN (("encode: can't encode value length\n"));
      xdr_destroy (xdr);
      return FALSE;
    }

  // encode argument value 
  if (this->length > 0)
    {
      aServiceMsg->myArglist.myValue1.resetPtr ();
      this->value = aServiceMsg->myArglist.myValue1.getPtr ();
      if (!aServiceMsg->myArglist.serialValue (xdr, 
					       this->type, 
					       this->length, 
					       this->value))
	return FALSE;
    }

  return TRUE;
}




