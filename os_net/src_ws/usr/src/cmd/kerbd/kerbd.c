#ident	"@(#)kerbd.c	1.4	96/04/04 SMI"
/*
 *  Usermode daemon which assists the kernel when handling kerberos ticket
 *  generation and validation.  It is kerbd which actually communicates
 *  with the kerberos KDC.
 *
 *  Copyright 1990,1991 Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <rpc/rpc.h>
#include <sys/syslog.h>
#include <sys/termios.h>
#include <sys/unistd.h>
#include <sys/resource.h>
#include <sys/utsname.h>
#include <sys/systeminfo.h>
#include <stdlib.h>
#include <stropts.h>
#include <fcntl.h>
#include "kerbd.h"

int kerbd_debug = 0;		/* enable debugging printfs */
int dogrouplist = 1;		/* return user's grouplist with uid/gid */
long sysmaxgroups = 0;		/* max number of groups from sys viewpoint */

void kerbprog_4();

/* following declarations needed in rpcgen-generated code */
int _rpcpmstart = 0;		/* Started by a port monitor ? */
int _rpcfdtype;			/* Whether Stream or Datagram ? */
int _rpcsvcdirty;		/* Still serving ? */

main(argc, argv)
int argc;
char **argv;
{
	register SVCXPRT *transp;
	extern char *optarg;
	extern int optind;
	int c, ret;
	char realm[REALM_SZ];
	char mname[FMNAMESZ + 1];
	char *netid;
	struct netconfig *nconf = NULL;

#ifdef DEBUG
	(void) setuid(0);		/* DEBUG: set ruid to root */
#endif DEBUG
	if (getuid()) {
		(void) fprintf(stderr, "%s must be run as root\n", argv[0]);
#ifdef DEBUG
		(void) fprintf(stderr, " [warning only]\n");
#else !DEBUG
		exit(1);
#endif DEBUG
	}

	while ((c = getopt(argc, argv, "dg")) != -1)
		switch (c) {
		    case 'd':
			/* turn on debugging */
			kerbd_debug = 1;
			break;
		    case 'g':
			/* don't get users' grouplist */
			dogrouplist = 0;
			break;
		    default:
			usage();
		}

	if (optind != argc) {
		usage();
	}

	/*
	 * Started by inetd if name of module just below stream
	 * head is either a sockmod or timod.
	 */
	if (!ioctl(0, I_LOOK, mname) &&
		((strcmp(mname, "sockmod") == 0) ||
			(strcmp(mname, "timod") == 0))) {

		openlog("kerbd", LOG_PID, LOG_DAEMON);

		if ((netid = getenv("NLSPROVIDER")) ==  NULL) {
			netid = "ticlts";
		}
		if ((nconf = getnetconfigent(netid)) == NULL)
			syslog(LOG_ERR, "cannot get transport info");

		if (strcmp(mname, "sockmod") == 0) {
			if (ioctl(0, I_POP, 0) || ioctl(0, I_PUSH, "timod")) {
				syslog(LOG_ERR,
					"could not get the right module");
				exit(1);
			}
		}

		if ((transp = svc_tli_create(0, nconf, NULL, 0, 0)) == NULL) {
			syslog(LOG_ERR, "cannot create server handle");
			exit(1);
		}

		if (!svc_reg(transp, KERBPROG, KERBVERS, kerbprog_4, 0)) {
			syslog(LOG_ERR,
				"unable to register (KERBPROG,KERBVERS).");
			exit(1);
		}

		if (__rpc_negotiate_uid(transp->xp_fd))
			syslog(LOG_ERR,
			"could not negotiate with loopback transport\n");
		if (nconf)
			freenetconfigent(nconf);
	} else {
		if (!kerbd_debug)
			detachfromtty();

		openlog("kerbd", LOG_PID, LOG_DAEMON);
		if (svc_create_local_service(kerbprog_4, KERBPROG, KERBVERS,
						"netpath", "kerbd") == 0) {
			syslog(LOG_ERR, "unable to create service");
			exit(1);
		}
	}

	if (dogrouplist) {
		if ((sysmaxgroups = sysconf(_SC_NGROUPS_MAX)) <= 0)
			sysmaxgroups = 0;
	}

	/* set the local realm name into the kernel */
	if (krb_get_lrealm(realm, 1) != KSUCCESS) {
		syslog(LOG_ERR, "unable to get kerberos realm");
		exit(1);
	}
	ret = sysinfo(SI_SET_KERB_REALM, realm, strlen(realm + 1));
	if (ret < 0) {
		syslog(LOG_ERR, "unable to set realm with sysinfo (error %d)",
			ret);
		exit(1);
	}

	if (kerbd_debug) {
		fprintf(stderr,
		    "kerbd start: realm `%s' dogrouplist %d sysmaxgroups %d\n",
			    realm, dogrouplist, sysmaxgroups);
	}

	svc_run();
	abort();
	/* NOTREACHED */
}

static
usage()
{
	(void) fprintf(stderr, "usage: kerbd [-dg]\n");
	exit(1);
}


/*
 * detach from tty
 */
detachfromtty()
{
	register int i;
	struct rlimit rl;

	switch (fork()) {
	case -1:
		perror("kerbd: can't fork");
		exit(1);
	case 0:
		break;
	default:
		exit(0);
	}

	/*
	 * Close existing file descriptors, open "/dev/null" as
	 * standard input, output, and error, and detach from
	 * controlling terminal.
	 */
	getrlimit(RLIMIT_NOFILE, &rl);
	for (i = 0; i < rl.rlim_max; i++)
		(void) close(i);
	(void) open("/dev/null", O_RDONLY);
	(void) open("/dev/null", O_WRONLY);
	(void) dup(1);
	(void) setsid();
}
