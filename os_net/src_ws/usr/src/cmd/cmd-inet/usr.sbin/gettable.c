/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)gettable.c	1.6	93/12/20 SMI"	/* SVr4.0 1.2	*/

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 * 	          All rights reserved.
 *  
 */


#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>

#include <stdio.h>
#include <netdb.h>

#define	OUTFILE		"hosts.txt"	/* default output file */
#define	VERFILE		"hosts.ver"	/* default version file */
#define	QUERY		"ALL\r\n"	/* query to hostname server */
#define	VERSION		"VERSION\r\n"	/* get version number */

#define	equaln(s1, s2, n)	(!strncmp(s1, s2, n))

#ifdef SYSV
#define bcopy(a,b,c)	  memcpy(b,a,c)
#endif

struct	sockaddr_in sin;
struct	sockaddr_in sintmp;
char	buf[BUFSIZ];
char	*outfile = OUTFILE;

main(argc, argv)
	int argc;
	char *argv[];
{
	int s;
	register len;
	register FILE *sfi, *sfo, *hf;
	char *host;
	register struct hostent *hp;
	struct servent *sp;
	int version = 0;
	int beginseen = 0;
	int endseen = 0;

	argv++, argc--;
	if (argc > 0 && **argv == '-') {
		if (argv[0][1] != 'v')
			fprintf(stderr, "unknown option %s ignored\n", *argv);
		else
			version++, outfile = VERFILE;
		argv++, argc--;
	}
	if (argc < 1 || argc > 2) {
		fprintf(stderr, "usage: gettable [-v] host [ file ]\n");
		exit(1);
	}
	sp = getservbyname("hostnames", "tcp");
	if (sp == NULL) {
		fprintf(stderr, "gettable: hostnames/tcp: unknown service\n");
		exit(3);
	}
	host = *argv;
	argv++, argc--;
	sintmp.sin_addr.s_addr = inet_addr(host);
	if (sintmp.sin_addr.s_addr != -1 && sintmp.sin_addr.s_addr != 0) {
		sin.sin_family = AF_INET;
	} else {
		hp = gethostbyname(host);
		if (hp == NULL) {
			fprintf(stderr, "gettable: %s: host unknown\n", host);
			exit(2);
		} else {
			sin.sin_family = hp->h_addrtype;
			host = hp->h_name;
		}
	}
	if (argc > 0)
		outfile = *argv;
	s = socket(sin.sin_family, SOCK_STREAM, 0);
	if (s < 0) {
		perror("gettable: socket");
		exit(4);
	}
	if (bind(s, (struct sockaddr *)&sin, sizeof (sin)) < 0) {
		perror("gettable: bind");
		exit(5);
	}
	if (sintmp.sin_addr.s_addr != -1 && sintmp.sin_addr.s_addr != 0)
		bcopy((char *)&sintmp.sin_addr, (char *)&sin.sin_addr,
			sizeof(sintmp.sin_addr));
	else
		bcopy(hp->h_addr, (char *)&sin.sin_addr, hp->h_length);
	sin.sin_port = sp->s_port;
	if (connect(s, (struct sockaddr *)&sin, sizeof (sin)) < 0) {
		perror("gettable: connect");
		exit(6);
	}
	fprintf(stderr, "Connection to %s opened.\n", host);
	sfi = fdopen(s, "r");
	sfo = fdopen(s, "w");
	if (sfi == NULL || sfo == NULL) {
		perror("gettable: fdopen");
		close(s);
		exit(1);
	}
	hf = fopen(outfile, "w");
	if (hf == NULL) {
		fprintf(stderr, "gettable: "); perror(outfile);
		close(s);
		exit(1);
	}
	fprintf(sfo, version ? VERSION : QUERY);
	fflush(sfo);
	while (fgets(buf, sizeof(buf), sfi) != NULL) {
		len = strlen(buf);
		buf[len-2] = '\0';
		if (!version && equaln(buf, "BEGIN", 5)) {
			if (beginseen || endseen) {
				fprintf(stderr,
				    "gettable: BEGIN sequence error\n");
				exit(90);
			}
			beginseen++;
			continue;
		}
		if (!version && equaln(buf, "END", 3)) {
			if (!beginseen || endseen) {
				fprintf(stderr,
				    "gettable: END sequence error\n");
				exit(91);
			}
			endseen++;
			continue;
		}
		if (equaln(buf, "ERR", 3)) {
			fprintf(stderr,
			    "gettable: hostname service error: %s", buf);
			exit(92);
		}
		fprintf(hf, "%s\n", buf);
	}
	fclose(hf);
	if (!version) {
		if (!beginseen) {
			fprintf(stderr, "gettable: no BEGIN seen\n");
			exit(93);
		}
		if (!endseen) {
			fprintf(stderr, "gettable: no END seen\n");
			exit(94);
		}
		fprintf(stderr, "Host table received.\n");
	} else
		fprintf(stderr, "Version number received.\n");
	close(s);
	fprintf(stderr, "Connection to %s closed\n", host);
	exit(0);
	/* NOTREACHED */
}
