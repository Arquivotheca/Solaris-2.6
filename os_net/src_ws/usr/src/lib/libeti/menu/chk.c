/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)chk.c	1.2	92/07/14 SMI"	/* SVr4.0 1.4	*/

#include "private.h"

/* Make sure top is not within a page of the end of the menu */

void
_chk_top (m, top, current)
register MENU *m;
register int *top;
register ITEM *current;
{
  if (Y(current) < *top) {
    *top = Y(current);
  }
  if (Y(current) >= *top + Height(m)) {
    *top = Y(current) - Height(m) + 1;
  }
}

/* This routine makes sure top is in the correct position */
/* relative to current.  It is only used when current is */
/* explicitly set. */

void 
_chk_current (m, top, current)
register MENU *m;
register int *top;
register ITEM *current;
{
  if (Y(current) < *top) {
    *top = Y(current);
  }
  if (Y(current) >= *top + Height(m)) {
    *top = min (Y(current), Rows(m) - Height(m));
  }
}
