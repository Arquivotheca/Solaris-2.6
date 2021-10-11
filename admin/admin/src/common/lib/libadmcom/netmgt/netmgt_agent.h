
/**************************************************************************
 *  File:	include/netmgt_agent.h
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
 *  SCCSID:	@(#)netmgt_agent.h  1.69  90/12/05
 *
 *  Comments:	SunNet Manager RPC agent definitions
 *
 **************************************************************************
 */

#ifndef _netmgt_agent_h
#define _netmgt_agent_h

/* agent initialization flags */
#define NETMGT_DONT_FORK	(u_int)(1 << 0) /* don't fork subprocess */
#define NETMGT_DONT_EXIT	(u_int)(1 << 1) /* don't exit if no requests */

/* agent identification information */
typedef struct
{
  char name[NETMGT_NAMESIZ] ;	/* agent name */
  u_int serial ;		/* agent serial number */
  char arch[NETMGT_NAMESIZ] ;	/* host architecture */
}      Netmgt_agent_ID ;

#endif !_netmgt_agent_h
