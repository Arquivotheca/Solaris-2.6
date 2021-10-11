#if !defined(lint) && !defined(NOID)
  static char sccsid[] = "@(#)putarg.cc	1.39 5/5/91 SMI";
#endif

/**************************************************************************
 *  File:	lib/libnetmgt/putarg.cc
 *
 *  Author:	Lynn Monsanto, Sun Microsystems Inc.
 *
 *  SCCSID:     @(#)putarg.cc  1.39  91/05/05
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
 *  Comments:	routines for appending arguments to arglist
 *
 **************************************************************************
 */

/*LINTLIBRARY*/

#include "netmgt/netmgt_hdrs.h"
#include "netmgt/netmgt.h"
#include "libnetmgt/libnetmgt.h"

/* -------------------------------------------------------------------
 *  NetmgtGeneric::putArg - append generic argument to message arglist
 *      returns TRUE is successful; otherwise returns FALSE
 * -------------------------------------------------------------------
 */
bool_t
NetmgtGeneric::putArg (NetmgtServiceMsg *aServiceMsg)
				// message pointer
{
  u_int arglen ;		// argument tuple length 

  NETMGT_PRN (("putarg: NetmgtGeneric::putArg\n")) ;

  // allocate message arglist buffer 
  arglen = strlen (this->name) + 1 + 5 * sizeof (u_int) + this->length + 
    this->length + + sizeof (NETMGT_ENDOFARGS) + 1 ;
  if (!aServiceMsg->myArglist.alloc (arglen, NETMGT_MINARGSIZ))
    return FALSE;

  // append attribute name string to arglist 
  if (!aServiceMsg->myArglist.putName (this->name))
    return FALSE;

  // a sentinal name string marks the end of the arglist 
  if (strcmp (this->name, NETMGT_ENDOFARGS) == 0)
    return TRUE;

  // append attribute type to arglist 
  if (!aServiceMsg->myArglist.putType (this->type))
    return FALSE;

  // append attribute value length 
  if (!aServiceMsg->myArglist.putLength (this->length))
    return FALSE;

  // append attribute value to arglist 
  if (this->length > 0 &&
      !aServiceMsg->myArglist.putValue (this->type, this->length, this->value))
    return FALSE;

  // append relational operator to arglist 
  if (!aServiceMsg->myArglist.putRelop (this->relop))
    return FALSE;

  // append threshold value length 
  if (!aServiceMsg->myArglist.putLength (this->thresh_len))
    return FALSE;

  // (possibly) append threshold value to arglist 
  if (this->thresh_len > 0 &&
      !aServiceMsg->myArglist.putValue (this->type, this->thresh_len, 
				 this->thresh_val))
    return FALSE;

  // (possibly) append priority to arglist 
  if (this->thresh_len > 0 &&
      !aServiceMsg->myArglist.putPriority (this->priority))
    return FALSE;

  // append sentinal to arglist 
  if (!aServiceMsg->myArglist.putEndTag (FALSE))
    return FALSE;

  return TRUE;
}

/* ------------------------------------------------------------------
 *  NetmgtSetarg::putSetval - append set argument (data) to arglist
 *      returns TRUE is successful; otherwise returns FALSE
 * ------------------------------------------------------------------
 */
bool_t
NetmgtSetarg::putSetval (Netmgt_setval *setval, NetmgtServiceMsg *aServiceMsg)
				// set argument pointer
				// message pointer
{

  NETMGT_PRN (("putarg: NetmgtSetarg::putSetval\n")) ;
  
  // copy set argument data to set argument
  (void) strncpy (this->group, setval->group, sizeof (this->group));
  (void) strncpy (this->key, setval->key, sizeof (this->key));
  (void) strncpy (this->name, setval->name, sizeof (this->name));
  this->type = setval->type;
  this->length = setval->length;
  this->value = setval->value;

  // append set argument to arglist
  return this->putArg (aServiceMsg);
}

/* ------------------------------------------------------------------
 *  NetmgtSetarg::putArg - append set argument to arglist
 *      returns TRUE is successful; otherwise returns FALSE
 * ------------------------------------------------------------------
 */
bool_t
NetmgtSetarg::putArg (NetmgtServiceMsg *aServiceMsg)
				// message pointer
{
  u_int arglen ;		// argument tuple length 

  NETMGT_PRN (("putarg: NetmgtSetarg::putArg\n")) ;

  // allocate message arglist buffer 
  arglen = strlen (this->group) + 1 + strlen (this->key) + 1  
    + strlen (this->name) + 1 + 2 * sizeof (u_int) + this->length 
      + sizeof (NETMGT_ENDOFARGS) + 1 ;
  if (!aServiceMsg->myArglist.alloc (arglen, NETMGT_MINARGSIZ))
    return FALSE;

  // append tag to arglist 
  if (!aServiceMsg->myArglist.putTag (NETMGT_SETARG_TAG))
    return FALSE;

  // append group/table name string to arglist 
  if (!aServiceMsg->myArglist.putName (this->group))
    return FALSE;

  // a sentinal string marks the end of the arglist 
  if (strcmp (this->name, NETMGT_ENDOFARGS) == 0)
    return TRUE;

  // append key name string to arglist 
  if (!aServiceMsg->myArglist.putName (this->key))
    return FALSE;

  // append attribute name string to arglist 
  if (!aServiceMsg->myArglist.putName (this->name))
    return FALSE;

  // append attribute type to arglist 
  if (!aServiceMsg->myArglist.putType (this->type))
    return FALSE;

  // append attribute length to arglist 
  if (!aServiceMsg->myArglist.putLength (this->length))
    return FALSE;

  // append attribute value to arglist 
  if (!aServiceMsg->myArglist.putValue (this->type, this->length, this->value))
    return FALSE;

  // append sentinal to arglist 
  if (!aServiceMsg->myArglist.putEndTag (TRUE))
    return FALSE;

  return TRUE;
}

/* -------------------------------------------------------------------
 *  NetmgtArglist::putTag - append argument tag to argument list
 *	returns TRUE if successful; otherwise returns FALSE
 * -------------------------------------------------------------------
 */
bool_t
NetmgtArglist::putTag (NetmgtArgTag tag)
{

  NETMGT_PRN (("putarg: tag: %u\n", tag));
  (void) memcpy (this->getPtr (), (caddr_t) & tag, 
		 sizeof (NetmgtArgTag));
  this->incrPtr (sizeof (NetmgtArgTag));
  return TRUE;
}

/* -------------------------------------------------------------------
 *  NetmgtArglist::putName - append argument name to argument list
 *	returns TRUE if successful; otherwise returns FALSE
 * -------------------------------------------------------------------
 */
bool_t
NetmgtArglist::putName (char *name)
{

  NETMGT_PRN (("putarg: name: %s\n", name));
  (void) sprintf (this->getPtr (), "%s", name);
  this->incrPtr ((strlen (this->getPtr ()) + 1));
  return TRUE;
}

/* -------------------------------------------------------------------
 *  NetmgtArglist::putType - append argument type to argument list
 *	returns TRUE if successful; otherwise returns FALSE
 * -------------------------------------------------------------------
 */
bool_t
NetmgtArglist::putType (u_int type)
{

  NETMGT_PRN (("putarg: type: %u\n", type));
  (void) memcpy (this->getPtr (), (caddr_t) & type, sizeof (type));
  this->incrPtr (sizeof (type));
  return TRUE;
}

/* -------------------------------------------------------------------
 *  NetmgtArglist::putLength - append argument length to argument list
 *	returns TRUE if successful; otherwise returns FALSE
 * -------------------------------------------------------------------
 */
bool_t
NetmgtArglist::putLength (u_int length)
{

  NETMGT_PRN (("putarg: length: %u\n", length));
  (void) memcpy (this->getPtr (), (caddr_t) & length, sizeof (length));
  this->incrPtr (sizeof (length));
  return TRUE;
}

/* -------------------------------------------------------------------
 *  NetmgtArglist::putRelop - append relational operator to argument list
 *	returns TRUE if successful; otherwise returns FALSE
 * -------------------------------------------------------------------
 */
bool_t
NetmgtArglist::putRelop (u_int relop)
{

  NETMGT_PRN (("putarg: relop: %u\n", relop));
  (void) memcpy (this->getPtr (), (caddr_t) & relop, sizeof (relop));
  this->incrPtr (sizeof (relop));
  return TRUE;
}

/* -------------------------------------------------------------------
 *  NetmgtArglist::putPriority - append event priority to argument list
 *	returns TRUE if successful; otherwise returns FALSE
 * -------------------------------------------------------------------
 */
bool_t
NetmgtArglist::putPriority (u_int priority)
{

  NETMGT_PRN (("putarg: priority: %u\n", priority));
  (void) memcpy (this->getPtr (), (caddr_t) & priority, 
		 sizeof (priority));
  this->incrPtr (sizeof (priority));
  return TRUE;
}

/* -------------------------------------------------------------------
 *  NetmgtArglist::putEndTag - append argument list sentinal
 *	returns TRUE if successfule; otherwise returns FALSE
 * -------------------------------------------------------------------
 */
bool_t
NetmgtArglist::putEndTag (bool_t isTagged)
				// whether arglist is tagged
{
  off_t savedOffset;		// saved arglist offset 

  // save current arglist offset 
  savedOffset = this->getOffset ();

  // append arglist end tag to arglist 
  if (isTagged && !this->putTag (NETMGT_END_TAG))
    return FALSE;

  // mark arglist end with a sentinal string 
  if (!this->putName (NETMGT_ENDOFARGS))
    return FALSE;

  // restore arglist offset 
  this->setOffset (savedOffset);
  return TRUE;
}

/* -------------------------------------------------------------------
 *  NetmgtArglist::putValue - append value to argument list
 *	returns TRUE if successful; otherwise returns FALSE
 * -------------------------------------------------------------------
 */
bool_t
NetmgtArglist::putValue (u_int type, u_int length, caddr_t value)
                		// value type 
                  		// value length 
                   		// value buffer 
{
  struct in_addr inaddr;	// IP address 

  NETMGT_PRN (("putarg: NetmgtArglist::putValue\n"));

  switch (type)
    {
    case NETMGT_SHORT:
      NETMGT_PRN (("putarg: value: %d\n", *(short *) value));
      (void) memcpy (this->getPtr (), value, sizeof (short));
      this->incrPtr (sizeof (short));
      break;

    case NETMGT_U_SHORT:
      NETMGT_PRN (("putarg: value: %d\n", *(u_short *) value));
      (void) memcpy (this->getPtr (), value, sizeof (u_short));
      this->incrPtr (sizeof (u_short));
      break;

    case NETMGT_INT:
      NETMGT_PRN (("putarg: value: %d\n", *(int *) value));
      (void) memcpy (this->getPtr (), value, sizeof (int));
      this->incrPtr (sizeof (int));
      break;

    case NETMGT_U_INT:
    case NETMGT_UNIXTIME:
    case NETMGT_ENUM:
    case NETMGT_INTEGER:
    case NETMGT_COUNTER:
    case NETMGT_GAUGE:
    case NETMGT_TIMETICKS:
      NETMGT_PRN (("putarg: value: %u\n", *(u_int *) value));
      (void) memcpy (this->getPtr (), value, sizeof (u_int));
      this->incrPtr (sizeof (u_int));
      break;

    case NETMGT_LONG:
      NETMGT_PRN (("putarg: value: %d\n", *(long *) value));
      (void) memcpy (this->getPtr (), value, sizeof (long));
      this->incrPtr (sizeof (long));
      break;

    case NETMGT_U_LONG:
      NETMGT_PRN (("putarg: value: %u\n", *(u_long *) value));
      (void) memcpy (this->getPtr (), value, sizeof (u_long));
      this->incrPtr (sizeof (u_long));
      break;

    case NETMGT_FLOAT:
      NETMGT_PRN (("putarg: value: %f\n", *(float *) value));
      (void) memcpy (this->getPtr (), value, sizeof (float));
      this->incrPtr (sizeof (float));
      break;

    case NETMGT_DOUBLE:
      NETMGT_PRN (("putarg: value: %f\n", *(double *) value));
      (void) memcpy (this->getPtr (), value, sizeof (double));
      this->incrPtr (sizeof (double));
      break;

    case NETMGT_STRING:
      NETMGT_PRN (("putarg: value: %s\n", value));
      (void) sprintf (this->getPtr (), "%s", (char *) value);
      this->incrPtr (strlen (this->getPtr ()) + 1);
      break;

    case NETMGT_OCTET:
    case NETMGT_OCTET_STRING:
#ifdef DEBUG
      if (netmgt_debug > 3)
	{
	  (void) (printf, "putarg: value: ");
	  for (int i = 0; i < length; i++)
	    {
	      if ((i % 16) == 0)
		(void) printf ("\n");
	      (void) printf ("%2.2x ", value [i]);
	    }
	  (void) printf ("\n");
	}
#endif DEBUG
      (void) memcpy (this->getPtr (), value, (int) length);
      this->incrPtr (length);
      break;

    case NETMGT_INADDR:
      NETMGT_PRN (("putarg: value: %s\n",
		   inet_ntoa (*(struct in_addr *) value)));
      (void) memcpy (this->getPtr (), value, sizeof (struct in_addr));
      this->incrPtr (sizeof (struct in_addr));
      break;

    case NETMGT_TIMEVAL:
      NETMGT_PRN (("putarg: value: %d.%d\n",
		   ((struct timeval *) value)->tv_sec,
		   ((struct timeval *) value)->tv_usec));
      (void) memcpy (this->getPtr (), value, sizeof (struct timeval));
      this->incrPtr (sizeof (struct timeval));
      break;

    case NETMGT_OBJECT_IDENTIFIER:
      NETMGT_PRN (("putarg: value: `OBJECT_IDENTIFIER'\n"));
      (void) memcpy (this->getPtr (), value, (int) length);
      this->incrPtr (length);
      break;

    case NETMGT_NET_ADDRESS:
    case NETMGT_IP_ADDRESS:
      (void) memcpy ((caddr_t) & inaddr, value, sizeof (struct in_addr));
      inaddr.s_addr = ntohl (inaddr.s_addr);
      NETMGT_PRN (("putarg: value: %s\n", inet_ntoa (inaddr)));
      (void) memcpy (this->getPtr (), value, sizeof (struct in_addr));
      this->incrPtr (sizeof (struct in_addr));
      break;

    case NETMGT_OPAQUE:
      NETMGT_PRN (("putarg: value: `OPAQUE'\n"));
      (void) memcpy (this->getPtr (), value, (int) length);
      this->incrPtr (length);
      break;

    default:
      NETMGT_PRN (("putarg: unknown type: %d\n", type));
      _netmgtStatus.setStatus (NETMGT_UNKNOWNTYPE, 0, NULL);
      return FALSE;
    }
  return TRUE;
}

