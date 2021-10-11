#if !defined(lint) && !defined(NOID)
  static char sccsid[] = "@(#)getarg.cc	1.41 5/5/91 SMI";
#endif

/**************************************************************************
 *  File:	lib/libnetmgt/getarg.cc
 *
 *  Author:	Lynn Monsanto, Sun Microsystems Inc.
 *
 *  SCCSID:     @(#)getarg.cc  1.41  91/05/05
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
 *  Comments:	routines for getting arguments from arglist
 *
 **************************************************************************
 */

/*LINTLIBRARY*/

#include "netmgt/netmgt_hdrs.h"
#include "netmgt/netmgt.h"
#include "libnetmgt/libnetmgt.h"

/* ----------------------------------------------------------------------
 *  NetmgtGeneric::getArg - fetch generic argument from message arglist
 *      returns TRUE if successful; otherwise returns FALSE
 * ----------------------------------------------------------------------
 */
bool_t
NetmgtGeneric::getArg (NetmgtServiceMsg *aServiceMsg)
				// service message pointer
{
  NETMGT_PRN (("getarg: NetmgtGeneric::getArg\n"));

  // get attribute name string 
  if (!aServiceMsg->myArglist.getName (this->name))
    return FALSE;

  // a sentinal string marks the end of the arglist 
  if (strcmp (this->name, NETMGT_ENDOFARGS) == 0)
    {
      this->tag = NETMGT_END_TAG;
      return TRUE;
    }

  // get attribute type 
  if (!aServiceMsg->myArglist.getType (&this->type))
    return FALSE;

  // get attribute value length 
  if (!aServiceMsg->myArglist.getLength (&this->length))
    return FALSE;

  // (possibly) get attribute value 
  if (this->length > 0)
    {
      aServiceMsg->myArglist.myValue1.resetPtr ();
      this->value = aServiceMsg->myArglist.myValue1.getPtr ();
      if (!aServiceMsg->myArglist.getval (this->type, 
					  this->length, 
					  this->value))
	return FALSE;
    }

  // get relational operator 
  if (!aServiceMsg->myArglist.getRelop (&this->relop))
    return FALSE;

  // set argument tag 
  if (!this->setTag (aServiceMsg, this->relop, &this->tag))
    {
      NETMGT_PRN (("getarg: can't set argument tag\n"));
      return FALSE;
    }

  // get threshold value length 
  if (!aServiceMsg->myArglist.getLength (&this->thresh_len))
    return FALSE;

  // (possibly) get threshold value 
  if (this->thresh_len > 0)
    {
      aServiceMsg->myArglist.myValue2.resetPtr ();
      this->thresh_val = aServiceMsg->myArglist.myValue2.getPtr();
      if (!aServiceMsg->myArglist.getval (this->type, 
				this->length, 
				this->thresh_val))
	return FALSE;

      // get event report priority 
      if (!aServiceMsg->myArglist.getPriority (&this->priority))
	return FALSE;
    }

  return TRUE;
}

/* -------------------------------------------------------------------
 *  NetmgtSetarg::getArg - get set argument from arglist
 *      returns TRUE if successful; otherwise returns FALSE
 * -------------------------------------------------------------------
 */
bool_t
NetmgtSetarg::getArg (NetmgtServiceMsg *aServiceMsg)
				// service message pointer
{
  NetmgtArgTag getArgtag ;		// argument tag

  NETMGT_PRN (("getarg: NetmgtSetarg::getArg\n"));

  // get argument tag 
  if (!aServiceMsg->myArglist.getTag (&getArgtag))
    return FALSE;

  // get group  name string 
  if (!aServiceMsg->myArglist.getName (this->group))
    return FALSE;

  // a sentinal string marks the end of the arglist 
  if (strcmp (this->group, NETMGT_ENDOFARGS) == 0)
    return TRUE;

  // get key name string 
  if (!aServiceMsg->myArglist.getName (this->key))
    return FALSE;

  // get attribute  name string 
  if (!aServiceMsg->myArglist.getName (this->name))
    return FALSE;

  // get attribute type 
  if (!aServiceMsg->myArglist.getType (&this->type))
    return FALSE;

  // get attribute length 
  if (!aServiceMsg->myArglist.getLength (&this->length))
    return FALSE;

  // get attribute value 
  aServiceMsg->myArglist.myValue1.resetPtr ();
  this->value = aServiceMsg->myArglist.myValue1.getPtr ();
  if (!aServiceMsg->myArglist.getval (this->type, this->length, this->value))
    return FALSE;

  return TRUE;
}

/* ----------------------------------------------------------------------
 *  NetmgtArglist::getTag - get argument tag from argument list
 *	returns TRUE if successful; otherwise returns FALSE
 * ----------------------------------------------------------------------
 */
bool_t
NetmgtArglist::getTag (NetmgtArgTag *ptag)
{
  (void) memcpy ((caddr_t) ptag, this->getPtr (), sizeof (NetmgtArgTag));
  this->incrPtr (sizeof (NetmgtArgTag));

  if (*ptag != NETMGT_SETARG_TAG &&
      *ptag != NETMGT_OPTION_TAG &&
      *ptag != NETMGT_END_TAG)
    {
      if (strncmp ((char *) this->getPtr (),
                   NETMGT_ENDOFARGS,
                   sizeof (NETMGT_ENDOFARGS)) == 0)
        *ptag = NETMGT_END_TAG;
      else
        *ptag = NETMGT_OPTION_TAG;
    }
  NETMGT_PRN (("getarg: tag: %u\n", *ptag));
  return TRUE;
}

/* ------------------------------------------------------------------------
 *  NetmgtArglist::peekTag - peek at argument tag in argument list
 *	returns TRUE if successful; otherwise returns FALSE
 * ------------------------------------------------------------------------
 */
bool_t
NetmgtArglist::peekTag (NetmgtArgTag *ptag)
{
  (void) memcpy ((caddr_t) ptag, this->getPtr (), 
		 sizeof (NetmgtArgTag));

  if (*ptag != NETMGT_SETARG_TAG &&
      *ptag != NETMGT_OPTION_TAG &&
      *ptag != NETMGT_END_TAG)
    {
      if (strncmp ((char *) this->getPtr (),
                   NETMGT_ENDOFARGS,
                   sizeof (NETMGT_ENDOFARGS)) == 0)
        *ptag = NETMGT_END_TAG;
      else
        *ptag = NETMGT_OPTION_TAG;
    }
  NETMGT_PRN (("getarg: tag: %u\n", *ptag));
  return TRUE;
}

/* --------------------------------------------------------------------
 *  NetmgtArglist::getName - get argument name string from argument 
 *	list and copy it to name buffer.  Returns TRUE if successful;
 *      otherwise returns FALSE
 * --------------------------------------------------------------------
 */
bool_t
NetmgtArglist::getName (char *name)
{
  (void) strcpy (name, this->getPtr ());
  this->incrPtr (strlen (name) + 1);
  NETMGT_PRN (("getarg: name: %s\n", name));
  return TRUE;
}

/* --------------------------------------------------------------------
 *  NetmgtArglist::getType - get argument type from argument list
 *	and copy it to type buffer.  Returns TRUE if successful;
 *      otherwise returns FALSE
 * --------------------------------------------------------------------
 */
bool_t
NetmgtArglist::getType (u_int *ptype)
{
  (void) memcpy ((caddr_t) ptype, this->getPtr (), sizeof (u_int));
  this->incrPtr (sizeof (u_int));
  NETMGT_PRN (("getarg: type: %u\n", *ptype));
  return TRUE;
}

/* --------------------------------------------------------------------
 *  NetmgtArglist::getLength - get argument length from argument list
 *	and copy it to length buffer.  Returns TRUE if successful;
 *      otherwise returns FALSE
 * --------------------------------------------------------------------
 */
bool_t
NetmgtArglist::getLength (u_int *plength)
{
  (void) memcpy ((caddr_t) plength, this->getPtr (), sizeof (*plength));
  this->incrPtr (sizeof (*plength));
  NETMGT_PRN (("getarg: length: %u\n", *plength));
  return TRUE;
}

/* --------------------------------------------------------------------
 *  NetmgtArglist::getRelop - get relational operator from argument 
 *	list and copy it to operator buffer.  Returns TRUE if successful;
 *      otherwise returns FALSE
 * --------------------------------------------------------------------
 */
bool_t
NetmgtArglist::getRelop (u_int *prelop)
{
  (void) memcpy ((caddr_t) prelop, this->getPtr (), sizeof (*prelop));
  this->incrPtr (sizeof (*prelop));
  NETMGT_PRN (("getarg: relop: %u\n", *prelop));
  return TRUE;
}

/* --------------------------------------------------------------------
 *  NetmgtArglist::getval - get value from argument list
 *	returns TRUE if successful; otherwise returns FALSE
 * --------------------------------------------------------------------
 */
bool_t
NetmgtArglist::getval (u_int type, u_int length, caddr_t getvalbase)
                		// value type 
                  		// value length 
                  		// value buffer base 
{
  struct in_addr inaddr;	// IP address 

  NETMGT_PRN (("getarg: NetmgtArglist::getval\n"));

  switch (type)
    {
    case NETMGT_SHORT:
      (void) memcpy (getvalbase, this->getPtr (), sizeof (short));
      this->incrPtr (sizeof (short));
      NETMGT_PRN (("getarg: value: %d\n", *(short *) getvalbase));
      break;

    case NETMGT_U_SHORT:
      (void) memcpy (getvalbase, this->getPtr (), sizeof (u_short));
      NETMGT_PRN (("getarg: value: %u\n", *(u_short *) getvalbase));
      this->incrPtr (sizeof (u_short));
      break;

    case NETMGT_INT:
      (void) memcpy (getvalbase, this->getPtr (), sizeof (int));
      NETMGT_PRN (("getarg: value: %d\n", *(int *) getvalbase));
      this->incrPtr (sizeof (int));
      break;

    case NETMGT_U_INT:
    case NETMGT_UNIXTIME:
    case NETMGT_ENUM:
    case NETMGT_INTEGER:
    case NETMGT_COUNTER:
    case NETMGT_GAUGE:
    case NETMGT_TIMETICKS:
      (void) memcpy (getvalbase, this->getPtr (), sizeof (u_int));
      NETMGT_PRN (("getarg: value: %u\n", *(u_int *) getvalbase));
      this->incrPtr (sizeof (u_int));
      break;

    case NETMGT_LONG:
      (void) memcpy (getvalbase, this->getPtr (), sizeof (long));
      NETMGT_PRN (("getarg: value: %d\n", *(long *) getvalbase));
      this->incrPtr (sizeof (long));
      break;

    case NETMGT_U_LONG:
      (void) memcpy (getvalbase, this->getPtr (), sizeof (u_long));
      NETMGT_PRN (("getarg: value: %lu\n", *(u_long *) getvalbase));
      this->incrPtr (sizeof (u_long));
      break;

    case NETMGT_FLOAT:
      (void) memcpy (getvalbase, this->getPtr (), sizeof (float));
      NETMGT_PRN (("getarg: value: %f\n", *(float *) getvalbase));
      this->incrPtr (sizeof (float));
      break;

    case NETMGT_DOUBLE:
      (void) memcpy (getvalbase, this->getPtr (), sizeof (double));
      NETMGT_PRN (("getarg: value: %f\n", *(double *) getvalbase));
      this->incrPtr (sizeof (double));
      break;

    case NETMGT_STRING:
      (void) sprintf (getvalbase, "%s", this->getPtr ());
      NETMGT_PRN (("getarg: value: %s\n", (char *) getvalbase));
      this->incrPtr (strlen (this->getPtr ()) + 1);
      break;

    case NETMGT_OCTET:
    case NETMGT_OCTET_STRING:
      (void) memcpy (getvalbase, this->getPtr (), (int) length);
#ifdef DEBUG
      if (netmgt_debug > 3)
	{
	  (void) (printf, "getarg: value: ");
	  for (int i = 0; i < length; i++)
	    {
	      if ((i % 16) == 0)
		(void) printf ("\n");
	      (void) printf ("%2.2x ", getvalbase [i]);
	    }
	  (void) printf ("\n");
	}
#endif DEBUG
      this->incrPtr (length);
      break;

    case NETMGT_INADDR:
      (void) memcpy (getvalbase, this->getPtr (), sizeof (struct in_addr));
      NETMGT_PRN (("getarg: value: %s\n",
		   inet_ntoa (*(struct in_addr *) getvalbase)));
      this->incrPtr (sizeof (struct in_addr));
      break;

    case NETMGT_TIMEVAL:
      (void) memcpy (getvalbase, this->getPtr (), sizeof (struct timeval));
      NETMGT_PRN (("getarg: value: %d.%d\n",
		   ((struct timeval *) getvalbase)->tv_sec,
		   ((struct timeval *) getvalbase)->tv_usec));
      this->incrPtr (sizeof (struct timeval));
      break;

    case NETMGT_OBJECT_IDENTIFIER:
      (void) memcpy (getvalbase, this->getPtr (), (int) length);
      NETMGT_PRN (("getarg: value: `OBJECT_IDENTIFIER'\n"));
      this->incrPtr (length);
      break;

    case NETMGT_NET_ADDRESS:
    case NETMGT_IP_ADDRESS:
      (void) memcpy (getvalbase, this->getPtr (), sizeof (struct in_addr));
      (void) memcpy ((caddr_t) & inaddr, this->getPtr (), 
		     sizeof (struct in_addr));
      inaddr.s_addr = ntohl (inaddr.s_addr);
      NETMGT_PRN (("getarg: value: %s\n", inet_ntoa (inaddr)));
      this->incrPtr (sizeof (struct in_addr));
      break;

    case NETMGT_OPAQUE:
      (void) memcpy (getvalbase, this->getPtr (), (int) length);
      NETMGT_PRN (("getarg: value: `OPAQUE'\n"));
      this->incrPtr (length);
      break;

    default:
      NETMGT_PRN (("getarg: unknown type: %d\n", type));
      _netmgtStatus.setStatus (NETMGT_UNKNOWNTYPE, 0, NULL);
      return FALSE;
    }

  return TRUE;
}

/* --------------------------------------------------------------------
 *  NetmgtArglist::getPriority - get event priority from argument 
 *	list and copy it to length buffer.  Returns TRUE if successful;
 *      otherwise returns FALSE
 * --------------------------------------------------------------------
 */
bool_t
NetmgtArglist::getPriority (u_int *ppriority)
{
  (void) memcpy ((caddr_t) ppriority, this->getPtr (), 
		 sizeof (*ppriority));
  this->incrPtr (sizeof (*ppriority));
  NETMGT_PRN (("getarg: priority: %u\n", *ppriority));
  return TRUE;
}


