/*	Co/pyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved. The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
     
/*
 * Copyright (c) 1983, 1984 1985, 1986, 1987, 1988, Sun Microsystems, Inc.
 * All Rights Reserved.
 */
  
#ident	"@(#)t1.c	1.6	96/02/22 SMI"	/* SVr4.0 1.1	*/

 /* t1.c: main control and input switching */
#
#include <locale.h>
# include "t..c"
#include <signal.h>
# ifdef gcos
/* required by GCOS because file is passed to "tbl" by troff preprocessor */
# define _f1 _f
extern FILE *_f[];
# endif

# ifdef unix
# define MACROS "/usr/doctools/tmac/tmac.s"
# define MACROSS "/usr/share/lib/tmac/s"
# define PYMACS "/usr/doctools/tmac/tmac.m"
# define PYMACSS "/usr/share/lib/tmac/m"
# define MEMACSS "/usr/share/lib/tmac/e"
# endif

# ifdef gcos
# define MACROS "cc/troff/smac"
# define PYMACS "cc/troff/mmac"
# endif

# define ever (;;)

main(argc,argv)
	char *argv[];
{
# ifdef unix
void badsig();
# endif
	(void) setlocale(LC_ALL, "");
#if !defined(TEXT_DOMAIN)
#define TEXT_DOMAIN "SYS_TEST"
#endif
	(void) textdomain(TEXT_DOMAIN);
# ifdef unix
signal(SIGPIPE, badsig);
# endif
# ifdef gcos
if(!intss()) tabout = fopen("qq", "w"); /* default media code is type 5 */
# endif
exit(tbl(argc,argv));
}


tbl(argc,argv)
	char *argv[];
{
char line[BUFSIZ];
/* required by GCOS because "stdout" is set by troff preprocessor */
tabin=stdin; tabout=stdout;
setinp(argc,argv);
while (gets1(line, sizeof line))
	{
	fprintf(tabout, "%s\n",line);
	if (prefix(".TS", line))
		tableput();
	}
fclose(tabin);
return(0);
}
int sargc;
char **sargv;
setinp(argc,argv)
	char **argv;
{
	sargc = argc;
	sargv = argv;
	sargc--; sargv++;
	if (sargc>0)
		swapin();
}
swapin()
{
	while (sargc>0 && **sargv=='-') /* Mem fault if no test on sargc */
		{
		if (sargc<=0) return(0);
		if (match("-me", *sargv))
			{
			*sargv = MEMACSS;
			break;
			}
		if (match("-ms", *sargv))
			{
			*sargv = MACROSS;
			break;
			}
		if (match("-mm", *sargv))
			{
			*sargv = PYMACSS;
			break;
			}
		if (match("-TX", *sargv))
			pr1403=1;
		else
			fprintf(stderr,"tbl: Invalid option (%s) ignored.\n",
				*sargv);
		sargc--; sargv++;
		}
	if (sargc<=0) return(0);
# ifdef unix
/* file closing is done by GCOS troff preprocessor */
	if (tabin!=stdin) fclose(tabin);
# endif
	tabin = fopen(ifile= *sargv, "r");
	iline=1;
# ifdef unix
/* file names are all put into f. by the GCOS troff preprocessor */
	fprintf(tabout, ".ds f. %s\n",ifile);
# endif
	if (tabin==NULL)
		error(gettext("Can't open file"));
	sargc--;
	sargv++;
	return(1);
}
# ifdef unix
void badsig()
{
signal(SIGPIPE, SIG_IGN);
 exit(0);
}
# endif
