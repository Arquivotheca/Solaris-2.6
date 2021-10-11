/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)scandir.c	1.3	96/04/18 SMI"	/* SVr4.0 1.1	*/

/*******************************************************************

		PROPRIETARY NOTICE (Combined)

This source code is unpublished proprietary information
constituting, or derived under license from AT&T's UNIX(r) System V.
In addition, portions of such source code were derived from Berkeley
4.3 BSD under license from the Regents of the University of
California.



		Copyright Notice 

Notice of copyright on this source code product does not indicate 
publication.

	(c) 1986,1987,1988,1989,1996  Sun Microsystems, Inc
	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
	          All rights reserved.
********************************************************************/ 

/*
 * Scan the directory dirname calling select to make a list of selected
 * directory entries then sort using qsort and compare routine dcomp.
 * Returns the number of entries and a pointer to a list of pointers to
 * struct direct (through namelist). Returns -1 if there were any errors.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <errno.h>
#include <limits.h>

#ifndef NULL
#define NULL 0
#endif

/*
 * The macro DIRSIZ(dp) gives an amount of space required to represent
 * a directory entry.  For any directory entry dp->d_reclen >= DIRSIZ(dp).
 * Specific filesystem types may use this use this macro to construct the value
 * for d_reclen.
 */
#undef DIRSIZ
#define DIRSIZ(dp)  \
	((sizeof (struct direct) - sizeof ((dp)->d_name) + \
	(strlen((dp)->d_name)+1) + 3) & ~3)


int
scandir64(char *dirname, struct direct64 *(*namelist[]),
    int (*select)(struct direct64 *),
    int (*dcomp)(struct direct64 **, struct direct64 **))
{
	register struct direct64 *d, *p, **names;
	register int nitems;
	register char *cp1, *cp2;
	struct stat64 stb;
	long arraysz;
	DIR *dirp;

	if ((dirp = opendir(dirname)) == NULL)
		return(-1);
	if (fstat64(dirp->dd_fd, &stb) < 0)
		return(-1);

	/*
	 * estimate the array size by taking the size of the directory file
	 * and dividing it by a multiple of the minimum size entry. 
	 */
	arraysz = (stb.st_size / 24);
	names = (struct direct64 **)malloc(arraysz * sizeof(struct direct64 *));
	if (names == NULL)
		return(-1);

	nitems = 0;
	while ((d = readdir64(dirp)) != NULL) {
		if (select != NULL && !(*select)(d))
			continue;	/* just selected names */
		/*
		 * Make a minimum size copy of the data
		 */
		p = (struct direct64 *)malloc(DIRSIZ64(d));
		if (p == NULL)
			return(-1);
		p->d_ino = d->d_ino;
		p->d_reclen = d->d_reclen;
		p->d_namlen = d->d_namlen;
		for (cp1 = p->d_name, cp2 = d->d_name; *cp1++ = *cp2++; );
		/*
		 * Check to make sure the array has space left and
		 * realloc the maximum size.
		 */
		if (++nitems >= arraysz) {
			if (fstat64(dirp->dd_fd, &stb) < 0)
				return(-1);	/* just might have grown */
			arraysz = stb.st_size / 12;
			names = (struct direct64 **)realloc((char *)names,
				arraysz * sizeof(struct direct64 *));
			if (names == NULL)
				return(-1);
		}
		names[nitems-1] = p;
	}
	closedir(dirp);
	if (nitems && dcomp != NULL)
		qsort(names, nitems, sizeof(struct direct64 *), dcomp);
	*namelist = names;
	return(nitems);
}


int
scandir(char *dirname, struct direct *(*namelist[]),
    int (*select)(struct direct *),
    int (*dcomp)(struct direct **, struct direct **))
{
	register struct direct *d, *p, **names;
	register int nitems;
	register char *cp1, *cp2;
	struct stat64 stb;
	long arraysz;
	DIR *dirp;

	if ((dirp = opendir(dirname)) == NULL)
		return(-1);
	if (fstat64(dirp->dd_fd, &stb) < 0)
		return(-1);
	/*
	 * estimate the array size by taking the size of the directory file
	 * and dividing it by a multiple of the minimum size entry. 
	 */
	if(stb.st_size > SSIZE_MAX) {
		errno = EOVERFLOW;
		return (-1);
	}
	arraysz = (stb.st_size / 24);

	names = (struct direct **)malloc(arraysz * sizeof(struct direct *));
	if (names == NULL)
		return(-1);

	nitems = 0;
	while ((d = readdir(dirp)) != NULL) {
		if (select != NULL && !(*select)(d))
			continue;	/* just selected names */
		/*
		 * Make a minimum size copy of the data
		 */
		p = (struct direct *)malloc(DIRSIZ(d));
		if (p == NULL)
			return(-1);
		p->d_ino = d->d_ino;
		p->d_reclen = d->d_reclen;
		p->d_namlen = d->d_namlen;
		for (cp1 = p->d_name, cp2 = d->d_name; *cp1++ = *cp2++; );
		/*
		 * Check to make sure the array has space left and
		 * realloc the maximum size.
		 */
		if (++nitems >= arraysz) {
			if (fstat64(dirp->dd_fd, &stb) < 0)
				return(-1);	/* just might have grown */
			arraysz = stb.st_size / 12;
			names = (struct direct **)realloc((char *)names,
				arraysz * sizeof(struct direct *));
			if (names == NULL)
				return(-1);
		}
		names[nitems-1] = p;
	}
	closedir(dirp);
	if (nitems && dcomp != NULL)
		qsort(names, nitems, sizeof(struct direct *), dcomp);
	*namelist = names;
	return(nitems);
}

/*
 * Alphabetic order comparison routine for those who want it.
 */
int
alphasort(struct direct **d1, struct direct **d2)
{
	return(strcmp((*d1)->d_name, (*d2)->d_name));
}

int
alphasort64(struct direct64 **d1, struct direct64 **d2)
{
	return(strcmp((*d1)->d_name, (*d2)->d_name));
}
