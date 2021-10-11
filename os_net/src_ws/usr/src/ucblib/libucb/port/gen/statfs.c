/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * Copyright (c) 1985 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
/*
 * Copyright (c) 1998-1996, by Sun Microsystems, Inc.
 * All Rights reserved.
 */

#ident	"@(#)statfs.c	1.3	96/04/18 SMI"

#include <errno.h>
#include <sys/types.h>
#include <sys/statvfs.h>
#include <sys/vfs.h>

static void
cnvtvfs64(struct statfs64 *buf, struct statvfs64 *vbuf)
{
	buf->f_type = 0;
	buf->f_bsize = vbuf->f_frsize;
	buf->f_blocks = vbuf->f_blocks;
	buf->f_bfree = vbuf->f_bfree;
	buf->f_bavail = vbuf->f_bavail;
	buf->f_files = vbuf->f_files;
	buf->f_ffree = vbuf->f_ffree;
	buf->f_fsid.val[0] = vbuf->f_fsid;
	buf->f_fsid.val[1] = 0;
}

int
statfs64(char *path, struct statfs64 *buf)
{
	int ret;
	struct statvfs64 vbuf;

	if ((int)buf == -1) {
		errno = EFAULT;
		return(-1);
	}	

	if ((ret = statvfs64(path, &vbuf)) != -1)
		cnvtvfs64(buf, &vbuf);
	return(ret);
}


int
fstatfs64(int fd, struct statfs64 *buf)
{
	int ret;
	struct statvfs64 vbuf;

	if ((ret = fstatvfs64(fd, &vbuf)) != -1)
		cnvtvfs64(buf, &vbuf);
	return(ret);
}


static void
cnvtvfs(struct statfs *buf, struct statvfs *vbuf)
{
	buf->f_type = 0;
	buf->f_bsize = vbuf->f_frsize;
	buf->f_blocks = vbuf->f_blocks;
	buf->f_bfree = vbuf->f_bfree;
	buf->f_bavail = vbuf->f_bavail;
	buf->f_files = vbuf->f_files;
	buf->f_ffree = vbuf->f_ffree;
	buf->f_fsid.val[0] = vbuf->f_fsid;
	buf->f_fsid.val[1] = 0;
}

int
statfs(char *path, struct statfs *buf)
{
	int ret;
	struct statvfs vbuf;

	if ((int)buf == -1) {
		errno = EFAULT;
		return(-1);
	}	

	if ((ret = statvfs(path, &vbuf)) != -1)
		cnvtvfs(buf, &vbuf);
	return(ret);
}


int
fstatfs(int fd, struct statfs *buf)
{
	int ret;
	struct statvfs vbuf;

	if ((ret = fstatvfs(fd, &vbuf)) != -1)
		cnvtvfs(buf, &vbuf);
	return(ret);
}

