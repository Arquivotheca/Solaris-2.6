
/**************************************************************************
 *  File:	include/netmgt_rpc.h
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
 *  SCCSID:	@(#)netmgt_rpc.h  1.60  90/12/05
 *
 *  Comments:	SunNet Manager agent RPC definitions
 *
 **************************************************************************
 */

#ifndef	_netmgt_rpc_h
#define _netmgt_rpc_h

/* current RPC version number */
#define NETMGT_VERS_1_0		(u_long)10
#define NETMGT_VERS		NETMGT_VERS_1_0

/* ancillary agent RPC program numbers */
#define	NETMGT_EVENT_PROG	(u_long)100101	/* event dispatcher */
#define NETMGT_EVENT_VERS	NETMGT_VERS

#define	NETMGT_LOGGER_PROG 	(u_long)100102	/* report logger */
#define NETMGT_LOGGER_VERS	NETMGT_VERS

#define NETMGT_ACTIVITY_PROG    (u_long)100109	/* activity daemon */
#define NETMGT_ACTIVITY_VERS    NETMGT_VERS

#endif /* !_netmgt_rpc_h */
