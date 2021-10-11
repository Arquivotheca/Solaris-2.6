#if !defined(lint) && !defined(NOID)
  static char sccsid[] = "@(#)manager.cc	1.18 5/5/91 SMI";
#endif

/**************************************************************************
 *  File:	lib/libnetmgt/manager.cc
 *
 *  Author:	Lynn Monsanto, Sun Microsystems Inc.
 *
 *  SCCSID:     @(#)manager.cc  1.18  91/05/05
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
 *  Comments:	manager class member functions
 *
 **************************************************************************
 */

/*LINTLIBRARY*/

#include "netmgt/netmgt_hdrs.h"
#include "netmgt/netmgt.h"
#include "libnetmgt/libnetmgt.h"

/* --------------------------------------------------------------------
 *  _netmgt_init_rpc_manager - create and initialize manager object
 *	returns TRUE if successful; otherwise returns FALSE
 * --------------------------------------------------------------------
 */
bool_t
_netmgt_init_rpc_manager (void)
{

  NETMGT_PRN (("manager: _netmgt_init_rpc_manager\n")) ;

  // create a manager instance
  aNetmgtManager = (NetmgtManager *) calloc (1, sizeof (NetmgtManager));
  if (!aNetmgtManager)
    {
      _netmgtStatus.setStatus (NETMGT_MALLOC, 0, strerror (errno));
      if (netmgt_debug)
	perror ("manager: initialize");
      return FALSE;
    }

  // inititialize manager instance
  if (!aNetmgtManager->myConstructor ())
    {
      (void) cfree ((caddr_t) aNetmgtManager);
      aNetmgtManager = (NetmgtManager *) NULL;
      return FALSE;
    }
  return TRUE;
}

/* ------------------------------------------------------------------------
 *  NetmgtManager::myConstructor - initialize manager instance
 *	returns TRUE if successful; otherwise returns FALSE
 * ------------------------------------------------------------------------
 */
bool_t
NetmgtManager::myConstructor (void)
{
  NETMGT_PRN (("manager: NetmgtManager::myConstructor\n"));

  // create a message object for sending messages
  this->myServiceMsg 
    = (NetmgtServiceMsg *) calloc (1, sizeof (NetmgtServiceMsg));
  if (!this->myServiceMsg)
    {
      if (netmgt_debug)
	perror("rendez: calloc");
      _netmgtStatus.setStatus (NETMGT_MALLOC, 0, strerror (errno));
      return FALSE;
    }

  // remember who's referencing this message
  this->myServiceMsg->setMyEntity (this);

  // create a request object for holding request during
  // the request initialization phase
  this->myRequest = (NetmgtRequest *) calloc (1, sizeof (NetmgtRequest));
  if (!this->myRequest)
    {
      _netmgtStatus.setStatus (NETMGT_MALLOC, 0, NULL);
      return NULL;
    }

  // remember whol's referencing this request
  this->myRequest->setMyEntity (this);

  // set global manager pointer
  aNetmgtManager = this;
  return TRUE;
}

/* ------------------------------------------------------------------------
 *   NetmgtManager::sendGetRequest - send data/event request to agent
 *	returns request timestamp if successful; otherwise NULL
 * ------------------------------------------------------------------------
 */
struct timeval *
NetmgtManager::sendGetRequest (u_int request_type,      // request type
			       char *agent_host,        // agent hostname
			       u_long agent_prog,       // agent RPC program
			       u_long agent_vers,       // agent RPC version
			       char *rendez_host,       // rendez hostname
			       u_long rendez_prog,      // rendez RPC program
			       u_long rendez_vers,      // rendez RPC version
			       u_int count,	        // report count
			       struct timeval interval, // report interval
			       struct timeval timeout,	// RPC timeout
			       u_int flags)		// request flags
{

  NETMGT_PRN (("manager: NetmgtManager::sendRequest\n"));
  assert (this->myServiceMsg != (NetmgtServiceMsg *) NULL);

  // send the request message to the agent
  return this->myServiceMsg->sendGetRequest (request_type,
					     agent_host,
					     agent_prog,
					     agent_vers,
					     rendez_host,
					     rendez_prog,
					     rendez_vers,
					     count,
					     interval,
					     timeout,
					     flags);
}

/* ----------------------------------------------------------------------
 *  NetmgtManager::sendSetRequest - request agent to set attribute value
 *	returns TRUE if successful; otherwise returns FALSE
 * ----------------------------------------------------------------------
 */
bool_t
NetmgtManager::sendSetRequest (char *agent_host,        // request type
			       u_long agent_prog,       // agent RPC program  
			       u_long agent_vers,       // agent RPC version
			       char *rendez_host,	// rendez hostname
			       u_long rendez_prog,      // rendez RPC program
			       u_long rendez_vers,	// rendez RPC version
			       struct timeval timeout,	// RPC timeout
			       u_int flags)		// request flags
{
  NETMGT_PRN (("manager: NetmgtManager::sendSetRequest\n"));
  assert (this->myServiceMsg != (NetmgtServiceMsg *) NULL);

  // send the request message to the agent
  return this->myServiceMsg->sendSetRequest (agent_host,
					     agent_prog,
					     agent_vers,
					     rendez_host,
					     rendez_prog,
					     rendez_vers,
					     timeout,
					     flags);
}
				  
/* -----------------------------------------------------------------------
 *  NetmgtManager::getRequestMsg - get pointer to request message
 *	returns pointer if successful; otherwise returns NULL
 * -----------------------------------------------------------------------
 */
NetmgtServiceMsg *
NetmgtManager::getRequestMsg (void) 
{
 
  NETMGT_PRN (("message: NetmgtManager::getRequestMsg\n"));
  return this->myServiceMsg; 
}

/* -----------------------------------------------------------------------
 *  NetmgtManager::getRequestArglist - get pointer to request arglist
 *	returns pointer if successful; otherwise returns NULL
 * -----------------------------------------------------------------------
 */
NetmgtArglist *
NetmgtManager::getRequestArglist (void) 
{ 

  NETMGT_PRN (("message: NetmgtManager::getRequestArglist\n"));
  assert (this->myServiceMsg != (NetmgtServiceMsg *) NULL);
  return &this->myServiceMsg->myArglist; 
}

/* -----------------------------------------------------------------------
 *  NetmgtManager::setRequestHandle - set request message handle
 *	returns TRUE if successful; otherwise returns FALSE
 * -----------------------------------------------------------------------
 */
bool_t
NetmgtManager::setRequestHandle (u_int handle)
				// request handle
{

  NETMGT_PRN (("message: NetmgtManager::setRequestHandle: %d\n", handle));

  assert (this->myRequest != (NetmgtRequest *) NULL);
  this->myRequest->setRequestHandle (handle);
  return TRUE;
}

/* -----------------------------------------------------------------------
 *  NetmgtRendez::getReportHandle - get report message handle
 *	returns request handle
 * -----------------------------------------------------------------------
 */
u_int 
NetmgtRendez::getReportHandle (void)
{

  NETMGT_PRN (("message: NetmgtRendez::getReportHandle\n"));

  // get message header 
  NetmgtServiceHeader header;
  if (!this->myServiceMsg->getHeader (&header))
    return 0;

  return header.handle;
}
