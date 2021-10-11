/*
 * Copyright (c) 1995, by Sun Microsystems, Inc.
 * All rights reserved.
 */

#ident	"@(#)scrreg.c 1.1	95/12/22 SMI"

/*
 * scrreg.c
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
static char rcsID[] = "$Header: /rd/src/libc/xcurses/rcs/scrreg.c 1.1 1995/06/05 19:19:46 ant Exp $";
#endif
#endif

#include <private.h>

int
(setscrreg)(top, bottom)
int top, bottom;
{
#ifdef M_CURSES_TRACE
	__m_trace("setscrreg(%d, %d)", top, bottom);
#endif

	if (top < 0 || bottom < top || stdscr->_maxy <= bottom)
		return __m_return_code("setscrreg", ERR);

	/* Set _top (inclusive) to _bottom (exclusive). */
	stdscr->_top = top;
	stdscr->_bottom = bottom + 1;

	return __m_return_code("setscrreg", OK);
}
