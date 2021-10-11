#ifndef lint
#ident "@(#)ttyutil.c 1.2 94/10/07 SMI"
#endif

/*
 * Copyright (c) 1993 by Sun Microsystems, Inc.
 */

/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

# include <stdio.h>
# include <unistd.h>
# include <stdlib.h>
# include <string.h>
# include <sys/types.h>
# include <ctype.h>
# include <sys/stat.h>
/*
# include "tmstruct.h"
# include "ttymon.h"
 */

#define	TTYDEFS	"/etc/ttydefs"

/*
 *	find_label - return line# if ttylabel already exists
 *		   - return 0 otherwise
 */

int
find_label(FILE *fp, const char *ttylabel)
{
	register char *p;	/* working pointer */
	int line = 0;		/* line number we found entry on */
	static char buf[BUFSIZ];/* scratch buffer */

	while (fgets(buf, BUFSIZ, fp)) {
		line++;
		p = buf;
		while (isspace(*p))
			p++;
		if ((p = strtok(p," :")) != NULL) {
			if (!(strcmp(p, ttylabel)))
				return(line);
		}
	}
	if (!feof(fp)) {
#ifdef NOTDEF
		(void)fprintf(stderr, "error reading \"%s\"\n", TTYDEFS);
#endif
		return(0);
	}
	return(0);
}

/*
 *	check_label	- if ttylabel exists in /etc/ttydefs, return 0
 *			- otherwise, return -1
 */

int
check_label(const char *ttylabel)
{
	FILE *fp;

	if ((ttylabel == NULL) || (*ttylabel == '\0')) {
		(void)fprintf(stderr, "error -- ttylabel is missing");
		return(-1);
	}
	if ((fp = fopen(TTYDEFS, "r")) == NULL) {
#ifdef NOTDEF
		(void)fprintf(stderr, "error -- \"%s\" does not exist, can't verify ttylabel <%s>\n", TTYDEFS, ttylabel);
#endif
		return(-1);
	}
	if (find_label(fp,ttylabel)) {
		(void)fclose(fp);
		return(0);
	}	
	(void)fclose(fp);
#ifdef NOTDEF
	(void)fprintf(stderr,"error -- can't find ttylabel <%s> in \"%s\"\n",
		ttylabel, TTYDEFS);
#endif
	return(-1);
}
