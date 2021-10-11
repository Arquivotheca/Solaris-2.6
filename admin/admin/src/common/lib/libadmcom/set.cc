#if !defined(lint) && !defined(NOID)
  static char sccsid[] = "@(#)set.cc	1.36 5/5/91 SMI";
#endif

/**************************************************************************
 *  File:	lib/libnetmgt/set.cc
 *
 *  Author:	Lynn Monsanto, Sun Microsystems Inc.
 *
 *  SCCSID:     @(#)set.cc  1.36  91/05/05
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
 *  Comments:	set argument values
 *
 **************************************************************************
 */

/*LINTLIBRARY*/

#include "netmgt/netmgt_hdrs.h"
#include "netmgt/netmgt.h"
#include "libnetmgt/libnetmgt.h"

/* ---------------------------------------------------------------------
 *  netmgt_set_argument - append argument to arglist
 *	returns TRUE if successful; otherwise returns FALSE
 * ---------------------------------------------------------------------
 */
bool_t
netmgt_set_argument (Netmgt_arg *option)
                      		// optional argument 
{
  NetmgtGeneric generic ;	// generic argument

  NETMGT_PRN (("value: netmgt_set_argument\n")) ;

  // reset internal error buffer 
  _netmgtStatus.clearStatus ();

  // verify input 
  if (!option)
    {
      NETMGT_PRN (("value: no argument\n"));
      _netmgtStatus.setStatus (NETMGT_NOARGBUF, 0, NULL);
      return FALSE;
    }
  if (!option->name)
    {
      NETMGT_PRN (("value: no argument name\n"));
      _netmgtStatus.setStatus (NETMGT_NOARGNAME, 0, NULL);
      return FALSE;
    }
  if (strlen (option->name) >= NETMGT_NAMESIZ)
    {
      NETMGT_PRN (("value: argument name length >= %d\n", NETMGT_NAMESIZ));
      _netmgtStatus.setStatus (NETMGT_NAME2BIG, 0, NULL);
      return FALSE;
    }
  if (!option->value)
    {
      NETMGT_PRN (("value: no argument value pointer\n"));
      _netmgtStatus.setStatus (NETMGT_NOARGVALUE, 0, NULL);
      return FALSE;
    }

  // append option argument to message argument list
  assert (aNetmgtManager != (NetmgtManager *) NULL);
  assert (aNetmgtManager->myServiceMsg != (NetmgtServiceMsg *) NULL);
  return generic.putOption (option, aNetmgtManager->myServiceMsg);
}

/* ---------------------------------------------------------------------
 *  netmgt_set_threshold - append threshold to message argument list
 *	returns TRUE if successful; otherwise returns FALSE
 * ---------------------------------------------------------------------
 */
bool_t
netmgt_set_threshold (Netmgt_thresh *thresh)
                           	// threshold argument 
{
  NetmgtGeneric generic ;	// generic argument
  char message [32];		// error message 

  NETMGT_PRN (("value: netmgt_set_threshold\n"));

  // reset internal error buffer 
  _netmgtStatus.clearStatus ();

  // verify input 
  if (!thresh->name)
    {
      NETMGT_PRN (("value: no argument name\n"));
      _netmgtStatus.setStatus (NETMGT_NOTHRESHBUF, 0, NULL);
      return FALSE;
    }

  if (strlen (thresh->name) >= NETMGT_NAMESIZ)
    {
      NETMGT_PRN (("value: argument name length >= %d\n", NETMGT_NAMESIZ));
      _netmgtStatus.setStatus (NETMGT_NAME2BIG, 0, NULL);
      return FALSE;
    }

  switch (thresh->type)
    {
    case NETMGT_SHORT:
    case NETMGT_U_SHORT:
    case NETMGT_INT:
    case NETMGT_ENUM:
    case NETMGT_U_INT:
    case NETMGT_LONG:
    case NETMGT_U_LONG:
    case NETMGT_FLOAT:
    case NETMGT_DOUBLE:
    case NETMGT_OCTET:
    case NETMGT_INADDR:
    case NETMGT_TIMEVAL:
    case NETMGT_INTEGER:
    case NETMGT_COUNTER:
    case NETMGT_GAUGE:
    case NETMGT_TIMETICKS:
      switch (thresh->relop)
	{
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
	  break;
	default:
	  NETMGT_PRN (("value: invalid operator: %d\n", thresh->relop));
	  (void) sprintf (message, "%d", thresh->relop);
	  _netmgtStatus.setStatus (NETMGT_BADRELOP, 0, message);
	  return FALSE;
	}
      break;
    case NETMGT_STRING:
      switch (thresh->relop)
	{
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
	  break;
	default:
	  NETMGT_PRN (("value: invalid operator: %d\n", thresh->relop));
	  (void) sprintf (message, "%d", thresh->relop);
	  _netmgtStatus.setStatus (NETMGT_BADRELOP, 0, message);
	  return FALSE;
	}
      break;
    case NETMGT_OCTET_STRING:
    case NETMGT_OBJECT_IDENTIFIER:
    case NETMGT_IP_ADDRESS:
    case NETMGT_OPAQUE:
      switch (thresh->relop)
	{
	case NETMGT_EQ:
	case NETMGT_NE:
	case NETMGT_GT:
	case NETMGT_GE:
	case NETMGT_LT:
	case NETMGT_LE:
	case NETMGT_CHANGED:
	  break;
	default:
	  NETMGT_PRN (("value: invalid operator: %d\n", thresh->relop));
	  (void) sprintf (message, "%d", thresh->relop);
	  _netmgtStatus.setStatus (NETMGT_BADRELOP, 0, message);
	  return FALSE;
	}
      break;
    default:
      NETMGT_PRN (("value: invalid type: %d\n", thresh->type));
      (void) sprintf (message, "%d", thresh->type);
      _netmgtStatus.setStatus (NETMGT_UNKNOWNTYPE, 0, message);
      return FALSE;
    }

  if (!thresh->thresh_val)
    {
      NETMGT_PRN (("value: no threshold value pointer\n"));
      _netmgtStatus.setStatus (NETMGT_NOTHRESH, 0, NULL);
      return FALSE;
    }

  // append threshold argument to message argument list
  assert (aNetmgtManager != (NetmgtManager *) NULL);
  assert (aNetmgtManager->myServiceMsg != (NetmgtServiceMsg *) NULL);
  return generic.putThresh (thresh, aNetmgtManager->myServiceMsg);
}

/* ---------------------------------------------------------------------
 *  netmgt_set_value - append set value argument to arglist
 *	returns TRUE if successful; otherwise returns FALSE
 * ---------------------------------------------------------------------
 */
bool_t
netmgt_set_value (Netmgt_setval *setval)
                      		// set argument data
{
  NetmgtSetarg setarg ;	// set argument

  NETMGT_PRN (("value: netmgt_set_value\n")) ;

  // reset internal error buffer 
  _netmgtStatus.clearStatus ();

  // verify input 
  if (!setval)
    {
      NETMGT_PRN (("value: no argument\n"));
      _netmgtStatus.setStatus (NETMGT_NOARGBUF, 0, NULL);
      return FALSE;
    }
  if (!setval->group)
    {
      NETMGT_PRN (("value: no group name\n"));
      _netmgtStatus.setStatus (NETMGT_NOGROUPNAME, 0, NULL);
      return FALSE;
    }
  if (strlen (setval->group) >= NETMGT_NAMESIZ)
    {
      NETMGT_PRN (("value: group name length >= %d\n", NETMGT_NAMESIZ));
      _netmgtStatus.setStatus (NETMGT_NAME2BIG, 0, NULL);
      return FALSE;
    }

  if (!setval->key)
      setval->key[0] = '\0';

  if (!setval->name)
    {
      NETMGT_PRN (("value: no attribute name\n"));
      _netmgtStatus.setStatus (NETMGT_NOGROUPNAME, 0, NULL);
      return FALSE;
    }
  if (strlen (setval->name) >= NETMGT_NAMESIZ)
    {
      NETMGT_PRN (("value: attribute name length >= %d\n", NETMGT_NAMESIZ));
      _netmgtStatus.setStatus (NETMGT_NAME2BIG, 0, NULL);
      return FALSE;
    }

  if (!setval->value)
    {
      NETMGT_PRN (("value: no argument value pointer\n"));
      _netmgtStatus.setStatus (NETMGT_NOARGVALUE, 0, NULL);
      return FALSE;
    }

  // append set request argument to arglist
  assert (aNetmgtManager != (NetmgtManager *) NULL);
  assert (aNetmgtManager->myServiceMsg != (NetmgtServiceMsg *) NULL);
  return setarg.putSetval (setval, aNetmgtManager->myServiceMsg);
}

