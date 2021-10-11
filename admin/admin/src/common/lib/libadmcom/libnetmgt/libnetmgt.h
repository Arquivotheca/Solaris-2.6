
/**************************************************************************
 *  File:	include/libnetmgt/libnetmgt.h
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
 *  SCCSID:     @(#)libnetmgt.h  1.38  91/01/14
 *
 *  Comments:	SunNet Manager package private header files
 *
 **************************************************************************
 */
#ifndef	_libnetmgt_h
#define _libnetmgt_h

#include <thread.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#ifdef __cplusplus
typedef void (*SIG_PFV) (int);
#else
typedef void (*SIG_PFV) ();
#endif

#ifdef __cplusplus
typedef void (*SIG_PFV2) (svc_req*, __svcxprt*);
#else
typedef void (*SIG_PFV2) ();
#endif

#ifdef __cplusplus
extern "C" void cfree (char*) ;
extern "C" int utimes (char*, struct timeval*) ;
extern "C" int strcasecmp (const char *, const char *) ;
#endif

#include "define.h"
#include "activity.h"
#include "security.h"

// root class
#include "object.h"

// container classes
#include "container.h"
#include "buffer.h"
#include "queue.h"

// management services status
#include "status.h"

// request class
#include "request.h"

// client class
#include "client.h"

// entity classes
#include "entity.h"
#include "rendez.h"

// argument list class
#include "arglist.h"

// argument classes
#include "argument.h"
#include "generic.h"
#include "setarg.h"

// message classes
#include "header.h"
#include "message.h"
#include "service.h"
#include "control.h"

// manager class (subclass of entity class)
#include "manager.h"

// agent class (subclass of entity class)
#include "agent.h"
#include "performer.h"
#include "dispatcher.h"

#include "extern.h"
#include "wrappers.h"


#endif  _libnetmgt_h











