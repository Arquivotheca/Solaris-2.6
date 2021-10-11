/*
 * Copyright (c) 1995, by Sun Microsystems, Inc.
 * All rights reserved.
 */

#ident	"@(#)waddnstr.c 1.1	95/12/22 SMI"

/*
 * waddnstr.c
 * 
 * XCurses Library
 *
 * Copyright 1990, 1995 by Mortice Kern Systems Inc.  All rights reserved.
 *
 * This Software is unpublished, valuable, confidential property of
 * Mortice Kern Systems Inc.  Use is authorized only in accordance
 * with the terms and conditions of the source licence agreement
 * protecting this Software.  Any unauthorized use or disclosure of
 * this Software is strictly prohibited and will result in the
 * termination of the licence agreement.
 *
 * If you have any questions, please consult your supervisor.
 */

#if M_RCSID
#ifndef lint
static char rcsID[] = "$Header: /rd/src/libc/xcurses/rcs/waddnstr.c 1.3 1995/06/21 20:30:55 ant Exp $";
#endif
#endif

#include <private.h>
#include <limits.h>

int
waddnstr(w, str, n)
WINDOW *w;
const char *str;
int n;
{
	int i;
	cchar_t cc;
	short oflags;

#ifdef M_CURSES_TRACE
	__m_trace("waddnstr(%p, %p = \"%s\", %d)", w, str, str, n);
#endif

	if (n < 0)
		n = INT_MAX;

	/* Disable window flushing until the entire string has 
	 * been written into the window.
	 */
	oflags = w->_flags & (W_FLUSH | W_SYNC_UP);
	w->_flags &= ~(W_FLUSH | W_SYNC_UP);

	for ( ; *str != '\0' && 0 < n; n -= i, str += i) {
                if ((i = __m_mbs_cc(str, w->_bg._at, w->_bg._co, &cc)) < 0
		|| wadd_wch(w, &cc) == ERR)
			return __m_return_code("waddnstr", ERR);
        }

	w->_flags |= oflags;

	WSYNC(w);

	return __m_return_code("waddnstr", WFLUSH(w));
}
