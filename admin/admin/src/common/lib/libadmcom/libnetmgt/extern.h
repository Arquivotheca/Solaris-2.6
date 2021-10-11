
/**************************************************************************
 *  File:	include/libnetmgt/extern.h
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
 *  SCCSID:	@(#)extern.h  1.43  91/08/01
 *
 *  Comments:	package private external declarations
 *
 **************************************************************************
 */

#ifndef	_extern_h
#define _extern_h

/* useful external data */
extern int errno ;
extern char *sys_errlist [] ;

#if defined (__cplusplus)

// package private objects
extern NetmgtDispatcher *aNetmgtDispatcher; // an agent dispatcher pointer
extern NetmgtManager *aNetmgtManager ;      // a manager pointer
extern NetmgtPerformer *aNetmgtPerformer;   // an agent performer pointer
extern NetmgtRendez *aNetmgtRendez ;	    // a rendezvous pointer
extern NetmgtStatus _netmgtStatus ;	    // management status

// package private global data 
extern u_int netmgt_debug ;               // debugging level - can be set by adb 
extern Netmgt_error netmgt_error ;	  // error buffer 

// package private functions with C linkage 
extern "C" {

#ifdef _SVR4_
int _netmgt_get_auth_flavor (uid_t *uid, 
			     gid_t *gid, 
			     int *numgids, 
			     gid_t gidlist [], 
			     u_int maxnetnamelen,
			     char netname []) ;
#else
int _netmgt_get_auth_flavor (int *uid, 
			     int *gid, 
			     int *numgids, 
			     int gidlist [], 
			     u_int maxnetnamelen,
			     char netname []) ;
#endif _SVR4_

struct timeval * _netmgt_call_agent (u_long agent_prog, 
				     struct in_addr agent_addr, 
				     u_long agent_vers, 
				     struct timeval timeout) ;
bool_t _netmgt_check_activity (struct timeval request_time,
			       struct in_addr agent_addr,
			       u_long agent_prog,
			       u_long agent_vers,
			       struct timeval timeout) ;
#ifdef _SVR4_
pid_t _netmgt_get_child_pid (u_int sequence) ;
#else
int _netmgt_get_child_pid (u_int sequence) ;
#endif _SVR4_

u_int _netmgt_get_child_sequence (void) ;
char *_netmgt_get_config (char *name) ;
u_int _netmgt_get_priority (void) ;
u_int _netmgt_get_report_handle (void) ;
u_int _netmgt_get_request_flag (void) ;
u_int _netmgt_get_request_handle (void) ;
u_int _netmgt_get_request_sequence (void) ;

#ifdef _SVR4_
pid_t _netmgt_get_child_pid (u_int handle) ;
#else
int _netmgt_get_child_pid (u_int handle) ;
#endif _SVR4_

struct timeval *_netmgt_get_request_time (void) ;
bool_t _netmgt_init_rpc_manager (void) ;
bool_t _netmgt_init_rpc_rendez (char *name, 
				u_int serial, 
				u_long program, 
				u_long version, 
				u_long proto, 
				struct timeval timeout, 
				u_int flags, 
				void (*dispatch) (u_int type, char *system, 
						  char *group, char *key, 
						  u_int count, 
						  struct timeval interval, 
						  u_int flags), 
				void (*shutdown) (int sig, int code, 
						  struct sigcontext *scp, 
						  char *addr)) ;
bool_t _netmgt_lock_file (int fd) ;
struct timeval *_netmgt_request_action (char *agent_host,
					u_long agent_prog,
					u_long agent_vers,	
					char *rendez_host,
					u_long rendez_prog,
					u_long rendez_vers,	
					struct timeval timeout,
					u_int flags,
					struct in_addr *) ;
	
bool_t _netmgt_set_auth_flavor (int flavor) ;
bool_t _netmgt_set_request_flag (u_int handle) ;
bool_t _netmgt_set_request_handle (u_int handle) ;
void _netmgt_start_rendez (void) ;
bool_t _netmgt_unlock_file (int fd) ;

}

// missing prototypes 
extern "C" {
extern char *dgettext (const char *domain, const char *message) ;
extern void get_myaddress (struct sockaddr_in *addr) ;
extern u_long inet_addr (char *addr) ;
extern char *inet_ntoa (struct in_addr) ;
extern int pmap_set (u_long prognum, u_long versnum, u_long protocol, int port) ;
extern int pmap_unset (u_long prognum, u_long versnum) ;
extern char *strerror (int error) ;
extern int yp_bind (char *domainname) ;

}

#else	/* Sun C */

extern bool_t _netmgt_check_activity () ;
extern int _netmgt_get_auth_flavor () ;

#ifdef _SVR4_
pid_t _netmgt_get_child_pid () ;
#else
int _netmgt_get_child_pid () ;
#endif _SVR4_

extern u_int _netmgt_get_child_sequence () ;
extern char *_netmgt_get_config();
extern u_int _netmgt_get_priority () ;
extern u_int _netmgt_get_report_handle () ;
extern u_int _netmgt_get_request_flag () ;
extern u_int _netmgt_get_request_handle () ;
extern u_int _netmgt_get_request_sequence () ;
extern bool_t _netmgt_lock_file () ;
extern struct timeval *_netmgt_request_action () ;
extern bool_t _netmgt_set_auth_flavor () ;
extern bool_t _netmgt_set_request_flag () ;
extern bool_t _netmgt_set_request_handle () ;
extern void _netmgt_start_rendez () ;
extern bool_t _netmgt_unlock_file () ;

#endif 	/* defined __cplusplus */

#endif _extern_h 

