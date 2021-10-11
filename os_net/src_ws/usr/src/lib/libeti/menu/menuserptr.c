/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)menuserptr.c	1.2	92/07/14 SMI"	/* SVr4.0 1.1	*/

#include "private.h"

int
set_menu_userptr(m, c)
register MENU *m;
char *c;
{
  if (m) {
    Muserptr(m) = c;
  }
  else {
    Muserptr(Dfl_Menu) = c;
  }
  return E_OK;
}

char *
menu_userptr(m)
register MENU *m;
{
  return Muserptr(m ? m : Dfl_Menu);
}
