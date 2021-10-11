/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 *
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 *
 *
 *
 * 		Copyright Notice
 *
 * Notice of copyright on this source code product does not indicate
 * publication.
 *
 * 	(c) 1986,1987,1988,1989,1996  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 *		All rights reserved.
 *
 */

#ident	"@(#)quota.c	1.46	96/09/04 SMI"
/* from "quota.c 1.17     90/01/08 SMI"	*/

/*
 * Code pertaining to management of the in-core data structures.
 */
#ifdef QUOTA
#include <sys/types.h>
#include <sys/t_lock.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signal.h>
#include <sys/errno.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/uio.h>
#include <sys/buf.h>
#include <sys/fs/ufs_fs.h>
#include <sys/fs/ufs_inode.h>
#include <sys/fs/ufs_quota.h>
#include <sys/cmn_err.h>
#include <sys/kmem.h>
#include <sys/debug.h>
#include <sys/file.h>
#include <sys/fs/ufs_panic.h>


/*
 * Dquot in core hash chain headers
 */
struct	dqhead	dqhead[NDQHASH];

static kmutex_t dq_cachelock;
static kmutex_t dq_freelock;

krwlock_t dq_rwlock;

/*
 * Dquot free list.
 */
struct dquot dqfreelist;

#define	dqinsheadfree(DQP) { \
	mutex_enter(&dq_freelock); \
	(DQP)->dq_freef = dqfreelist.dq_freef; \
	(DQP)->dq_freeb = &dqfreelist; \
	dqfreelist.dq_freef->dq_freeb = (DQP); \
	dqfreelist.dq_freef = (DQP); \
	mutex_exit(&dq_freelock); \
}

#define	dqinstailfree(DQP) { \
	mutex_enter(&dq_freelock); \
	(DQP)->dq_freeb = dqfreelist.dq_freeb; \
	(DQP)->dq_freef = &dqfreelist; \
	dqfreelist.dq_freeb->dq_freef = (DQP); \
	dqfreelist.dq_freeb = (DQP); \
	mutex_exit(&dq_freelock); \
}

#define	dqremfree(DQP) { \
	(DQP)->dq_freeb->dq_freef = (DQP)->dq_freef; \
	(DQP)->dq_freef->dq_freeb = (DQP)->dq_freeb; \
}

typedef	struct dquot *DQptr;

/*
 * Initialize quota caches.
 */
void
qtinit()
{
	mutex_init(&dq_cachelock, "ufs dq cache lock", MUTEX_DEFAULT,
								DEFAULT_WT);
	mutex_init(&dq_freelock, "ufs dq free list lock", MUTEX_DEFAULT,
								DEFAULT_WT);
	rw_init(&dq_rwlock, "ufs dq rw lock", RW_DEFAULT, DEFAULT_WT);
}

/*
 * qtinit2 allocated space for the quota structures.  Only do this if
 * if quotas are going to be used so that we can save the space if quotas
 * aren't used.
 */
void
qtinit2(void)
{
	register struct dqhead *dhp;
	register struct dquot *dqp;
	extern int ndquot;

	ASSERT(RW_WRITE_HELD(&dq_rwlock));
	dquot = kmem_zalloc(ndquot * sizeof (struct dquot), KM_SLEEP);

	dquotNDQUOT = dquot + ndquot;
	/*
	 * Initialize the cache between the in-core structures
	 * and the per-file system quota files on disk.
	 */
	for (dhp = &dqhead[0]; dhp < &dqhead[NDQHASH]; dhp++) {
		dhp->dqh_forw = dhp->dqh_back = (DQptr)dhp;
	}
	dqfreelist.dq_freef = dqfreelist.dq_freeb = (DQptr)&dqfreelist;
	for (dqp = dquot; dqp < dquotNDQUOT; dqp++) {
		mutex_init(&dqp->dq_lock, "ufs per dq lock", MUTEX_DEFAULT,
		    DEFAULT_WT);
		dqp->dq_forw = dqp->dq_back = dqp;
		dqinsheadfree(dqp);
	}
}

/*
 * Obtain the user's on-disk quota limit for file system specified.
 * dqpp is returned locked.
 */
int
getdiskquota(
	uid_t uid,
	struct ufsvfs *ufsvfsp,
	int force,			/* don't do enable checks */
	struct dquot **dqpp)		/* resulting dquot ptr */
{
	struct dquot *dqp;
	struct dqhead *dhp;
	struct inode *qip;
	int error;
	extern struct cred *kcred;
	daddr_t	bn;

	ASSERT(RW_LOCK_HELD(&dq_rwlock));

	dhp = &dqhead[DQHASH(uid, ufsvfsp)];
loop:
	/*
	 * This routine assumes that it was called with either the read
	 * or write dq_rwlock.
	 */
	/*
	 * Check for quotas enabled.
	 */
	if ((ufsvfsp->vfs_qflags & MQ_ENABLED) == 0 && !force)
		return (ESRCH);
	qip = ufsvfsp->vfs_qinod;
	if (!qip)
		return (ufs_fault(ufsvfsp->vfs_root, "getdiskquota: NULL qip"));
	/*
	 * XXX Maybe I should hold qip and release dqrwlock at this point
	 * so that I don't hold dq_rwlock for so long.
	 */
	/*
	 * Check the cache first.
	 */
	mutex_enter(&dq_cachelock);
	for (dqp = dhp->dqh_forw; dqp != (DQptr)dhp; dqp = dqp->dq_forw) {
		if (dqp->dq_uid != uid || dqp->dq_ufsvfsp != ufsvfsp)
			continue;
		mutex_exit(&dq_cachelock);
		mutex_enter(&dqp->dq_lock);
		/*
		 * I may have slept in the mutex_enter.  Make sure this is
		 * still the one I want.
		 */
		if (dqp->dq_uid != uid || dqp->dq_ufsvfsp != ufsvfsp) {
			mutex_exit(&dqp->dq_lock);
			goto loop;
		}
		if (dqp->dq_flags & DQ_ERROR) {
			mutex_exit(&dqp->dq_lock);
			return (EINVAL);
		}
		/*
		 * Cache hit with no references.
		 * Take the structure off the free list.
		 */
		if (dqp->dq_cnt == 0) {
			mutex_enter(&dq_freelock);
			dqremfree(dqp);
			mutex_exit(&dq_freelock);
		}
		dqp->dq_cnt++;
		mutex_exit(&dqp->dq_lock);
		*dqpp = dqp;
		return (0);
	}
	/*
	 * Not in cache.
	 * Get dqot at head of free list.
	 */
	mutex_enter(&dq_freelock);
	if ((dqp = dqfreelist.dq_freef) == &dqfreelist) {
		mutex_exit(&dq_freelock);
		mutex_exit(&dq_cachelock);
		cmn_err(CE_WARN, "dquot table full");
		return (EUSERS);
	}

	if (dqp->dq_cnt != 0 || dqp->dq_flags != 0) {
		mutex_exit(&dq_freelock);
		return (ufs_fault(ITOV(qip),
	"getdiskquota: dqp->dq_cnt: %ld != 0 || dqp->dq_flags: 0x%x != 0 (%s)",
				    dqp->dq_cnt, dqp->dq_flags,
				    qip->i_fs->fs_fsmnt));
	}
	/*
	 * Take it off the free list, and off the hash chain it was on.
	 * Then put it on the new hash chain.
	 */
	dqremfree(dqp);
	mutex_exit(&dq_freelock);
	remque(dqp);
	dqp->dq_cnt = 1;
	dqp->dq_uid = uid;
	dqp->dq_ufsvfsp = ufsvfsp;
	dqp->dq_mof = UFS_HOLE;
	mutex_enter(&dqp->dq_lock);
	insque(dqp, dhp);
	mutex_exit(&dq_cachelock);
	/*
	 * Check the uid in case it's too large to fit into the 2Gbyte
	 * 'quotas' file (higher than 67 million or so).
	 */
	/*
	 * Large Files: i_size need to be accessed atomically now.
	 */

/*
 * Locknest gets very confused when I lock the quota inode.  It thinks
 * that qip and ip (the inode that caused the quota routines to get called)
 * are the same inode.
 */
#ifndef LOCKNEST
		rw_enter(&qip->i_contents, RW_READER);
#endif
	if (uid >= 0 && uid < (UFS_MAXOFFSET_T / sizeof (struct dqblk)) &&
	    dqoff(uid) >= 0 && dqoff(uid) < qip->i_size) {
		/*
		 * Read quota info off disk.
		 */
		error = ufs_rdwri(UIO_READ, FREAD, qip, (caddr_t)&dqp->dq_dqb,
		    (int)sizeof (struct dqblk), dqoff(uid), UIO_SYSSPACE,
		    (int *)NULL, kcred);
#ifndef LOCKNEST
		rw_exit(&qip->i_contents);
#endif

		if (TRANS_ISTRANS(qip->i_ufsvfs) && !error) {
			int err, contig = 0;
			rw_enter(&qip->i_contents, RW_READER);
			err = bmap_read(qip, dqoff(uid), &bn, &contig);
			rw_exit(&qip->i_contents);
			if ((bn != UFS_HOLE) && !err) {
				dqp->dq_mof = ldbtob(bn) +
				(offset_t) (dqoff(uid) & (DEV_BSIZE - 1));
			} else {
				dqp->dq_mof = UFS_HOLE;
			}
		}
		if (error) {
			/*
			 * I/O error in reading quota file.
			 * Put dquot on a private, unfindable hash list,
			 * put dquot at the head of the free list and
			 * reflect the problem to caller.
			 */
			dqp->dq_flags = DQ_ERROR;
			/*
			 * I must exit the dq_lock so that I can acquire the
			 * dq_cachelock.  If another thread finds dqp before
			 * I remove it from the cache it will see the
			 * DQ_ERROR and just return EIO.
			 */
			mutex_exit(&dqp->dq_lock);
			mutex_enter(&dq_cachelock);
			mutex_enter(&dqp->dq_lock);
			remque(dqp);
			mutex_exit(&dqp->dq_lock);
			mutex_exit(&dq_cachelock);
			/*
			 * Don't bother reacquiring dq_lock because the dq is
			 * not on the freelist or in the cache so only I have
			 * access to it.
			 */
			dqp->dq_cnt = 0;
			dqp->dq_ufsvfsp = NULL;
			dqp->dq_forw = dqp;
			dqp->dq_back = dqp;
			dqp->dq_mof = UFS_HOLE;
			dqp->dq_flags = 0;
			dqinsheadfree(dqp);
			return (EIO);
		}
	} else {
#ifndef LOCKNEST
		/*
		 * Largefiles: We read i_size atomically by holding
		 * the contents lock and releasing it here.
		 */
		rw_exit(&qip->i_contents);
#endif
		bzero((caddr_t)&dqp->dq_dqb, sizeof (struct dqblk));
		dqp->dq_mof = UFS_HOLE;
	}
	mutex_exit(&dqp->dq_lock);
	*dqpp = dqp;
	return (0);
}

/*
 * Release dquot.
 */
void
dqput(dqp)
	register struct dquot *dqp;
{

	ASSERT(MUTEX_HELD(&dqp->dq_lock));
	if (dqp->dq_cnt == 0) {
		(void) ufs_fault(
			dqp->dq_ufsvfsp && dqp->dq_ufsvfsp->vfs_root?
			dqp->dq_ufsvfsp->vfs_root: NULL,
						    "dqput: dqp->dq_cnt == 0");
		return;
	}
	if (--dqp->dq_cnt == 0) {
		if (dqp->dq_flags & DQ_MOD)
			dqupdate(dqp);
		/*
		 * DQ_MOD was cleared by dqupdate().
		 * DQ_ERROR shouldn't be set if this dquot was being used.
		 * DQ_FILES/DQ_BLKS don't matter at this point.
		 */
		dqp->dq_flags = 0;
		dqinstailfree(dqp);
	}
}

/*
 * Update on disk quota info.
 */
void
dqupdate(dqp)
	register struct dquot *dqp;
{
	register struct inode *qip;
	extern struct cred *kcred;
	struct ufsvfs	*ufsvfsp;
	int		newtrans	= 0;

	ASSERT(MUTEX_HELD(&dqp->dq_lock));

	if (!dqp->dq_ufsvfsp) {
		(void) ufs_fault(NULL, "dqupdate: NULL dq_ufsvfsp");
		return;
	}
	if (!dqp->dq_ufsvfsp->vfs_root) {
		(void) ufs_fault(NULL, "dqupdate: NULL vfs_root");
		return;
	}
	/*
	 * I don't need to hold dq_rwlock when looking at vfs_qinod here
	 * because vfs_qinod is only cleared by closedq after it has called
	 * dqput on all dq's.  Since I am holding dq_lock on this dq, closedq
	 * will have to wait until I am done before it can call dqput on
	 * this dq so vfs_qinod will not change value until after I return.
	 */
	if (!dqp->dq_ufsvfsp->vfs_qinod) {
		(void) ufs_fault(dqp->dq_ufsvfsp->vfs_root,
				    "dqupdate: NULL vfs_qinod");
		return;
	}
	if (!dqp->dq_ufsvfsp->vfs_qinod->i_ufsvfs) {
		(void) ufs_fault(dqp->dq_ufsvfsp->vfs_root,
				    "dqupdate: NULL vfs_qinod->i_ufsvfs");
		return;
	}
	if (dqp->dq_ufsvfsp->vfs_qinod->i_ufsvfs != dqp->dq_ufsvfsp) {
		(void) ufs_fault(dqp->dq_ufsvfsp->vfs_root,
			    "dqupdate: vfs_qinod->i_ufsvfs != dqp->dq_ufsvfsp");
		return;
	}
	if (!(dqp->dq_flags & DQ_MOD)) {
		(void) ufs_fault(dqp->dq_ufsvfsp->vfs_root,
				    "dqupdate: !(dqp->dq_flags & DQ_MOD)");
		return;
	}

	qip = dqp->dq_ufsvfsp->vfs_qinod;
	ufsvfsp = qip->i_ufsvfs;
	if (!(curthread->t_flag & T_DONTBLOCK)) {
		newtrans++;
		curthread->t_flag |= T_DONTBLOCK;
		TRANS_BEGIN_ASYNC(ufsvfsp, TOP_QUOTA, TOP_QUOTA_SIZE);
	}
/*
 * Locknest gets very confused when I lock the quota inode.  It thinks
 * that qip and ip (the inode that caused the quota routines to get called)
 * are the same inode.
 */
	if (TRANS_ISTRANS(ufsvfsp)) {
		TRANS_LOG(ufsvfsp, (caddr_t)&dqp->dq_dqb, dqp->dq_mof,
		    (int) sizeof (struct dqblk));
	} else {
#ifndef LOCKNEST
		rw_enter(&qip->i_contents, RW_WRITER);
#endif
		/*
		 * refuse to push if offset would be illegal
		 */
		if (dqoff(dqp->dq_uid) >= 0) {
			(void) ufs_rdwri(UIO_WRITE, FWRITE, qip,
					(caddr_t)&dqp->dq_dqb,
					(int)sizeof (struct dqblk),
					dqoff(dqp->dq_uid), UIO_SYSSPACE,
					(int *)NULL, kcred);
		}
#ifndef LOCKNEST
		rw_exit(&qip->i_contents);
#endif
	}

	dqp->dq_flags &= ~DQ_MOD;
	if (newtrans) {
		TRANS_END_ASYNC(ufsvfsp, TOP_QUOTA, TOP_QUOTA_SIZE);
		curthread->t_flag &= ~T_DONTBLOCK;
	}
}

/*
 * Invalidate a dquot.
 * Take the dquot off its hash list and put it on a private,
 * unfindable hash list. Also, put it at the head of the free list.
 */
void
dqinval(dqp)
	register struct dquot *dqp;
{
	ASSERT(MUTEX_HELD(&dqp->dq_lock));

	if (dqp->dq_cnt || (dqp->dq_flags & DQ_MOD)) {
		(void) ufs_fault(dqp->dq_ufsvfsp && dqp->dq_ufsvfsp->vfs_root?
				    dqp->dq_ufsvfsp->vfs_root: NULL,
	"dqinval: dqp->dq_cnt:%ld != 0 || (dqp->dq_flags:0x%x & DQ_MOD)",
				    dqp->dq_cnt, dqp->dq_flags);
		return;
	}
	/*
	 * I set DQ_ERROR as a safety precaution.  dqinval is only called
	 * from closedq after it has cleared the MQ_ENABLED flag.  This
	 * means no other thread should be waiting on dq_lock or be able
	 * to get to a point where it can wait on dq_lock.  Just in case
	 * this situation happens due to a coding error DQ_ERROR will
	 * keep the other thread from using dq.  The pre-MT code used to
	 * panic if DQ_WANT was set.
	 */

	/*
	 * XXX should add some pointer validity checking here,
	 * XXX rather than just trusting them
	 */
	dqp->dq_flags = DQ_ERROR;
	mutex_exit(&dqp->dq_lock);
	mutex_enter(&dq_cachelock);
	remque(dqp);
	mutex_exit(&dq_cachelock);
	mutex_enter(&dq_freelock);
	dqremfree(dqp);
	mutex_exit(&dq_freelock);
	dqp->dq_ufsvfsp = NULL;
	dqp->dq_forw = dqp;
	dqp->dq_back = dqp;
	dqp->dq_flags = 0;
	dqinsheadfree(dqp);
}
#endif
