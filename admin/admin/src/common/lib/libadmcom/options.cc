#if !defined(lint) && !defined(NOID)
  static char sccsid[] = "@(#)options.cc	1.37 5/5/91 SMI";
#endif

/**************************************************************************
 *  File:	lib/libnetmgt/options.cc
 *
 *  Author:	Lynn Monsanto, Sun Microsystems Inc.
 *
 *  SCCSID:     @(#)options.cc  1.37  91/05/05
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
 *  Comments:	append request arguments to argument queues
 *
 **************************************************************************
 */

/*LINTLIBRARY*/

#include "netmgt/netmgt_hdrs.h"
#include "netmgt/netmgt.h"
#include "libnetmgt/libnetmgt.h"

/* --------------------------------------------------------------------------
 *  NetmgtDispatcher::initRequestQueues - initialize request argument queues
 *	returns TRUE if successful; otherwise FALSE
 *	--- also NetmgtRendez member function ---
 * --------------------------------------------------------------------------
 */
bool_t
NetmgtDispatcher::initRequestQueues (void)
{

  NETMGT_PRN (("options: NetmgtDispatcher::initRequestQueues\n")) ;

  // reset the current arglist position 
  this->myServiceMsg->myArglist.resetPtr ();

  // get request information
  NetmgtRequestInfo requestInfo ;   // request information buffer
  this->getRequestInfo (&requestInfo);

  NetmgtArgTag tag ;		// argument tag 
  Netmgt_arg option ;		// request option
  Netmgt_thresh thresh ;	// threshold argument
  NetmgtGeneric aGeneric ;	// generic argument
  NetmgtSetarg aSetarg ;	// set request argument

  if (requestInfo.type == NETMGT_SET_REQUEST)
    {
      while (TRUE)
	{
	  
	  // peek at argument tag 
	  if (!this->myServiceMsg->myArglist.peekTag (&tag))
	    return FALSE;
	  
	  switch (tag)
	    {
	    case NETMGT_OPTION_TAG: 	// request option argument 
	      
	      // get next argument from arglist 
	      if (!aGeneric.getArg (this->myServiceMsg))
		return FALSE; 

	      // get request option from arglist 
	      if (!aGeneric.getOption (&option))
		return FALSE;
	      
	      // append request option to option queue 
	      if (!this->appendOption (&option))
		return FALSE;
	      break;
	      
	    case NETMGT_SETARG_TAG:	// set request argument 
	      
	      // initialize argument from the messge arglist 
	      if (!aSetarg.getArg (this->myServiceMsg))
		return FALSE;
	      
	      // append set value argument to set queue 
	      if (!this->appendSetarg (&aSetarg))
		return FALSE;
	      break;
	      
	    case NETMGT_END_TAG:
	      
	      this->myServiceMsg->myArglist.resetPtr ();
	      return TRUE;
	      
	    default:
	      NETMGT_PRN (("options: unknown request: %d\n", 
			   aGeneric.getTag ()));
	      _netmgtStatus.setStatus (NETMGT_UNKNOWNREQUEST, 0, NULL);
	      return FALSE;
	    }
	}
    }
  else
    {
      
      while (TRUE)
	{
	  
	  // get next argument from arglist 
	  if (!aGeneric.getArg (this->myServiceMsg))
	    return FALSE; 

	  switch (aGeneric.getTag ())
	    {
	    case NETMGT_OPTION_TAG: // request option argument 
	      
	      // get request option from arglist 
	      if (!aGeneric.getOption (&option))
		return FALSE;
	      
	      // append request option to option queue 
	      if (!this->appendOption (&option))
		return FALSE;
	      break;
	      
	    case NETMGT_THRESH_TAG: // threshold argument 
	      
	      // get threshold from arglist 
	      if (!aGeneric.getThresh (&thresh))
		return FALSE;
	      
	      // append threshold to threshold queue 
	      if (!this->appendThresh (&thresh))
		return FALSE;
	      break;
	      
	    case NETMGT_END_TAG:
	      
	      this->myServiceMsg->myArglist.resetPtr ();
	      return TRUE;
	      
	    default:
	      NETMGT_PRN (("options: unknown request: %d\n", 
			   aGeneric.getTag ()));
	      _netmgtStatus.setStatus (NETMGT_UNKNOWNREQUEST, 0, NULL);
	      return FALSE;
	    }
	}
    }
      /*NOTREACHED*/
}

/* ----------------------------------------------------------------------
 *  NetmgtRendez::initRequestQueues - initialize request argument queues
 *	returns TRUE if successful; otherwise FALSE
 *	--- also NetmgtDispatcher member function ---
 * ----------------------------------------------------------------------
 */
bool_t
NetmgtRendez::initRequestQueues (void)
{

  NETMGT_PRN (("options: NetmgtRendez::initRequestQueues\n")) ;

  // reset the current arglist position 
  this->myServiceMsg->myArglist.resetPtr ();

  // get request information
  NetmgtRequestInfo request ;   // request information buffer
  this->getRequestInfo (&request);

  NetmgtGeneric aGeneric;	// generic argument
  Netmgt_arg option ;		// request option

  while (TRUE)
    {

      // get next argument from arglist 
      if (!aGeneric.getArg (this->myServiceMsg))
	return FALSE; 

      switch (aGeneric.getTag ())
	{
	case NETMGT_OPTION_TAG: // request option argument 

	  // get request option from arglist 
	  if (!aGeneric.getOption (&option))
	    return FALSE;

	  // append request option to option queue 
	  if (!this->appendOption (&option))
	    return FALSE;
	  break;

	case NETMGT_END_TAG:

	  this->myServiceMsg->myArglist.resetPtr ();
	  return TRUE;

	default:
	  NETMGT_PRN (("options: unknown request: %d\n", aGeneric.getTag ()));
	  _netmgtStatus.setStatus (NETMGT_UNKNOWNREQUEST, 0, NULL);
	  return FALSE;
	}
    }
  /*NOTREACHED*/
}

/* ------------------------------------------------------------------------
 *  NetmgtEntity::appendOption - append request argument to argument queue
 *	returns TRUE if successful; otherwise FALSE
 * ------------------------------------------------------------------------
 */
bool_t
NetmgtEntity::appendOption (Netmgt_arg *option)
				// option argument pointer 
{

  NETMGT_PRN (("options: NetmgtEntity::appendOption: %s\n", option->name));

  assert (option != (Netmgt_arg *) NULL);

  // allocate an argument queue node 
  Netmgt_arg *arg;		// argument queue node 

  arg = (Netmgt_arg *) calloc (1, sizeof (Netmgt_arg));
  if (!arg)
    {
      if (netmgt_debug)
	perror ("options: calloc");
      _netmgtStatus.setStatus (NETMGT_MALLOC, 0, strerror (errno));
      return FALSE;
    }

  // append queue node to agent argument queue 
  (void) strcpy (arg->name, option->name);
  arg->type = option->type;
  arg->length = option->length;
  arg->value = (caddr_t) calloc (1, arg->length);
  if (!arg->value)
    {
      if (netmgt_debug)
	perror ("options: calloc");
      (void) cfree ((caddr_t) arg);
      arg = (Netmgt_arg *) NULL;
      _netmgtStatus.setStatus (NETMGT_MALLOC, 0, strerror (errno));
      return FALSE;
    }
  (void) memcpy (arg->value, option->value, (int) arg->length);

  if (!this->myOptionsQueue.append ((caddr_t) arg, FALSE))
    {
      (void) cfree (arg->value);
      arg->value = (caddr_t) NULL;
      (void) cfree ((caddr_t) arg);
      arg = (Netmgt_arg *) NULL;
      return FALSE;
    }

  return TRUE;
}

/* ----------------------------------------------------------------------
 *  NetmgtDispatcher::appendSetarg - append set value argument to queue
 *	returns TRUE if successful; otherwise returns FALSE
 * ----------------------------------------------------------------------
 */
bool_t
NetmgtDispatcher::appendSetarg (NetmgtSetarg *aSetarg)
				// set argument pointer
{

  // allocate an argument queue node 
  Netmgt_setval *setval;        // set argument queue node

  setval = (Netmgt_setval *) calloc (1, sizeof (Netmgt_setval));
  if (!setval)
    {
      if (netmgt_debug)
	perror ("options: calloc");
      _netmgtStatus.setStatus (NETMGT_MALLOC, 0, strerror (errno));
      return FALSE;
    }

  // initialize queue node
  if (!aSetarg->initialize (setval))
    {
      (void) cfree ((caddr_t) setval);
      return FALSE;
    }

  // append queue node to agent's queue
  if (!this->mySetargQueue .append ((caddr_t) setval, FALSE))
    {
      (void) cfree ((caddr_t) setval);
      setval = (Netmgt_setval *) NULL;
      return FALSE;
    }

  return TRUE;
}

/* ---------------------------------------------------------------------
 *  NetmgtSetarg::initialize - initialize set argument queue node
 * 	returns TRUE if successful; otherwise returns FALSE
 * ---------------------------------------------------------------------
 */
bool_t
NetmgtSetarg::initialize (Netmgt_setval *setval)
{
  NETMGT_PRN (("options: NetmgtSetarg::initialize: %s\n", setval->name));

  assert (setval != (Netmgt_setval *) NULL);

  (void) strcpy (setval->group, this->group);
  (void) strcpy (setval->key, this->key);
  (void) strcpy (setval->name, this->name);
  setval->type = this->type;
  setval->length = this->length;
  setval->value = (caddr_t) calloc (1, setval->length);
  if (!setval->value)
    {
      if (netmgt_debug)
	perror ("options: calloc");
      _netmgtStatus.setStatus (NETMGT_MALLOC, 0, strerror (errno));
      return FALSE;
    }
  (void) memcpy (setval->value, this->value, (int) setval->length);

  return TRUE;
}

/* ---------------------------------------------------------------------
 *  NetmgtDispatcher::appendThresh - append threshold to threshold queue
 *	returns TRUE if success; otherwise returns FALSE
 * ---------------------------------------------------------------------
 */
bool_t
NetmgtDispatcher::appendThresh (Netmgt_thresh *nextThresh)
				// threshold argument pointer 
{

  NETMGT_PRN (("options: NetmgtDispatcher::appendThresh: %s\n", 
	       nextThresh->name));

  assert (nextThresh != (Netmgt_thresh *) NULL);

  // allocate a threshold queue node 
  Netmgt_thresh *thresh;	// threshold queue node 

  thresh = (Netmgt_thresh *) calloc (1, sizeof (Netmgt_thresh));
  if (!thresh)
    {
      if (netmgt_debug)
	perror ("options: calloc");
      _netmgtStatus.setStatus (NETMGT_MALLOC, 0, strerror (errno));
      return FALSE;
    }

  // append queue node to agent threshold queue 
  (void) strcpy (thresh->name, nextThresh->name);
  thresh->type = nextThresh->type;
  thresh->relop = nextThresh->relop;
  thresh->thresh_len = nextThresh->thresh_len;
  thresh->thresh_val = (caddr_t) calloc (1, thresh->thresh_len);
  if (!thresh->thresh_val)
    {
      if (netmgt_debug)
	perror ("options: calloc");
      (void) cfree ((caddr_t) thresh);
      thresh = (Netmgt_thresh *) NULL;
      _netmgtStatus.setStatus (NETMGT_MALLOC, 0, strerror (errno));
      return FALSE;
    }
  (void) memcpy (thresh->thresh_val, nextThresh->thresh_val, 
		 (int) thresh->thresh_len);
  thresh->prev_val = (caddr_t) calloc (1, thresh->thresh_len);
  if (!thresh->prev_val)
    {
      if (netmgt_debug)
	perror ("options: calloc");
      (void) cfree ((caddr_t) thresh->thresh_val);
      thresh->thresh_val = (caddr_t) NULL;
      (void) cfree ((caddr_t) thresh);
      thresh = (Netmgt_thresh *) NULL;
      _netmgtStatus.setStatus (NETMGT_MALLOC, 0, strerror (errno));
      return FALSE;
    }
  thresh->priority = nextThresh->priority;

  if (!this->myThreshQueue.append ((caddr_t) thresh, FALSE))
    {
      (void) cfree (thresh->thresh_val);
      thresh->thresh_val = (caddr_t) NULL;
      (void) cfree ((caddr_t) thresh);
      thresh = (Netmgt_thresh *) NULL;
      return FALSE;
    }

  return TRUE;
}


