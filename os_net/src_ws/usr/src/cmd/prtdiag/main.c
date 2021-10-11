/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma	ident	"@(#)main.c	1.17	95/10/16 SMI"	/* SVr4.0 1.7 */

#include	<stdio.h>
#include	<locale.h>
#include	<stdlib.h>
#include	<libintl.h>
#include 	<string.h>
#include	<unistd.h>
#include 	<sys/openpromio.h>
#include	"pdevinfo.h"

char		*progname;
char		*promdev = "/dev/openprom";
int		logging = 0;
int		print_flag = 1;

static char *usage = "%s [ -v ] [ -l ]\n";

static void setprogname(char *);

void
main(int argc, char *argv[])
{
	int	c;
	int	syserrlog = 0;

	/* set up for internationalization */
	(void) setlocale(LC_ALL, "");
	(void) textdomain(TEXT_DOMAIN);

	setprogname(argv[0]);
	while ((c = getopt(argc, argv, "vl")) != -1)  {
		switch (c)  {
		case 'v':
			++syserrlog;
			break;

		case 'l':
			logging = 1;
			break;

		default:
			(void) fprintf(stderr, usage, progname);
			exit(1);
			/*NOTREACHED*/
		}
	}

	exit(do_prominfo(syserrlog));
} 	/* main */

static void
setprogname(char *name)
{
	char *p;

	if (p = strrchr(name, '/'))
		progname = p + 1;
	else
		progname = name;
}
