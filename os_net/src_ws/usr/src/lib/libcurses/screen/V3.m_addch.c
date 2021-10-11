/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)V3.m_addch.c	1.5	92/07/14 SMI"	/* SVr4.0 1.2	*/

#include	"curses_inc.h"
extern	int	_outchar();

#ifdef	_VR3_COMPAT_CODE
m_addch(c)
int	c;
{
    return (waddch(stdscr, (_ochtype ) c));
}
#endif	/* _VR3_COMPAT_CODE */