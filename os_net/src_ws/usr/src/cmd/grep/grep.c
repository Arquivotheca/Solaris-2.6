/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * Copyright (c) 1996, by Sun Microsystems, Inc.
 * All rights reserved.
 */

#ident	"@(#)grep.c	1.20	96/04/18 SMI" 	/* SVr4.0 1.22 */

/*	Copyright (c) 1987, 1988 Microsoft Corporation	*/
/*	  All Rights Reserved	*/

/*	This Module contains Proprietary Information of Microsoft  */
/*	Corporation and should be treated as Confidential.	   */
/*
 * grep -- print lines matching (or not matching) a pattern
 *
 *	status returns:
 *		0 - ok, and some matches
 *		1 - ok, but no matches
 *		2 - some error
 */

#include <stdio.h>
#include <ctype.h>
#include <memory.h>
#include <regexpr.h>
#include <sys/types.h>
#include <locale.h>
#include <fcntl.h>
#include <unistd.h>

#ifdef __STDC__
const char errstr[][64] = {
#else
char errstr[][64] = {
#endif
	"Range endpoint too large.",
	"Bad number.",
	"``\\digit'' out of range.",
	"No remembered search string.",
	"\\( \\) imbalance.",
	"Too many \\(.",
	"More than 2 numbers given in \\{ \\}.",
	"} expected after \\.",
	"First number exceeds second in \\{ \\}.",
	"[ ] imbalance.",
	"Regular expression overflow.",
	"Illegal byte sequence.",
	"Unknown regexp error code!!",
	NULL
};

#define	errmsg(msg, arg)	fprintf(stderr, gettext(msg), arg)
#define	BLKSIZE	512

char	*strrchr();
int	temp;
long long lnum;
char	linebuf[2*BUFSIZ];
char	prntbuf[2*BUFSIZ];
int	nflag;
int	bflag;
int	lflag;
int	cflag;
int	vflag;
int	sflag;
int	iflag;
int	wflag;
int	hflag;
int	errflg;
int	nfile;
long long tln;
int	nsucc;
int	nlflag;
char	*ptr, *ptrend;
char *expbuf;

main(argc, argv)
register argc;
char **argv;
{
	register	c;
	register char	*arg;
	extern int	optind;

	(void) setlocale(LC_ALL, "");
#if !defined(TEXT_DOMAIN)	/* Should be defined by cc -D */
#define	TEXT_DOMAIN "SYS_TEST"	/* Use this only if it weren't */
#endif
	(void) textdomain(TEXT_DOMAIN);

	while ((c = getopt(argc, argv, "hblcnsviyw")) != -1)
		switch (c) {
		case 'h':
			hflag++;
			break;
		case 'v':
			vflag++;
			break;
		case 'c':
			cflag++;
			break;
		case 'n':
			nflag++;
			break;
		case 'b':
			bflag++;
			break;
		case 's':
			sflag++;
			break;
		case 'l':
			lflag++;
			break;
		case 'y':
		case 'i':
			iflag++;
			break;
		case 'w':
			wflag++;
			break;
		case '?':
			errflg++;
		}

	if (errflg || (optind >= argc)) {
		errmsg("Usage: grep -hblcnsviw pattern file . . .\n",
			(char *)NULL);
		exit(2);
	}

	argv = &argv[optind];
	argc -= optind;
	nfile = argc - 1;

	if (strrchr(*argv, '\n'))
		regerr(41);

	if (iflag) {
		for (arg = *argv; *arg != NULL; ++arg)
			*arg = (char)tolower((int)((unsigned char)*arg));
	}

	if (wflag) {
		unsigned int	wordlen;
		char		*wordbuf;
		register char	*wp;
		char		*malloc();

		wordlen = strlen(*argv) + 4;
		if ((wordbuf = malloc(wordlen)) == NULL) {
			errmsg("grep: Out of memory for word\n", (char *)NULL);
			exit(2);
		}
		wp = wordbuf;
		(void) strcpy(wordbuf, "\\<");
		(void) strcat(wordbuf, *argv);
		(void) strcat(wordbuf, "\\>");
		*argv = wordbuf;
	}

	expbuf = compile(*argv, (char *)0, (char *)0);
	if (regerrno)
		regerr(regerrno);

	if (--argc == 0)
		execute(NULL);
	else
		while (argc-- > 0)
			execute(*++argv);

	exit(nsucc == 2 ? 2 : nsucc == 0);
}

execute(file)
register char *file;
{
	register char *lbuf, *p;
	int count, count1;

	if (file == NULL)
		temp = 0;
	else if ((temp = open(file, O_RDONLY)) == -1) {
		if (!sflag)
			errmsg("grep: can't open %s\n", file);
		nsucc = 2;
		return;
	}
	/* read in first block of bytes */
	if ((count = read(temp, prntbuf, BUFSIZ)) <= 0) {
		close(temp);

		if (cflag) {
			if (nfile > 1 && !hflag && file)
				fprintf(stdout, "%s:", file);
			fprintf(stdout, "%lld\n", tln);
		}
		return;
	}

	lnum = 0;
	tln = 0;
	ptr = prntbuf;
	for (;;) {
		/* look for next newline */
		if ((ptrend = memchr(ptr, '\n', prntbuf + count - ptr))
			== NULL) {
			count = prntbuf + count - ptr;
			if (count <= BUFSIZ) {
				/*
				 * shift end of block to beginning of buffer
				 * if necessary
				 * and fill up buffer until newline
				 * is found
				 */
				if (ptr != prntbuf)
				/*
				 * assumes memcpy copies correctly with
				 * overlap
				 */
					memmove(prntbuf, ptr, count);
				p = prntbuf + count;
				ptr = prntbuf;
			} else {
				/*
				 * No newline in current block.
				 * Throw it away and get next
				 * block.
				 */
				count = 0;
				ptr = p = prntbuf;
			}
			if ((count1 = read(temp, p, BUFSIZ)) > 0) {
				count += count1;
				continue;
			}
			/* end of file - last line has no newline */
			ptrend = ptr + count;
			nlflag = 0;
		} else
			nlflag = 1;
		lnum++;
		*ptrend = '\0';
		if (iflag) {
			p = ptr;
			for (lbuf = linebuf; p < ptrend; )
				*lbuf++ = (char)tolower((int)
						(unsigned char)*p++);
			*lbuf = '\0';
			lbuf = linebuf;
		} else
			lbuf = ptr;

		if ((step(lbuf, expbuf) ^ vflag) && succeed(file) == 1)
			break;	/* lflag only once */
		if (!nlflag)
			break;
		ptr = ptrend + 1;
		if (ptr >= prntbuf + count) {
			/* at end of block; read in another block */
			ptr = prntbuf;
			if ((count = read(temp, prntbuf, BUFSIZ)) <= 0)
				break;
		}
	}
	close(temp);

	if (cflag) {
		if (nfile > 1 && !hflag && file)
			fprintf(stdout, "%s:", file);
		fprintf(stdout, "%lld\n", tln);
	}
}

succeed(f)
register char *f;
{
	int nchars;
	nsucc = (nsucc == 2) ? 2 : 1;
	if (f == NULL)
		f = "<stdin>";
	if (cflag) {
		tln++;
		return (0);
	}
	if (lflag) {
		fprintf(stdout, "%s\n", f);
		return (1);
	}

	if (nfile > 1 && !hflag)	/* print filename */
		fprintf(stdout, "%s:", f);

	if (bflag)	/* print block number */
		fprintf(stdout, "%lld:", (lseek(temp, (off_t)0,
			SEEK_CUR)-1)/BLKSIZE);

	if (nflag)	/* print line number */
		fprintf(stdout, "%lld:", lnum);
	if (nlflag) {
		/* newline at end of line */
		*ptrend = '\n';
		nchars = ptrend - ptr + 1;
	} else
		nchars = ptrend - ptr;
	fwrite(ptr, 1, nchars, stdout);
	return (0);
}

regerr(err)
register err;
{
	errmsg("grep: RE error %d: ", err);
	switch (err) {
		case 11:
			err = 0;
			break;
		case 16:
			err = 1;
			break;
		case 25:
			err = 2;
			break;
		case 41:
			err = 3;
			break;
		case 42:
			err = 4;
			break;
		case 43:
			err = 5;
			break;
		case 44:
			err = 6;
			break;
		case 45:
			err = 7;
			break;
		case 46:
			err = 8;
			break;
		case 49:
			err = 9;
			break;
		case 50:
			err = 10;
			break;
		case 67:
			err = 11;
			break;
		default:
			err = 12;
			break;
	}

	errmsg("%s\n", gettext(errstr[err]));
	exit(2);
}
