
/**************************************************************************
 *  File:	include/netmgt_hdrs.h
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
 *  SCCSID:	@(#)netmgt_hdrs.h  1.54  91/05/16
 *
 *  Comments:	SunNet Manager common include files
 *
 **************************************************************************
 */

#ifndef	_netmgt_hdrs_h
#define _netmgt_hdrs_h

#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <rpc/rpc.h>
#include <nfs/nfs.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <signal.h>
#include <net/if.h>
#include <netdb.h>
#include <sys/ioctl.h>
#ifdef _SVR4_
#include <string.h>
#include <sys/fcntl.h>
#include <netinet/in.h>
#include <rpc/rpcent.h>
#include <locale.h>
#else
#include <strings.h>
#endif _SVR4_

#include <memory.h>
#include <syslog.h>

/* Header files that only exist in C++ */

#ifndef _SVR4_
#if defined (__cplusplus)
#include <bstring.h>
#endif
#endif
#endif /* !_netmgt_hdrs_h */
