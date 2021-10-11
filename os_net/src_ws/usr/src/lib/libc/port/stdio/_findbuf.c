/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)_findbuf.c	1.14	96/02/15 SMI"	/* SVr4.0 1.6	*/

/*LINTLIBRARY*/

#define	_LARGEFILE64_SOURCE	1
#include "synonyms.h"
#include "shlib.h"
#include <stdlib.h>
#include <stdio.h>
#include "stdiom.h"
#include <sys/types.h>
#include <sys/stat.h>

extern int isatty(/* int fd */);
extern Uchar _smbuf[][_SMBFSZ]; 	/* shared library compatibility */

/*
* If buffer space has been pre-allocated use it otherwise malloc space.
* PUSHBACK causes the base pointer to be bumped forward. At least 4 bytes
* of pushback are required to meet international specifications.
* Extra space at the end of the buffer allows for synchronization problems.
* If malloc() fails stdio bails out; assumption being the system is in trouble.
*/
Uchar *
_findbuf(iop)	/* associate a buffer with stream; return 0 for success */
	register FILE *iop;
{
	register int fd = iop->_file;
	register Uchar *buf;
	int size = BUFSIZ;
	Uchar *endbuf;
	int tty = -1;

	struct stat64 stbuf;		/* used to get file system block size */

	if (iop->_flag & _IONBF)	/* need a small buffer, at least */
	{
	trysmall:;
		size = _SMBFSZ - PUSHBACK;
		if (fd < _NFILE)
			buf = _smbuf[fd];
		else if ((buf = (Uchar *)malloc(_SMBFSZ * sizeof (Uchar))) != 0)
			iop->_flag |= _IOMYBUF;
	}
#ifndef _STDIO_ALLOCATE
	else if (fd < 2 && (tty = isatty(fd)))
	{
		buf = (fd == 0) ? _sibuf : _sobuf; /* special buffer */
						/* for std{in,out} */
	}
#endif
	else {
		/*
		 * The operating system can tell us the
		 * right size for a buffer
		 */
		if (fstat64(fd, &stbuf) == 0)
			size = stbuf.st_blksize;

		if ((buf = (Uchar *)malloc(sizeof (Uchar)*(size+_SMBFSZ))) != 0)
			iop->_flag |= _IOMYBUF;
		else
			goto trysmall;
	}
	if (buf == 0)
		return (0); 	/* malloc() failed */
	iop->_base = buf + PUSHBACK;	/* bytes for pushback */
	iop->_ptr = buf + PUSHBACK;
	endbuf = iop->_base + size;
	_setbufend(iop, endbuf);
	if (!(iop->_flag & _IONBF) && ((tty != -1) ? tty : isatty(fd)))
		iop->_flag |= _IOLBF;
	return (endbuf);
}
