/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)V3.wechochar.c	1.5	92/07/14 SMI"	/* SVr4.0 1.1	*/

#include	"curses_inc.h"
extern	int	_outchar();

#ifdef	_VR3_COMPAT_CODE
#undef	wechochar
wechochar(win, c)
WINDOW		*win;
_ochtype	c;
{
    return (w32echochar(win, _FROM_OCHTYPE(c)));
}
#endif	/* _VR3_COMPAT_CODE */
