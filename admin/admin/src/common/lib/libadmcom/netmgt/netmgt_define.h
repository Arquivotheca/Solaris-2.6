
/**************************************************************************
 *  File:	include/netmgt_define.h
 *
 *  Author:	Lynn Monsanto, Sun Microsystems Inc.
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
 *  SCCSID:	@(#)netmgt_define.h  1.71  91/02/11
 *
 *  Comments:	SunNet Manager definitions
 *
 **************************************************************************
 */

#ifndef	_netmgt_define_h
#define _netmgt_define_h

/* agent debugging macros */
#define NETMGT_DBG              if (netmgt_debug > 0) (void) printf
#define NETMGT_DBG1             if (netmgt_debug > 0) (void) printf
#define NETMGT_DBG2             if (netmgt_debug > 1) (void) printf
#define NETMGT_DBG3             if (netmgt_debug > 2) (void) printf

/* miscellaneous management definitions */
#define NETMGT_NAMESIZ		1024	/* name buffer size */
#define NETMGT_ERRORSIZ		4096	/* error message buffer size */
#define NETMGT_TIMEOUT	        30L	/* default RPC timeout (seconds) */

/* rendezvous request definitions */
#define NETMGT_ANY_AGENT	(u_long)0
#define NETMGT_ANY_EVENT	(u_long)0

/* option argument name used by "snm" and "snm_cmd" */
#define NETMGT_OPTSTRING	"netmgt_optstring"

#endif /* !_netmgt_define_h */
