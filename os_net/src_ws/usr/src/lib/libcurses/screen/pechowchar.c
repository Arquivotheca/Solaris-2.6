/*  Copyright (c) 1988 AT&T */
/*    All Rights Reserved   */

/*  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T */
/*  The copyright notice above does not evidence any    */
/*  actual or intended publication of such source code. */

#ident  "@(#)pechowchar.c 1.1 93/05/05 SMI"
 
/*
 *  These routines short-circuit much of the innards of curses in order to get
 *  a single character output to the screen quickly!
 *
 *  pechochar(WINDOW *pad, chtype ch) is functionally equivalent to
 *  waddch(WINDOW *pad, chtype ch), prefresh(WINDOW *pad, `the same arguments
 *  as in the last prefresh or pnoutrefresh')
 */

#include	"curses_inc.h"

pechowchar(pad, ch)
register	WINDOW	*pad;
chtype			ch;
{
    register WINDOW *padwin;
    int	     rv;

    /*
     * If pad->_padwin exists (meaning that p*refresh have been
     * previously called), call wechochar on it.  Otherwise, call
     * wechochar on the pad itself
     */

    if ((padwin = pad->_padwin) != NULL)
    {
	padwin->_cury = pad->_cury - padwin->_pary;
	padwin->_curx = pad->_curx - padwin->_parx;
	rv = wechowchar (padwin, ch);
	pad->_cury = padwin->_cury + padwin->_pary;
	pad->_curx = padwin->_curx + padwin->_parx;
	return (rv);
    }
    else
        return (wechowchar (pad, ch));
}
