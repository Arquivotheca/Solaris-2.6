/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)getpass.c	1.8	96/05/15 SMI"	/* SVr4.0 1.4	*/

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
 *		All rights reserved.
 *
 */


#include "ftp_var.h"

static	struct sgttyb ttyb;
static	int flags;
static	FILE *fi;
static	int ttcompat;

#define	signal(s, f)	sigset(s, f)

static void
intfix(void)
{
	ttyb.sg_flags = flags;
	if (fi != NULL)
		(void) ioctl(fileno(fi), TIOCSETP, &ttyb);
	if (ttcompat)
		(void) ioctl(fileno(fi), I_POP, "ttcompat");
	exit(SIGINT);
}

char *
mygetpass(char *prompt)
{
	register char *p;
	register int c;
	static char pbuf[50+1];
	void (*sig)();

	if ((fi = fopen("/dev/tty", "r")) == NULL)
		fi = stdin;
	else
		setbuf(fi, (char *)NULL);

	sig = signal(SIGINT, (void (*)())intfix);
	c = ioctl(fileno(fi), I_FIND, "ttcompat");
	if (c == 0) {
		if (ioctl(fileno(fi), I_PUSH, "ttcompat") < 0) {
			perror("ftp: ioctl I_PUSH ttcompat");
		}
		ttcompat = 1;
	} else if (c < 0)
		perror("ftp: ioctl I_FIND ttcompat");

	if (ioctl(fileno(fi), TIOCGETP, &ttyb) < 0)
		perror("ftp: ioctl(TIOCGETP)");	/* go ahead, anyway */
	flags = ttyb.sg_flags;
	ttyb.sg_flags &= ~ECHO;
	(void) ioctl(fileno(fi), TIOCSETP, &ttyb);
	fprintf(stderr, "%s", prompt); (void) fflush(stderr);
	p = pbuf;
	while ((c = getc(fi)) != '\n' && c != EOF) {
		if (p < &pbuf[sizeof (pbuf)-1])
			*p++ = c;
	}
	*p = '\0';
	fprintf(stderr, "\n"); (void) fflush(stderr);
	ttyb.sg_flags = flags;
	(void) ioctl(fileno(fi), TIOCSETP, &ttyb);
	(void) signal(SIGINT, sig);
	if (ttcompat)
		(void) ioctl(fileno(fi), I_POP, "ttcompat");
	if (fi != stdin)
		(void) fclose(fi);
	return (pbuf);
}
