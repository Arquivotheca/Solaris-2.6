/*
 * Copyright (c) 1996, by Sun Microsystems, Inc.
 * All rights reserved.
 */

#pragma	ident	"@(#)__wcstombs_dense_eucjp.c 1.7	96/07/30  SMI"

/*
 * static char sccsid[] = "@(#)48	1.4.1.2  src/bos/usr/ccs/lib/libc/"
 * "__wcstombs_dense_eucjp.c, bos, bos410 1/12/93 11:11:41";
*/
/*
 * COMPONENT_NAME: (LIBCCPPC) LIBC Code-Point/Process-Code Conversion Functions
 *
 * FUNCTIONS: __wcstombs_dense_eucjp
 *
 * ORIGINS: 27
 *
 * This module contains IBM CONFIDENTIAL code. -- (IBM
 * Confidential Restricted when combined with the aggregated
 * modules for this product)
 * OBJECT CODE ONLY SOURCE MATERIALS
 * (C) COPYRIGHT International Business Machines Corp. 1991 , 1992
 * All Rights Reserved
 *
 * US Government Users Restricted Rights - Use, duplication or
 * disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
 *
 */

#include <stdlib.h>
#include <ctype.h>
#include <sys/localedef.h>
#include <limits.h>

size_t
__wcstombs_dense_eucjp(_LC_charmap_t *hdl, char *s, const wchar_t *pwcs,
				size_t n)
{
	int cnt = 0;
	int len = 0;
	int i = 0;
	char tmps[MB_LEN_MAX+1];

	/*
	 * if s is a NULL pointer, then just count the number of bytes
	 *  the converted string of process codes would require (does not
	 *  include the terminating NULL)
	 */
	if (s == (char *)NULL) {
		cnt = 0;
	while (*pwcs != (wchar_t)'\0') {
		if ((len = __wctomb_dense_eucjp(hdl, tmps, *pwcs)) == -1)
			return ((size_t)-1);
	    cnt += len;
	    pwcs++;
	}
	return (cnt);
	}

	/*
	 * if pwcs[0] is null, set *s = to NULL and
	 * return (0);
	 */
	if (*pwcs == (wchar_t)'\0') {
		*s = '\0';
		return (0);
	}


	while (1) {

	/*
	 * get the length of the characters in the process code
	 */
	if ((len = __wctomb_dense_eucjp(hdl, tmps, *pwcs)) == -1)
	    return ((size_t)-1);

	/*
	 *  if there is no room in s for this character(s),
	 * set a null and break out of the while
	 */
	else if (cnt+len > n) {
	    *s = '\0';
	    break;
	}

	/*
	 *  if a null was encounterd in pwcs, end s with a null and
	 *  break out of the while
	 */
	if (tmps[0] == '\0') {
	    *s = '\0';
	    break;
	}
	/*
	 * Otherwise, append the temporary string to the
	 * end of s.
	 */
	for (i = 0; i < len; i++) {
	    *s = tmps[i];
	    s++;
	}

	/*
	 * incrent the number of bytes put in s
	 */
	cnt += len;

	/*
	 * if the number of bytes processed is
	 * n then time to return, do not null terminate
	 */
	if (cnt == n)
	    break;

	/*
	 * increment pwcs to the next process code
	 */
	pwcs++;
	}
	/*
	 *  if there was not enough space for even 1 char,
	 *  return the len
	 */
	if (cnt == 0)
		cnt = len;
	return (cnt);
}
