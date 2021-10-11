#if !defined(lint) && !defined(NOID)
  static char sccsid[] = "@(#)error.cc	1.41 5/5/91 SMI";
#endif

/**************************************************************************
 *  File:	lib/libnetmgt/error.cc
 *
 *  Author:	Lynn Monsanto, Sun Microsystems Inc.
 *
 *  SCCSID:     @(#)error.cc  1.41  91/05/05
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
 *  Comments:	error reporting routines
 *
 **************************************************************************
 */

/*LINTLIBRARY*/

#include "netmgt/netmgt_hdrs.h"
#include "netmgt/netmgt.h"
#include "libnetmgt/libnetmgt.h"

/* -----------------------------------------------------------------
 *  netmgt_fetch_error - fetch error report
 *	returns TRUE if successful; otherwise returns FALSE
 * -----------------------------------------------------------------
 */
bool_t
netmgt_fetch_error (Netmgt_error *error)
                          	// error buffer 
{
  NETMGT_PRN (("error: netmgt_fetch_error\n")) ;

  // verify input 
  if (!error)
    {
      NETMGT_PRN (("error: no error structure\n"));
      _netmgtStatus.setStatus (NETMGT_NOERRORBUF, 0, NULL);
      return FALSE;
    }

  return _netmgtStatus.getStatus (error);
}

/* -----------------------------------------------------------------
 *  netmgt_send_error - send error report
 *	returns TRUE if successful; otherwise returns FALSE
 * -----------------------------------------------------------------
 */
bool_t
netmgt_send_error (Netmgt_error *error)
                         	// error argument
{

  NETMGT_PRN (("error: netmgt_send_error\n"));

  // set internal status if agent is verify a request
  if (aNetmgtDispatcher->getState () == NETMGT_VERIFYING)
    {
      _netmgtStatus.setStatus (error->service_error,
			       error->agent_error,
			       error->message);
      return TRUE;
    }

  // otherwise, send error report to the rendezvous
  assert (aNetmgtPerformer != (NetmgtPerformer *) NULL);
  return aNetmgtPerformer->myServiceMsg->sendErrorReport (aNetmgtPerformer, 
							  error);
}


/* -----------------------------------------------------------------
 *  NetmgtServiceMsg::sendErrorReport - send error message to rendezvous
 *      returns TRUE if successful; otherwise returns FALSE
 * -----------------------------------------------------------------
 */
bool_t
NetmgtServiceMsg::sendErrorReport (NetmgtPerformer *aPerformer, 
				   Netmgt_error *error)
     // an performer agent pointer
     // error report argument 
{

  NETMGT_PRN (("errno: NetmgtServiceMsg::sendErrorReport\n"));

  assert (aPerformer != (NetmgtPerformer *) NULL);
  assert (error != (Netmgt_error *) NULL);

  // reset internal error code 
  _netmgtStatus.clearStatus ();

  // get request information
  NetmgtRequestInfo requestInfo ;  // request information buffer
  aPerformer->getRequestInfo (&requestInfo);

  // get a client handle if we don't have one already or if we
  // have cached reports because the rendezvous was unreachable 
  if (!this->getClient (requestInfo.rendez_addr,
			requestInfo.proto,
			requestInfo.rendez_prog,
			requestInfo.rendez_vers,
			NETMGT_NO_SECURITY,
			aPerformer->getTimeout ()))
	return FALSE;

  // rewind arglist read/write pointer 
  this->myArglist.resetPtr ();

  // insert error message into message arglist 
  NetmgtGeneric aGeneric ;	// a generic argument
  Netmgt_arg option;		// message argument
  static char *noMessage = "";	// empty message string 

  (void) strcpy (option.name, NETMGT_ERROR_CODE);
  option.type = NETMGT_U_INT;
  option.length = sizeof (u_int);
  option.value = (caddr_t) & error->service_error;
  if (!aGeneric.putOption (&option, this))
    return FALSE;

  (void) strcpy (option.name, NETMGT_ERROR_INDEX);
  option.type = NETMGT_U_INT;
  option.length = sizeof (u_int);
  option.value = (caddr_t) & error->agent_error;
  if (!aGeneric.putOption (&option, this))
    return FALSE;

  (void) strcpy (option.name, NETMGT_ERROR_MESSAGE);
  option.type = NETMGT_STRING;
  if (!error->message)
    error->message = noMessage;
  option.length = (u_int) strlen (error->message);
  option.value = error->message;
  if (!aGeneric.putOption (&option, this))
    return FALSE;

  // fill out report message header from request information
  this->request2report (&requestInfo) ;

  // fill out rest of message header 
  if (gettimeofday (&this->report_time, (struct timezone *) NULL ) == -1)
    {
      if (netmgt_debug)
	perror ("error: gettimeofday");
      return FALSE;
    }

  if (requestInfo.type == NETMGT_SET_REQUEST)
    this->type = NETMGT_SET_REPORT;
  else
    this->type = NETMGT_ERROR_REPORT;
  this->status = error->service_error;
  if ((int) error->service_error > (int) NETMGT_WARNING)
    this->flags = NETMGT_LAST;
  else
    this->flags = (u_int) 0;
  this->priority = NETMGT_HIGH_PRIORITY;

  this->agent_addr = aPerformer->getLocalAddr ();
  this->agent_prog = aPerformer->getProgram ();
  this->agent_vers = aPerformer->getVersion ();
  if (this->myArglist.getOffset () == 0)
    this->length = 0;
  else
    this->length = u_int (this->myArglist.getOffset ()
			  + sizeof (NETMGT_END_TAG)
			  + sizeof (NETMGT_ENDOFARGS) + 1);

  // and send the error message ... 
  enum clnt_stat clnt_stat;	// clnt_call return status 
  struct rpc_err rpc_err;	// RPC error status 
  int retries;			// #clnt_call retries 

  retries = 1;
  do
    {
      clnt_stat = clnt_call (this->myClient.getHandle (), 
			     NETMGT_SERVICE_PROC,
			     (xdrproc_t) _netmgt_serialMsg, 
			     (caddr_t) this,
			     (xdrproc_t) xdr_void, 
			     (caddr_t) NULL,
			     aPerformer->getTimeout ());

      if (clnt_stat != RPC_SUCCESS)
	{
	  if (netmgt_debug)
	    clnt_perror (this->myClient.getHandle(), "errno: clnt_call");

	  CLNT_GETERR (this->myClient.getHandle(), &rpc_err);
	  if (rpc_err.re_status != RPC_TIMEDOUT)
	    return FALSE;

	  /* resend the error message up to NETMGT_MAXRESEND times before
	     giving up */
	  retries++;
	}

    } while (clnt_stat != RPC_SUCCESS && retries < NETMGT_MAXRESEND) ;

  // cache any error-report messages that could not be delivered if the
  // rendezous is not associated with a transient RPC program number 
  if (clnt_stat != RPC_SUCCESS && this->rendez_prog < NETMGT_TRANSIENT)
    {
      return aPerformer->cacheReport ();
    }

  // remember if this is the last message sent 
  if (this->flags & NETMGT_LAST)
    aPerformer->setState (NETMGT_SENT_LAST);
  
  return TRUE;
}

