#if !defined(lint) && !defined(NOID)
  static char sccsid[] = "@(#)instance.cc	1.26 5/5/91 SMI";
#endif

/**************************************************************************
 *  File:	lib/libnetmgt/instance.cc
 *
 *  Author:	Lynn Monsanto, Sun Microsystems Inc.
 *
 *  SCCSID:     @(#)instance.cc  1.26  91/05/05
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
 *  Comments:	set managed object instance
 *
 **************************************************************************
 */

/*LINTLIBRARY*/

#include "netmgt/netmgt_hdrs.h"
#include "netmgt/netmgt.h"
#include "libnetmgt/libnetmgt.h"

/* ------------------------------------------------------------------
 *  netmgt_set_instance - wrapper to set managed object instance
 *	returns TRUE if successful; otherwise returns FALSE
 * ------------------------------------------------------------------
 */
bool_t
netmgt_set_instance (char *system, char *group, char *key)
                   		// target system name 
                  		// group or table name 
                		// optional table key 
{

  NETMGT_PRN (("instance: netmgt_set_instance\n")) ;

  // create and inititialize a manager object if necessary 
  if (!aNetmgtManager)
    {
      if (!_netmgt_init_rpc_manager ())
	return FALSE;
    }

  // set managed object instance information
  if (!aNetmgtManager->setObjectInstance (system, group, key))
    {
      (void) cfree ((caddr_t) aNetmgtManager);
      return FALSE ;
    }
  return TRUE;
}

/* ------------------------------------------------------------------
 *  NetmgtManager::setObjectInstance - set managed object instance
 *	returns TRUE if successful; otherwise returns FALSE
 * ------------------------------------------------------------------
 */
bool_t
NetmgtManager::setObjectInstance (char *system, char *group, char *key)
                   		// target system name 
                  		// group or table name 
                		// optional table key 
{
  static char empty [] = "" ;	// empty string

  NETMGT_PRN (("instance: NetmgtManager::setObjectInstance\n")) ;

  // reset internal error buffer 
  _netmgtStatus.clearStatus ();

  // verify and copy system name manager's request buffer
  if (!system)
    this->myRequest->setSystem ((char *) empty);

  else if (strlen (system) >= NETMGT_NAMESIZ)
    {
      NETMGT_PRN (("instance: system name length > %d\n",
		   NETMGT_NAMESIZ - 1));
      _netmgtStatus.setStatus (NETMGT_NAME2BIG, 0, NULL);
      return FALSE;
    }
  else
    this->myRequest->setSystem (system);

  // verify and copy group name to manager's request buffer
  if (!group)
    this->myRequest->setGroup ((char *) empty);

  else if (strlen (group) >= NETMGT_NAMESIZ)
    {
      NETMGT_PRN (("instance: group name length > %d\n",
		   NETMGT_NAMESIZ - 1));
      _netmgtStatus.setStatus (NETMGT_NAME2BIG, 0, NULL);
      return FALSE;
    }
  else
    this->myRequest->setGroup (group);

  // verify and copy table key to manager's request buffer
  if (!key)
    this->myRequest->setKey ((char *) empty);

  else if (strlen (key) >= NETMGT_NAMESIZ)
    {
      NETMGT_PRN (("instance: group key length > %d\n",
		   NETMGT_NAMESIZ - 1));
      _netmgtStatus.setStatus (NETMGT_NAME2BIG, 0, NULL);
      return FALSE;
    }
  else
    this->myRequest->setKey (key);

  // set request time
  if (!this->myRequest->setRequestTime ())
    return FALSE;

  // reset arglist pointers
  this->myServiceMsg->myArglist.resetPtr ();
  this->myServiceMsg->myArglist.myValue1.resetPtr ();
  this->myServiceMsg->myArglist.myValue2.resetPtr ();

  return TRUE;
}


/* ------------------------------------------------------------------
 *  NetmgtRequest::setRequestTime - set request timestamp
 *	returns TRUE if successful; otherwise FALSE
 * ------------------------------------------------------------------
 */
bool_t
NetmgtRequest::setRequestTime (void)
{

  static struct timeval timestamp ; 
  if (gettimeofday (&this->info.request_time, (struct timezone *)NULL ) == -1)
    {
      if (netmgt_debug)
	perror ("instance: gettimeofday");
      _netmgtStatus.setStatus (NETMGT_GETTIMEOFDAY, 0, strerror (errno));
      return FALSE;
    }
  return TRUE;
}

