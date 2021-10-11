#ident "@(#)bpgetfile.c 1.4 94/07/21 SMI"

/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 */

/*
 * get-bpgetfile
 *
 * perform a bootparams.getfile lookup
 *  return the result on 'stdout' so it can be used from within scripts
 *  do it in a transport-independent way
 *   (even though bootparams is currently always UDP)
 *  do it even though /usr may not be available yet
 *   (e.g. "statically linked")
 *
 * To be both transport independent and statically linked is hard.  
 * The current method of doing this is with a very lon link line that
 * includes -ldl_stubs as well as a whole slew of other libraries.  
 * This results in an executable that's ~1/2MByte!
 *
 * To "provoke" a usage message, just execute something like
 * get-bpgetfile -help
 * The usage message describes features like -debug and -retries.  
 *
 * This program is set up to make debugging easy even when run in a harsh
 * environment (e.g. no /usr mounted).  The -debug flag makes lots of
 * useful information come out of 'stderr' (where it won't get in the
 * way of variable setting that's going on via 'stdout').  
 */

#include <stdio.h>
#include <rpc/rpc.h>
#include <rpcsvc/bootparam.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/utsname.h>
#include <sys/systeminfo.h>

/* this gives a total time of 28 seconds (4 + 8 + 16) */
#define DEFAULT_RETRIES 2
static int inittime = 4000;	/* Time to wait initially */
static int waittime = 16000;	/* Maximum time to wait */
#define DEFAULT_SERVER BROADCAST
#define DEFAULT_KEY "root"

#ifndef UDPONLY
#define PROTOCOLS "datagram_v"
#else
#define PROTOCOLS "udp"
#endif

#define BROADCAST "broadcast"
#ifdef SYS_NMLN
#define SHORTBUF SYS_NMLN
#else
#define SHORTBUF 257
#endif
#define LONGBUF 1025
#ifndef CLCR_SET_LOWVERS
#define CLCR_SET_LOWVERS 3
#endif

/* PrintNull */
#define PN(string) ((string) ? (string) : "(null)")
/* UseNull */
#define UN(string) ((string) ? (string) : "")

static char *progname = "";
static int debug = 0;
static int maxretries = DEFAULT_RETRIES;
/* If even on reply is found, set this */
static int found = 0; 
void
print_bp_getfile_arg(bp_getfile_arg *in)
{
	if (in == NULL) {
		fprintf(stderr, "NULL bp_getfile_arg\n");
	} else {
		fprintf(stderr, "bp_getfile_arg.client_name: %s\n",
		    PN(in->client_name));
		fprintf(stderr, "bp_getfile_arg.file_id: %s\n",
		    PN(in->file_id));
	}

	fflush(stdout);
	/* set if reply found */
	sleep(1);
	return;
}

void
print_bp_getfile_res(bp_getfile_res *out)
{
	if (out == NULL) {
		fprintf(stderr, "NULL bp_getfile_res\n");
	} else {
		fprintf(stderr, "bp_getfile_res.server_name: %s\n",
		    PN(out->server_name));
		fprintf(stderr,
    "bp_getfile_res.server_address: address_type = %d, address = %d.%d.%d.%d\n",
		    out->server_address.address_type,
		    (int) (*((unsigned char *)
			&(out->server_address.bp_address_u.ip_addr.net))), 
		    (int) (*((unsigned char *)
			&(out->server_address.bp_address_u.ip_addr.host))), 
		    (int) (*((unsigned char *)
			&(out->server_address.bp_address_u.ip_addr.lh))), 
		    (int) (*((unsigned char *)
			&(out->server_address.bp_address_u.ip_addr.impno))));
		fprintf(stderr, "bp_getfile_res.server_path: %s\n",
		    PN(out->server_path));
	}

	fflush(stdout);
	sleep(1);
	return;
}

void
usage()
{
	fprintf(stderr, "usage: %s [-debug] [-retries <max>]\n", progname);
	fprintf(stderr, "\t[<keyword> [<nameserver> [<clientname>]]]\n");
	fprintf(stderr, "  default is no debug\n");
	fprintf(stderr, "  default <max> number of retries is %d\n",
		DEFAULT_RETRIES);
	fprintf(stderr, "  <keyword> defaults to %s\n", DEFAULT_KEY);
	fprintf(stderr,
	"  <nameserver> defaults to %s (%s means 'any host on local net')\n",
	    DEFAULT_SERVER, BROADCAST);
	fprintf(stderr, "  <clientname> defaults to the executing host\n");
	fprintf(stderr,
	" returns on stdout if successful: servername serverIPaddr path\n");
	exit(1);
}

static bp_getfile_arg getfile_in = { 0 };
static bp_getfile_res getfile_out = { 0 };
static char servername[SHORTBUF];
static char serverIPaddr[SHORTBUF];
static char path[LONGBUF];

int
each_bp_getfile_res(bp_getfile_res *out, struct sockaddr_in *addr,
	struct netconfig *nconf)
{
	if (debug) {
		fprintf(stderr, "answer returned from %d.%d.%d.%d:\n",
		    addr->sin_addr.S_un.S_un_b.s_b1,
		    addr->sin_addr.S_un.S_un_b.s_b2,
		    addr->sin_addr.S_un.S_un_b.s_b3,
		    addr->sin_addr.S_un.S_un_b.s_b4);
		print_bp_getfile_res(out);
	}
	strcpy(servername, UN(out->server_name));
	strcpy(path, UN(out->server_path));
	if (strlen(servername) || strlen(path))
		sprintf(serverIPaddr, "%d.%d.%d.%d", 
		    (int) (*((unsigned char *)
			&(out->server_address.bp_address_u.ip_addr.net))), 
		    (int) (*((unsigned char *)
			&(out->server_address.bp_address_u.ip_addr.host))), 
		    (int) (*((unsigned char *)
			&(out->server_address.bp_address_u.ip_addr.lh))), 
		    (int) (*((unsigned char *)
			&(out->server_address.bp_address_u.ip_addr.impno))));
	else
		strcpy(serverIPaddr, "");
	
	found = 1; 
	return(1);
}

main(int argc, char *argv[])
{
	int rc;
	char *clientname;
	char *keyword;
	char *nameserver;
	int retried, i;


	progname = strrchr(argv[0], '/');
	if (progname == NULL)
		progname = argv[0];

	while ((argc >= 2) && (argv[1][0] == '-')) {
		switch(argv[1][1]) {
		case 'd':
		case 'D':
			++debug;
			++argv;
			-- argc;
			break;
		case 'r':
		case 'R':
			if (argc < 3)
				usage();
			maxretries = atoi(argv[2]);

        		for (i = 0, waittime = inittime; i < maxretries; i++)
		                waittime += waittime;
			++argv;
			++argv;
			--argc;
			--argc;
			break;
		default:
			usage();
		}
	}
	if ((argc > 4) || ((argc >= 2) && (strlen(argv[1]) != 0)
	    && ((argv[1][0] == '?') || (argv[1][0] == '-')
	    || ((argv[1][0] < 'A'))))) 
		usage();

	if ((argc < 2) || (argv[1] == NULL) || (strlen(argv[1]) <= (size_t)0)) 
		keyword = DEFAULT_KEY;
	else
		keyword = argv[1];

	if ((argc < 3) || (argv[2] == NULL) || (strlen(argv[2]) <= (size_t)0)) 
		nameserver = DEFAULT_SERVER;
	else
		nameserver = argv[2];

	if ((argc < 4) || (argv[3] == NULL) || (strlen(argv[3]) <= (size_t)0)) {
		clientname = (char *)malloc(SHORTBUF);
		sysinfo(SI_HOSTNAME, clientname, SHORTBUF);
	} else {
		clientname = argv[3];
	}


	retried = 0;
	getfile_in.client_name = clientname;
	getfile_in.file_id = keyword;

	if (strcmp(nameserver, BROADCAST) == 0) {
		int val = 1;
		if (debug) {
			fprintf(stderr,
"going to call broadcast.BOOTPARAMPROG.BOOTPARAMVERS.BOOTPARAMPROC_GETFILE\n");
			fprintf(stderr, "  = broadcast.%d.%d.%d with: \n",
			    BOOTPARAMPROG, BOOTPARAMVERS,
			    BOOTPARAMPROC_GETFILE);
			print_bp_getfile_arg(&getfile_in);
		}

		/* 
		 * Broadcast only using version 2, since version 3
		 * may cause broadcast storm
		 */
		(void) __rpc_control(CLCR_SET_LOWVERS, (void *)&val);

		rc = rpc_broadcast_exp(BOOTPARAMPROG,
		    BOOTPARAMVERS, BOOTPARAMPROC_GETFILE, 
		    xdr_bp_getfile_arg, (caddr_t) &getfile_in,
		    xdr_bp_getfile_res, (caddr_t) &getfile_out,
		    (resultproc_t) each_bp_getfile_res,
		    inittime, waittime,
		    PROTOCOLS);
		/* no reply, try version 3 */
		/* never use version 3, since there doesn't appear
		 * to be any version 3 only systems and this will
		 * cause a broadcast storm for 4.x and other vendors
		 * systems.
		 */
/*
		if (!found) {
			val = 0;
			__rpc_control(CLCR_SET_LOWVERS, (void *)&val);
			rc = rpc_broadcast(BOOTPARAMPROG,
				BOOTPARAMVERS, BOOTPARAMPROC_GETFILE, 
				xdr_bp_getfile_arg, &getfile_in,
				xdr_bp_getfile_res, &getfile_out,
				each_bp_getfile_res, PROTOCOLS);
		}
*/

		if (debug) {
			fprintf(stderr, "rpc_broadcast returned rc = %d = %s\n",
			    rc, clnt_sperrno(rc));
			fprintf(stderr, "returned arg structure: \n");
			print_bp_getfile_res(&getfile_out);
		}
	} else {
		do {
			if (debug) {
				fprintf(stderr,
"going to call %s.BOOTPARAMPROG.BOOTPARAMVERS.BOOTPARAMPROC_GETFILE\n",
				    nameserver);
				fprintf(stderr, "  = %s.%d.%d.%d with: \n",
				    nameserver,
				    BOOTPARAMPROG, BOOTPARAMVERS,
				    BOOTPARAMPROC_GETFILE);
				print_bp_getfile_arg(&getfile_in);
			}
			rc = rpc_call(nameserver, BOOTPARAMPROG,
			    BOOTPARAMVERS, BOOTPARAMPROC_GETFILE, 
			    xdr_bp_getfile_arg, (caddr_t) &getfile_in,
			    xdr_bp_getfile_res, (caddr_t) &getfile_out,
			    "datagram_v");
			if (debug) {
				fprintf(stderr,
				    "rpc_call returned rc = %d = %s\n",
				    rc, clnt_sperrno(rc));
				fprintf(stderr, "returned arg structure: \n");
				print_bp_getfile_res(&getfile_out);
			}
			strcpy(servername, UN(getfile_out.server_name));
			strcpy(path, UN(getfile_out.server_path));
			if (strlen(servername) || strlen(path))
				sprintf(serverIPaddr, "%d.%d.%d.%d", 
				    (int) (*((unsigned char *)
		&(getfile_out.server_address.bp_address_u.ip_addr.net))), 
				    (int) (*((unsigned char *)
		&(getfile_out.server_address.bp_address_u.ip_addr.host))), 
				    (int) (*((unsigned char *)
		&(getfile_out.server_address.bp_address_u.ip_addr.lh))), 
				    (int) (*((unsigned char *)
		&(getfile_out.server_address.bp_address_u.ip_addr.impno))));
			else
				strcpy(serverIPaddr, "");
		} while ((rc != RPC_SUCCESS) && (++retried < maxretries));
	}

	printf("%s %s %s\n",
	    servername,
	    serverIPaddr,
	    path);

	exit(rc);
}
