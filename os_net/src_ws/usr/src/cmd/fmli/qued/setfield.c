/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)setfield.c	1.4	92/07/14 SMI"       /* SVr4.0 1.8 */

#include <stdio.h>
#include <curses.h>
#include <malloc.h>
#include "token.h"
#include "winp.h"
#include "fmacs.h"
#include "terror.h"
#include "attrs.h"

#define FSIZE(x)	(x->rows * (x->cols + 1))

setfieldflags(fld, flags)
register ifield *fld;
register int flags;
{
    fld->flags = (flags & I_CHANGEABLE) | (fld->flags & ~(I_CHANGEABLE));
    if (fld->flags & I_INVISIBLE)
    {
	if (fld->value)
	    free(fld->value);	/* abs */
	if ((fld->value = (char *) malloc(FSIZE(fld))) == NULL)
	    fatal(NOMEM, "");
	fld->valptr = fld->value;
    }
    fld->fieldattr = (fld->flags & I_FILL ? Attr_underline: Attr_normal);
}

