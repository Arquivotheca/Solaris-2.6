#if !defined(lint) && !defined(NOID)
  static char sccsid[] = "@(#)generic.cc	1.34 5/5/91 SMI";
#endif

/**************************************************************************
 *  File:	lib/libnetmgt/generic.cc
 *
 *  Author:	Lynn Monsanto, Sun Microsystems Inc.
 *
 *  SCCSID:     @(#)generic.cc  1.34  91/05/05
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
 *  Comments:	message argument list decoding routines
 *
 **************************************************************************
 */

/*LINTLIBRARY*/

#include "netmgt/netmgt_hdrs.h"
#include "netmgt/netmgt.h"
#include "libnetmgt/libnetmgt.h"

/* ------------------------------------------------------------------
 *  NetmgtGeneric::deserial - deserialize generic argument
 *      returns TRUE is successful; otherwise returns FALSE
 * -------------------------------------------------------------------
 */
bool_t
NetmgtGeneric::deserial (XDR *xdr, 
			 NetmgtServiceMsg *aServiceMsg, 
			 bool_t deserialTag)
              			// XDR stream handle 
				// service message pointer
				// whether deserialize tag 
{
  char *pname;			// name pointer 

  NETMGT_PRN (("generic: NetmgtGeneric::deserial\n"));

  assert (xdr != (XDR *) NULL);

  // deserialize tag if requested 
  if (deserialTag)
    {
      if (!xdr_enum (xdr, (int *) &this->tag))
	{
	  NETMGT_PRN (("generic: can't decode argument tag\n"));
	  xdr_destroy (xdr);
	  return FALSE;
	}
    }

  // decode argument name string 
  pname = (char *) this->name;
  if (!xdr_string (xdr, &pname, NETMGT_NAMESIZ))
    {
      NETMGT_PRN (("generic: can't decode argument name\n"));
      xdr_destroy (xdr);
      return FALSE;
    }

  // a sentinal string marks the end of the arglist 
  if (strcmp (this->name, NETMGT_ENDOFARGS) == 0)
    {
      this->tag = NETMGT_END_TAG;
      return TRUE;
    }

  // decode argument type 
  if (!xdr_u_int (xdr, &this->type))
    {
      NETMGT_PRN (("generic: can't decode argument type\n"));
      xdr_destroy (xdr);
      return FALSE;
    }

  // decode argument value length 
  if (!xdr_u_int (xdr, &this->length))
    {
      NETMGT_PRN (("generic: can't decode value length\n"));
      xdr_destroy (xdr);
      return FALSE;
    }

  // (possibly) decode argument value 
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

  // decode relational operator 
  if (!xdr_u_int (xdr, &this->relop))
    {
      NETMGT_PRN (("generic: can't decode relational operator\n"));
      xdr_destroy (xdr);
      return FALSE;
    }

  // set argument tag if it wasn't encoded with the argument list 
  if (!deserialTag && !this->setTag (aServiceMsg, this->relop, &this->tag))
    {
      NETMGT_PRN (("generic: can't set argument tag\n"));
      xdr_destroy (xdr);
      return FALSE;
    }

  // decode threshold value length 
  if (!xdr_u_int (xdr, &this->thresh_len))
    {
      NETMGT_PRN (("generic: can't decode threshold value length\n"));
      xdr_destroy (xdr);
      return FALSE;
    }

  // (possibly) decode threshold value and event report priority 
  if (this->thresh_len > 0)
    {
      aServiceMsg->myArglist.myValue2.resetPtr ();
      this->thresh_val = aServiceMsg->myArglist.myValue2.getPtr ();
      if (!aServiceMsg->myArglist.deserialValue (xdr, 
				       this->type, 
				       this->thresh_len,
				       this->thresh_val))
	return FALSE;

      // decode event report priority 
      if (!xdr_u_int (xdr, &this->priority))
	{
	  NETMGT_PRN (("generic: can't decode priority\n"));
	  xdr_destroy (xdr);
	  return FALSE;
	}
    }

  return TRUE;
}

/* --------------------------------------------------------------------
 *  NetmgtGeneric::setTag - set generic argument tag
 *	returns TRUE if successful; otherwise returns FALSE
 * --------------------------------------------------------------------
 */
bool_t
NetmgtGeneric::setTag (NetmgtServiceMsg *aServiceMsg, 
		       u_int setTagrelop, 
		       NetmgtArgTag *ptag)
     // a service message pointer
     // threshold relational operator
     // argument tag pointer
{

  switch (setTagrelop)
    {
    case NETMGT_NOP:		
      switch (aServiceMsg->getType ())
	{
	case NETMGT_ACTION_REQUEST:
	case NETMGT_CREATE_REQUEST: 
	case NETMGT_DELETE_REQUEST: 
	case NETMGT_DATA_REQUEST:
	case NETMGT_EVENT_REQUEST:
	case NETMGT_SET_REQUEST:
	  *ptag = NETMGT_OPTION_TAG;
	  return TRUE;

	case NETMGT_ACTION_REPORT:
	case NETMGT_DATA_REPORT:
	case NETMGT_EVENT_REPORT:
	case NETMGT_TRAP_REPORT:
	  *ptag = NETMGT_DATA_TAG;
	  return TRUE;

	case NETMGT_ERROR_REPORT:
	case NETMGT_SET_REPORT:
	  *ptag = NETMGT_ERROR_TAG;
	  return TRUE;

	default:		// error 
	  NETMGT_PRN (("generic: bad tag: setTagrelop == %d, type == %d\n", 
		       setTagrelop, aServiceMsg->getType ()));
	  *ptag = NETMGT_BAD_TAG;
	  return FALSE;
	}

    case NETMGT_EQ:
    case NETMGT_NE:
    case NETMGT_GT:
    case NETMGT_GE:
    case NETMGT_LT:
    case NETMGT_LE:
    case NETMGT_CHANGED:
    case NETMGT_INCRBY:
    case NETMGT_DECRBY:
    case NETMGT_INCRBYMORE:
    case NETMGT_INCRBYLESS:
    case NETMGT_DECRBYMORE:
    case NETMGT_DECRBYLESS:
      switch (aServiceMsg->getType ())
	{
	case NETMGT_EVENT_REQUEST:
	  *ptag = NETMGT_THRESH_TAG;
	  return TRUE;
	  
	case NETMGT_EVENT_REPORT:
	  *ptag = NETMGT_EVENT_TAG;
	  return TRUE;

	default:		// error 
	  NETMGT_PRN (("generic: bad tag: setTagrelop == %d, type == %d\n", 
		       setTagrelop, aServiceMsg->getType ()));
	  *ptag = NETMGT_BAD_TAG;
	  return FALSE;
	}
    default:			// error 
      NETMGT_PRN (("generic: bad tag: setTagrelop == %d, type == %d\n", 
		   setTagrelop, aServiceMsg->getType ()));
      *ptag = NETMGT_BAD_TAG;
      return FALSE;
    }
  /*NOTREACHED*/
}

/* -------------------------------------------------------------------
 *  NetmgtGeneric::serial - serialize generic argument
 *      returns TRUE is successful; otherwise returns FALSE
 * -------------------------------------------------------------------
 */
bool_t
NetmgtGeneric::serial (XDR *xdr, NetmgtServiceMsg *aServiceMsg, bool_t serialTag)
              			// XDR stream handle 
				// sending service message pointer
				// whether serialize tag 
{
  char *pname;			// name pointer 

  NETMGT_PRN (("encode: NetmgtGeneric::serial\n"));

  assert (xdr != (XDR *) NULL);

  // serialize tag if requested 
  if (serialTag)
    {
      if (!xdr_enum (xdr, (int *) &this->tag))
	{
	  NETMGT_PRN (("encode: can't encode argument tag\n"));
	  xdr_destroy (xdr);
	  return FALSE;
	}
    }

  // encode argument name string 
  pname = (char *) this->name;
  if (!xdr_string (xdr, &pname, NETMGT_NAMESIZ))
    {
      NETMGT_PRN (("encode: can't encode argument name\n"));
      xdr_destroy (xdr);
      return FALSE;
    }

  // return if this is the last argument 
  if (this->tag == NETMGT_END_TAG)
    return TRUE;

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

  // (possibly) encode argument value 
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

  // encode relational operator 
  if (!xdr_u_int (xdr, &this->relop))
    {
      NETMGT_PRN (("encode: can't encode relational operator\n"));
      xdr_destroy (xdr);
      return FALSE;
    }

  // encode theshold value length 
  if (!xdr_u_int (xdr, &this->thresh_len))
    {
      NETMGT_PRN (("encode: can't encode value length\n"));
      xdr_destroy (xdr);
      return FALSE;
    }

  // (possibly) encode threshold value and event priority 
  if (this->thresh_len > 0)
    {
      aServiceMsg->myArglist.myValue2.resetPtr ();
      this->thresh_val = aServiceMsg->myArglist.myValue2.getPtr ();
      if (!aServiceMsg->myArglist.serialValue (xdr, 
				     this->type, 
				     this->thresh_len,
				     this->thresh_val))
	return FALSE;

      // encode event report priority 
      if (!xdr_u_int (xdr, &this->priority))
	{
	  NETMGT_PRN (("encode: can't encode priority\n"));
	  xdr_destroy (xdr);
	  return FALSE;
	}
    }

  return TRUE;
}

/* ------------------------------------------------------------------
 *  NetmgtGeneric::putOption - append option argument to arglist
 *      returns TRUE is successful; otherwise returns FALSE
 * ------------------------------------------------------------------
 */
bool_t
NetmgtGeneric::putOption (Netmgt_arg *option, NetmgtServiceMsg *aServiceMsg)
				// options argument pointer
				// service message pointer
{

  NETMGT_PRN (("putarg: NetmgtGeneric::putOption\n")) ;

  this->tag = NETMGT_OPTION_TAG;
  (void) strncpy (this->name, option->name, NETMGT_NAMESIZ); 
  this->type = option->type;
  this->length = option->length;
  this->value = option->value;
  this->relop = NETMGT_NOP;
  this->thresh_len = (u_int) 0;
  this->thresh_val = (caddr_t) NULL;
  this->priority = (u_int) 0;

  return this->putArg (aServiceMsg);
}

/* ----------------------------------------------------------------------
 *  NetmgtGeneric::getOption - get request option argument from arglist
 *      returns TRUE if successful; otherwise returns FALSE
 * ----------------------------------------------------------------------
 */
bool_t
NetmgtGeneric::getOption (Netmgt_arg *option)
                       		// option argument pointer
{
  static char nullString[] = ""; // null string 

  NETMGT_PRN (("value: NetmgtGeneric::getOption\n"));

  // reset internal error buffer 
  _netmgtStatus.clearStatus ();

  if (!option)
    {
      NETMGT_PRN (("value: no performance argument\n"));
      _netmgtStatus.setStatus (NETMGT_NOPERFBUF, 0, NULL);
      return FALSE;
    }
  if (!option->name)
    {
      NETMGT_PRN (("value: no attribute name pointer\n"));
      _netmgtStatus.setStatus (NETMGT_NOARGNAME, 0, NULL);
      return FALSE;
    }

  // copy argument to user's buffer 
  (void) strncpy (option->name, this->name, NETMGT_NAMESIZ);
  option->type = this->type;
  option->length = this->length;
  option->value = this->value;

  // make sure zero length strings contain null 
  if (option->type == NETMGT_STRING && option->length == (u_int) 0)
    option->value = nullString;

  return TRUE;
}

/* -----------------------------------------------------------------------
 *  NetmgtGeneric::putThresh - append threshold argument to arglist
 *      returns TRUE is successful; otherwise returns FALSE
 * -----------------------------------------------------------------------
 */
bool_t
NetmgtGeneric::putThresh (Netmgt_thresh *thresh, NetmgtServiceMsg *aServiceMsg)
				// threshold pointer
				// service message pointer
{

  NETMGT_PRN (("putarg: NetmgtGeneric::putThresh\n")) ;

  this->tag = NETMGT_THRESH_TAG;
  (void) strncpy (this->name, thresh->name, NETMGT_NAMESIZ);
  this->type = thresh->type;
  this->length = (u_int) 0;
  this->value = (caddr_t) NULL;
  this->relop = thresh->relop;
  this->thresh_len = thresh->thresh_len;
  this->thresh_val = thresh->thresh_val; 
  this->priority = thresh->priority;

  return this->putArg (aServiceMsg);

}

/* ---------------------------------------------------------------------
 *  NetmgtGeneric::getThresh - get threshold argument from arglist
 *      returns TRUE if successful; otherwise returns FALSE
 * ---------------------------------------------------------------------
 */
bool_t
NetmgtGeneric::getThresh (Netmgt_thresh *thresh)
                         	// thresh argument pointer
{
  static NetmgtGeneric generic; // generic argument 
  static char nullString[] = ""; // null string 

  NETMGT_PRN (("value: NetmgtGeneric::getThresh\n"));

  // reset internal error buffer 
  _netmgtStatus.clearStatus ();

  if (!thresh)
    {
      NETMGT_PRN (("value: no thresh argument\n"));
      _netmgtStatus.setStatus (NETMGT_NOEVENTBUF, 0, NULL);
      return FALSE;
    }
  if (!thresh->name)
    {
      NETMGT_PRN (("value: no attribute name pointer\n"));
      _netmgtStatus.setStatus (NETMGT_NOARGNAME, 0, NULL);
      return FALSE;
    }

  // copy argument to user's buffer 
  (void) strncpy (thresh->name, this->name, NETMGT_NAMESIZ);
  thresh->type = this->type;
  thresh->relop = this->relop;
  thresh->thresh_len = this->thresh_len;
  thresh->thresh_val = this->thresh_val;
  thresh->priority = this->priority;

  // make sure zero length strings contain null 
  if (thresh->type == NETMGT_STRING && thresh->thresh_len == (u_int) 0)
      thresh->thresh_val = nullString;

  return TRUE;
}

/* ------------------------------------------------------------------
 *  NetmgtGeneric::putData - append data argument to arglist
 *      returns TRUE is successful; otherwise returns FALSE
 * ------------------------------------------------------------------
 */
bool_t
NetmgtGeneric::putData (Netmgt_data *data, NetmgtServiceMsg *aServiceMsg)
				// data argument pointer
				// service message pointer
{

  NETMGT_PRN (("putarg: NetmgtGeneric::putData\n")) ;

  this->tag = NETMGT_DATA_TAG;
  (void) strncpy (this->name, data->name, NETMGT_NAMESIZ); 
  this->type = data->type;
  this->length = data->length;
  this->value = data->value;
  this->relop = NETMGT_NOP;
  this->thresh_len = (u_int) 0;
  this->thresh_val = (caddr_t) NULL;
  this->priority = (u_int) 0;

  return this->putArg (aServiceMsg);
}

/* ---------------------------------------------------------------------
 *  NetmgtGeneric::getData - get data argument from arglist
 *      returns TRUE if successful; otherwise returns FALSE
 * ---------------------------------------------------------------------
 */
bool_t
NetmgtGeneric::getData (Netmgt_data *data, NetmgtServiceMsg *aServiceMsg)
                       		// data argument pointer
				// service message pointer
{
  static char nullString[] = ""; // null string 

  NETMGT_PRN (("value: NetmgtGeneric::getData\n"));

  // reset internal error buffer 
  _netmgtStatus.clearStatus ();

  if (!data)
    {
      NETMGT_PRN (("value: no performance argument\n"));
      _netmgtStatus.setStatus (NETMGT_NOPERFBUF, 0, NULL);
      return FALSE;
    }
  if (!data->name)
    {
      NETMGT_PRN (("value: no attribute name pointer\n"));
      _netmgtStatus.setStatus (NETMGT_NOARGNAME, 0, NULL);
      return FALSE;
    }

  // get next argument from arglist 
  if (!this->getArg (aServiceMsg))
    return FALSE; 

  // copy argument to user's buffer 
  (void) strncpy (data->name, this->name, NETMGT_NAMESIZ);
  data->type = this->type;
  data->length = this->length;
  data->value = this->value;

  // make sure zero length strings contain null 
  if (data->type == NETMGT_STRING && data->length == (u_int) 0)
    data->value = nullString;

  return TRUE;
}

/* ------------------------------------------------------------------
 *  NetmgtGeneric::putEvent - append event argument to message arglist
 *      returns TRUE is successful; otherwise returns FALSE
 * ------------------------------------------------------------------
 */
bool_t
NetmgtGeneric::putEvent (Netmgt_event *event, NetmgtServiceMsg *aServiceMsg)
				// event buffer pointer
				// service message pointer
{

  NETMGT_PRN (("putarg: NetmgtGeneric::putEvent\n")) ;

  this->tag = NETMGT_EVENT_TAG;
  (void) strncpy (this->name, event->name, NETMGT_NAMESIZ); 
  this->type = event->type;
  this->length = event->length;
  this->value = event->value;
  this->relop = event->relop;
  this->thresh_len = event->length;
  this->thresh_val = event->thresh_val;
  this->priority = event->priority;

  return this->putArg (aServiceMsg);
}

/* ---------------------------------------------------------------------
 *  NetmgtGeneric::getEvent - get event report argument from arglist
 *      returns TRUE if successful; otherwise returns FALSE
 * ---------------------------------------------------------------------
 */
bool_t
NetmgtGeneric::getEvent (Netmgt_event *event, NetmgtServiceMsg *aServiceMsg)
                         	// event argument pointer
				// service message pointer
{
  static NetmgtGeneric generic; // generic argument 
  static char nullString[] = ""; // null string 

  NETMGT_PRN (("value: NetmgtGeneric::getEvent\n"));

  // reset internal error buffer 
  _netmgtStatus.clearStatus ();

  if (!event)
    {
      NETMGT_PRN (("value: no event argument\n"));
      _netmgtStatus.setStatus (NETMGT_NOEVENTBUF, 0, NULL);
      return FALSE;
    }
  if (!event->name)
    {
      NETMGT_PRN (("value: no attribute name pointer\n"));
      _netmgtStatus.setStatus (NETMGT_NOARGNAME, 0, NULL);
      return FALSE;
    }

  // get next argument from arglist 
  if (!this->getArg (aServiceMsg))
    return FALSE;

  // copy argument to user's buffer 
  (void) strncpy (event->name, this->name, NETMGT_NAMESIZ);
  event->type = this->type;
  event->length = this->length;
  event->value = this->value;
  event->relop = this->relop;
  event->thresh_val = this->thresh_val;
  event->priority = this->priority;

  // make sure zero length strings contain null 
  if (event->type == NETMGT_STRING && event->length == (u_int) 0)
    {
      event->value = nullString;
      event->thresh_val = nullString;
    }

  return TRUE;
}

