#ifndef _SYSIDTOOL_TEST_H
#define	_SYSIDTOOL_TEST_H

/*
 * Copyright (c) 1991,1992,1993 Sun Microsystems, Inc.  All Rights Reserved.
 * Sun considers its source code as an unpublished, proprietary trade secret,
 * and it is available only under strict license provisions.  This copyright
 * notice is placed here only to protect Sun in the event the source is
 * deemed a published work.  Dissassembly, decompilation, or other means of
 * reducing the object code to human readable form is prohibited by the
 * license agreement under which this code is provided to the user or company
 * in possession of this copy.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the Government
 * is subject to restrictions as set forth in subparagraph (c)(1)(ii) of the
 * Rights in Technical Data and Computer Software clause at DFARS 52.227-7013
 * and in similar clauses in the FAR and NASA FAR Supplement.
 */

#pragma	ident	"@(#)sysidtool_test.h	1.5 94/12/06 SMI"

#ifdef  __cplusplus
extern "C" {
#endif

/* Enums for test */
typedef enum simulator_type {
	/* char *errmess */
	SIM_GET_NODENAME_MTHD,

	/* char *if_name, char *eaddr, char *errmess */
	SIM_GET_NET_IF_ETHER_MTHD,

	/* char *if_name, char *ipaddr, char *errmess */
	SIM_GET_NET_IF_IP_ADDR_MTHD,

	/* char *hostname, char *when, char *errmessp */
	SIM_SET_NODENAME_MTHD,

	/* char *hostname, char *errmessp */
	SIM_SET_LB_NTOA_MTHD,

	/* char *ifname, char *netmask, char *ermessp */
	SIM_SET_NET_IF_IP_NETMASK_MTHD,

	/* char *timehost, char *errmess */
	SIM_SYNC_DATE_MTHD,

	/* char *date, char *errmess */
	SIM_SET_DATE_MTHD,

	/* char *timezone, char *errmess */
	SIM_SET_TIMEZONE_MTHD,

	/* char *if_name, char *ifup, char *y_or_n, char *errmess */
	SIM_SET_NET_IF_STATUS_MTHD,

	/* char *hostname, int hlen, char *errmess */
	SIM_SI_HOSTNAME,

	/* char *domainname, int dlen, char *errmess */
	SIM_SI_SRPC_DOMAIN,

	/* char *domainname, char *when, char *errmess */
	SIM_SET_DOMAIN_MTHD,

	/* char *domain, char *errmess */
	SIM_YP_INIT_ALIASES_MTHD,

	/* char *domain, int bcast, char *servers, char *errmess */
	SIM_YP_INIT_BINDING_MTHD,

	/* char *bcast, char *server, char *errmess */
	SIM_NIS_INIT_MTHD,

	/* char *nsval, char *errmess */
	SIM_CONFIG_NSSWITCH_MTHD,

	/* int * */
	SIM_GET_STATE,
	SIM_PUT_STATE,

	/* char *name, pid_t *pid, none, 0 success, 1 failure */
	SIM_CHECK_DAEMON,

	/* int pid, int signal (set errno if return != 0) */
	SIM_KILL,

	/* struct sim_dblookup * */
	SIM_DB_LOOKUP,

	/* char *ns_type, int tableno, ... */
	SIM_DB_MODIFY,

	/* char *ifname, struct sockaddr *sap, int *errno */
	SIM_SET_IF_ADDR,

	/*
	 * struct if_list **list
	 * returns 0 on error and number of interfaces on success
	 */
	SIM_GET_IFLIST,

	/* char *name (return 0 or 1) */
	SIM_GET_FIRSTUP_IF,

	/* char *ifname, char *newent (returns 0 (success), errno (failure) */
	SIM_REPLACE_HOSTNAME,

	/* char *domainname<input>, char *ns_type<output> */
	SIM_GET_NSINFO,

	/* char *domainname<input>, char *ns_type<output> */
	SIM_AUTOBIND,

	/* char *key<input>, char *val<output> */
	SIM_BPGETFILE,

	/* int f */
	SIM_TEST,

	/* char *sp, int f */
	SIM_TEST2
} Sim_type;

#if defined(__cplusplus)
extern int	(*sim_handle())(Sim_type type ...);
#else
extern int	(*sim_handle())(Sim_type type, ...);
#endif

extern  int	testing; 		/* are we testing? */
extern  void	test_enable();		/* called to enable testing */
extern  int	sim_load(char *); 	/* load the simulator library */
extern	int	sim_init(char *, char *);
extern	void	test_disable();

#ifdef  __cplusplus
}
#endif

#endif /* _SYSIDTOOL_TEST_H */
