
/**************************************************************************
 *  File:	include/libnetmgt/wrappers.h
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
 *  SCCSID:	@(#)wrappers.h  1.29  91/02/04
 *
 *  Comments:	 wrapper declarations
 *
 **************************************************************************
 */

#ifndef	_wrappers_h
#define _wrappers_h

#ifdef __cplusplus
extern "C" {
#endif

// RPC routines
extern void _netmgt_receiveReport (struct svc_req *rqst, 
				   register SVCXPRT *xprt) ;

extern void _netmgt_receiveRequest (struct svc_req *rqst, 
				    register SVCXPRT *xprt) ;

// signal handlers
#ifdef _SVR4_
extern void _netmgt_killUnverified (int sig) ;
#else
extern void _netmgt_killUnverified (int sig,
				    int code, 
				    struct sigcontext *scp, 
				    char *addr) ;
#endif _SVR4_

#ifdef _SVR4_
extern void _netmgt_reapRequest (int sig) ; 
#else
extern void _netmgt_reapRequest (int sig, int code, 
				 struct sigcontext *scp, 
				 char *addr) ;
#endif _SVR4_

#ifdef _SVR4_
extern void _netmgt_sendDeferred (int sig) ; 
#else
extern void _netmgt_sendDeferred (int sig, 
				  int code, 
				  struct sigcontext *scp, 
				  char *addr) ;
#endif _SVR4_

#ifdef _SVR4_
extern void _netmgt_shutdownRendez (int sig) ;
#else
extern void _netmgt_shutdownRendez (int sig, 
				    int code, 
				    struct sigcontext *scp, 
				    char *addr) ;
#endif _SVR4_

// XDR routine
extern int _netmgt_deserialMsg (XDR *xdr, NetmgtServiceMsg *msg) ;

extern int _netmgt_serialMsg (XDR *xdr, NetmgtServiceMsg *msg) ;

extern bool_t _netmgt_xdrAgentId (XDR *xdr, Netmgt_agent_ID *ident) ;

extern bool_t _netmgt_xdrControl (XDR *xdr, NetmgtControlMsg *control) ;

extern bool_t _netmgt_xdrStatus (XDR *xdr, Netmgt_error *error) ;

#ifdef __cplusplus
}
#endif

#endif // !_wrappers_h

