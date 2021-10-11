#if !defined(lint) && !defined(NOID)
  static char sccsid[] = "@(#)arglist.cc	1.35 5/5/91 SMI";
#endif

/**************************************************************************
 *  File:	lib/libnetmgt/arglist.cc
 *
 *  Author:	Lynn Monsanto, Sun Microsystems Inc.
 *
 *  SCCSID:     @(#)arglist.cc  1.35  91/05/05
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
 *  Comments:	message argument list serialization/deserialization functions
 *
 **************************************************************************
 */

/*LINTLIBRARY*/

#include "netmgt/netmgt_hdrs.h"
#include "netmgt/netmgt.h"
#include "libnetmgt/libnetmgt.h"

/* ---------------------------------------------------------------------
 *  NetmgtArglist::dealloc - deallocate buffer memory
 *	returns FALSE if successful; otherwise returns FALSE
 * ---------------------------------------------------------------------
 */
bool_t
NetmgtArglist::dealloc (void)
{
  (void) this->myValue1.dealloc ();
  (void) this->myValue2.dealloc ();
  (void) NetmgtBuffer::dealloc ();
  return TRUE;
}

/* ---------------------------------------------------------------------
 *  NetmgtArglist::serialize - XDR encode and serialize message arglist
 *	returns FALSE if successful; otherwise returns FALSE
 * ---------------------------------------------------------------------
 */
bool_t
NetmgtArglist::serialize (XDR *xdr, NetmgtServiceMsg *aServiceMsg)
               			// xdr stream handle 
				// service message pointer
{

  NETMGT_PRN (("arglist: NetmgtArglist::serialize\n")) ;

  assert (xdr != (XDR *) NULL) ;

  // reset the current arglist offset 
  this->resetPtr ();

  // (re)allocate arglist buffer 
  if (!this->alloc (aServiceMsg->getLength (), NETMGT_MINARGSIZ))
    return FALSE;

  // (re)allocate attribute value buffer 
  if (!this->myValue1.alloc (aServiceMsg->getLength (), NETMGT_MINARGSIZ))
    return FALSE;

  // (re)allocate threshold value buffer if this is
  // an event request or report
  if (aServiceMsg->getType () == NETMGT_EVENT_REQUEST ||
      aServiceMsg->getType () == NETMGT_EVENT_REPORT)
    {
      if (!this->myValue2.alloc (aServiceMsg->getLength (), NETMGT_MINARGSIZ))
	return FALSE;
    }

  // mark arglist beginning with a sentinal string and return if
  // there are no arguments to encode 
  if (!aServiceMsg->isData ())
    {
      this->resetPtr ();
      (void) strcpy (this->getPtr (), NETMGT_ENDOFARGS);
      return TRUE;
    }

  // serialize message arguments from argument list 
  bool_t retval; 		// return value
  switch (aServiceMsg->getType ())
    {
    case NETMGT_ACTION_REQUEST: 
    case NETMGT_ACTION_REPORT: 
    case NETMGT_CREATE_REQUEST: 
    case NETMGT_DELETE_REQUEST: 
    case NETMGT_DATA_REQUEST: 
    case NETMGT_DATA_REPORT:
    case NETMGT_EVENT_REQUEST:
    case NETMGT_EVENT_REPORT:
    case NETMGT_ERROR_REPORT:
    case NETMGT_SET_REPORT:
    case NETMGT_TRAP_REQUEST:
    case NETMGT_TRAP_REPORT:
      retval = this->serialUntagged (xdr, aServiceMsg) ;
      this->resetPtr ();
      return retval;

    case NETMGT_SET_REQUEST:
      retval = this->serialTagged (xdr, aServiceMsg) ;
      this->resetPtr ();
      return retval;

    default:
      _netmgtStatus.setStatus (NETMGT_UNKNOWNTYPE, 0, (char *) NULL);
      return FALSE;
    }
  /*NOTREACHED*/
}

/* -------------------------------------------------------------------
 *  NetmgtArglist::deserialize - xdr deserialize and decode arglist
 *	returns FALSE if successful; otherwise returns FALSE
 * -------------------------------------------------------------------
 */
bool_t
NetmgtArglist::deserialize (XDR *xdr, NetmgtServiceMsg *aServiceMsg)
               			// xdr stream handle
				// service message pointer
{
  NETMGT_PRN (("arglist: NetmgtArglist::deserialize\n")) ;

  assert (xdr != (XDR *) NULL) ;

  // reset the current arglist offset 
  this->resetPtr ();

  // (re)allocate arglist buffer 
  if (!this->alloc (aServiceMsg->getLength (), NETMGT_MINARGSIZ))
    return FALSE;

  // (re)allocate attribute value buffer 
  if (!this->myValue1.alloc (aServiceMsg->getLength (), NETMGT_MINARGSIZ))
    return FALSE;

  // (re)allocate threshold value buffer if this is
  // an event request or report
  if (aServiceMsg->getType () == NETMGT_EVENT_REQUEST ||
      aServiceMsg->getType () == NETMGT_EVENT_REPORT)
    {
      if (!this->myValue2.alloc (aServiceMsg->getLength (), NETMGT_MINARGSIZ))
	return FALSE;
    }

  // deserialize message arguments and copy to arglist 
  bool_t retval; 		// return value
  switch (aServiceMsg->getType ())
    {
    case NETMGT_ACTION_REQUEST: 
    case NETMGT_ACTION_REPORT: 
    case NETMGT_CREATE_REQUEST: 
    case NETMGT_DELETE_REQUEST: 
    case NETMGT_DATA_REQUEST: 
    case NETMGT_DATA_REPORT:
    case NETMGT_EVENT_REQUEST:
    case NETMGT_EVENT_REPORT:
    case NETMGT_ERROR_REPORT:
    case NETMGT_SET_REPORT:
    case NETMGT_TRAP_REQUEST:
    case NETMGT_TRAP_REPORT:
      this->tagged = FALSE;

      // mark arglist beginning with a sentinal tag and return if
      // there are no arguments to decode  
      if (!aServiceMsg->isData ())
	{
	  (void) this->putEndTag (FALSE);
	  return TRUE;
	}

      retval = this->deserialUntagged (xdr, aServiceMsg);

      // set message arglist length
      aServiceMsg->setLength (u_int (this->getOffset ()
				     + sizeof (NETMGT_END_TAG)
				     + sizeof (NETMGT_ENDOFARGS) + 1));
      this->resetPtr ();
      return retval;

    case NETMGT_SET_REQUEST:
      this->tagged = TRUE;

      // mark arglist beginning with a sentinal tag and return if
      // there are no arguments to decode  
      if (!aServiceMsg->isData ())
	{
	  (void) this->putEndTag (TRUE);
	  return TRUE;
	}

      retval = this->deserialTagged (xdr, aServiceMsg);

      // set message arglist length
      aServiceMsg->setLength (u_int (this->getOffset ()
				     + sizeof (NETMGT_END_TAG)
				     + sizeof (NETMGT_ENDOFARGS) + 1));
      this->resetPtr ();
      return retval;

    default:
      _netmgtStatus.setStatus (NETMGT_UNKNOWNTYPE, 0, (char *) NULL);
      return FALSE;
    }
  /*NOTREACHED*/
}

