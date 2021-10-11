/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)_flsbuf.c	1.10	96/04/21 SMI"	/* SVr4.0 1.7	*/

/*LINTLIBRARY*/

#ifdef __STDC__
#pragma weak _flsbuf = __flsbuf
#endif
#include "synonyms.h"
#include "shlib.h"
#include <stdio.h>
#include "stdiom.h"
#include <thread.h>
#include <synch.h>
#include <mtlib.h>

extern int write();

int
_flsbuf(ch, iop)	/* flush (write) buffer, save ch, */
			/* return EOF on failure */
	int ch;
	register FILE *iop;
{
	Uchar uch;

	do	/* only loop if need to use _wrtchk() on non-_IOFBF */
	{
		switch (iop->_flag & (_IOFBF | _IOLBF | _IONBF |
				_IOWRT | _IOEOF))
		{
		case _IOFBF | _IOWRT:	/* okay to do full-buffered case */
			if (iop->_base != 0 && iop->_ptr > iop->_base)
				goto flush_putc;	/* skip _wrtchk() */
			break;
		case _IOLBF | _IOWRT:	/* okay to do line-buffered case */
			if (iop->_ptr >= _bufend(iop))
				/*
				 * which will recursively call
				 * __flsbuf via putc because of no room
				 * in the buffer for the character
				 */
				goto flush_putc;
			if ((*iop->_ptr++ = (unsigned char)ch) == '\n')
				_xflsbuf(iop);
			goto out;
		case _IONBF | _IOWRT:	/* okay to do no-buffered case */
			iop->_cnt = 0;
			uch = (unsigned char)ch;
			if (write(iop->_file, (char *)&uch, 1) != 1)
				iop->_flag |= _IOERR;
			goto out;
		}
		if (_wrtchk(iop) != 0)	/* check, correct permissions */
			return (EOF);
	} while (iop->_flag & (_IOLBF | _IONBF));
flush_putc:;
	_xflsbuf(iop);
	(void) PUTC(ch, iop); /*  recursive call */
out:;
	return ((iop->_flag & _IOERR) ? EOF : ch); /* necessary for putc() */
}
