#if !defined(lint) && !defined(NOID)
  static char sccsid[] = "@(#)signal.cc	1.37 5/5/91 SMI";
#endif

/**************************************************************************
 *  File:	lib/libnetmgt/signal.c
 *
 *  Author:	Lynn Monsanto, Sun Microsystems Inc.
 *
 *  SCCSID:     @(#)signal.cc  1.37  91/05/05
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
 *  Comments:	signal handling routines
 *
 **************************************************************************
 */

/*LINTLIBRARY*/

#include "netmgt/netmgt_hdrs.h"
#include "netmgt/netmgt.h"
#include "libnetmgt/libnetmgt.h"
#include <signal.h>

/* -------------------------------------------------------------------------
 *  NetmgtObject::blockSignals - block signals at beginning of 
 *      critical section
 *	ASSUMES NO NESTED SIGNAL BLOCKING
 * -------------------------------------------------------------------------
 */
sigset_t
NetmgtObject::blockSignals (void)
{
  sigset_t newmask ;	        /* new signal mask */
  static sigset_t oldmask ;	/* previous signal mask */

  NETMGT_PRN (("NetmgtObject::blockSignals\n"));

  // clear signal mask
  (void) sigemptyset (& newmask);

  /* set signal blocking mask */
  (void) sigaddset (& newmask, SIGINT);
  (void) sigaddset (& newmask, SIGTERM);
  (void) sigaddset (& newmask, SIGCHLD);
  (void) sigaddset (& newmask, SIGUSR1);
  (void) sigaddset (& newmask, SIGUSR2);

  /* block signals */
  (void) sigprocmask (SIG_SETMASK, & newmask, & oldmask);

  return (oldmask) ;
}

/* -------------------------------------------------------------------------
 *  NetmgtObject::unblockSignals - unblock signals at end of 
 *      critical section
 *	ASSUMES NO NESTED SIGNAL BLOCKING
 * -------------------------------------------------------------------------
 */
void
NetmgtObject::unblockSignals (sigset_t oldmask)
                  		/* previous signal mask */
{
  NETMGT_PRN (("NetmgtObject::unblockSignals\n"));

  /* unblock signals */
  (void) sigprocmask (SIG_SETMASK, & oldmask, (sigset_t *)NULL) ;

  return ;
}
