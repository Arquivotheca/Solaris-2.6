/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * Copyright (c) 1985 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ident	"@(#)getusershell.c	1.14	96/10/01 SMI"	/* SVr4.0 1.2	*/

#include "synonyms.h"
#include <sys/types.h>
#include <sys/param.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <ctype.h>
#include <stdio.h>
#include <limits.h>

#define	SHELLS "/etc/shells"

/*
 * Do not add local shells here.  They should be added in /etc/shells
 */
#ifdef __STDC__
const char *okshells[] =
#else
static char *okshells[] =
#endif
	{ "/usr/bin/sh", "/usr/bin/csh", "/usr/bin/ksh", "/usr/bin/jsh",
	"/bin/sh",     "/bin/csh",     "/bin/ksh",     "/bin/jsh",
	"/sbin/sh",	"/sbin/jsh",	0 };

static char **shells, *strings;
static char **curshell;
extern char **initshells();

/*
 * Get a list of shells from SHELLS, if it exists.
 */
char *
getusershell()
{
	char *ret;

	if (curshell == NULL)
		curshell = initshells();
	ret = *curshell;
	if (ret != NULL)
		curshell++;
	return (ret);
}

endusershell()
{

	if (shells != NULL)
		free((char *)shells);
	shells = NULL;
	if (strings != NULL)
		free(strings);
	strings = NULL;
	curshell = NULL;
}

setusershell()
{

	curshell = initshells();
}

static char **
initshells()
{
	register char **sp, *cp;
	register FILE *fp;
	struct stat64 statb;
	extern char *malloc(), *calloc();

	if (shells != NULL)
		free((char *)shells);
	shells = NULL;
	if (strings != NULL)
		free(strings);
	strings = NULL;
	if ((fp = fopen(SHELLS, "r")) == (FILE *)0)
		return ((char **)okshells);
	if ((fstat64(fileno(fp), &statb) == -1) || (statb.st_size > LONG_MAX) ||
	    ((strings = malloc((unsigned)statb.st_size)) == NULL)) {
		(void) fclose(fp);
		return ((char **)okshells);
	}
	
	shells = (char **)calloc((unsigned)statb.st_size / 3, sizeof (char *));
	if (shells == NULL) {
		(void) fclose(fp);
		free(strings);
		strings = NULL;
		return ((char **)okshells);
	}
	sp = shells;
	cp = strings;
	while (fgets(cp, MAXPATHLEN + 1, fp) != NULL) {
		while (*cp != '#' && *cp != '/' && *cp != '\0')
			cp++;
		if (*cp == '#' || *cp == '\0')
			continue;
		*sp++ = cp;
		while (!isspace(*cp) && *cp != '#' && *cp != '\0')
			cp++;
		*cp++ = '\0';
	}
	*sp = (char *)0;
	(void) fclose(fp);
	return (shells);
}
