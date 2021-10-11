
/**************************************************************************
 *  File:	include/netmgt_extern.h
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
 *  SCCSID:     @(#)netmgt_extern.h  1.119  91/02/11
 *
 *  Comments:	SunNet Manager external definitions
 *
 **************************************************************************
 */

#ifndef	_netmgt_extern_h
#define _netmgt_extern_h

/* Package public global data */
extern u_int netmgt_debug ;		/* debugging level */
extern Netmgt_error netmgt_error ;	/* error buffer */
extern Netmgt_agent_ID netmgt_agent_ID ; /* agent identification buffer */

/* package public functions with C linkage */
#if defined (__cplusplus)
extern "C" {
#endif

/* Package public manager and agent services */

extern bool_t netmgt_fetch_msginfo (Netmgt_msginfo *msginfo) ;

extern char *netmgt_oid2string (u_int length, caddr_t value) ;

extern char *netmgt_oid2decodedstring (u_int length, caddr_t value, caddr_t oid_map) ;

extern caddr_t netmgt_get_oid_map (void) ;

extern void netmgt_set_debug (u_int level) ;

extern char *netmgt_sperror (void) ;

extern u_long *netmgt_string2oid (char *string) ;

/* Package public manager services */
extern struct timeval *netmgt_request_events (char *agent_host, 
				       u_long agent_prog, 
				       u_long agent_vers, 
				       char *rendez_host, 
				       u_long rendez_prog, 
				       u_long rendez_vers, 
				       u_int count, 
				       struct timeval interval, 
				       struct timeval timeout, 
				       u_int flags) ;

extern Netmgt_agent_ID *netmgt_request_agent_ID (char *agent_host, 
					  u_long agent_prog, 
					  u_long agent_vers, 
					  struct timeval timeout) ;

extern bool_t netmgt_fetch_data (Netmgt_data *data) ;

extern bool_t netmgt_fetch_error (Netmgt_error *error) ;

extern bool_t netmgt_fetch_event (Netmgt_event *event) ;

extern bool_t netmgt_kill_request (char *agent_name, 
			    u_long agent_prog, 
			    u_long agent_vers, 
			    char *manager_name, 
			    struct timeval request_time, 
			    struct timeval timeout) ;

extern bool_t netmgt_register_rendez (char *dispatch_host, 
			       char *rendez_host, 
			       u_long rendez_prog, 
			       u_long rendez_vers, 
			       u_long agent_prog, 
			       u_int event_priority, 
			       struct timeval timeout) ;

extern u_long netmgt_register_callback (void (*) (u_int type, char *system, 
					   char *group, char *key, 
					   u_int count, 
					   struct timeval interval, 
					   u_int flags), 
				 int *udpSockp, 
				 int *tcpSockp, 
				 u_long vers, 
				 u_long proto) ;

extern struct timeval *netmgt_request_data (char *agent_host, 
				     u_long agent_prog, 
				     u_long agent_vers, 
				     char *rendez_host, 
				     u_long rendez_prog, 
				     u_long rendez_vers, 
				     u_int count, 
				     struct timeval interval, 
				     struct timeval timeout, 
				     u_int flags) ;

extern bool_t netmgt_request_deferred (char *agent_name, 
				u_long agent_prog, 
				u_long agent_vers, 
				char *manager_name, 
				struct timeval request_time, 
				struct timeval timeout) ;

extern bool_t netmgt_request_set (char *agent_host, 
			   u_long agent_prog, 
			   u_long agent_vers, 
			   char *rendez_host,
			   u_long rendez_prog,
			   u_long rendez_vers,
			   struct timeval timeout, 
			   u_int flags) ;


extern bool_t netmgt_set_argument (Netmgt_arg *arg) ;

extern bool_t netmgt_set_instance (char *system, char *group, char *key) ;

extern bool_t netmgt_set_threshold (Netmgt_thresh *thresh) ;

extern bool_t netmgt_set_value (Netmgt_setval *setval) ;

extern bool_t netmgt_unregister_callback (u_long prog, u_long vers) ;

extern bool_t netmgt_unregister_rendez (char *dispatch_host, 
				 char *rendez_host, 
				 u_long rendez_prog, 
				 u_long rendez_vers, 
				 u_long agent_prog, 
				 u_int event_priority, 
				 struct timeval timeout) ;

/* Package public agent services */
extern bool_t netmgt_build_report (Netmgt_data *data, bool_t *event) ;

extern bool_t netmgt_fetch_argument (char *name, Netmgt_arg *arg) ;

extern bool_t netmgt_fetch_setval (Netmgt_setval *setval) ;

extern bool_t netmgt_init_rpc_agent (char *name, 
			      u_int serial, 
			      u_long program, 
			      u_long version, 
			      u_long proto, 
			      struct timeval timeout, 
			      u_int reserved, 
			      u_int flags, 
			      boolean_t (*verify) (u_int type, char *system, 
						char *group, char *key, 
						u_int count, 
						struct timeval interval, 
						u_int flags), 
			      void (*dispatch) (u_int type, char *system, 
						char *group, char *key, 
						u_int count, 
						struct timeval interval, 
						u_int flags), 
#ifdef _SVR4_
			      void (*reap_child) (int sig, pid_t child_pid, 
						  int *stat_loc, 
						  caddr_t usagep), 
#else
		 	      void (*reap_child) (int sig, int code, 
						  struct sigcontext *scp, 
						  char *addr, int child_pid, 
						  union wait *waitp, 
						  struct rusage *usagep), 
#endif _SVR4_
#ifdef _SVR4_
			      void (*shutdown) (int sig)) ;
#else 
			      void (*shutdown) (int sig, int code, 
						struct sigcontext *scp, 
						char *addr)) ;
#endif _SVR4_

extern bool_t netmgt_mark_end_of_row (void) ;

extern bool_t netmgt_send_error (Netmgt_error *error) ;

extern bool_t netmgt_send_report (struct timeval delta_time, 
			   Netmgt_stat status, 
			   u_int flags) ;

#ifdef _SVR4_
	extern void netmgt_shutdown_agent (int sig) ;
#else
	extern void netmgt_shutdown_agent (int sig, 
			    int code, 
			    struct sigcontext *scp,
			    char *addr) ;
#endif _SVR4_

extern void netmgt_start_agent (void) ;

extern bool_t netmgt_start_trap (char *system, 
			  u_int agent_prog,
			  u_int agent_vers,
			  char *group,
			  char *host, 
			  u_int prog, 
			  u_int vers, 
			  u_int priority, 
			  struct timeval timeout) ; 

#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */

#endif /* !_netmgt_extern_h */
