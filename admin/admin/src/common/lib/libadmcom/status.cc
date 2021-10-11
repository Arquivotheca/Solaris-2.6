#if !defined(lint) && !defined(NOID)
  static char sccsid[] = "@(#)status.cc	1.27 5/5/91 SMI";
#endif

/**************************************************************************
 *  File:	lib/libnetmgt/status.cc
 *
 *  Author:	Lynn Monsanto, Sun Microsystems Inc.
 *
 *  SCCSID:     @(#)status.cc  1.27  91/05/05
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
 *  Comments:	management status routines
 *
 **************************************************************************
 */

/*LINTLIBRARY*/

#include "netmgt/netmgt_hdrs.h"
#include "netmgt/netmgt.h"
#include "libnetmgt/libnetmgt.h"

/* ----------------------------------------------------------------
 *  NetmgtStatus::clearStatus - clear management error status
 *	no return value
 * ----------------------------------------------------------------
 */
void 
NetmgtStatus::clearStatus (void)
{

  netmgt_error.service_error = NETMGT_SUCCESS;
  netmgt_error.agent_error = (u_int) 0;

  // make sure we have a message buffer
  if (!netmgt_error.message)
    {
      netmgt_error.message = (char *) calloc (1, NETMGT_ERRORSIZ);
      if (!netmgt_error.message)
	{
	  NETMGT_PRN (("status: can't allocate status message buffer: %s\n",
		       strerror (errno)));
	  return;
	}
    }
  netmgt_error.message [0] = '\0';
}

/* -----------------------------------------------------------------
 *  NetmgtStatus::getStatus - get management status
 *	returns TRUE if successful; otherwise returns FALSE
 * -----------------------------------------------------------------
 */
bool_t 
NetmgtStatus::getStatus (Netmgt_error *error)
{
  static char emptyString [] = "" ;

  error->service_error = netmgt_error.service_error;
  error->agent_error = netmgt_error.agent_error;

  if (!netmgt_error.message || netmgt_error.message [0] == '\0')
    error->message = emptyString;

  else
    {
      error->message = strdup (netmgt_error.message);
      if (!error->message)
	{
	  NETMGT_PRN (("status: can't allocate status message buffer: %s\n",
		       strerror (errno)));

	  // just in case user doesn't check return value
	  error->message = emptyString;
	  return FALSE;
	}
    }
  return TRUE;
}

/* -----------------------------------------------------------------
 *  NetmgtStatus::setStatus - set management status
 *	returns TRUE if successful; otherwise returns FALSE
 * -----------------------------------------------------------------
 */
bool_t 
NetmgtStatus::setStatus (Netmgt_stat service_error, 
			 u_int agent_error,
			 char *message)
     // service error code
     // agent error code
     // message string
{

  netmgt_error.service_error = service_error;
  netmgt_error.agent_error = agent_error;

  // make sure we have a message buffer
  if (!netmgt_error.message)
    {
      netmgt_error.message = (char *) calloc (1, NETMGT_ERRORSIZ);
      if (!netmgt_error.message)
	{
	  NETMGT_PRN (("status: can't allocate status message buffer: %s\n",
		       strerror (errno)));
	  return FALSE;
	}
    }

  if (message)
    (void) strncpy (netmgt_error.message, message, NETMGT_ERRORSIZ);
  else
    netmgt_error.message [0] = '\0';

  return TRUE;
}

/* -------------------------------------------------------------------
 *  NetmgtGeneric::getError - set management status from error report
 *	returns TRUE if successful; otherwise returns FALSE
 * -------------------------------------------------------------------
 */
bool_t
NetmgtGeneric::getError (NetmgtServiceMsg *aServiceMsg)
				// message pointer
{

  NETMGT_PRN (("status: NetmgtGeneric::getError\n"));

  // clear error buffer 
  _netmgtStatus.clearStatus ();

  // copy error report arguments to management status buffer
  while (TRUE)
    {
      // get error report argument from arglist 
      if (!this->getArg (aServiceMsg))
	return FALSE;

      // a sentinal name string marks the end of the arglist 
      if (this->tag == NETMGT_END_TAG)
	return TRUE;

      // verify we got an error report argument 
      if (this->tag != NETMGT_ERROR_TAG)
	{
	  NETMGT_PRN (("error: wrong argument tag: %d\n", this->tag));
	  _netmgtStatus.setStatus (NETMGT_UNKNOWNREQUEST, 0, NULL);
	  return FALSE;
	}

      // get service error code
      if (strcmp (this->name, NETMGT_ERROR_CODE) == 0)
	netmgt_error.service_error = *(Netmgt_stat *) this->value;

      // get agent error code
      else if (strcmp (this->name, NETMGT_ERROR_INDEX) == 0)
	netmgt_error.agent_error = *(u_int *) this->value;

      // get error message string --- must be "" if no message 
      else if (strcmp (this->name, NETMGT_ERROR_MESSAGE) == 0)
	{
	  assert (this->value != (caddr_t) NULL);

	  // make sure we have a message buffer
	  if (!netmgt_error.message)
	    {
	      netmgt_error.message = (char *) calloc (1, NETMGT_ERRORSIZ);
	      if (!netmgt_error.message)
		{
		  NETMGT_PRN (("status: can't allocate status message: %s\n",
			       strerror (errno)));
		  return FALSE;
		}
	    }
	  (void) strncpy (netmgt_error.message, (char *) this->value,
			  NETMGT_ERRORSIZ);
	}
      else
	{
	  NETMGT_PRN (("error: unknown attribute name: %s\n", this->name));
	  _netmgtStatus.setStatus (NETMGT_UNKNOWNARGNAME, 0, NULL);
	  return FALSE;
	}
    }
}

/* ------------------------------------------------------------------
 *  _netmgt_xdrStatus - de/serialize management status message
 *      returns TRUE if successful; otherwise returns FALSE
 * ------------------------------------------------------------------
 */
bool_t
_netmgt_xdrStatus (XDR *xdr, Netmgt_error *error)
              			// transport handle 
                         	// management status
{
  static char emptyString [] = "" ; // empty message string
  char *message;		// message string pointer 

  NETMGT_PRN (("status: _netmgt_xdrStatus\n"));

  assert (xdr != (XDR *) NULL);
  assert (error != (Netmgt_error *) NULL);

  // make sure we have a message string to serialize
  if (xdr->x_op == XDR_ENCODE)
    {
      if (!error->message)
	message = emptyString;
      else
	message = error->message;
    }

  if (xdr->x_op == XDR_DECODE)
    {
      // make sure we have a message buffer for deserialization
      if (!error->message)
	{
	  error->message = (char *) calloc (1, NETMGT_ERRORSIZ);
	  if (!error->message)
	    {
	      NETMGT_PRN (("status: can't allocate status message: %s\n",
			   strerror (errno)));
	      return FALSE;
	    }
	}
      message = error->message;
    }

#ifdef DEBUG
  if (xdr->x_op == XDR_ENCODE)
    {
      NETMGT_PRN (("error: sending status message...\n"));
      NETMGT_PRN (("error:   service_error: %u\n", error->service_error));
      NETMGT_PRN (("error:     agent_error: %u\n", error->agent_error));
      NETMGT_PRN (("error:         message: %s\n", message));
    }
#endif DEBUG

  // de/serialize status message
  if (!xdr_u_int (xdr, (u_int *) & error->service_error) ||
      !xdr_u_int (xdr, (u_int *) & error->agent_error) ||
      !xdr_string (xdr, &message, NETMGT_ERRORSIZ))
    {
      NETMGT_PRN (("error: can't de/serialize management status\n"));
      return FALSE;
    }

#ifdef DEBUG
  if (xdr->x_op == XDR_DECODE)
    {
      NETMGT_PRN (("error: received status message ...\n"));
      NETMGT_PRN (("error:   service_error: %u\n", error->service_error));
      NETMGT_PRN (("error:     agent_error: %u\n", error->agent_error));
      NETMGT_PRN (("error:         message: %s\n", message));
    }
#endif DEBUG

  return TRUE;
}

