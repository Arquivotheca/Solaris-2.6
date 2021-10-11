#ifndef lint
#pragma ident   "@(#)sp_mmap.c 1.7 95/02/06 SMI"
#endif
/*
 * Copyright (c) 1991 Sun Microsystems, Inc.  All Rights Reserved. Sun
 * considers its source code as an unpublished, proprietary trade secret, and
 * it is available only under strict license provisions.  This copyright
 * notice is placed here only to protect Sun in the event the source is
 * deemed a published work.  Dissassembly, decompilation, or other means of
 * reducing the object code to human readable form is prohibited by the
 * license agreement under which this code is provided to the user or company
 * in possession of this copy.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the Government
 * is subject to restrictions as set forth in subparagraph (c)(1)(ii) of the
 * Rights in Technical Data and Computer Software clause at DFARS 52.227-7013
 * and in similar clauses in the FAR and NASA FAR Supplement.
 */
#include "sw_lib.h"
#include "sw_space.h"

#include <malloc.h>
#include <fcntl.h>

/* Public Function Prototypes */

MFILE	*mopen(char *);
void 	mclose(MFILE *);
char 	*mgets(char *, int, MFILE *);

/* ******************************************************************** */
/*			PUBLIC SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

/*
 * mopen()
 *	Open and memory-map a file (for reading, only).  This is
 *	optimized for the type of files we'll be reading from CD-ROM,
 *	as it uses madvise(MADV_WILLNEED) to get the kernel to read
 *	in the whole file so we don't have to wait for it.  MFILE
 *	structs are dynamically allocated as needed and destroyed
 *	on close.
 * Parameters:
 *	name	- pathname of file to be mmapped in
 * Return:
 *	NULL 	- invalid parameter or open failed or fstat
 *	          failed or mmap failed or calloc failed
 * Status:
 *	public
 * Note:
 *	alloc routine
 */
MFILE *
mopen(char *name)
{
#ifdef MMAP
	struct stat st;
	MFILE	*mp;
	caddr_t	addr;
	int	fd;

	/* parameter check */
	if (name == (char *)NULL)
		return ((MFILE *)NULL);

	fd = open(name, O_RDONLY);
	if (fd < 0)
		return ((MFILE *)NULL);

	if (fstat(fd, &st) < 0) {
		(void) close(fd);
		return ((MFILE *)NULL);
	}
	addr =
	    mmap((caddr_t)0, st.st_size, PROT_READ, MAP_PRIVATE, fd, (off_t)0);
	if (addr == (caddr_t)-1) {
		(void) close(fd);
		return ((MFILE *)NULL);
	}
	if (madvise(addr, st.st_size, MADV_WILLNEED) < 0) {
		perror("madvise");
		(void) fprintf(stderr, "Warning: madvise failed in mopen()");
	}
	mp = (MFILE *) calloc((size_t)1, (size_t)sizeof (MFILE));
	if (mp == NULL)
		return ((MFILE *)NULL);
	mp->m_fd = fd;
	mp->m_base = addr;
	mp->m_ptr = addr;
	mp->m_size = st.st_size;
	return (mp);
#else
	return (fopen(name, "r"));
#endif
}

/*
 * mclose()
 *	Unmap, close, and free resources associated with an mopen'ed
 *	file.
 * Parameters:
 *	mp	- mmap file data structure pointer
 * Return:
 *	none
 * Status:
 *	public
 */
void
mclose(MFILE *mp)
{
	/* parameter check */
	if (mp == (MFILE *)NULL)
		return;
#ifdef MMAP
	(void) munmap(mp->m_base, mp->m_size);
	(void) close(mp->m_fd);
	free(mp);
#else
	(void) fclose((FILE *)mp);
#endif
}

/*
 * mgets()
 *	Search mmapped data area for a specific pattern up to the next
 *	'\n'. Advance the m_ptr passed the next '\n'. If MMAP is not
 *	defined, use the MFILE parameter with fgets().
 * Parameters:
 *	buf	- pattern you are searching for
 *	len	- length of pattern you are searching for
 *	mp	- file to MFILE if MMAP is not defined
 * Return:
 *	NULL	- EOF without match
 *	# > 0	- pointer to location in 'buf'
 * Status:
 *	public
 */
char *
mgets(char *buf, int len, MFILE *mp)
{
#ifdef MMAP
	register char *src, *dest;

	/* parameter check */
	if (mp == (MFILE *)NULL || mp->m_ptr == NULL)
		return;

	src = (char *)mp->m_ptr;
	dest = buf;

	while (src < mp->m_base + mp->m_size && /* end of mapped data */
	    *src != '\0' &&			/* end of file text   */
	    src < mp->m_ptr + len - 1) {	/* end of buffer len */
		if ((*dest++ = *src++) == '\n')
			break;
	}

	if (mp->m_ptr == src)
		return ((char *)NULL);		/* return EOF indication */

	*dest = '\0';				/* null-terminate */
	mp->m_ptr = src;			/* advance file pointer */
	return (dest);
#else
	return (fgets(buf, len, (FILE *)mp));
#endif
}
