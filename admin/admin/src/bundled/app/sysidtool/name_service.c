
/*
 *  Copyright (c) 1991 Sun Microsystems, Inc.  All Rights Reserved.
 *  All rights reserved.
 */

/*
 *	File:		name_service.c
 *
 *	Description:	This file contains the routines needed to prompt
 *			the user naming service information.
 */

#pragma ident	"@(#)name_service.c	1.44	95/01/30 SMI"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <netdb.h>
#include <netdir.h>
#include <malloc.h>
#include <stdio.h>
#include <sys/types.h>
#include <arpa/nameser.h>
#include <netinet/in.h>
#include <rpc/rpc.h>
#include <rpcsvc/nis.h>
#include <arpa/inet.h>
#include <signal.h>
#include <setjmp.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/bootparam.h>
#include <sys/utsname.h>
#include <sys/systeminfo.h>
#include "sysidtool.h"
#include "nis_svc.h"
#include "nis_proc.h"
#include "sysid_msgs.h"
#include "cl_database_parms.h"

#ifndef CLCR_SET_LOWVERS
#define	CLCR_SET_LOWVERS 3
#endif

#define	LONGBUF 1025

void __rpc_control();

static int get_ns_policy(char **pnetmask);
static int try_nisplus(char *domain);
static int try_nis(char *domain);
static int vrfy_nis();
static int vrfy_yp();
static void sigalarm_handler(int);

static char *target_name;
static jmp_buf env;
static int ypbind_run = 0;

/* UseNull */
#define	UN(string) ((string) ? (string) : "")

static char name_service[LONGBUF];

char name_server_name[MAXHOSTNAMELEN + 1];
char name_server_addr[MAX_IPADDR + 1];

/*
 * attempt to initialize the name service
 * return 0 if ok, >0 if an error occured
 */
int
init_ns(char *ns_type, char *domainname, int bcast_flg, char *ns_name,
	char *ns_addr, char *errmess)
{
	char	*a[3];

	fprintf(debugfp, "init_ns: ns_type %s, flag %d\n", ns_type, bcast_flg);

	/* Configure the /etc/nsswitch.conf file. */
	if (setup_nsswitch((ns_type[0] == NULL ? NO_NAMING_SERVICE : ns_type),
	    errmess) != SUCCESS)
		return (INIT_NS_SWITCH);

	/* If no name service is to be used, we are done.  */
	if (ns_type[0] == NULL)
		return (INIT_NS_OK);

	/* Set the domainname on this system */
	if (set_domainname(domainname, errmess) != SUCCESS)
		return (INIT_NS_DOMAIN);

	/* Set up an entry in /etc/hosts for the name server */
	if (!bcast_flg)
		if (set_ent_hosts(ns_addr, ns_name, NULL, errmess) != SUCCESS)
			return (INIT_NS_HOSTS);

	/*
	 * Actually initialize NIS or NIS+.
	 * Call methods that perform ypinit or nisinit.
	 */

	a[0] = "/usr/sbin/rpcbind";
	a[1] = NULL;
	run(a);

	if (!name_server_name[0])
		strcpy(name_server_name, gettext("none"));

	if (strcmp(ns_type, NIS_VERSION_2) == 0) {
		/* Initialize YP */

		if (init_yp_aliases(domainname, errmess) != SUCCESS) {
			return (INIT_NS_ALIASES);
		} else if (init_yp_binding(domainname, bcast_flg, ns_name,
		    errmess) != SUCCESS) {
			return (INIT_NS_BIND);
		}

		a[0] = "/usr/lib/netsvc/yp/ypbind";
		if (bcast_flg) {
			a[1] = "-broadcast";
			a[2] = NULL;
		} else
			a[1] = NULL;
		run(a);
		ypbind_run = 1;

		if (!vrfy_yp()) {
			strcpy(errmess,
			    gettext("ypbind cannot communicate with ypserv"));
			return (INIT_NS_YPSRV);
		}

	} else if (strcmp(ns_type, NIS_VERSION_3) == 0) {
		/* Initialize NIS+ */

		if (init_nis_plus(bcast_flg, ns_name, errmess) != SUCCESS)
			return (INIT_NS_NISP);

		if (!vrfy_nis())
			return (INIT_NS_NISP_ACC);
	}

	return (INIT_NS_OK);
}

/*
 * return 1 if org_dir access allows nobody, otherwise 0
 */
static int
vrfy_nis()
{
	nis_result 	*res;
	nis_object	*dirobj;
	int		ret = 0;
	nis_error	status;

	fprintf(debugfp, "vrfy_nis\n");

	if (testing)
		return (1);

	/* Get the directory object using expand name */
	for (;;) {
		res = nis_lookup((nis_name) "org_dir", EXPAND_NAME);
		status = res->status;
		(void) fprintf(debugfp, "lookup status %d\n", status);
		if (status == NIS_SUCCESS) {
			dirobj = NIS_RES_OBJECT(res);
			(void) fprintf(debugfp, "access rights: 0x%0x\n",
				dirobj->zo_access);

			if (NOBODY(dirobj->zo_access, NIS_READ_ACC))
				ret = 1;

			(void) nis_freeresult(res);
			return (ret);
		}

		(void) nis_freeresult(res);
		if (status != NIS_TRYAGAIN)
			return (ret);
	}
}

/*
 * return 1 if ypbind binds to a yp server, otherwise 0
 */
static int
vrfy_yp()
{
	int	i, res, len, hlen;
	char	*entry;
	char	domain[DOM_NM_LN + 1], hostname[DOM_NM_LN + 1];
	void (*savesig) (int);

	fprintf(debugfp, "vrfy_yp\n");

	if (testing)
		return (1);

	savesig = signal(SIGALRM, sigalarm_handler);

	if (setjmp(env) != 0) {
		fprintf(debugfp, "vrfy_yp: timed out!\n");
		(void) signal(SIGALRM, savesig);
		return (0);
	}

	(void) sysinfo(SI_SRPC_DOMAIN, domain, DOM_NM_LN);
	(void) sysinfo(SI_HOSTNAME, hostname, DOM_NM_LN);
	hlen = strlen(hostname);

	for (i = 0; i < 5; i++) {
		(void) alarm(30);
		res = yp_match(domain, "hosts.byname", hostname, hlen,
			&entry, &len);
		(void) alarm(0);
		(void) fprintf(debugfp, "yp_match status: %d\n", res);
		switch (res) {
		case 0:
		case YPERR_KEY:
		case YPERR_MAP:
			(void) signal(SIGALRM, savesig);
			return (1);
		case YPERR_YPBIND:
			sleep(3);
			break;
		default:
			(void) signal(SIGALRM, savesig);
			return (0);
		}
	}
	(void) signal(SIGALRM, savesig);
	return (0);
}

void
kill_ypbind()
{
	if (ypbind_run) {
		fprintf(debugfp, "kill ypbind\n");
		system("kill -9 `ps -e | awk '/ypbind/{print $1}'`");
	}
}

/*ARGSUSED*/
static int
each_bp_getfile_res(bp_getfile_res *out, struct sockaddr_in *addr,
	struct netconfig *nconf)
{
	strcpy(name_server_name, UN(out->server_name));
	strcpy(name_service, UN(out->server_path));
	if (strlen(name_server_name) || strlen(name_service))
		sprintf(name_server_addr, "%d.%d.%d.%d",
		    (int) (*((unsigned char *)
			&(out->server_address.bp_address_u.ip_addr.net))),
		    (int) (*((unsigned char *)
			&(out->server_address.bp_address_u.ip_addr.host))),
		    (int) (*((unsigned char *)
			&(out->server_address.bp_address_u.ip_addr.lh))),
		    (int) (*((unsigned char *)
			&(out->server_address.bp_address_u.ip_addr.impno))));
	else
		strcpy(name_server_addr, "");

	return (1);
}

/*ARGSUSED*/
static bool_t
bc_proc(caddr_t x, struct netbuf *haddr, struct netconfig *nc)
{
	fd_result	*res = (fd_result *) x;
	char		*val;
	int		len;
	XDR		xdrs;
	directory_obj	dobj;

	if (res->status == NIS_SUCCESS) {
		val = (char *)res->dir_data.dir_data_val;
		len = res->dir_data.dir_data_len;
		xdrmem_create(&xdrs, val, len, XDR_DECODE);
		memset((char *)&dobj, 0, sizeof (dobj));
		if (! xdr_directory_obj(&xdrs, &dobj)) {
			xdr_destroy(&xdrs);
			return (0);
		}
		fprintf(debugfp, "%s serves %s\n", res->source, dobj.do_name);

		if (nis_dir_cmp(dobj.do_name, target_name) != SAME_NAME) {
			xdr_destroy(&xdrs);
			xdr_free(xdr_directory_obj, (char *)&dobj);
			return (0);
		}

		if (res->source) {
			(void) strcpy(name_server_name, res->source);
			fprintf(debugfp, "bc_proc: %s\n", res->source);
		}
		else
			name_server_name[0] = '\0';

		xdr_destroy(&xdrs);
		xdr_free(xdr_directory_obj, (char *)&dobj);

		return (1);
	}
	else
		return (0);
}

/*
 * bcast_proc:
 *
 *	Collects replies from name service location broadcasts.
 *	Returns 1 when a reply is received from
 *	a server that will service the domain.
 */

/*ARGSUSED*/
static bool_t
bcast_proc(caddr_t *res, struct netbuf *who, struct netconfig *nc)
{
	char *uaddr;
	struct nd_hostservlist *hs;

	if (netdir_getbyaddr(nc, &hs, who) == ND_OK)
		(void) strcpy(name_server_name, hs->h_hostservs->h_host);
	else {
		uaddr = (char *)taddr2uaddr(nc, who);
		if (uaddr)
			(void) strcpy(name_server_name, uaddr);
		else
			name_server_name[0] = '\0';
	}
	fprintf(debugfp, "NS responded: %s\n", name_server_name);
	return (1);
}

/* ARGSUSED */
static void
sigalarm_handler(int sig)
{
	longjmp(env, 1);
}

/*
 * autobind:
 *
 *	Determine the name service in use on the network by
 *	broadcasting to locate a server.  Selected name
 *	services in order of preference:
 *
 *		NIS Version 3  (i.e. NIS+)
 *		NIS Version 2
 *		NIS Version 1
 *
 *	If successful, this routine returns 1 and
 *	sets nstype to the name service selected.  If no
 *	name service is found, this routine returns 0.  Can also return -1.
 *
 * Before broadcasting, check if name service policy is set in bootparams
 * with the ns key.  This has syntax [<server>]:<name service>[(<netmask>)]
 * If an explicit server is named, return -1, since we will not used
 * broadcast to bind to the server.  If a netmask is specified in bootparams,
 * it will be put in the netmask string, otherwise, the netmask string will
 * be null.
 *
 * Each rpc_broadcast waits for 12 seconds if no response
 * (from usr/src/lib/libnsl/rpc/clnt_bcast.c) INITTIME=4000, WAITTIME=8000
 * 4+8 = 12
 *
 * set a timer in case something is screwed up with the net and we hang
 * in the broadcast (bug 1174160 misconfigured Intel network board)
 */

int
autobind(char *domain, char *ifname, char *nstype, char *netmask)
{
	int status;
	void (*savesig) (int);
	char *pnetmask;

	fprintf(debugfp, "autobind: %s %s\n", domain, ifname);

	if (setjmp(env) != 0) {
		fprintf(debugfp, "autobind: timed out!\n");
		return (0);
	}

	savesig = signal(SIGALRM, sigalarm_handler);
	(void) alarm(60);

	nstype[0] = 0;

	status = 0;

	if (get_ns_policy(&pnetmask)) {
		if (name_server_name[0])
			status = -1;
		else
			status = 1;

		if (strcmp(name_service, "nis") == 0)
			(void) strcpy(nstype, NIS_VERSION_2);
		else if (strcmp(name_service, "nisplus") == 0)
			(void) strcpy(nstype, NIS_VERSION_3);
		else if (strcmp(name_service, "none") == 0)
			(void) strcpy(nstype, NO_NAMING_SERVICE);
		else
			status = 0;

		if (pnetmask)
			/* handle net mask */
			strcpy(netmask, pnetmask);
	}

	if (status == 0) {
		status = 1;

		if (pnetmask) {
			char errmess[1024];

			/*
			 * set the netmask because it may effect the broadcast
			 */
			set_net_netmask(ifname, netmask, errmess);
		}

		if (testing)
			status = (*sim_handle())(SIM_AUTOBIND, domain, nstype);

			/* Try NIS+ first */
		else if (try_nisplus(domain))
			(void) strcpy(nstype, NIS_VERSION_3);

			/* Now try NIS (YP) */
		else if (try_nis(domain))
			(void) strcpy(nstype, NIS_VERSION_2);

		else
			status = 0;
	}

	/* restore any SIGALRM handler */
	(void) alarm(0);
	(void) signal(SIGALRM, savesig);

	fprintf(debugfp, "autobind ns type: %s\n", (status) ? nstype : "fail");

	return (status);
}

static int
get_ns_policy(char **pnetmask)
{
	int rc;
	char clientname[SYS_NMLN];
	int val = 1;
	bp_getfile_arg getfile_in = { 0 };
	bp_getfile_res getfile_out = { 0 };
	/* this gives a total time of 9 seconds (3 + 6) */
	int inittime = 3000;	/* Time to wait initially */
	int waittime = 6000;	/* Maximum time to wait */

	fprintf(debugfp, "get_ns_policy\n");

	sysinfo(SI_HOSTNAME, clientname, SYS_NMLN);

	getfile_in.client_name = clientname;
	getfile_in.file_id = "ns";

	/*
	 * Broadcast only using version 2, since version 3
	 * may cause broadcast storm
	 */
	(void) __rpc_control(CLCR_SET_LOWVERS, (void *)&val);

	if (testing)
		rc = (*sim_handle())(SIM_BPGETFILE, "ns", name_server_name);
	else
		rc = rpc_broadcast_exp(BOOTPARAMPROG, BOOTPARAMVERS,
			BOOTPARAMPROC_GETFILE,
			xdr_bp_getfile_arg, (caddr_t) &getfile_in,
			xdr_bp_getfile_res, (caddr_t) &getfile_out,
			(resultproc_t) each_bp_getfile_res,
			inittime, waittime, NULL);

	fprintf(debugfp, "ns_policy: %d %s %s %s\n", rc, name_server_name,
		name_server_addr, name_service);

	if ((*pnetmask = strchr(name_service, '(')) != NULL) {
		char *pr;

		**pnetmask = '\0';
		(*pnetmask)++;
		if ((pr = strrchr(*pnetmask, ')')) != NULL)
			*pr = '\0';
	}

	return (!rc);
}


static int
try_nisplus(char *domain)
{
	fd_args		fdarg;
	fd_result	fdres;
	enum clnt_stat	rpc_stat;
	int		val = 1;

	fprintf(debugfp, "try_nisplus\n");

	/*
	 * Broadcast only using version 2, since version 3
	 * may cause broadcast storm
	 */
	__rpc_control(CLCR_SET_LOWVERS, (void *)&val);

	fdarg.dir_name = domain;
	fdarg.requester = "broadcast";
	target_name = fdarg.dir_name;
	memset((char *)&fdres, 0, sizeof (fdres));
	rpc_stat = rpc_broadcast(NIS_PROG, NIS_VERSION, NIS_FINDDIRECTORY,
		xdr_fd_args, (char *) &fdarg, xdr_fd_result, (char *) &fdres,
		(resultproc_t) bc_proc, NULL);

	val = 0;
	__rpc_control(CLCR_SET_LOWVERS, (void *)&val);

	if (rpc_stat == RPC_SUCCESS) {
		fprintf(debugfp, "found nisplus\n");
		return (1);
	}

	if (rpc_stat != RPC_TIMEDOUT)
		fprintf(debugfp, "nisplus error %d\n", rpc_stat);

	return (0);
}

static int
try_nis(char *domain)
{
	/* NIS protocols to try - in order of preference */
	u_long		vers;
	enum clnt_stat	rpc_stat;

	fprintf(debugfp, "try_nis\n");

	for (vers = YPVERS; vers >= YPVERS_ORIG; vers--) {
		rpc_stat = rpc_broadcast(YPPROG, vers, YPPROC_DOMAIN_NONACK,
			(const xdrproc_t) xdr_ypdomain_wrap_string,
			(caddr_t) &domain, xdr_void,
			(char *)NULL, (resultproc_t) bcast_proc, "udp");

		if (rpc_stat == RPC_SUCCESS) {
			fprintf(debugfp, "found nis\n");
			return (1);
		}

		if (rpc_stat != RPC_TIMEDOUT) {
			fprintf(debugfp, "nis error %d\n", rpc_stat);
			break;
		}
	}

	return (0);
}
