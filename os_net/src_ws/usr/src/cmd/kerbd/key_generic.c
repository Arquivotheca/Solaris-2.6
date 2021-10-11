/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)key_generic.c	1.5	96/04/04 SMI"

/*
*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*	PROPRIETARY NOTICE(Combined)
*
* This source code is unpublished proprietary information
* constituting, or derived under license from AT&T's UNIX(r) System V.
* In addition, portions of such source code were derived from Berkeley
* 4.3 BSD under license from the Regents of the University of
* California.
*
*
*
*	Copyright Notice
*
* Notice of copyright on this source code product does not indicate
*  publication.
*
*	(c) 1986, 1987, 1988, 1989  Sun Microsystems, Inc
*	(c) 1983, 1984, 1985, 1986, 1987, 1988, 1989  AT&T.
*	All rights reserved.
*/
#include <stdio.h>
#include <rpc/rpc.h>
#include <errno.h>
#ifdef SYSLOG
#include <sys/syslog.h>
#else
#define	LOG_ERR 3
#endif				/* SYSLOG */
#include <rpc/nettype.h>
#include <netconfig.h>
#include <netdir.h>

extern int t_errno;
extern char *t_errlist[];

extern char *strdup();

/*
 * The highest level interface for server creation.
 * It tries for all the nettokens in that particular class of token
 * and returns the number of handles it can create and/or find.
 *
 * It creates a link list of all the handles it could create.
 * If svc_create() is called multiple times, it uses the handle
 * created earlier instead of creating a new handle every time.
 *
 * Copied from svc_generic.c and ../keyserv/key_generic.c
 */
int
svc_create_local_service(dispatch, prognum, versnum, nettype, servname)
void (*dispatch) ();		/* Dispatch function */
u_long prognum;			/* Program number */
u_long versnum;			/* Version number */
char *nettype;			/* Networktype token */
char *servname;			/* name of the service */
{
	struct xlist {
		SVCXPRT *xprt;		/* Server handle */
		struct xlist *next;	/* Next item */
	} *l;
	static struct xlist *xprtlist;
	int num = 0;
	SVCXPRT *xprt;
	struct netconfig *nconf;
	struct t_bind *bind_addr;
	void *net;
	int fd;
	struct nd_hostserv ns;
	struct nd_addrlist *nas;

	if ((net = __rpc_setconf(nettype)) == 0) {
		(void) syslog(LOG_ERR,
		"svc_create: could not read netconfig database");
		return (0);
	}
	while (nconf = __rpc_getconf(net)) {
		if (strcmp(nconf->nc_protofmly, NC_LOOPBACK))
			continue;
		if (nconf->nc_semantics == NC_TPI_CLTS)
			break;
	}
	/* Now create a new one */
	if ((fd = t_open(nconf->nc_device, O_RDWR, NULL)) < 0) {
		syslog(LOG_ERR,
			"svc_create: %s: cannot open connection: %s",
				nconf->nc_netid, t_errlist[t_errno]);
		return (num);
	}
	bind_addr = (struct t_bind *) t_alloc(fd, T_BIND, T_ADDR);
	if ((bind_addr == NULL)) {
		t_close(fd);
		(void) syslog(LOG_ERR, "svc_create: t_alloc failed\n");
		return (num);
	}
	ns.h_host = HOST_SELF;
	ns.h_serv = servname;
	if (!netdir_getbyname(nconf, &ns, &nas)) {
		/* Copy the address */
		bind_addr->addr.len = nas->n_addrs->len;
		memcpy(bind_addr->addr.buf, nas->n_addrs->buf,
			(int) nas->n_addrs->len);
		netdir_free((char *) nas, ND_ADDRLIST);
	} else {
		syslog(LOG_ERR,
		"svc_create: no well known address for %s on transport %s\n",
			servname, nconf->nc_netid);
		(void) t_free((char *) bind_addr, T_BIND);
		bind_addr = NULL;
	}
	xprt = svc_tli_create(fd, nconf, bind_addr, 0, 0);
	if (bind_addr)
		(void) t_free((char *) bind_addr, T_BIND);
	if (xprt) {
		(void) rpcb_unset(prognum, versnum, nconf);
		if (svc_reg(xprt, prognum, versnum,
			dispatch, nconf) == FALSE) {
			(void) syslog(LOG_ERR,
			"svc_create: could not register prog %d vers %d on %s",
			    prognum, versnum, nconf->nc_netid);
			SVC_DESTROY(xprt);
			return (num);
		}
		num++;
		__rpc_negotiate_uid(xprt->xp_fd);
	}
	__rpc_endconf(net);
	return (num);
}
