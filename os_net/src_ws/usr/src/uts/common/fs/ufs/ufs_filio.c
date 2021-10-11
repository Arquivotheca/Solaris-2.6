/*	Copyright (c) 1991,1996 Sun Microsystems, Inc (SMI) */
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF SMI	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ufs_filio.c	1.24	96/09/05 SMI"

#include <sys/types.h>
#include <sys/t_lock.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/sysmacros.h>
#include <sys/resource.h>
#include <sys/signal.h>
#include <sys/cred.h>
#include <sys/user.h>
#include <sys/buf.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/proc.h>
#include <sys/disp.h>
#include <sys/file.h>
#include <sys/fcntl.h>
#include <sys/flock.h>
#include <sys/kmem.h>
#include <sys/uio.h>
#include <sys/conf.h>
#include <sys/mman.h>
#include <sys/pathname.h>
#include <sys/debug.h>
#include <sys/vmmeter.h>
#include <sys/vmsystm.h>
#include <sys/cmn_err.h>
#include <sys/vtrace.h>
#include <sys/filio.h>
#include <sys/dnlc.h>

#include <sys/fs/ufs_filio.h>
#include <sys/fs/ufs_lockfs.h>
#include <sys/fs/ufs_fs.h>
#include <sys/fs/ufs_inode.h>
#include <sys/fs/ufs_fsdir.h>
#ifdef QUOTA
#include <sys/fs/ufs_quota.h>
#endif
#include <sys/fs/ufs_trans.h>
#include <sys/dirent.h>		/* must be AFTER <sys/fs/fsdir.h>! */
#include <sys/errno.h>
#include <sys/sysinfo.h>

#include <vm/hat.h>
#include <vm/page.h>
#include <vm/pvn.h>
#include <vm/as.h>
#include <vm/seg.h>
#include <vm/seg_map.h>
#include <vm/seg_vn.h>
#include <vm/rm.h>
#include <sys/swap.h>

#include "fs/fs_subr.h"

/*
 * ufs_fioio is the ufs equivalent of NFS_CNVT and is tailored to
 * metamucil's needs.  It may change at any time.
 */
/* ARGSUSED */
int
ufs_fioio(vp, fiou, cr)
	struct vnode	*vp;		/* any file on the fs */
	struct fioio	*fiou;		/* fioio struct in userland */
	struct cred	*cr;		/* credentials from ufs_ioctl */
{
	int		error	= 0;
	struct vnode	*vpio	= NULL;	/* vnode for inode open */
	struct inode	*ipio	= NULL;	/* inode for inode open */
	struct file	*fpio	= NULL;	/* file  for inode open */
	struct inode	*ip;		/* inode for file system */
	struct fs	*fs;		/* fs    for file system */
	struct fioio	fio;		/* copy of user's fioio struct */

	/*
	 * must be superuser
	 */
	if (!suser(cr))
		return (EPERM);

	/*
	 * get user's copy of fioio struct
	 */
	if (copyin((caddr_t)fiou, (caddr_t)&fio, (u_int)(sizeof (fio))))
		return (EFAULT);

	ip = VTOI(vp);
	fs = ip->i_fs;

	/*
	 * check the inode number against the fs's inode number bounds
	 */
	if (fio.fio_ino < UFSROOTINO)
		return (ESRCH);
	if (fio.fio_ino >= fs->fs_ncg * fs->fs_ipg)
		return (ESRCH);

	/*
	 * get the inode
	 */
	if (error = ufs_iget(ip->i_vfs, fio.fio_ino, &ipio, cr))
		return (error);

	/*
	 * check the generation number
	 */
	rw_enter(&ipio->i_contents, RW_READER);
	if (ipio->i_gen != fio.fio_gen) {
		error = ESTALE;
		rw_exit(&ipio->i_contents);
		goto errout;
	}

	/*
	 * check if the inode is free
	 */
	if (ipio->i_mode == 0) {
		error = ENOENT;
		rw_exit(&ipio->i_contents);
		goto errout;
	}
	rw_exit(&ipio->i_contents);

	/*
	 *	Adapted from copen: get a file struct
	 *	Large Files: We open this file descriptor with FOFFMAX flag
	 *	set so that it will be like a large file open.
	 */
	if (falloc((struct vnode *)NULL, (FREAD|FOFFMAX), &fpio, &fio.fio_fd))
		goto errout;

	/*
	 *	Adapted from vn_open: check access and then open the file
	 */
	vpio = ITOV(ipio);
	if (error = VOP_ACCESS(vpio, VREAD, 0, cr))
		goto errout;

	if (error = VOP_OPEN(&vpio, FREAD, cr))
		goto errout;

	/*
	 *	Adapted from copen: initialize the file struct
	 */
	fpio->f_vnode = vpio;

	/*
	 * return the fd
	 */
	if (suword((int *)&fiou->fio_fd, fio.fio_fd)) {
		error = EFAULT;
		goto errout;
	}
	setf(fio.fio_fd, fpio);
	mutex_exit(&fpio->f_tlock);
	return (0);
errout:
	/*
	 * free the file struct and fd
	 */
	if (fpio) {
		setf(fio.fio_fd, NULLFP);
		unfalloc(fpio);
	}

	/*
	 * release the hold on the inode
	 */
	if (ipio)
		VN_RELE(ITOV(ipio));
	return (error);
}
/*
 * ufs_fioai
 *	Get file allocation information.  This ioctl is tailored
 *	to metamucil's needs and may change at any time.
 */
ufs_fioai(vp, faiu, cr)
	struct vnode	*vp;		/* file's vnode */
	struct fioai	*faiu;		/* fioai struct in userland */
	struct cred	*cr;		/* credentials from ufs_ioctl */
{
	int		error = 0;
	int		na;		/* # allocations returned */
	u_long		ne;		/* # entries left in array */
	int		len;		/* for bmap_read call */
	ssize_t		size;		/* byte length of range */
	off_t		off;		/* byte offset into file */
	daddr_t		lbn;		/* logical fs block */
	daddr_t		bn;		/* disk sector number */
	daddr_t		bor;		/* beginning of range (sector) */
	daddr_t		lor;		/* length of range (sector) */
	daddr_t		lof;		/* length of file (sector) */
	struct fioai	fai;		/* copy of users fioai */
	struct fs	*fs;		/* file system (superblock) */
	struct inode	*ip;		/* vnode's inode */
	daddr_t		*da;		/* address of user array */

	/*
	 * must be superuser
	 */
	if (!suser(cr))
		return (EPERM);

	/*
	 * get user's copy of fioai struct
	 */
	if (copyin((caddr_t)faiu, (caddr_t)&fai, (u_int)(sizeof (fai))))
		return (EFAULT);

	/*
	 * inode and superblock
	 */
	ip = VTOI(vp);
	fs = ip->i_fs;

	rw_enter(&ip->i_contents, RW_READER);
	/*
	 * range checks
	 *	offset >= 1T || size >= 1T || (offset+size) >= 1T
	 *	offset >= length of file
	 *
	 */
	na = 0;
	if ((size = fai.fai_size) == 0)
		size = ip->i_size - fai.fai_off;

	if (fai.fai_off < 0 || fai.fai_off > UFS_MAXOFFSET_T)
		goto errrange;
	if (size < 0 || size > MAXOFFSET_T)
		goto errrange;
	if (((fai.fai_off + size) < 0) || ((fai.fai_off + size) >= MAXOFFSET_T))
		goto errrange;
	if (fai.fai_off >= ip->i_size)
		goto errrange;

	/*
	 * beginning of range in sectors
	 * length of range in sectors
	 * length of file in sectors
	 */
	bor = (daddr_t)lbtodb(fai.fai_off);
	off = ldbtob(bor);
	lor = (daddr_t)(lbtodb(size) + ((size & (DEV_BSIZE-1)) ? 1 : 0));
	lof = (daddr_t)lbtodb(ip->i_size) + ((ip->i_size & (DEV_BSIZE-1)) ?
		1 : 0);
	if (lof < (bor + lor))
		lor = lof - bor;

	/*
	 * return allocation info until:
	 *	array fills
	 *	range is covered (end of file accounted for above)
	 */
	ne = fai.fai_num;
	da = fai.fai_daddr;
	while (lor && ne) {

		/*
		 * file system block and offset within block
		 */
		lbn  = (daddr_t)lblkno(fs, off);

		/*
		 * get frag address and convert to disk address
		 */
		len = 0;
		if (error = bmap_read(ip, off, &bn, &len))
			goto errout;
		if (bn == UFS_HOLE)
			bn = _FIOAI_HOLE;

		/*
		 * return disk addresses.
		 * 	(file system blocks are contiguous on disk)
		 */
		do {
			if (suword((int *)da, (int)bn)) {
				error = EFAULT;
				goto errout;
			}
			if (bn != _FIOAI_HOLE)
				bn++;
			off += DEV_BSIZE;
			na++;
			da++;
			lor--;
			ne--;
		} while ((lbn == lblkno(fs, off)) && lor && ne);
	}
	/*
	 * update # of entries returned and current offset
	 */
	fai.fai_off = off;
errrange:
	fai.fai_num = na;
	if (copyout((caddr_t)&fai, (caddr_t)faiu, sizeof (fai))) {
		error = EFAULT;
		goto errout;
	}

errout:
	rw_exit(&ip->i_contents);
	return (error);
}
/*
 * ufs_fiosatime
 *	set access time w/o altering change time.  This ioctl is tailored
 *	to metamucil's needs and may change at any time.
 */
ufs_fiosatime(vp, tvu, cr)
	struct vnode	*vp;		/* file's vnode */
	struct timeval	*tvu;		/* struct timeval in userland */
	struct cred	*cr;		/* credentials from ufs_ioctl */
{
	struct inode	*ip;		/* inode for vp */
	struct timeval	tv;		/* copy of user's timeval */
	int		now = 0;

	/*
	 * must be superuser
	 */
	if (!suser(cr))
		return (EPERM);

	/*
	 * get user's copy of timeval struct and check values
	 * if input is NULL, will set time to now
	 */
	if ((caddr_t) tvu == NULL)
		now = 1;
	else {
		if (copyin((caddr_t)tvu, (caddr_t)&tv, (u_int)(sizeof (tv))))
			return (EFAULT);
		if (tv.tv_usec < 0 || tv.tv_usec >= 1000000)
			return (EINVAL);
	}

	/*
	 * update access time
	 */
	ip = VTOI(vp);
	rw_enter(&ip->i_contents, RW_WRITER);
	ITIMES_NOLOCK(ip);
	if (now)
		ip->i_atime = iuniqtime;
	else
		ip->i_atime = tv;
	ip->i_flag |= IMODACC;
	rw_exit(&ip->i_contents);

	return (0);
}
/*
 * ufs_fiogdio
 *	Get delayed-io state.  This ioctl is tailored
 *	to metamucil's needs and may change at any time.
 */
/* ARGSUSED */
int
ufs_fiogdio(vp, diop, cr)
	struct vnode	*vp;		/* file's vnode */
	u_long		*diop;		/* dio state returned here */
	struct cred	*cr;		/* credentials from ufs_ioctl */
{
	struct ufsvfs	*ufsvfsp	= VTOI(vp)->i_ufsvfs;

	/*
	 * forcibly unmounted
	 */
	if (ufsvfsp == NULL)
		return (EIO);

	if (suword((int *)diop, (int)(ufsvfsp->vfs_dio)))
		return (EFAULT);
	return (0);
}
/*
 * ufs_fiosdio
 *	Set delayed-io state.  This ioctl is tailored
 *	to metamucil's needs and may change at any time.
 */
ufs_fiosdio(vp, diop, cr)
	struct vnode	*vp;		/* file's vnode */
	u_long		*diop;		/* dio flag */
	struct cred	*cr;		/* credentials from ufs_ioctl */
{
	u_long		dio;		/* copy of user's dio */
	struct inode	*ip;		/* inode for vp */
	struct ufsvfs	*ufsvfsp;
	struct fs	*fs;
	struct ulockfs	*ulp;
	int		error = 0;

	/* check input conditions */
	if (!suser(cr))
		return (EPERM);

	if (copyin((caddr_t)diop, (caddr_t)&dio, (u_int)(sizeof (u_long))))
		return (EFAULT);

	if (dio > 1)
		return (EINVAL);

	/* file system has been forcibly unmounted */
	if (VTOI(vp)->i_ufsvfs == NULL)
		return (EIO);

	ip = VTOI(vp);
	ufsvfsp = ip->i_ufsvfs;
	ulp = &ufsvfsp->vfs_ulockfs;

	/* logging file system; dio ignored */
	if (TRANS_ISTRANS(ufsvfsp))
		return (error);

	/* hold the mutex to prevent race with a lockfs request */
	vfs_lock_wait(vp->v_vfsp);
	mutex_enter(&ulp->ul_lock);

	if (ULOCKFS_IS_HLOCK(ulp)) {
		error = EIO;
		goto out;
	}

	if (ULOCKFS_IS_ELOCK(ulp)) {
		error = EBUSY;
		goto out;
	}
	/* wait for outstanding accesses to finish */
	if (error = ufs_quiesce(ulp))
		goto out;

	/* flush w/invalidate */
	if (error = ufs_flush(vp->v_vfsp))
		goto out;

	/*
	 * update dio
	 */
	mutex_enter(&ufsvfsp->vfs_lock);
	ufsvfsp->vfs_dio = dio;

	/*
	 * enable/disable clean flag processing
	 */
	fs = ip->i_fs;
	if (fs->fs_ronly == 0 &&
	    fs->fs_clean != FSBAD &&
	    fs->fs_clean != FSLOG) {
		if (dio)
			fs->fs_clean = FSSUSPEND;
		else
			fs->fs_clean = FSACTIVE;
		ufs_sbwrite(ufsvfsp);
		mutex_exit(&ufsvfsp->vfs_lock);
	} else
		mutex_exit(&ufsvfsp->vfs_lock);
out:
	/*
	 * we need this broadcast because of the ufs_quiesce call above
	 */
	cv_broadcast(&ulp->ul_cv);
	mutex_exit(&ulp->ul_lock);
	vfs_unlock(vp->v_vfsp);
	return (error);
}

/*
 * ufs_fioffs - ioctl handler for flushing file system
 */
/* ARGSUSED */
int
ufs_fioffs(vp, vap, cr)
	struct vnode	*vp;
	char 		*vap;		/* must be NULL - reserved */
	struct cred	*cr;		/* credentials from ufs_ioctl */
{
	int error;
	struct ufsvfs	*ufsvfsp;
	struct ulockfs	*ulp;

	/* file system has been forcibly unmounted */
	ufsvfsp = VTOI(vp)->i_ufsvfs;
	if (ufsvfsp == NULL)
		return (EIO);

	ulp = &ufsvfsp->vfs_ulockfs;

	/*
	 * suspend the delete thread
	 *	this must be done outside the lockfs locking protocol
	 */
	ufs_thread_suspend(&ufsvfsp->vfs_delete);

	vfs_lock_wait(vp->v_vfsp);
	/* hold the mutex to prevent race with a lockfs request */
	mutex_enter(&ulp->ul_lock);

	if (ULOCKFS_IS_HLOCK(ulp)) {
		error = EIO;
		goto out;
	}
	if (ULOCKFS_IS_ELOCK(ulp)) {
		error = EBUSY;
		goto out;
	}
	/* wait for outstanding accesses to finish */
	if (error = ufs_quiesce(ulp))
		goto out;
	/* synchronously flush dirty data and metadata */
	error = ufs_flush(vp->v_vfsp);
out:
	cv_broadcast(&ulp->ul_cv);
	mutex_exit(&ulp->ul_lock);
	vfs_unlock(vp->v_vfsp);

	/*
	 * allow the delete thread to continue
	 */
	ufs_thread_continue(&ufsvfsp->vfs_delete);
	return (error);
}

/*
 * ufs_fioisbusy
 *	Get number of references on this vnode.
 *	Contract-private interface for Legato's NetWorker product.
 */
/* ARGSUSED */
int
ufs_fioisbusy(struct vnode *vp, u_long *isbusy, struct cred *cr)
{
	int is_it_busy;

	/*
	 * The caller holds one reference, there may be one in the dnlc
	 * so we need to flush it.
	 */
	if (vp->v_count > 1)
		dnlc_purge_vp(vp);
	/*
	 * Since we've just flushed the dnlc and we hold a reference
	 * to this vnode, then anything but 1 means busy (this had
	 * BETTER not be zero!). Also, it's possible for someone to
	 * have this file mmap'ed with no additional reference count.
	 */
	ASSERT(vp->v_count > 0);
	if ((vp->v_count == 1) && (VTOI(vp)->i_mapcnt == 0))
		is_it_busy = 0;
	else
		is_it_busy = 1;

	if (suword((int *)isbusy, is_it_busy))
		return (EFAULT);
	return (0);
}

/* ARGSUSED */
int
ufs_fiodirectio(struct vnode *vp, int cmd, struct cred *cr)
{
	int		error	= 0;
	struct inode	*ip	= VTOI(vp);

	/*
	 * Acquire reader lock and set/reset direct mode
	 */
	rw_enter(&ip->i_contents, RW_READER);
	mutex_enter(&ip->i_tlock);
	if (cmd == DIRECTIO_ON)
		ip->i_flag |= IDIRECTIO;	/* enable direct mode */
	else if (cmd == DIRECTIO_OFF)
		ip->i_flag &= ~IDIRECTIO;	/* disable direct mode */
	else
		error = EINVAL;
	mutex_exit(&ip->i_tlock);
	rw_exit(&ip->i_contents);
	return (error);
}
