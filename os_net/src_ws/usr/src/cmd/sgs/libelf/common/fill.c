/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 * All rights reserved.
 */

#pragma ident	"@(#)fill.c	1.7	95/06/14 SMI" 	/* SVr4.0 1.2	*/

/*LINTLIBRARY*/

#pragma weak	elf_fill = _elf_fill

#include <libelf.h>
#include "syn.h"
#include "decl.h"


void
elf_fill(int fill)
{
	ELFACCESSDATA(_elf_byte, fill)
}
