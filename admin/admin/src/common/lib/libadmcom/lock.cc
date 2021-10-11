#if !defined(lint) && !defined(NOID)
  static char sccsid[] = "@(#)lock.cc	1.37 5/5/91 SMI";
#endif

/**************************************************************************
 *  File:	lib/libnetmgt/lock.c
 *
 *  Author:	Lynn Monsanto, Sun Microsystems Inc.
 *
 *  SCCSID:     @(#)lock.cc  1.37  91/05/05
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
 *  Comments:	file locking routines
 *
 **************************************************************************
 */

/*LINTLIBRARY*/

#include "netmgt/netmgt_hdrs.h"
#include "netmgt/netmgt.h"
#include "libnetmgt/libnetmgt.h"

#ifndef SEEK_SET
#define SEEK_SET 0
#endif SEEK_SET

/* --------------------------------------------------------------------
 *  NetmgtObject::lockFile - lock file for exclusive access
 *	returns TRUE if successful; otherwise returns FALSE
 * --------------------------------------------------------------------
 */
bool_t
NetmgtObject::lockFile (int fd)
             			/* file descriptor to lock */
{

  NETMGT_PRN (("lock: NetmgtObject::lockFile\n")) ;

  struct flock flock;
  flock.l_type = F_WRLCK;
  flock.l_whence = SEEK_SET;
  flock.l_start = 0;
  flock.l_len = 0;
  
  if (fcntl (fd, F_SETLKW, (int)&flock) == -1)
    {
      if (netmgt_debug)
	perror ("lock: fcntl, F_SETLW");
      return FALSE;
    }
  return TRUE;
}

/* --------------------------------------------------------------------
 *  NetmgtObject::unlockFile - unlock file
 *	returns TRUE if successful; otherwise returns FALSE
 * --------------------------------------------------------------------
 */
bool_t
NetmgtObject::unlockFile (int fd)
            			/* file descriptor to unlock */
{

  NETMGT_PRN (("lock: NetmgtObject::unlockFile\n"));

  struct flock flock;
  flock.l_type = F_UNLCK;
  flock.l_whence = SEEK_SET;
  flock.l_start = 0;
  flock.l_len = 0;
  
  if (fcntl (fd, F_SETLKW, (int)&flock) == -1)
    {
      if (netmgt_debug)
	perror ("lock: fcntl, F_UNLCK");
      return FALSE;
    }
  return TRUE;
}
