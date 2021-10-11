/*
 * Copyright (c) 1995, by Sun Microsystems, Inc.
 * All rights reserved.
 */

#ident	"@(#)addch.c 1.1	95/12/22 SMI"

/*
 * addch.c
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
static char rcsID[] = "$Header: /rd/src/libc/xcurses/rcs/addch.c 1.3 1995/07/07 17:59:07 ant Exp $";
#endif
#endif

#include <private.h>

int
(addch)(chtype ch)
{
	int code;

#ifdef M_CURSES_TRACE
	__m_trace("addch(%lx)", ch);
#endif

	code = waddch(stdscr, ch);

	return __m_return_code("addch", code);
}

int
(mvaddch)(int y, int x, chtype ch)
{
	int code;

#ifdef M_CURSES_TRACE
	__m_trace("mvaddch(%d, %d, %lx)", y, x, ch);
#endif

	if ((code = wmove(stdscr, y, x)) == OK)
		code = waddch(stdscr, ch);

	return __m_return_code("mvaddch", code);
}

int
(mvwaddch)(WINDOW *w, int y, int x, chtype ch)
{
	int code;

#ifdef M_CURSES_TRACE
	__m_trace("mvwaddch(%p, %d, %d, %lx)", w, y, x, ch);
#endif

	if ((code = wmove(w, y, x)) == OK)
		code = waddch(w, ch);

	return __m_return_code("mvwaddch", code);
}

