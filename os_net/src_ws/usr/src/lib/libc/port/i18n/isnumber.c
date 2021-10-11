/*
 * Copyright (c) 1996, by Sun Microsystems, Inc.
 * All rights reserved.
 */

#pragma	ident	"@(#)isnumber.c 1.4	96/07/31  SMI"

/*
 * COMPONENT_NAME: (LIBCCHR) LIBC Character Classification Funcions
 *
 * FUNCTIONS: isnumber
 *
 */
/*
 *
 * FUNCTION: Determines if the process code, pc, is a number character
 *
 *
 * PARAMETERS: pc  -- character to be classified
 *
 *
 * RETURN VALUES: 0 -- if pc is not a number character
 *                >0 - If c is a number character
 *
 *
 */

#include <ctype.h>
#include <wchar.h>
#include <sys/localedef.h>
#include <wctype.h>

#ifdef isnumber
#undef isnumber
#endif

#pragma weak isnumber = _isnumber

int
_isnumber(wint_t pc)
{
	if ((unsigned long)pc > 0x9f)
		return (METHOD(__lc_ctype, iswctype)(__lc_ctype, pc, _E4));
	else
		return (0);
} 
