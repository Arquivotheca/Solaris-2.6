/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)yppush.c	1.17	96/04/25 SMI"

/*
*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*	PROPRIETARY NOTICE [Combined]
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
*	(c) 1986, 1987, 1988, 1989, 1990  Sun Microsystems, Inc
*	(c) 1983, 1984, 1985, 1986, 1987, 1988, 1989  AT&T.
*	All rights reserved.
*/

#define	_SVID_GETTOD
#include	<sys/time.h>
extern int gettimeofday(struct timeval *);

#include	<sys/types.h>
#include	<stdio.h>
#include	<string.h>
#include	<malloc.h>
#include	<errno.h>
#include	<signal.h>
#include	<limits.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<sys/types.h>
#include	<sys/wait.h>
#include	<sys/stat.h>
#include	<ctype.h>
#include	<dirent.h>
#include	<rpc/rpc.h>
#include	<rpc/nettype.h>
#include	<rpc/rpcb_prot.h>
#include	<rpc/rpcb_clnt.h>
#include	<sys/systeminfo.h>
#include	<sys/select.h>
#include	"ypsym.h"
#include	"ypdefs.h"
#include	"yp_b.h"


#ifdef DEBUG
#undef YPPROG
#define	YPPROG ((u_long)109999)
#undef YPBINDPROG
#define	YPBINDPROG ((u_long)109998)
#endif

#define	INTER_TRY 12			/* Seconds between tries */
#define	TIMEOUT INTER_TRY*4		/* Total time for timeout */
#define	GRACE_PERIOD 800		/* Total seconds we'll wait for	*/
					/* responses from ypxfrs, yes	*/
					/* virginia yp map transfers	*/
					/* can take a long time, we	*/
					/* only worry if the slave 	*/
					/* crashes ...			*/

USE_YPDBPATH
static char *pusage;
static char *domain = NULL;
static char *host = NULL;
static char my_name[YPMAXPEER +1];
static char default_domain_name[YPMAXDOMAIN];
static char domain_alias[MAXNAMLEN]; 	/* nickname for domain -	*/
					/*	used in sysv filesystems */
static char map_alias[MAXNAMLEN];	/* nickname for map -		*/
					/*	used in sysv filesystems */
static char *map = NULL;
static bool verbose = FALSE;
static bool onehost = FALSE;
static bool oldxfr = FALSE;
static bool callback_timeout = FALSE;	/* set when a callback times out */
static char ypmapname[1024];		/* Used to check for map's existence */

static struct timeval intertry = {
	INTER_TRY,			/* Seconds */
	0				/* Microseconds */
};
static struct timeval timeout = {
	TIMEOUT,			/* Seconds */
	0				/* Microseconds */
};
static SVCXPRT *transport;
struct server {
	struct server *pnext;
	struct dom_binding domb;
	char svc_name[YPMAXPEER+1];
	unsigned long xactid;
	unsigned short state;
	unsigned long status;
	bool oldvers;
};
#define	n_conf dom_binding->ypbind_nconf
#define	svc_addr dom_binding->ypbind_svcaddr
static struct server *server_list = (struct server *) NULL;

/*  State values for server.state field */

#define	SSTAT_INIT 0
#define	SSTAT_CALLED 1
#define	SSTAT_RESPONDED 2
#define	SSTAT_PROGNOTREG 3
#define	SSTAT_RPC 4
#define	SSTAT_RSCRC 5
#define	SSTAT_SYSTEM 6

static char err_usage[] =
"Usage:\n\typpush [-d <domainname>] [-h <hostname>] [-v] map\n";
static char err_bad_args[] =
	"The %s argument is bad.\n";
static char err_cant_get_kname[] =
	"Can't get %s from system call.\n";
static char err_null_kname[] =
	"The %s hasn't been set on this machine.\n";
static char err_bad_domainname[] = "domainname";
static char err_cant_bind[] =
	"Can't find a yp server for domain %s.  Reason:  %s.\n";
static char err_cant_build_serverlist[] =
	"Can't build server list from map \"ypservers\".  Reason:  %s.\n";
static char err_cant_find_host[] =
	"Can't find host %s in map \"ypservers\".\n";
/*
 * State_duple table.  All messages should take 1 arg - the node name.
 */
struct state_duple {
	int state;
	char *state_msg;
};
static struct state_duple state_duples[] = {
	{SSTAT_INIT, "Internal error trying to talk to %s."},
	{SSTAT_CALLED, "%s has been called."},
	{SSTAT_RESPONDED, "%s (v1 ypserv) sent an old-style request."},
	{SSTAT_PROGNOTREG, "nis server not registered at %s."},
	{SSTAT_RPC, "RPC error to %s:  "},
	{SSTAT_RSCRC, "Local resource allocation failure - can't talk to %s."},
	{SSTAT_SYSTEM, "System error talking to %s:  "},
	{0, (char *) NULL}
};
/*
 * Status_duple table.  No messages should require any args.
 */
static struct status_duple {
	long status;
	char *status_msg;
};
static struct status_duple status_duples[] = {
	{YPPUSH_SUCC, "Map successfully transferred."},
	{YPPUSH_AGE,
	    "Transfer not done:  master's version isn't newer."},
	{YPPUSH_NOMAP, "Failed - ypxfr there can't find a server for map."},
	{YPPUSH_NODOM, "Failed - domain isn't supported."},
	{YPPUSH_RSRC, "Failed - local resource allocation failure."},
	{YPPUSH_RPC, "Failed - ypxfr had an RPC failure"},
	{YPPUSH_MADDR, "Failed - ypxfr couldn't get the map master's address."},
	{YPPUSH_YPERR, "Failed - nis server or map format error."},
	{YPPUSH_BADARGS, "Failed - args to ypxfr were bad."},
	{YPPUSH_DBM, "Failed - dbm operation on map failed."},
	{YPPUSH_FILE, "Failed - file I/O operation on map failed"},
	{YPPUSH_SKEW, "Failed - map version skew during transfer."},
	{YPPUSH_CLEAR,
		"Map successfully transferred, but ypxfr \
		couldn't send \"Clear map\" to ypserv "},
	{YPPUSH_FORCE,
	    "Failed - no local order number in map - use -f flag to ypxfr."},
	{YPPUSH_XFRERR, "Failed - ypxfr internal error."},
	{YPPUSH_REFUSED, "Failed - Transfer request refused."},
	{YPPUSH_NOALIAS,
		"Failed - System V domain/map alias not in alias file."},
	{0, (char *) NULL}
};
/*
 * rpcerr_duple table
 */
static struct rpcerr_duple {
	enum clnt_stat rpc_stat;
	char *rpc_msg;
};
static struct rpcerr_duple rpcerr_duples[] = {
	{RPC_SUCCESS, "RPC success"},
	{RPC_CANTENCODEARGS, "RPC Can't encode args"},
	{RPC_CANTDECODERES, "RPC Can't decode results"},
	{RPC_CANTSEND, "RPC Can't send"},
	{RPC_CANTRECV, "RPC Can't recv"},
	{RPC_TIMEDOUT, "NIS server registered, but does not respond"},
	{RPC_VERSMISMATCH, "RPC version mismatch"},
	{RPC_AUTHERROR, "RPC auth error"},
	{RPC_PROGUNAVAIL, "RPC remote program unavailable"},
	{RPC_PROGVERSMISMATCH, "RPC program mismatch"},
	{RPC_PROCUNAVAIL, "RPC unknown procedure"},
	{RPC_CANTDECODEARGS, "RPC Can't decode args"},
	{RPC_UNKNOWNHOST, "unknown host"},
	{RPC_RPCBFAILURE, "rpcbind failure (host is down?)"},
	{RPC_PROGNOTREGISTERED, "RPC prog not registered"},
	{RPC_SYSTEMERROR, "RPC system error"},
	{RPC_SUCCESS, (char *) NULL}		/* Duplicate rpc_stat 	*/
						/* unused in list-end 	*/
						/* entry */
};

static void get_default_domain_name(void);
static void get_command_line_args(int argc, char **argv);
static unsigned short send_message(struct server *ps,
					unsigned long program, long *err);
static void make_server_list(void);
static void one_host_list(void);
static void add_server(char *sname, int namelen);
static void generate_callback(unsigned long *program);
static void xactid_seed(unsigned long *xactid);
static void main_loop(unsigned long program);
static void listener_exit(unsigned long program, int stat);
static void listener_dispatch(struct svc_req *rqstp, SVCXPRT *transp);
static void print_state_msg(struct server *s, long e);
static void print_callback_msg(struct server *s);
static void rpcerr_msg(enum clnt_stat e);
static void get_xfr_response(SVCXPRT *transp);
static void set_time_up(void);

#ifdef SYSVCONFIG
extern void sysvconfig(void);
#endif
extern int yp_getalias(char *key, char *key_alias, int maxlen);
extern int getdomainname(char *, int);

extern struct rpc_createerr rpc_createerr;
extern char *sys_errlist[];
extern int sys_nerr;
extern CLIENT *__yp_clnt_create_rsvdport();

int
main(int argc, char **argv)
{
	unsigned long program;
	struct stat sbuf;

	get_command_line_args(argc, argv);

	if (!domain) {
		get_default_domain_name();
	}

#ifdef SYSVCONFIG
	sysvconfig();
#endif

	if (yp_getalias(domain, domain_alias, NAME_MAX) != 0)
		fprintf(stderr, "domain alias for %s not found\n", domain);
	if (yp_getalias(map, map_alias, MAXALIASLEN) != 0)
		fprintf(stderr, "map alias for %s not found\n", map);

	/* check to see if the map exists in this domain */
	sprintf(ypmapname, "%s/%s/%s.dir", ypdbpath, domain_alias, map_alias);
	if (stat(ypmapname, &sbuf) < 0) {
		fprintf(stderr, "yppush: Map does not exist.\n");
		exit(1);
	}

	if (onehost) {
		one_host_list();
	} else {
		make_server_list();
	}

	/*
	* All process exits after the call to generate_callback should be
	* through listener_exit(program, status), not exit(status), so the
	* transient server can get unregistered with the portmapper.
	*/

	generate_callback(&program);

	main_loop(program);

	listener_exit(program, 0);

	/* NOTREACHED */
	return (0);
}

/*
 * This does the command line parsing.
 */
static void
get_command_line_args(int argc, char **argv)
{
	pusage = err_usage;
	argv++;

	if (argc < 2) {
		fprintf(stderr, pusage);
		exit(1);
	}

	while (--argc) {
		if ((*argv)[0] == '-') {
			switch ((*argv)[1]) {
			case 'v':
				verbose = TRUE;
				argv++;
				break;
			case 'd':
				if (argc > 1) {
					argv++;
					argc--;
					domain = *argv;
					argv++;
					if (((int)strlen(domain)) >
								YPMAXDOMAIN) {
						fprintf(stderr,
							err_bad_args,
							err_bad_domainname);
						exit(1);
					}
				} else {
					fprintf(stderr, pusage);
					exit(1);
				}
				break;
			case 'h':
				if (argc > 1) {
					onehost = TRUE;
					argv++;
					argc--;
					host = *argv;
					argv++;
				} else {
					fprintf(stderr, pusage);
					exit(1);
				}
				break;
			default:
				fprintf(stderr, pusage);
				exit(1);
			}
		} else {
			if (!map) {
				map = *argv;
			} else {
				fprintf(stderr, pusage);
				exit(1);
			}
			argv++;
		}
	}

	if (!map) {
		fprintf(stderr, pusage);
		exit(1);
	}
}

/*
 *  This gets the local kernel domainname, and sets the global domain to it.
 */
static void
get_default_domain_name(void)
{
	if (!getdomainname(default_domain_name, YPMAXDOMAIN)) {
		domain = default_domain_name;
	} else {
		fprintf(stderr, err_cant_get_kname, err_bad_domainname);
		exit(1);
	}

	if ((int) strlen(domain) == 0) {
		fprintf(stderr, err_null_kname, err_bad_domainname);
		exit(1);
	}
}

/*
 * This verifies that the hostname supplied by the user is in the map
 * "ypservers" then calls add_server to make it the only entry on the
 * list of servers.
 */
static void
one_host_list(void)
{
	char *key;
	int keylen;
	char *val;
	int vallen;
	int err;
	char *ypservers = "ypservers";

	if (verbose) {
		printf("Verifying YP server: %s\n", host);
		fflush(stdout);
	}

	if (err = yp_bind(domain_alias)) {
		fprintf(stderr, err_cant_bind, domain, yperr_string(err));
		exit(1);
	}

	keylen = strlen(host);

	if (yp_match(domain_alias, ypservers, host, keylen,
			&val, &vallen)) {
		fprintf(stderr, err_cant_find_host, host);
		exit(1);
	}

	add_server(host, keylen);
}

/*
 * This uses yp operations to retrieve each server name in the map
 *  "ypservers".  add_server is called for each one to add it to the list of
 *  servers.
 */
static void
make_server_list(void)
{
	char *key;
	int keylen;
	char *outkey;
	int outkeylen;
	char *val;
	int vallen;
	int err;
	char *ypservers = "ypservers";
	int count;

	if (verbose) {
		printf("Finding YP servers: ");
		fflush(stdout);
		count = 4;
	}

	if (err = yp_bind(domain_alias)) {
		fprintf(stderr, err_cant_bind, domain, yperr_string(err));
		exit(1);
	}

	if (err = yp_first(domain_alias, ypservers, &outkey, &outkeylen,
				&val, &vallen)) {
		fprintf(stderr, err_cant_build_serverlist, yperr_string(err));
		exit(1);
	}

	for (;;) {
		add_server(outkey, outkeylen);
		if (verbose) {
			printf(" %s", outkey);
			fflush(stdout);
			if (count++ == 8) {
				printf("\n");
				count = 0;
			}
		}
		free(val);
		key = outkey;
		keylen = outkeylen;

		if (err = yp_next(domain_alias, ypservers, key, keylen,
				&outkey, &outkeylen, &val, &vallen)) {

		    if (err == YPERR_NOMORE) {
			break;
		    } else {
			fprintf(stderr, err_cant_build_serverlist,
				yperr_string(err));
			exit(1);
		    }
		}

		free(key);
	}
	if (count != 0) {
		printf("\n");
	}
}

/*
 *  This adds a single server to the server list.
 */
static void
add_server(char *sname, int namelen)
{
	struct server *ps;
	static unsigned long seq;
	static unsigned long xactid = 0;

	if (strcmp(sname, my_name) == 0)
		return;

	if (xactid == 0) {
		xactid_seed(&xactid);
	}

	if ((ps = (struct server *) malloc((unsigned) sizeof (struct server)))
		== (struct server *) NULL) {
		perror("yppush: malloc failure");
		exit(1);
	}

	sname[namelen] = '\0';
	strcpy(ps->svc_name, sname);
	ps->state = SSTAT_INIT;
	ps->status = 0;
	ps->oldvers = FALSE;
	ps->xactid = xactid + seq++;
	ps->pnext = server_list;
	server_list = ps;
}

/*
 * This sets the base range for the transaction ids used in speaking the the
 *  server ypxfr processes.
 */
static void
xactid_seed(unsigned long *xactid)
{
	struct timeval t;

	if (gettimeofday(&t) == -1) {
		perror("yppush gettimeofday failure");
		*xactid = 1234567;
	} else {
		*xactid = t.tv_sec;
	}
}

/*
 *  This generates the channel which will be used as the listener process'
 *  service rendezvous point, and comes up with a transient program number
 *  for the use of the RPC messages from the ypxfr processes.
 */
static void
generate_callback(unsigned long *program)
{
	long unsigned prognum = 0x40000000;
	struct netconfig *nconf;
	void *net;

	if ((net = __rpc_setconf("netpath")) == 0) {
		fprintf(stderr, "yppush: Unknown protocol\n");
		exit(1);
	}

	while (transport == (SVCXPRT *) NULL) {
		if ((nconf = __rpc_getconf(net)) == (struct netconfig *) NULL) {
			fprintf(stderr, "yppush: Unknown protocol\n");
			break;
		}
		transport = svc_tli_create(RPC_ANYFD, nconf,
						(struct t_bind *) NULL, 0, 0);
		if (transport)
			break;
	}

	if (transport == (SVCXPRT *)NULL) {
		fprintf(stderr, "yppush: Could not create server handle\n");
		exit(1);
	}

	/* now register the information with the local binder service */
	while (!rpcb_set(prognum++, YPPUSHVERS, nconf, &transport->xp_ltaddr));

	__rpc_endconf(net);

	*program = --prognum;
}

/*
 * This is the main loop. Send messages to each server,
 * and then wait for a response.
 */
static void
main_loop(unsigned long program)
{
	int readfds;
	register struct server *ps;
	long error;

	if (!svc_reg(transport, program, YPPUSHVERS, listener_dispatch, 0))
		fprintf(stderr, "Can't set up transient callback server.\n");

	sigset(SIGALRM, (void (*)(int)) set_time_up);

	for (ps = server_list; ps; ps = ps->pnext) {

		if (strcmp(ps->svc_name, my_name) == 0)
			continue;

		ps->state = send_message(ps, program, &error);
		print_state_msg(ps, error);

		if (ps->state != SSTAT_CALLED)
			continue;

		callback_timeout = FALSE;

		alarm(GRACE_PERIOD);
		while (callback_timeout == FALSE &&
				ps->state == SSTAT_CALLED) {

			readfds = svc_fds;
			errno = 0;
			switch ((int) select(32, (fd_set *) &readfds,
						NULL, NULL, NULL)) {
			case -1:
				if (errno != EINTR) {
					perror("main loop select");
					callback_timeout = TRUE;
				}
				break;

			case 0:
				fprintf(stderr,
				"Invalid timeout in main loop select.\n");
				break;

			default:
				svc_getreq(readfds);
				break;
			} /* switch */
		} /* while */

		alarm(0);
		if (ps->state == SSTAT_CALLED)
			fprintf(stderr,
				"No response from ypxfr on %s\n",
				ps->svc_name);

	} /* for each server */
}

/*
 * This does the listener process cleanup and process exit.
 */
static void
listener_exit(unsigned long program, int stat)
{
	rpcb_unset(program, YPPUSHVERS, (struct netconfig *)NULL);
	exit(stat);
}

/*
 * This is the listener process' RPC service dispatcher.
 */
static void
listener_dispatch(struct svc_req *rqstp, SVCXPRT *transp)
{
	switch (rqstp->rq_proc) {

	case YPPUSHPROC_NULL:
		if (!svc_sendreply(transp, xdr_void, 0)) {
		    fprintf(stderr, "Can't reply to rpc call.\n");
		}
		break;

	case YPPUSHPROC_XFRRESP:
		get_xfr_response(transp);
		break;

	default:
		svcerr_noproc(transp);
		break;
	}
}


/*
 *  This dumps a server state message to stdout.  It is called in cases where
 *  we have no expectation of receiving a callback from the remote ypxfr.
 */
static void
print_state_msg(struct server *s, long e)
{
	struct state_duple *sd;

	if (s->state == SSTAT_SYSTEM)
		return;			/* already printed */

	if (!verbose && (s->state == SSTAT_RESPONDED ||
				s->state == SSTAT_CALLED))
		return;

	for (sd = state_duples; sd->state_msg; sd++) {
		if (sd->state == s->state) {
			printf(sd->state_msg, s->svc_name);

			if (s->state == SSTAT_RPC) {
				rpcerr_msg((enum clnt_stat) e);
			}

			printf("\n");
			fflush(stdout);
			return;
		}
	}

	fprintf(stderr, "yppush: Bad server state value %d.\n", s->state);
}

/*
 *  This dumps a transfer status message to stdout.  It is called in
 *  response to a received RPC message from the called ypxfr.
 */
static void
print_callback_msg(struct server *s)
{
	register struct status_duple *sd;

	if (!verbose &&
		(s->status == YPPUSH_AGE) ||
		(s->status == YPPUSH_SUCC))

		return;

	for (sd = status_duples; sd->status_msg; sd++) {

		if (sd->status == s->status) {
			printf("Status received from ypxfr on %s:\n\t%s\n",
				s->svc_name, sd->status_msg);
			fflush(stdout);
			return;
		}
	}

	fprintf(stderr, "yppush listener: Garbage transaction "
			"status (value %d) from ypxfr on %s.\n",
			(int) s->status, s->svc_name);
}

/*
 *  This dumps an RPC error message to stdout.  This is basically a rewrite
 *  of clnt_perrno, but writes to stdout instead of stderr.
 */
static void
rpcerr_msg(enum clnt_stat e)
{
	struct rpcerr_duple *rd;

	for (rd = rpcerr_duples; rd->rpc_msg; rd++) {

		if (rd->rpc_stat == e) {
			printf(rd->rpc_msg);
			return;
		}
	}

	fprintf(stderr, "Bad error code passed to rpcerr_msg: %d.\n", e);
}

/*
 * This picks up the response from the ypxfr process which has been started
 * up on the remote node.  The response status must be non-zero, otherwise
 * the status will be set to "ypxfr error".
 */
static void
get_xfr_response(SVCXPRT *transp)
{
	struct yppushresp_xfr resp;
	register struct server *s;

	if (!svc_getargs(transp, (xdrproc_t) xdr_yppushresp_xfr,
			(caddr_t) &resp)) {
		svcerr_decode(transp);
		return;
	}

	if (!svc_sendreply(transp, xdr_void, 0)) {
		(void) fprintf(stderr, "Can't reply to rpc call.\n");
	}

	for (s = server_list; s; s = s->pnext) {

		if (s->xactid == resp.transid) {
			s->status  = resp.status ? resp.status: YPPUSH_XFRERR;
			print_callback_msg(s);
			s->state = SSTAT_RESPONDED;
			return;
		}
	}
}

/*
 * This is a UNIX signal handler which is called when the
 * timer expires waiting for a callback.
 */
static void
set_time_up(void)
{
	callback_timeout = TRUE;
}


/*
 * This sends a message to a single ypserv process.  The return value is
 * a state value.  If the RPC call fails because of a version
 * mismatch, we'll assume that we're talking to a version 1 ypserv process,
 * and will send him an old "YPPROC_GET" request, as was defined in the
 * earlier version of yp_prot.h
 */
static unsigned short
send_message(struct server *ps, unsigned long program, long *err)
{
	struct ypreq_newxfr req;
	struct ypreq_xfr oldreq;
	enum clnt_stat s;
	struct rpc_err rpcerr;

	if ((ps->domb.dom_client = __yp_clnt_create_rsvdport(ps->svc_name,
							YPPROG, YPVERS,
							(char *) NULL,
							0, 0))  == NULL) {

		if (rpc_createerr.cf_stat == RPC_PROGNOTREGISTERED) {
			return (SSTAT_PROGNOTREG);
		} else {
			printf("Error talking to %s: ", ps->svc_name);
			rpcerr_msg(rpc_createerr.cf_stat);
			printf("\n");
			fflush(stdout);
			return (SSTAT_SYSTEM);
		}
	}

	if (sysinfo(SI_HOSTNAME, my_name, sizeof (my_name)) == -1) {
		return (SSTAT_RSCRC);
	}

	if (!oldxfr) {
		req.ypxfr_domain = domain;
		req.ypxfr_map = map;
		req.ypxfr_ordernum = 0;
		req.ypxfr_owner = my_name;
		req.name = ps->svc_name;
		/*
		 * the creation of field req.name, instead of ypreq_xfr (old)
		 * req.port, does not make any sense. it doesn't give any
		 * information to receiving ypserv except its own name !!
		 * new ypserv duplicates work for YPPROC_XFR and YPPROC_NEWXFR
		 */
		req.transid = ps->xactid;
		req.proto = program;
		s = (enum clnt_stat) clnt_call(ps->domb.dom_client,
						YPPROC_NEWXFR,
						(xdrproc_t) xdr_ypreq_newxfr,
						(caddr_t) &req,
						xdr_void, 0, timeout);
	}

	clnt_geterr(ps->domb.dom_client, &rpcerr);

	if (s == RPC_PROCUNAVAIL) {
		oldreq.ypxfr_domain = domain;
		oldreq.ypxfr_map = map;
		oldreq.ypxfr_ordernum = 0;
		oldreq.ypxfr_owner = my_name;
		oldreq.transid = ps->xactid;
		oldreq.proto = program;
		oldreq.port = 0;
		s = (enum clnt_stat) clnt_call(ps->domb.dom_client,
						YPPROC_XFR,
						(xdrproc_t) xdr_ypreq_xfr,
						(caddr_t) &oldreq,
						xdr_void, 0, timeout);
		clnt_geterr(ps->domb.dom_client, &rpcerr);
	}

	clnt_destroy(ps->domb.dom_client);

	if (s == RPC_SUCCESS) {
		return (SSTAT_CALLED);
	} else {
		*err = (long) rpcerr.re_status;
		return (SSTAT_RPC);
	}
	/*NOTREACHED*/
}