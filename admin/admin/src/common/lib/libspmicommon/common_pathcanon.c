#ifndef lint
#pragma ident "@(#)common_pathcanon.c 1.2 96/05/21 SMI"
#endif
/*
 * Copyright (c) 1995, by Sun Microsystems, Inc.
 * All rights reserved.
 */

/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/
/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/* Public Function Prototypes */

void    	canoninplace(char *);

void
canoninplace(char *src)
{
	char *dst;
	char *src_start;

	/* keep a ptr to the beginning of the src string */
	src_start = src;

	dst = src;
	while (*src) {
		if (*src == '/') {
			*dst++ = '/';
			while (*src == '/')
				src++;
		} else
			*dst++ = *src++;
	}

	/*
	 * remove any trailing slashes, unless the whole string is just "/".
	 * If the whole string is "/" (i.e. if the last '/' cahr in dst
	 * in the beginning of the original string), just terminate it
	 * and return "/".
	 */
	if ((*(dst - 1) == '/') && ((dst - 1) != src_start))
		dst--;
	*dst = '\0';
}
