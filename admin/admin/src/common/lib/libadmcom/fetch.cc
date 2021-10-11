#if !defined(lint) && !defined(NOID)
  static char sccsid[] = "@(#)fetch.cc	1.38 5/5/91 SMI";
#endif

/**************************************************************************
 *  File:	lib/libnetmgt/fetch.cc
 *
 *  Author:	Lynn Monsanto, Sun Microsystems Inc.
 *
 *  SCCSID:     @(#)fetch.cc  1.38  91/05/05
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
 *  Comments:	fetch argument values
 *
 **************************************************************************
 */

/*LINTLIBRARY*/

#include "netmgt/netmgt_hdrs.h"
#include "netmgt/netmgt.h"
#include "libnetmgt/libnetmgt.h"

/* ---------------------------------------------------------------------
 *  netmgt_fetch_argument - C wrapper to fetch request argument
 *	return TRUE if successful; otherwise returns FALSE
 * ---------------------------------------------------------------------
 */
bool_t
netmgt_fetch_argument (char *name, Netmgt_arg *option)
                		// attribute name 
                        	// argument buffer 
{
  NETMGT_PRN (("value: netmgt_fetch_argument\n"));

  return aNetmgtDispatcher->fetchOption (name, option);
}
  
/* ---------------------------------------------------------------------
 *  NetmgtEntity::fetchOption - fetch request option argument
 *	return TRUE if successful; otherwise returns FALSE
 * ---------------------------------------------------------------------
 */
bool_t
NetmgtEntity::fetchOption (char *name, Netmgt_arg *option)
                		// attribute name 
                        	// argument buffer 
{

  NetmgtQueueNode *currNode;	// current argument queue node 
  static Netmgt_arg *currArg;	// argument queue data 
  bool_t found;			// whether found threshold 
  static char nullString[] = ""; // null string 

  NETMGT_PRN (("value: NetmgtEntity::fetchOption\n"));

  // verify input 
  if (!option)
    {
      NETMGT_PRN (("value: no argument value buffer\n"));
      _netmgtStatus.setStatus (NETMGT_NOARGVALUE, 0, NULL);
      return FALSE;
    }

  // if an option name is not specified, fetch the next option
  if (!name)
    {
      // get next node from options queue
      currNode = this->myOptionsQueue.getNext ();
      if (!currNode)
	{
	  (void) strncpy (option->name, NETMGT_ENDOFARGS, NETMGT_NAMESIZ);
	  option->length = (u_int) 0;
	  option->type = (u_int) 0;
	  option->value = (caddr_t) NULL;
	  
	  // reset queue
	  this->myOptionsQueue.reset ();
	  return TRUE;
	}
      
      // get request option argument from queue node data
      if (!currNode->isData ())
	return FALSE ;
      currArg = (Netmgt_arg *) currNode->getData ();
      
      // copy argument to user's buffer 
      (void) strncpy (option->name, currArg->name, NETMGT_NAMESIZ);
      option->length = currArg->length;
      option->type = currArg->type;
      option->value = currArg->value;
      return TRUE;
    }

  // else, an option name was given so fetch the specified option
  else
    {
      found = FALSE;
      currNode = this->myOptionsQueue.getHead ();
      while (currNode)
	{
	  
	  // get argument data 
	  assert (currNode->isData ());
	  currArg = (Netmgt_arg *) currNode->getData ();
	  
	  if (strcmp (name, currArg->name) == 0)
	    {
	      found = TRUE;
	      break;
	    }
	  currNode = currNode->getNext ();
	}
      
      if (!found)
	{
	  NETMGT_PRN (("value: can't find request argument: %s\n", name));
	  _netmgtStatus.setStatus (NETMGT_NOARGNAME, 0, NULL);
	  return FALSE;
	}
      
      // copy optional argument information
      (void) strncpy (option->name, currArg->name, NETMGT_NAMESIZ);
      option->type = currArg->type;
      option->length = currArg->length;
      option->value = currArg->value;
      
      // make sure zero length strings contain null 
      if (option->type == NETMGT_STRING && option->length == (u_int) 0)
	option->value = nullString;

      return TRUE;
    }
}

/* -----------------------------------------------------------------------
 *  netmgt_fetch_data - C wrapper to fetch performance value from arglist
 *      returns TRUE if successful; otherwise returns FALSE
 * -----------------------------------------------------------------------
 */
bool_t
netmgt_fetch_data (Netmgt_data *data)
                       		// data report argument 
{

  NETMGT_PRN (("value: netmgt_fetch_data\n"));

  NetmgtGeneric generic;	// generic argument 
  assert (aNetmgtRendez != (NetmgtRendez *) NULL);
  return generic.getData (data, aNetmgtRendez->myServiceMsg);
}

/* -------------------------------------------------------------------------
 *  netmgt_fetch_event - C wrapper to fetch event report value from arglist
 *      returns TRUE if successful; otherwise returns FALSE
 * -------------------------------------------------------------------------
 */
bool_t
netmgt_fetch_event (Netmgt_event *event)
                         	// event report argument
{
  NETMGT_PRN (("value: netmgt_fetch_event\n"));

  NetmgtGeneric aGeneric;	// generic argument 
  assert (aNetmgtRendez != (NetmgtRendez *) NULL);
  return aGeneric.getEvent (event, aNetmgtRendez->myServiceMsg);
}


/* ---------------------------------------------------------------------
 *  netmgt_fetch_setval - fetch set request argument from request queue
 *      returns TRUE if successful; otherwise returns FALSE
 * ---------------------------------------------------------------------
 */
bool_t
netmgt_fetch_setval (Netmgt_setval *setval)
                         	// set request data
{

  NETMGT_PRN (("value: netmgt_fetch_setval\n"));

  NetmgtSetarg setarg ;	       // set request argument
  assert (aNetmgtDispatcher != (NetmgtDispatcher *) NULL);
  return aNetmgtDispatcher->fetchSetval (setval);
  
}

/* ------------------------------------------------------------------------
 *  NetmgtAgent::fetchSetval - fetch set argument data from request queue
 *      returns TRUE if successful; otherwise returns FALSE
 * ------------------------------------------------------------------------
 */
bool_t
NetmgtAgent::fetchSetval (Netmgt_setval *setval)
				// set request argument
{

  NETMGT_PRN (("value: NetmgtAgent::fetchSetval\n"));

  // reset internal error buffer 
  _netmgtStatus.clearStatus ();

  if (!setval)
    {
      NETMGT_PRN (("value: no setval structure\n"));
      _netmgtStatus.setStatus (NETMGT_NOEVENTBUF, 0, NULL);
      return FALSE;
    }

  // get next node from setarg queue
  NetmgtQueueNode *node ;	 // set argument queue node

  node = this->mySetargQueue.getNext ();
  if (!node)
    {
      (void) strncpy (setval->group, NETMGT_ENDOFARGS, NETMGT_NAMESIZ);
      (void) strncpy (setval->key, NETMGT_ENDOFARGS, NETMGT_NAMESIZ);
      (void) strncpy (setval->name, NETMGT_ENDOFARGS, NETMGT_NAMESIZ);
      setval->length = (u_int) 0;
      setval->type = (u_int) 0;
      setval->value = (caddr_t) NULL;
      
      // reset queue
      this->mySetargQueue.reset ();
      return TRUE;
    }

  // get set request argument from queue node data
  Netmgt_setval *nextSetval ;	 // next set request argument

  if (!node->isData ())
    return FALSE ;
  nextSetval = (Netmgt_setval *) node->getData ();

  // copy argument to user's buffer 
  (void) strncpy (setval->group, nextSetval->group, NETMGT_NAMESIZ);
  (void) strncpy (setval->key, nextSetval->key, NETMGT_NAMESIZ);
  (void) strncpy (setval->name, nextSetval->name, NETMGT_NAMESIZ);
  setval->length = nextSetval->length;
  setval->type = nextSetval->type;
  setval->value = nextSetval->value;
  return TRUE;
}

