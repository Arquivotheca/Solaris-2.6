#if !defined(lint) && !defined(NOID)
  static char sccsid[] = "@(#)globals.cc	1.35 5/5/91 SMI";
#endif

/**************************************************************************
 *  File:	lib/libnetmgt/globals.cc
 *
 *  Author:	Lynn Monsanto, Sun Microsystems Inc.
 *
 *  SCCSID:     @(#)globals.cc  1.35  91/05/05
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
 *  Comments:	global data definitions
 *
 **************************************************************************
 */

/*LINTLIBRARY*/

#include "netmgt/netmgt_hdrs.h"
#include "netmgt/netmgt.h"
#include "libnetmgt/libnetmgt.h"

// package private objects
NetmgtDispatcher *aNetmgtDispatcher ; // agent dispatcher object pointer
NetmgtManager *aNetmgtManager ;     // manager object pointer
NetmgtPerformer *aNetmgtPerformer ; // agent performer object pointer
NetmgtRendez *aNetmgtRendez ;	    // rendezvous object pointer
NetmgtStatus _netmgtStatus ;	    // management status

// package private global data 
u_int netmgt_debug = 0 ; 	    // debugging level - can be set by adb 
Netmgt_error netmgt_error ;	    // error buffer 

