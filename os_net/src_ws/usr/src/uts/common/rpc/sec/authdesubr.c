/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)authdesubr.c	1.23	96/10/17 SMI"	/* SVr4.0 1.11	*/

/*
 *  		PROPRIETARY NOTICE (Combined)
 *
 *  This source code is unpublished proprietary information
 *  constituting, or derived under license from AT&T's Unix(r) System V.
 *  In addition, portions of such source code were derived from Berkeley
 *  4.3 BSD under license from the Regents of the University of
 *  California.
 *
 *
 *
 *  		Copyright Notice
 *
 *  Notice of copyright on this source code product does not indicate
 *  publication.
 *
 *  	(c) 1986, 1987, 1988, 1989, 1995  Sun Microsystems, Inc.
 *  	(c) 1983, 1984, 1985, 1986, 1987, 1988, 1989  AT&T.
 *		  All rights reserved.
 */

/*
 * Miscellaneous support routines for kernel implentation of AUTH_DES
 */

/*
 *  rtime - get time from remote machine
 *
 *  sets time, obtaining value from host
 *  on the udp/time socket.  Since timeserver returns
 *  with time of day in seconds since Jan 1, 1900,  must
 *  subtract 86400(365*70 + 17) to get time
 *  since Jan 1, 1970, which is what get/settimeofday
 *  uses.
 */
#include <sys/param.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/socket.h>
#include <sys/sysmacros.h>
#include <netinet/in.h>
#include <rpc/rpc.h>
#include <sys/stream.h>
#include <sys/strsubr.h>
#include <sys/cred.h>
#include <sys/utsname.h>
#include <sys/vnode.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/systeminfo.h>
#include <rpc/rpcb_prot.h>

static char	*atoa(char *, char *);

#define	TOFFSET ((u_long)86400*(365*70 + (70/4)))
#define	WRITTEN ((u_long)86400*(365*86 + (86/4)))

#define	NC_INET		"inet"		/* XXX */

int
rtime(struct knetconfig *synconfig, struct netbuf *addrp, int calltype,
	struct timeval *timep, struct timeval *wait)
{
	int			error;
	int			timo;
	time_t			thetime;
	int			dummy;
	struct t_kunitdata	*unitdata;
	TIUSER			*tiptr;
	int			type;
	int			uderr;
	int			retries;

	retries = 5;
	if (calltype == 0) {

again:
		RPCLOG(2, "rtime: using old method\n", 0);
		if ((error = t_kopen(NULL, synconfig->knc_rdev,
		    FREAD|FWRITE|FNDELAY, &tiptr, CRED())) != 0) {
			RPCLOG(1, "rtime: t_kopen %d\n", error);
			return (-1);
		}

		if ((error = t_kbind(tiptr, NULL, NULL)) != 0) {
			(void) t_kclose(tiptr, 1);
			RPCLOG(1, "rtime: t_kbind %d\n", error);
			return (-1);
		}

		if ((error = t_kalloc(tiptr, T_UNITDATA, T_UDATA|T_ADDR,
						(char **)&unitdata)) != 0) {
			RPCLOG(1, "rtime: t_kalloc %d\n", error);
			(void) t_kclose(tiptr, 1);
			return (-1);
		}

		unitdata->addr.len = addrp->len;
		bcopy(addrp->buf, unitdata->addr.buf, unitdata->addr.len);

		unitdata->udata.buf = (caddr_t)&dummy;
		unitdata->udata.len = sizeof (dummy);

		if ((error = t_ksndudata(tiptr, unitdata, NULL)) != 0) {
			RPCLOG(1, "rtime: t_ksndudata %d\n", error);
			(void) t_kfree(tiptr, (char *)unitdata, T_UNITDATA);
			(void) t_kclose(tiptr, 1);
			return (-1);
		}

		timo = (int)(wait->tv_sec * hz + wait->tv_usec / usec_per_tick);

		RPCLOG(8, "rtime: timo %x\n", timo);
		if ((error = t_kspoll(tiptr, timo, READWAIT, &type)) != 0) {
			RPCLOG(1, "rtime: t_kspoll %d\n", error);
			(void) t_kfree(tiptr, (char *)unitdata, T_UNITDATA);
			(void) t_kclose(tiptr, 1);
			return (-1);
		}

		if (type == 0) {
			RPCLOG(1, "rtime: t_kspoll timed out\n", 0);
			(void) t_kfree(tiptr, (char *)unitdata, T_UNITDATA);
			(void) t_kclose(tiptr, 1);
			return (-1);
		}

		error = t_krcvudata(tiptr, unitdata, &type, &uderr);
		if (error != 0) {
			RPCLOG(1, "rtime: t_krcvudata %d\n", error);
			(void) t_kfree(tiptr, (char *)unitdata, T_UNITDATA);
			(void) t_kclose(tiptr, 1);
			return (-1);
		}

		if (type == T_UDERR) {
			if (bcmp(addrp->buf, unitdata->addr.buf,
			    unitdata->addr.len) != 0) {
				/*
				 * Response comes from some other destination:
				 * ignore it since it's not related to the
				 * request we just sent out.
				 */
				(void) t_kfree(tiptr, (char *)unitdata,
					T_UNITDATA);
				(void) t_kclose(tiptr, 1);
				goto again;
			}
		}

		if (type != T_DATA) {
			RPCLOG(1, "rtime: t_krcvudata returned type %d\n",
				type);
			(void) t_kfree(tiptr, (char *)unitdata, T_UNITDATA);
			(void) t_kclose(tiptr, 1);
			if (retries-- == 0)
				return (-1);
			else	goto again;
		}

		if (unitdata->udata.len < sizeof (u_long)) {
			RPCLOG(1, "rtime: bad rcvd length %d\n",
						unitdata->udata.len);
			(void) t_kfree(tiptr, (char *)unitdata, T_UNITDATA);
			(void) t_kclose(tiptr, 1);
			if (retries-- == 0)
				return (-1);
			else	goto again;
		}

		/* LINTED pointer alignment */
		thetime = (time_t) ntohl(*(u_long *)unitdata->udata.buf);

		(void) t_kfree(tiptr, (char *)unitdata, T_UNITDATA);
		(void) t_kclose(tiptr, 1);
	} else	{
		CLIENT			*client;
		struct timeval		timeout;

		RPCLOG(2, "rtime: using new method\n", 0);


new_again:
		/*
		 *	We talk to rpcbind.
		 */
		error = clnt_tli_kcreate(synconfig, addrp, (u_long)RPCBPROG,
				(u_long)RPCBVERS, 0, retries, CRED(), &client);

		if (error != 0) {
			RPCLOG(1,
			    "rtime: clnt_tli_kcreate returned %d\n",
				error);
			return (-1);
		}
		timeout.tv_sec = 60;
		timeout.tv_usec = 0;
		error = clnt_call(client, RPCBPROC_GETTIME, (xdrproc_t)xdr_void,
				NULL, (xdrproc_t)xdr_u_long,
				(caddr_t)&thetime, timeout);
		auth_destroy(client->cl_auth);
		clnt_destroy(client);
		if (error == RPC_UDERROR) {
			if (retries-- > 0)
				goto new_again;
		}
		if (error != RPC_SUCCESS) {
			RPCLOG(1, "rtime: time sync clnt_call returned %d\n",
				error);
			error = EIO;
			return (-1);
		}
	}

	if (calltype != 0)
		thetime += TOFFSET;

	RPCLOG(8, "rtime: thetime = %x\n", thetime);

	if (thetime < WRITTEN) {
		RPCLOG(1, "rtime: time returned is too far in past %x",
						thetime);
		RPCLOG(1, "rtime: WRITTEN %x", WRITTEN);
		return (-1);
	}
	thetime -= TOFFSET;

	timep->tv_sec = thetime;
	RPCLOG(8, "rtime: timep->tv_sec = %x\n", timep->tv_sec);
	RPCLOG(8, "rtime: machine time  = %x\n", hrestime.tv_sec);
	timep->tv_usec = 0;
	RPCLOG(8, "rtime: returning success\n", 0);
	return (0);
}

static char *
atoa(char *dst, char *src)
{

	while (*dst++ = *src++)
		;
	return (dst - 1);
}

/*
 * What is my network name?
 * WARNING: this gets the network name in sun unix format.
 * Other operating systems (non-unix) are free to put something else
 * here.
 *
 * Return 1 on success, 0 if failed.
 */
int
kgetnetname(char *netname)
{
	char *p;
	struct cred *cr;

	cr = CRED();
	if (cr->cr_uid == 0) {
		p = atoa(netname, "unix.");
		p = atoa(p, utsname.nodename);
		*p++ = '@';
		p = atoa(p, srpc_domain);
		return (1);
	} else {
		if (key_getnetname(netname, cr) == RPC_SUCCESS)
			return (1);
	}
	return (0);
}
