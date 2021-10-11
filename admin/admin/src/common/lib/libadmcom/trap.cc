#if !defined(lint) && !defined(NOID)
  static char sccsid[] = "@(#)trap.cc	1.45 5/5/91 SMI";
#endif

/**************************************************************************
 *  File:	lib/libnetmgt/trap.cc
 *
 *  Author:	Lynn Monsanto, Sun Microsystems Inc.
 *
 *  SCCSID:     @(#)trap.cc  1.45  91/05/05
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
 *  Comments:	trap report routines
 *
 **************************************************************************
 */

/*LINTLIBRARY*/

#include "netmgt/netmgt_hdrs.h"
#include "netmgt/netmgt.h"
#include "libnetmgt/libnetmgt.h"

/* ---------------------------------------------------------------------
 *  netmgt_start_trap - C wrapper for initializing trap report
 *	returns TRUE if successful; otherwise returns FALSE
 * ---------------------------------------------------------------------
 */

bool_t
netmgt_start_trap (char *system,
		   u_int agent_prog,
		   u_int agent_vers,
		   char *group,
		   char *host,
		   u_int prog,
		   u_int vers,
		   u_int priority,
		   struct timeval timeout)
     
{
  NETMGT_PRN (("trap: netmgt_start_trap\n"));
  if (!aNetmgtDispatcher) {
      // create an agent dispatcher 
      aNetmgtDispatcher = (NetmgtDispatcher *) calloc (1,
		sizeof (NetmgtDispatcher));
      if (!aNetmgtDispatcher)
      {
          if (netmgt_debug)
	   perror ("trap: new failed");
	    _netmgtStatus.setStatus (NETMGT_MALLOC, 0, strerror (errno));
	    return FALSE;
      }
  }
  return aNetmgtDispatcher->startTrap (system,
				       agent_prog,
				       agent_vers,
				       group, 
				       host, 
				       prog, 
				       vers, 
				       priority,
				       timeout);
}


/* ---------------------------------------------------------------------
 *  netmgt_build_trap_report - C wrapper for building trap report
 *	returns TRUE if successful; otherwise returns FALSE
 * ---------------------------------------------------------------------
 */

bool_t
NetmgtDispatcher::startTrap(char *system,
			    u_int agent_prog,
			    u_int agent_vers,
			    char *group,
			    char *host,
			    u_int prog,
			    u_int vers,
			    u_int priority,
			    struct timeval traptimeout)
{
  struct sockaddr_in localname;

  // get local IP address 

  (void)get_myaddress (&localname);
  (void) memcpy ((caddr_t) & this->local_addr, 
		 (caddr_t) & localname.sin_addr.s_addr, 
		 sizeof (struct in_addr));

  // name not filled in..used for ?

  this->serial = 0;

  // proto not filled in..not used for traps?

  this->timeout = traptimeout;
  this->flags = 0;
  this->state = NETMGT_DISPATCHED;
  this->activity_log = 0;
  this->request_log = 0;

  this->program = agent_prog;
  this->version = agent_vers;

  // create a message object for sending and receiving messages
  if (!this->myServiceMsg) {
    this->myServiceMsg = (NetmgtServiceMsg *) calloc (1,
		sizeof (NetmgtServiceMsg));
     if (!this->myServiceMsg)  {
          if (netmgt_debug)
	    perror("traps: calloc");
         _netmgtStatus.setStatus (NETMGT_MALLOC, 0, strerror (errno));
         return FALSE;
      }
   }

  // remember who's referencing this message
  this->myServiceMsg->setMyEntity (this);

  // add the request structure to the agent if not allocated

  if (!this->myRequest) {
      this->myRequest = (NetmgtRequest *) calloc (1, sizeof (NetmgtRequest));
      if (!this->myRequest) {
          if (netmgt_debug)
	    perror("traps: calloc");
          _netmgtStatus.setStatus (NETMGT_MALLOC, 0, strerror (errno));
          return FALSE;
      }
  }

  // remember who's referencing this request
  this->myRequest->setMyEntity (this);

  // initialize request structure

  if (!this->myRequest->myConstructor (system,
				group, host, prog, vers, priority))
	return(FALSE);

   // Build performer structure if not already allocated

  if (!aNetmgtPerformer) {
      aNetmgtPerformer = (NetmgtPerformer *) calloc (1,
		sizeof (NetmgtPerformer));
      if (!aNetmgtPerformer) {
          NETMGT_PRN (("traps: can't create performer object: %s\n",
		   strerror (errno)));
          _netmgtStatus.setStatus (NETMGT_FORK, 0, strerror (errno));
          return(FALSE);
      }
      if (!aNetmgtPerformer->myConstructor (this)) {
          (void) cfree ((caddr_t) aNetmgtPerformer);
          return(FALSE);
      }
   }
  
   // could free up dispatcher at this point
  return TRUE;
}

/* ---------------------------------------------------------------------
 *  NetmgtRequest::myConstructor - initialize request object
 *	returns TRUE if successful; otherwise returns FALSE
 * ---------------------------------------------------------------------
 */
bool_t
NetmgtRequest::myConstructor (char *system, 
			      char *group,
			      char *host,	
			      u_int prog,
			      u_int vers,
			      u_int priority)
     // agent system name
     // target host name
     // agent RPC program number	 
     // agent RPC version number
     // trap priority
{

  u_long addr ;			// IP address (for inet_addr) 
  struct hostent *hp ;		// host table entry 

  this->info.pid = getpid ();
  this->info.request_time.tv_sec = this->info.request_time.tv_usec = 0;
  this->info.type = NETMGT_TRAP_REQUEST;
  this->info.handle = 0; 
  this->info.flags = 0;
  this->info.priority = priority;

  // get rendez IP address 
  addr = inet_addr (host);
  if (addr != -1)
    this->info.rendez_addr.s_addr = addr;
  else
    {
      hp = gethostbyname (host) ;
      if (!hp)
	{
	  NETMGT_PRN (("start trap: unknown host: %s\n", host));
	  _netmgtStatus.setStatus (NETMGT_UNKNOWNHOST, 0, host);
	  return FALSE;
	}
      (void) memcpy ((caddr_t) & this->info.rendez_addr, 
		     (caddr_t) hp->h_addr,
		     hp->h_length);
    }

  this->info.rendez_prog = prog;
  this->info.rendez_vers = vers;
  this->info.proto = IPPROTO_UDP;
  strncpy(this->info.system, system, NETMGT_NAMESIZ);
  strncpy(this->info.group, group, NETMGT_NAMESIZ);
  this->info.key [0] = '\0';
  this->info.length = 0;
  this->info.last_verified.tv_sec = this->info.last_verified.tv_usec = 0;
  return TRUE;
}  
	
/* ---------------------------------------------------------------------
 *  netmgt_build_trap_report - C wrapper for building trap report
 *	returns TRUE if successful; otherwise returns FALSE
 * ---------------------------------------------------------------------
 */
bool_t
netmgt_build_trap_report (Netmgt_data *data)
                       		// data buffer 
{
  NETMGT_PRN (("trap: netmgt_build_trap_report\n"));

  // tell agent to build trap report
  assert (aNetmgtPerformer != (NetmgtPerformer *) NULL);
  return aNetmgtPerformer->buildTrapReport (data);
}

/* ---------------------------------------------------------------------
 *  NetmgtPerformer::buildTrapReport - append trap report to arglist
 *	returns TRUE if successful; otherwise returns FALSE
 * ---------------------------------------------------------------------
 */
bool_t
NetmgtPerformer::buildTrapReport (Netmgt_data *data)
				// data argument 
{

  NETMGT_PRN (("trap: NetmgtDispatcher::buildTrap\n"));
  assert (data != (Netmgt_data *) NULL);

  // reset internal error buffer 
  _netmgtStatus.clearStatus ();

  // verify input 
  if (!data)
    {
      NETMGT_PRN (("trap: no performance structure\n"));
      _netmgtStatus.setStatus (NETMGT_NOPERFBUF, 0, NULL);
      return FALSE;
    }
  if (!data->name)
    {
      NETMGT_PRN (("trap: no attribute name\n"));
      _netmgtStatus.setStatus (NETMGT_NOARGNAME, 0, NULL);
      return FALSE;
    }
  if (strlen (data->name) >= NETMGT_NAMESIZ)
    {
      NETMGT_PRN (("trap: attribute name length >= %d\n",
		   NETMGT_NAMESIZ));
      _netmgtStatus.setStatus (NETMGT_NAME2BIG, 0, NULL);
      return FALSE;
    }
  if (!data->value)
    {
      NETMGT_PRN (("trap: no attribute value pointer\n"));
      _netmgtStatus.setStatus (NETMGT_NOARGVALUE, 0, NULL);
      return FALSE;
    }

  NetmgtGeneric generic;	// generic argument
  assert (this->myServiceMsg != (NetmgtServiceMsg *) NULL);
  return generic.putData (data, this->myServiceMsg);
}

/* -----------------------------------------------------------------
 *  NetmgtServiceMsg::sendTrapReport - send trap-report to rendezvous
 *      returns TRUE if successful; otherwise returns FALSE
 * -----------------------------------------------------------------
 */
bool_t
NetmgtServiceMsg::sendTrapReport (NetmgtPerformer *aPerformer,
				  struct timeval adelta_time, 
				  Netmgt_stat astatus, 
				  u_int aflags)
     // performer object pointer
     // system timestamp 
     // status code 
     // report flags 
{
  NETMGT_PRN (("trap: NetmgtServiceMsg::sendTrapReport\n")) ;

  assert (aPerformer != (NetmgtPerformer *) NULL) ;

  // reset internal error code 
  _netmgtStatus.clearStatus ();

  // get request information from agent
  NetmgtRequestInfo requestInfo ;   // request information buffer
  aNetmgtDispatcher->getRequestInfo (&requestInfo);

  // fill out report message header from request information
  this->request2report (&requestInfo) ;

  // fill out the rest of the message header
  if (gettimeofday (&this->report_time, (struct timezone *)NULL ) == -1)
    {
      if (netmgt_debug)
	perror ("trap: gettimeofday");
      _netmgtStatus.setStatus (NETMGT_GETTIMEOFDAY, 0, strerror (errno));
      return FALSE;
    }
  this->delta_time = adelta_time;
  this->type = NETMGT_TRAP_REPORT;
  this->status = astatus;
  this->flags = aflags;
  this->agent_addr = aPerformer->getLocalAddr ();
  this->agent_prog = aPerformer->getProgram ();
  this->agent_vers = aPerformer->getVersion ();
  if (this->myArglist.getOffset () == 0)
    this->length = 0;
  else
    this->length = u_int (this->myArglist.getOffset () 
			  + sizeof (NETMGT_ENDOFARGS));

  // send the report message
  return this->sendReport (aPerformer, &requestInfo);

}
