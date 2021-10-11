/*
 * Copyright (c) 1995, by Sun Microsystems, Inc.
 * All rights reserved.
 */

#ident	"@(#)echo_wch.c 1.1	95/12/22 SMI"

/*
 * echo_wch.c
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
static char rcsID[] = "$Header: /rd/src/libc/xcurses/rcs/echo_wch.c 1.1 1995/06/05 20:26:37 ant Exp $";
#endif
#endif

#include <private.h>

int
(echo_wchar)(const cchar_t *ch)
{
	int code;

#ifdef M_CURSES_TRACE
	__m_trace("echo_wchar(%p)", ch);
#endif

	if ((code = wadd_wch(stdscr, ch)) == OK)
		code = wrefresh(stdscr);

	return __m_return_code("echo_wchar", code);
}

int
(wecho_wchar)(WINDOW *w, const cchar_t *ch)
{
	int code;

#ifdef M_CURSES_TRACE
	__m_trace("wecho_wchar(%p, %p)", w, ch);
#endif

	if ((code = wadd_wch(w, ch)) == OK)
		code = wrefresh(w);

	return __m_return_code("wecho_wchar", code);
}
