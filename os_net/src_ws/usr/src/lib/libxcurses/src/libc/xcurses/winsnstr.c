/*
 * Copyright (c) 1995, by Sun Microsystems, Inc.
 * All rights reserved.
 */

#ident	"@(#)winsnstr.c 1.1	95/12/22 SMI"

/*
 * wins_nws.c
 *
 * XCurses Library
 *
 * Copyright 1990, 1994 by Mortice Kern Systems Inc.  All rights reserved.
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

#ifdef M_RCSID
#ifndef lint
static char rcsID[] = "$Header: /rd/src/libc/xcurses/rcs/winsnstr.c 1.2 1995/06/21 20:31:02 ant Exp $";
#endif
#endif

#include <private.h>

int
winsnstr(w, mbs, n)
WINDOW *w;
const char *mbs;
int n;
{
	cchar_t cc;
	int i, y, x;  
	short oflags;

#ifdef M_CURSES_TRACE
	__m_trace("winsnstr(%p, %p, n)", w, mbs, n);
#endif

	y = w->_cury; 
	x = w->_curx;

	if (n < 0)
		n = INT_MAX;

	/* Disable window flushing until the entire string has 
	 * been written into the window.
	 */
	oflags = w->_flags & (W_FLUSH | W_SYNC_UP);
	w->_flags &= ~(W_FLUSH | W_SYNC_UP);

	for ( ; *mbs != '\0' && 0 < n; n -= i, mbs += i) {
		if ((i = __m_mbs_cc(mbs, w->_bg._at, w->_bg._co, &cc)) < 0
		|| __m_wins_wch(w, y, x, &cc, &y, &x) == ERR)
			return __m_return_code("winsnstr", ERR);
	}

	w->_flags |= oflags;

	WSYNC(w);

	return __m_return_code("winsnstr", WFLUSH(w));
}
