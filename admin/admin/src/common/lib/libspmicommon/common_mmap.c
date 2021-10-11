#ifndef lint
#pragma ident "@(#)common_mmap.c 1.3 96/01/03 SMI"
#endif
/*
 * Copyright (c) 1991-1996 Sun Microsystems, Inc.  All Rights Reserved.
 */

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include "spmicommon_lib.h"

/* public prototypes */

MFILE *		mopen(char *);
void 		mclose(MFILE *);
char *		mgets(char *, int, MFILE *);

/* ---------------------- public prototypes -------------------------- */

/*
 * Function:	mopen
 * Description: Open and memory-map a file (for reading, only).  This is
 *		optimized for the type of files we'll be reading from CD-ROM,
 *		as it uses madvise(MADV_WILLNEED) to get the kernel to read
 *		in the whole file so we don't have to wait for it. MFILE
 *		structures are dynamically allocated and are destroyed
 *		on close.
 * Scope:	public
 * Parameters:	name	[RO, *RO]
 *			Path name of file to be mmapped in.
 * Return:	NULL 	- mmap failure
 *		!NULL	- pointer to MFILE structure for mmaped/opened file
 */
MFILE *
mopen(char *name)
{
	struct stat	sbuf;
	MFILE *		mp;
	caddr_t		addr;
	int		fd;

	/* validate parameter */
	if (name == NULL)
		return (NULL);

	if ((fd = open(name, O_RDONLY)) < 0 || stat(name, &sbuf) < 0)
		return (NULL);

	if ((addr = mmap((caddr_t)0, sbuf.st_size, PROT_READ,
				MAP_PRIVATE, fd, (off_t)0)) == MAP_FAILED) {
		(void)close(fd);
		return (NULL);
	}

	(void) close(fd);
	(void) madvise(addr, sbuf.st_size, MADV_WILLNEED);

	if ((mp = (MFILE *)calloc((size_t)1,
			(size_t)sizeof (MFILE))) != NULL) {
		mp->m_base = addr;
		mp->m_ptr = addr;
		mp->m_size = sbuf.st_size;
	}

	return (mp);
}

/*
 * Function:	mclose
 * Description: Unmap, close, and free resources associated with an mopen'ed
 *		file.
 * Scope:	public
 * Parameters:	mp	[RO, *RO]
 *			mmap file data structure pointer.
 * Return:	none
 */
void
mclose(MFILE *mp)
{
	if (mp != NULL) {
		(void) munmap(mp->m_base, mp->m_size);
		free(mp);
	}
}

/*
 * Function:	mgets
 * Description: Search mmapped data area up to the next '\n'. Advance the
 *		m_ptr passed the next '\n', and return the line.
 * Scope:	public
 * Parameters:	buf	- [RO, *RO]
 *			  Buffer used to retrieve the next line.
 *		len	- [RO]
 *			  Size of buffer.
 *		mp	- [RO, *RW]
 *			  Pointer to an opened mmaped file MFILE structure.
 * Return:	NULL	- EOF without match
 *		!NULL	- pointer to location in 'pattern'
 */
char *
mgets(char *buf, int len, MFILE *mp)
{
	char *	src;
	char *	dest;

	/* validate parameters */
	if (len <= 0 || buf == NULL || mp == NULL ||
			mp->m_base == NULL || mp->m_ptr == NULL)
		return (NULL);

	src = (char *)mp->m_ptr;
	dest = buf;

	/*
	 * search the mmapped area, up to the first NULL character
	 * to include, but not exceed either the buffer length or the
	 * first '\n' character. If previous mgets calls have been
	 * made the search will begin where the last line left off.
	 */
	while ((src < (mp->m_base + mp->m_size)) &&
			(*src != '\0') && (src < (mp->m_ptr + len - 1))) {
		if ((*dest++ = *src++) == '\n')
			break;
	}

	/*
	 * return an EOF indication if no data was read, otherwise
	 * NULL terminate the string, and advance the file pointer
	 */
	if (mp->m_ptr == src)
		return (NULL);

	*dest = '\0';
	mp->m_ptr = src;
	return (dest);
}
