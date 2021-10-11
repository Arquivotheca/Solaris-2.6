/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma	ident	"@(#)msg.c	1.37	96/06/14 SMI" /* from S5R4 1.15 */

/*
 * Inter-Process Communication Message Facility.
 */

#include <sys/types.h>
#include <sys/t_lock.h>
#include <sys/param.h>
#include <sys/cred.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/time.h>
#include <sys/map.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/systm.h>
#include <sys/sysmacros.h>
#include <sys/cpuvar.h>
#include <sys/kmem.h>
#include <sys/ddi.h>
#include <sys/errno.h>
#include <sys/cmn_err.h>
#include <sys/debug.h>

/*
 *	ATTENTION: All ye who enter here
 *	As an optimization, all global data items are declared in space.c
 *	When this module get unloaded, the data stays around. The data
 * 	is only allocated on first load.
 *
 *	XXX	Unloading disabled - there's more to leaving state
 *		lying around in the system than not freeing global data.
 */

/*
 * The following variables msginfo_* are there so that the
 * elements of the data structure msginfo can be tuned
 * (if necessary) using the /etc/system file syntax for
 * tuning of integer data types.
 */
int	msginfo_msgmap = 100;	/* # of entries in msg map */
int	msginfo_msgmax = 2048;	/* max message size */
int	msginfo_msgmnb = 4096;	/* max # bytes on queue */
int	msginfo_msgmni = 50;	/* # of message queue identifiers */
int	msginfo_msgssz = 8;	/* msg segment size (s/b word size multiple) */
int	msginfo_msgtql = 40;	/* # of system message headers */
ushort	msginfo_msgseg = 1024;	/* # of msg segments (MUST BE < 32768) */

static kmutex_t msg_lock;	/* protects msg mechanism data structures */
static kcondvar_t msgfp_cv;

/* define macro to round up to nearest int boundary "from machdep.c" */
#define	INTRND(X)	roundup((X), sizeof (int))

#include <sys/modctl.h>
#include <sys/syscall.h>

static int msgsys(int opcode, int a0, int a1, int a2, int a4, int a5);

static struct sysent ipcmsg_sysent = {
	6,
	SE_ARGC | SE_NOUNLOAD,
	msgsys
};

/*
 * Module linkage information for the kernel.
 */
static struct modlsys modlsys = {
	&mod_syscallops, "System V message facility", &ipcmsg_sysent
};

static struct modlinkage modlinkage = {
	MODREV_1, (void *)&modlsys, NULL
};

char _depends_on[] = "misc/ipc";	/* ipcaccess, ipcget */

int
_init(void)
{
	int retval;
	register int		i;	/* loop control */
	register struct msg	*mp;	/* ptr to msg being linked */
	struct msglock	*lp;		/* ptr to mutex being init'd */
	u_longlong_t		mavail;

	/*
	 * msginfo_* are inited in param.c to default values
	 * These values can be tuned if need be using the
	 * integer tuning capabilities in the /etc/system file.
	 */
	msginfo.msgmap = msginfo_msgmap;
	msginfo.msgmax = msginfo_msgmax;
	msginfo.msgmnb = msginfo_msgmnb;
	msginfo.msgmni = msginfo_msgmni;
	msginfo.msgssz = msginfo_msgssz;
	msginfo.msgtql = msginfo_msgtql;
	msginfo.msgseg = msginfo_msgseg;

	/*
	 * Don't use more than 25% of the available kernel memory
	 */
	mavail = kmem_maxavail() / 4;
	if ((INTRND((u_longlong_t)msginfo.msgseg * (u_longlong_t)msginfo.msgssz)
		* sizeof (char) +
	    (u_longlong_t)msginfo.msgmap * sizeof (struct map) +
	    (u_longlong_t)msginfo.msgmni * sizeof (struct msqid_ds) +
	    INTRND((u_longlong_t)msginfo.msgmni * sizeof (struct msglock)) +
	    (u_longlong_t)msginfo.msgtql * sizeof (struct msg)) > mavail) {
		cmn_err(CE_WARN,
		    "msgsys: can't load module, too much memory requested");
		return (ENOMEM);
	}

	mutex_init(&msg_lock, "Sys V msg data", MUTEX_DEFAULT, NULL);
	cv_init(&msgfp_cv, "msgfp cv", CV_DEFAULT, NULL);

	ASSERT(msg == NULL);
	ASSERT(msgmap == NULL);
	ASSERT(msgh == NULL);
	ASSERT(msgque == NULL);
	ASSERT(msglock == NULL);

	msg = kmem_zalloc(INTRND(msginfo.msgseg * msginfo.msgssz)
		* sizeof (char), KM_SLEEP);
	msgmap = kmem_zalloc(msginfo.msgmap * sizeof (struct map), KM_SLEEP);
	msgh = kmem_zalloc(msginfo.msgtql * sizeof (struct msg), KM_SLEEP);
	msgque = kmem_zalloc(msginfo.msgmni * sizeof (struct msqid_ds),
		KM_SLEEP);
	msglock = kmem_zalloc(INTRND(msginfo.msgmni * sizeof (struct msglock)),
		KM_SLEEP);

	mapinit(msgmap, (long)msginfo.msgseg, (ulong)1, "msgmap",
		msginfo.msgmap);

	for (i = 0, lp = msglock; ++i < msginfo.msgmni; lp++)
		mutex_init(&lp->msglock_lock, "Sys V msg queue",
		    MUTEX_DEFAULT, NULL);
	for (i = 0, mp = msgfp = msgh; ++i < msginfo.msgtql; mp++)
		mp->msg_next = mp + 1;

	if ((retval = mod_install(&modlinkage)) == 0)
		return (0);

	for (i = 0, lp = msglock; ++i < msginfo.msgmni; lp++)
		mutex_destroy(&lp->msglock_lock);
	kmem_free(msg, INTRND(msginfo.msgseg * msginfo.msgssz) * sizeof (char));
	kmem_free(msgmap, msginfo.msgmap * sizeof (struct map));
	kmem_free(msgh, msginfo.msgtql * sizeof (struct msg));
	kmem_free(msgque, msginfo.msgmni * sizeof (struct msqid_ds));
	kmem_free(msglock, INTRND(msginfo.msgmni * sizeof (struct msglock)));

	msg = NULL;
	msgmap = NULL;
	msgh = NULL;
	msgque = NULL;
	msglock = NULL;

	cv_destroy(&msgfp_cv);
	mutex_destroy(&msg_lock);

	return (retval);
}

/*
 * See comments in shm.c.
 */
int
_fini(void)
{
	return (EBUSY);
}

int
_info(struct modinfo *modinfop)
{
	return (mod_info(&modlinkage, modinfop));
}

/* HACK :: msgbuf structure renamed to avoid kernel compilation conflicts */
#define	msgbuf ipcmsgbuf

/* Convert bytes to msg segments. */
#define	btoq(X)	(((long)(X) + msginfo.msgssz - 1) / msginfo.msgssz)

/*
 * Argument vectors for the various flavors of msgsys().
 */

#define	MSGGET	0
#define	MSGCTL	1
#define	MSGRCV	2
#define	MSGSND	3


/*
 * msgfree - Free up space and message header, relink pointers on q,
 * and wakeup anyone waiting for resources.
 *
 * called with q locked, gets and releases msg_lock
 */
static void
msgfree(
	register struct msqid_ds	*qp, /* ptr to q of mesg being freed */
	register struct msg		*pmp, /* ptr to mp's predecessor */
	register struct msg		*mp) /* ptr to msg being freed */
{
	/* Unlink message from the q. */
	if (pmp == NULL)
		qp->msg_first = mp->msg_next;
	else
		pmp->msg_next = mp->msg_next;
	if (mp->msg_next == NULL)
		qp->msg_last = pmp;
	qp->msg_qnum--;
	if (qp->msg_perm.mode & MSG_WWAIT) {
		qp->msg_perm.mode &= ~MSG_WWAIT;
		cv_broadcast(&qp->msg_cv);
	}

	/* Free up message text. */
	if (mp->msg_ts) {
		rmfree(msgmap, btoq(mp->msg_ts), (ulong)(mp->msg_spot + 1));
	}

	/* Free up header */
	mutex_enter(&msg_lock);
	mp->msg_next = msgfp;
	if (msgfp == NULL)
		cv_broadcast(&msgfp_cv);
	msgfp = mp;
	mutex_exit(&msg_lock);
}

/*
 * msgconv - Convert a user supplied message queue id into a ptr to a
 * msqid_ds structure.
 */
static int
msgconv(register int id, struct msqid_ds **qpp)
{
	register struct msqid_ds	*qp;	/* ptr to associated q slot */
	register struct msglock		*lockp;	/* ptr to lock.		*/


	if (id < 0)
		return (EINVAL);

	qp = &msgque[id % msginfo.msgmni];
	lockp = MSGLOCK(qp);
	mutex_enter(&lockp->msglock_lock);
	if ((qp->msg_perm.mode & IPC_ALLOC) == 0 ||
	    (id / msginfo.msgmni) != qp->msg_perm.seq) {
		mutex_exit(&lockp->msglock_lock);
		return (EINVAL);

	}
	*qpp = qp;
	return (0);
}

/*
 * msgctl - Msgctl system call.
 *
 *  gets q lock (via msgconv), releases before return.
 *  may call users of msg_lock
 */
static int
msgctl(int msgid, int cmd, struct msqid_ds *buf)
{
	struct o_msqid_ds		ods;	/* SVR3 queue work area */
	struct msqid_ds			ds;	/* SVR4 queue work area */
	struct msqid_ds			*qp;	/* ptr to associated q */
	register struct msglock		*lockp;
	register int			error;
	register struct	cred		*cr;

	if (error = msgconv(msgid, &qp)) {
		/* get msqid_ds for this msgid */
		return (set_errno(error));
	}

	lockp = MSGLOCK(qp);

	cr = CRED();
	switch (cmd) {
	case IPC_O_RMID:
	case IPC_RMID:
		/*
		 * IPC_RMID is for use with expanded msqid_ds struct -
		 * msqid_ds not currently used with this command
		 */
		if (cr->cr_uid != qp->msg_perm.uid &&
		    cr->cr_uid != qp->msg_perm.cuid &&
		    !suser(cr)) {
			error = EPERM;
			break;
		}
		while (qp->msg_first)
			msgfree(qp, (struct msg *)NULL, qp->msg_first);
		qp->msg_cbytes = 0;
		if (msgid + msginfo.msgmni < 0)
			qp->msg_perm.seq = 0;
		else
			qp->msg_perm.seq++;
		if (qp->msg_perm.mode & MSG_RWAIT)
			cv_broadcast(&qp->msg_qnum_cv);
		if (qp->msg_perm.mode & MSG_WWAIT)
			cv_broadcast(&qp->msg_cv);
		qp->msg_perm.mode = 0;
		break;

	case IPC_O_SET:
		if (cr->cr_uid != qp->msg_perm.uid &&
		    cr->cr_uid != qp->msg_perm.cuid &&
		    !suser(cr)) {
			error = EPERM;
			break;
		}
		if (copyin((caddr_t)buf, (caddr_t)&ods, sizeof (ods))) {
			error = EFAULT;
			break;
		}
		if (ods.msg_qbytes > qp->msg_qbytes && !suser(cr)) {
			error = EPERM;
			break;
		}
		if ((u_long)ods.msg_perm.uid >= (u_long)USHRT_MAX ||
		    (u_long)ods.msg_perm.gid >= (u_long)USHRT_MAX) {
			error = EOVERFLOW;
			break;
		}
		if ((u_long)ods.msg_perm.uid > (u_long)MAXUID ||
		    (u_long)ods.msg_perm.gid > (u_long)MAXUID) {
			error = EINVAL;
			break;
		}
		qp->msg_perm.uid = ods.msg_perm.uid;
		qp->msg_perm.gid = ods.msg_perm.gid;
		qp->msg_perm.mode =
		    (qp->msg_perm.mode & ~0777) | (ods.msg_perm.mode & 0777);
		qp->msg_qbytes = ods.msg_qbytes;
		qp->msg_ctime = hrestime.tv_sec;
		break;

	case IPC_SET:
		if (cr->cr_uid != qp->msg_perm.uid &&
		    cr->cr_uid != qp->msg_perm.cuid &&
		    !suser(cr)) {
			error = EPERM;
			break;
		}
		if (copyin((caddr_t)buf, (caddr_t)&ds, sizeof (ds))) {
			error = EFAULT;
			break;
		}
		if (ds.msg_qbytes > qp->msg_qbytes && !suser(cr)) {
			error = EPERM;
			break;
		}
		if (ds.msg_perm.uid < (uid_t)0 || ds.msg_perm.uid > MAXUID ||
		    ds.msg_perm.gid < (gid_t)0 || ds.msg_perm.gid > MAXUID) {
			error = EINVAL;
			break;
		}
		qp->msg_perm.uid = ds.msg_perm.uid;
		qp->msg_perm.gid = ds.msg_perm.gid;
		qp->msg_perm.mode =
		    (qp->msg_perm.mode & ~0777) | (ds.msg_perm.mode & 0777);
		qp->msg_qbytes = ds.msg_qbytes;
		qp->msg_ctime = hrestime.tv_sec;
		break;

	case IPC_O_STAT:
		if (error = ipcaccess(&qp->msg_perm, MSG_R, cr))
			break;

		/*
		 * copy expanded msqid_ds struct to SVR3 msqid_ds structure -
		 * support for non-eft applications.
		 * Check whether SVR4 values are too large to store into an SVR3
		 * msqid_ds structure.
		 */
		if ((unsigned)qp->msg_perm.uid > USHRT_MAX ||
		    (unsigned)qp->msg_perm.gid > USHRT_MAX ||
		    (unsigned)qp->msg_perm.cuid > USHRT_MAX ||
		    (unsigned)qp->msg_perm.cgid > USHRT_MAX ||
		    (unsigned)qp->msg_perm.seq > USHRT_MAX ||
		    (unsigned)qp->msg_cbytes > USHRT_MAX ||
		    (unsigned)qp->msg_qnum > USHRT_MAX ||
		    (unsigned)qp->msg_qbytes > USHRT_MAX ||
		    (unsigned)qp->msg_lspid > SHRT_MAX ||
		    (unsigned)qp->msg_lrpid > SHRT_MAX) {
				error = EOVERFLOW;
				break;
		}
		ods.msg_perm.uid = (o_uid_t)qp->msg_perm.uid;
		ods.msg_perm.gid = (o_gid_t)qp->msg_perm.gid;
		ods.msg_perm.cuid = (o_uid_t)qp->msg_perm.cuid;
		ods.msg_perm.cgid = (o_gid_t)qp->msg_perm.cgid;
		ods.msg_perm.mode = (o_mode_t)qp->msg_perm.mode;
		ods.msg_perm.seq = (ushort)qp->msg_perm.seq;
		ods.msg_perm.key = qp->msg_perm.key;
		ods.msg_first = NULL; 	/* kernel addr */
		ods.msg_last = NULL;
		ods.msg_cbytes = (ushort)qp->msg_cbytes;
		ods.msg_qnum = (ushort)qp->msg_qnum;
		ods.msg_qbytes = (ushort)qp->msg_qbytes;
		ods.msg_lspid = (o_pid_t)qp->msg_lspid;
		ods.msg_lrpid = (o_pid_t)qp->msg_lrpid;
		ods.msg_stime = qp->msg_stime;
		ods.msg_rtime = qp->msg_rtime;
		ods.msg_ctime = qp->msg_ctime;

		if (copyout((caddr_t)&ods, (caddr_t)buf, sizeof (ods))) {
			error = EFAULT;
			break;
		}
		break;

	case IPC_STAT:
		if (error = ipcaccess(&qp->msg_perm, MSG_R, cr))
			break;

		if (copyout((caddr_t)qp, (caddr_t)buf, sizeof (*qp))) {
			error = EFAULT;
			break;
		}
		break;

	default:
		error = EINVAL;
		break;
	}

	mutex_exit(&lockp->msglock_lock);
	if (error)
		return (set_errno(error));
	return (0);
}

/*
 * msgget - Msgget system call.
 *
 * allocation of new msgqid controlled by msg_lock
 */
static int
msgget(key_t key, int msgflg)
{
	struct msqid_ds		*qp;	/* ptr to associated q */
	int			s;	/* ipcget status return */
	register int		error;

	mutex_enter(&msg_lock);
	if (error = ipcget(key, msgflg, (struct ipc_perm *)msgque,
	    msginfo.msgmni, sizeof (*qp), &s, (struct ipc_perm **)&qp)) {
			mutex_exit(&msg_lock);
			return (set_errno(error));
		}

	if (s) {
		/* This is a new queue.  Finish initialization. */
		qp->msg_first = qp->msg_last = NULL;
		qp->msg_qnum = 0;
		qp->msg_qbytes = msginfo.msgmnb;
		qp->msg_lspid = qp->msg_lrpid = 0;
		qp->msg_stime = qp->msg_rtime = 0;
		qp->msg_ctime = hrestime.tv_sec;

		/* initialize reserve area */
		{
		int i;
			for (i = 0; i < 4; i++)
				qp->msg_perm.pad[i] = 0;
			qp->msg_pad1 = 0;
			qp->msg_pad2 = 0;
			qp->msg_pad3 = 0;

			for (i = 0; i < 3; i++)
				qp->msg_pad4[i] = 0;
		}
	}
	mutex_exit(&msg_lock);
	return (qp->msg_perm.seq * msginfo.msgmni + (qp - msgque));
}

/*
 * msgrcv - Msgrcv system call.
 */
static int
msgrcv(int msqid, struct msgbuf *msgp, int msgsz, long msgtyp, int msgflg)
{
	register struct msg		*mp;	/* ptr to msg on q */
	register struct msg		*pmp;	/* ptr to mp's predecessor */
	register struct msg		*smp;	/* ptr to best msg on q */
	register struct msg		*spmp;	/* ptr to smp's predecessor */
	struct msqid_ds			*qp;	/* ptr to associated q */
	register struct msglock		*lockp;
	int				sz;	/* transfer byte count */
	int				error;


	CPU_STAT_ADD(CPU, cpu_sysinfo.msg, 1);	/* bump msg send/rcv count */
	if (error = msgconv(msqid, &qp))
		return (set_errno(error));
	lockp = MSGLOCK(qp);
	if (error = ipcaccess(&qp->msg_perm, MSG_R, CRED()))
		goto msgrcv_out;
	if (msgsz < 0) {
		error = EINVAL;
		goto msgrcv_out;
	}
	smp = spmp = NULL;

findmsg:

	pmp = NULL;
	mp = qp->msg_first;
	if (msgtyp == 0)
		smp = mp;
	else
		for (; mp; pmp = mp, mp = mp->msg_next) {
			if (msgtyp > 0) {
				if (msgtyp != mp->msg_type)
					continue;
				smp = mp;
				spmp = pmp;
				break;
			}
			if (mp->msg_type <= -msgtyp) {
				if (smp && smp->msg_type <= mp->msg_type)
					continue;
				smp = mp;
				spmp = pmp;
			}
		}

	if (smp) {
		if ((unsigned)msgsz < smp->msg_ts)
			if (!(msgflg & MSG_NOERROR)) {
				error = E2BIG;
				goto msgrcv_out;
			} else
				sz = msgsz;
		else
			sz = smp->msg_ts;
		if (copyout((caddr_t)&smp->msg_type, (caddr_t)msgp,
		    sizeof (smp->msg_type))) {
			error = EFAULT;
			goto msgrcv_out;
		}
		if (sz &&
		    copyout((caddr_t)(msg + msginfo.msgssz * smp->msg_spot),
		    (caddr_t)msgp + sizeof (smp->msg_type), sz)) {
			error = EFAULT;
			goto msgrcv_out;
		}
		qp->msg_cbytes -= smp->msg_ts;
		qp->msg_lrpid = ttoproc(curthread)->p_pid;
		qp->msg_rtime = hrestime.tv_sec;
		msgfree(qp, spmp, smp);
		goto msgrcv_out;
	}
	if (msgflg & IPC_NOWAIT) {
		error = ENOMSG;
		goto msgrcv_out;
	}
	qp->msg_perm.mode |= MSG_RWAIT;

	/* Wait for new message  */
	if (!cv_wait_sig(&qp->msg_qnum_cv, &lockp->msglock_lock))  {
		mutex_exit(&lockp->msglock_lock);
		return (set_errno(EINTR));
	}

	/* Check for msg q deleted or reallocated */
	if ((qp->msg_perm.mode & IPC_ALLOC) == 0 ||
	    (msqid / msginfo.msgmni) != qp->msg_perm.seq)  {
		mutex_exit(&lockp->msglock_lock);
		return (set_errno(EIDRM));
	}
	goto findmsg;

msgrcv_out:

	mutex_exit(&lockp->msglock_lock);
	if (error)
		return (set_errno(error));
	return (sz);
}

/*
 * msgsnd - Msgsnd system call.
 */
static int
msgsnd(int msqid, struct msgbuf *msgp, int msgsz, int msgflg)
{
	struct msqid_ds			*qp;	/* ptr to associated q */
	register struct msg		*mp;	/* ptr to allocated msg hdr */
	register int			cnt;	/* byte count */
	register ulong			spot;	/* msg pool allocation spot */
	register struct msglock		*lockp;
	struct msqid_ds			*qp1;	/* alternate q ptr */
	long				type;	/* msg type */
	int				error;


	CPU_STAT_ADD(CPU, cpu_sysinfo.msg, 1);	/* bump msg send/rcv count */
	if (error = msgconv(msqid, &qp))
		return (set_errno(error));
	lockp = MSGLOCK(qp);
	if (error = ipcaccess(&qp->msg_perm, MSG_W, CRED()))
		goto msgsnd_out;
	if ((cnt = msgsz) < 0 || cnt > msginfo.msgmax) {
		error = EINVAL;
		goto msgsnd_out;
	}
	if (copyin((caddr_t)msgp, (caddr_t)&type, sizeof (type))) {
		error = EFAULT;
		goto msgsnd_out;
	}
	if (type < 1) {
		error = EINVAL;
		goto msgsnd_out;
	}
getres:

	/* Allocate space on q, message header, & buffer space. */
	if (cnt + qp->msg_cbytes > (uint)qp->msg_qbytes) {
		if (msgflg & IPC_NOWAIT) {
			error = EAGAIN;
			goto msgsnd_out;
		}
		qp->msg_perm.mode |= MSG_WWAIT;

		if (!cv_wait_sig(&qp->msg_cv, &lockp->msglock_lock)) {
			mutex_exit(&lockp->msglock_lock);
			if (error = msgconv(msqid, &qp1)) {
				lockp = MSGLOCK(qp1);
				mutex_exit(&lockp->msglock_lock);
				return (set_errno(EIDRM));
			}
			error = EINTR;
			qp->msg_perm.mode &= ~MSG_WWAIT;
			cv_broadcast(&qp->msg_cv);
			goto msgsnd_out;
		}

		/* check for q removed or reallocated */
		if ((qp->msg_perm.mode & IPC_ALLOC) == 0 ||
		    (msqid / msginfo.msgmni) != qp->msg_perm.seq) {
			mutex_exit(&lockp->msglock_lock);
			return (set_errno(EIDRM));
		}
		goto getres;
	}
	mutex_enter(&msg_lock);
	if (msgfp == NULL) {
		if (msgflg & IPC_NOWAIT) {
			error = EAGAIN;
			mutex_exit(&msg_lock);
			goto msgsnd_out;
		}
		mutex_exit(&lockp->msglock_lock);
		if (!cv_wait_sig(&msgfp_cv, &msg_lock)) {
			mutex_exit(&msg_lock);
			if (error = msgconv(msqid, &qp1))
				return (set_errno(error));
			error = EINTR;
			if (qp1 != qp) {
				lockp = MSGLOCK(qp1);
				mutex_exit(&lockp->msglock_lock);
				return (set_errno(error));
			}
			goto msgsnd_out;
		}
		mutex_exit(&msg_lock);
		if (msgconv(msqid, &qp) != 0)
			return (set_errno(EIDRM));
		goto getres;
	}

	mp = msgfp;
	msgfp = mp->msg_next;
	mutex_exit(&msg_lock);

	mutex_enter(&maplock(msgmap));
	if (cnt && (spot = rmalloc_locked(msgmap, btoq(cnt))) == NULL) {
		if (msgflg & IPC_NOWAIT) {
			error = EAGAIN;
			mutex_exit(&maplock(msgmap));
			goto msgsnd_out1;
		}
		mapwant(msgmap)++;
		mutex_enter(&msg_lock);
		mp->msg_next = msgfp;
		if (msgfp == NULL)
			cv_broadcast(&msgfp_cv);
		msgfp = mp;
		mutex_exit(&msg_lock);
		mutex_exit(&lockp->msglock_lock);
		if (!cv_wait_sig(&map_cv(msgmap), &maplock(msgmap))) {
			mutex_exit(&maplock(msgmap));
			if (error = msgconv(msqid, &qp1))
				return (set_errno(EIDRM));
			error = EINTR;
			goto msgsnd_out;
		}
		mutex_exit(&maplock(msgmap));
		if (msgconv(msqid, &qp) != 0)
			return (set_errno(EIDRM));
		goto getres;
	} else
		mutex_exit(&maplock(msgmap));

	/* Everything is available, copy in text and put msg on q. */
	if (cnt && copyin((caddr_t)msgp + sizeof (type),
	    (caddr_t)(msg + msginfo.msgssz * --spot), cnt)) {
		error = EFAULT;
		rmfree(msgmap, btoq(cnt), spot + 1);
		goto msgsnd_out1;
	}
	qp->msg_qnum++;
	qp->msg_cbytes += cnt;
	qp->msg_lspid = curproc->p_pid;
	qp->msg_stime = hrestime.tv_sec;
	mp->msg_next = NULL;
	mp->msg_type = type;
	mp->msg_ts = (ushort)cnt;
	mp->msg_spot = (short)(cnt ? spot : -1);
	if (qp->msg_last == NULL)
		qp->msg_first = qp->msg_last = mp;
	else {
		qp->msg_last->msg_next = mp;
		qp->msg_last = mp;
	}
	if (qp->msg_perm.mode & MSG_RWAIT) {
		qp->msg_perm.mode &= ~MSG_RWAIT;
		cv_broadcast(&qp->msg_qnum_cv);
	}
	goto msgsnd_out;

msgsnd_out1:

	mutex_enter(&msg_lock);
	mp->msg_next = msgfp;
	if (msgfp == NULL)
		cv_broadcast(&msgfp_cv);
	msgfp = mp;
	mutex_exit(&msg_lock);

msgsnd_out:

	mutex_exit(&lockp->msglock_lock);
	if (error)
		return (set_errno(error));
	return (0);
}

/*
 * msgsys - System entry point for msgctl, msgget, msgrcv, and msgsnd
 * system calls.
 */
static int
msgsys(int opcode, int a1, int a2, int a3, int a4, int a5)
{
	register int error;


	switch (opcode) {
	case MSGGET:
		error = msgget((key_t)a1, a2);
		break;
	case MSGCTL:
		error = msgctl(a1, a2, (struct msqid_ds *)a3);
		break;
	case MSGRCV:
		error = msgrcv(a1, (struct msgbuf *)a2, a3, (long)a4, a5);
		break;
	case MSGSND:
		error = msgsnd(a1, (struct msgbuf *)a2, a3, a4);
		break;
	default:
		error = set_errno(EINVAL);
		break;
	}

	return (error);
}
