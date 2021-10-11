/*
 * Copyright (c) 1993 Sun Microsystems, Inc.  All Rights Reserved.
 */

#pragma	ident "@(#)emap.c	1.19	96/06/02 SMI"

/*
 * EMAP module:  XENIX PC keyboard extended mapping:  dead key and
 *		 compose key sequence processing or in short emapping.
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/errno.h>
#include <sys/debug.h>
#include <sys/cmn_err.h>
#include <sys/kmem.h>
#include <sys/vt.h>
#include <sys/ascii.h>
#include <sys/termios.h>
#include <sys/strtty.h>
#include <sys/kd.h>
#include <sys/ws/chan.h>
#include <sys/ws/tcl.h>
#include <sys/emap.h>
#include <sys/conf.h>
#include <sys/modctl.h>
#include <sys/ddi.h>
#include <sys/sunddi.h>

int _init(void);
int _fini(void);
int _info(struct modinfo *);
static int emap_open(queue_t *const, const dev_t *const,
    const int, const int, const cred_t *const);
static int emap_close(queue_t *const, const int, const cred_t *const);
static int emap_rput(queue_t *const, mblk_t *);
static int emap_rsrv(queue_t *const);
static int emap_wput(queue_t *const, mblk_t *);
static int emap_wsrv(queue_t *const);
static void emap_do_ioctl(queue_t *const, mblk_t *const);
static void emap_do_iocdata(queue_t *const, mblk_t *const,
    struct iocblk *const);
static void emap_copyin(queue_t *const, mblk_t *const, const int,
    const emap_state_t *const);
static void emap_copyout(queue_t *const, mblk_t *const, mblk_t *,
    const int, const emap_state_t *const);
static void emap_iocack(queue_t *const, mblk_t *const,
    struct iocblk *const, const int);
static void emap_iocnack(queue_t *const, mblk_t *const,
    struct iocblk *const, const int, const int);
static mblk_t *emap_input_msg(queue_t *const, mblk_t *, emap_state_t *);
static int emap_mapin(mblk_t *const, emap_state_t *const);
static mblk_t *emap_output_msg(queue_t *const, mblk_t *, emap_state_t *);
static mblk_t *emap_output_blk(mblk_t *, const int ibsize, emap_state_t *);
static unsigned char *emap_mapout(unsigned char, int *const,
    const emap_state_t *const);
static void emap_outchar(queue_t *, const unsigned char, emap_state_t *const);
static void emap_getmap(mblk_t *const, const emap_state_t *const);
static int emap_setmap(mblk_t *, emap_state_t *const);
static int emap_chkmap(const emp_t);

#define	ISDIGIT(c)	((c) >= '0' && (c) <= '9')
#define	M_TYPE(mp)	((mp)->b_datap->db_type)

static unsigned char e_map[EMAP_SIZE];

/*
 * emap is THE per module extended map, which should not change
 * regardless of number of calls to emap_open() or emap_close().
 */
static struct emap emap = {
	(emp_t) &e_map,
	(short) 0,
	(short) 0,
	(short) 0
};
/*
 * emap_state is THE per module state, which should not change
 * regardless of number of calls to emap_open() or emap_close().
 */
static emap_state_t emap_state = {
	(char) ES_NULL,
	(char) ES_NULL,
	(char) EF_NULL,
	(char) 0,
	(unsigned char) NULL,
	&emap
};

static kmutex_t emap_lock;

static struct module_info emap_iinfo = {
	0,
	"emap",
	0,
	EMAPPSZ,
	1000,
	100
};

static struct qinit emap_rinit = {
	emap_rput,
	emap_rsrv,
	emap_open,
	emap_close,
	NULL,
	&emap_iinfo,
	NULL
};

static struct module_info emap_oinfo = {
	0,
	"emap",
	0,
	EMAPPSZ,
	1000,
	100
};

static struct qinit emap_winit = {
	emap_wput,
	emap_wsrv,
	NULL,
	NULL,
	NULL,
	&emap_oinfo,
	NULL
};

struct streamtab emap_info = {
	&emap_rinit,
	&emap_winit,
	NULL,
	NULL
};

/*
 * This is the loadable module wrapper.
 */

/*
 * D_MTQPAIR effectively makes the module single threaded.
 * There can be only one thread active in the module at any time.
 * It may be a read or write thread.
 */
#define	EMAP_CONF_FLAG	(D_NEW | D_MTQPAIR | D_MP)

static struct fmodsw fsw = {
	"emap",
	&emap_info,
	EMAP_CONF_FLAG
};

static struct modlstrmod modlstrmod = {
	&mod_strmodops,
	"EMAP dead/compose key processor",
	&fsw
};

/*
 * Module linkage information for the kernel.
 */
static struct modlinkage modlinkage = {
	MODREV_1,
	&modlstrmod,
	NULL
};

int
_init(void)
{
	register int rc;

	mutex_init(&emap_lock, "emap lock", MUTEX_DEFAULT, NULL);
	if ((rc = mod_install(&modlinkage)) != 0) {
		mutex_destroy(&emap_lock);
	}
	return (rc);
}

int
_fini(void)
{
	register int rc;

	if ((rc = mod_remove(&modlinkage)) == 0)
		mutex_destroy(&emap_lock);
	return (rc);
}

int
_info(struct modinfo *modinfop)
{
	return (mod_info(&modlinkage, modinfop));
}

/*ARGSUSED1*/
static int
emap_open(queue_t *const qp, const dev_t *const devp,
    const int oflag, const int sflag, const cred_t *const crp)
{
	mutex_enter(&emap_lock);

	/*
	 * If q_ptr already set, we've allocated a struct already
	 */
	if (qp->q_ptr != NULL) {
		mutex_exit(&emap_lock);
		return (0);	 /* not failure -- just simultaneous open */
	}

	/*
	 * Both the read and write queues share the same state structures.
	 */
	qp->q_ptr = (caddr_t) &emap_state;
	WR(qp)->q_ptr = (caddr_t) &emap_state;
	mutex_exit(&emap_lock);

	qprocson(qp);
	return (0);
}

/*ARGSUSED1*/
static int
emap_close(queue_t *const qp, const int flag, const cred_t *const crp)
{
	ASSERT(qp != NULL);

	qprocsoff(qp);
	flushq(qp, FLUSHDATA);
	flushq(OTHERQ(qp), FLUSHDATA);
	mutex_enter(&emap_lock);

	/* Dump the associated state structure */
	qp->q_ptr = NULL;

	mutex_exit(&emap_lock);
	return (0);
}

/*
 * Put procedure for input from driver end of stream (read queue).
 */
static int
emap_rput(queue_t *const qp, mblk_t *mp)
{
	ASSERT(qp != NULL);
	ASSERT(mp != NULL);

	/*
	 * Handle all the related high priority messages here, hence
	 * should spend the least amount of time here.
	 */
	switch (M_TYPE(mp)) {

	case M_FLUSH:
		if (*mp->b_rptr & FLUSHR)
			flushq(qp, FLUSHDATA);
		return (putnext(qp, mp));		/* pass it on */

	case M_DATA: {
		register emap_state_t *sp = (emap_state_t *) qp->q_ptr;

		if (EMAP_STATE(sp) != ES_NULL &&
		    !(EMAP_FLAGS(sp) & EF_SCANCODE_RAW))
			return (putq(qp, mp));
		return (putnext(qp, mp));	/* nothing to process here */
	}

	default:
		return (putnext(qp, mp));		/* pass it on */
	}
	/*NOTREACHED*/
}

/*
 * Service procedure for input from driver end of stream (read queue).
 */
static int
emap_rsrv(queue_t *const qp)
{

	register mblk_t *mp;

	ASSERT(qp != NULL);

	while ((mp = getq(qp)) != NULL) {
		ASSERT(M_TYPE(mp) == M_DATA);

		if (!canputnext(qp))
			return (putbq(qp, mp));	/* read side is blocked */

		switch (M_TYPE(mp)) {
		/*
		 * If you add any other message types to be processed here,
		 * make sure putq() is done on them in the put procedure.
		 */

		case M_DATA: {
			register emap_state_t *sp = (emap_state_t *) qp->q_ptr;

			/*
			 * Although we have done the following test in the
			 * put routine, we should do it again, in case state
			 * has changed since then.
			 */
			if (EMAP_STATE(sp) != ES_NULL &&
			    !(EMAP_FLAGS(sp) & EF_SCANCODE_RAW))
				mp = emap_input_msg(qp, mp, sp);
			(void) putnext(qp, mp);
			break;
		}

		default:
			cmn_err(CE_WARN, "emap_rsrv: bad message type (%#x)\n",
				M_TYPE(mp));
			(void) putnext(qp, mp);		/* pass it on */
			break;
		}
	}
	return (0);
}

/*
 * Put procedure for write from user end of stream (write queue).
 */
static int
emap_wput(queue_t *const qp, mblk_t *mp)
{
	register emap_state_t *sp;

	ASSERT(qp != NULL);
	ASSERT(mp != NULL);

	sp = (emap_state_t *) qp->q_ptr;

	/*
	 * Handle all the related high priority messages here, hence
	 * should spend the least amount of time here.
	 */
	switch (M_TYPE(mp)) {	/* handle hi pri messages here */

	case M_FLUSH:
		if (*mp->b_rptr & FLUSHW)
			flushq(qp, FLUSHDATA);
		return (putnext(qp, mp));		/* pass it on */

	case M_IOCTL: {
		register struct iocblk *iocp = (struct iocblk *) mp->b_rptr;

		switch (iocp->ioc_cmd) {
		case LDSMAP:
		case LDGMAP:
		case LDNMAP:
		case LDDMAP:
		case LDEMAP:
			if (EMAP_FLAGS(sp) & EF_SCANCODE_RAW) {
				emap_iocnack(qp, mp, iocp, ENOTTY, -1);
				return (-1);
			}
			/*FALLTHROUGH*/
		case KDSKBMODE:
			/*
			 * Only the ioctl's that this module handles.
			 * Defer processing to the write queue service routine
			 */
			return (putq(qp, mp));
		default:
			/* nothing to process here */
			return (putnext(qp, mp));
		}
	}

	case M_IOCDATA: {
		register struct iocblk *iocp = (struct iocblk *) mp->b_rptr;

		switch (iocp->ioc_cmd) {
		case LDGMAP:
		case LDSMAP:
			/*
			 * Only the ioctl's which do
			 * emap_copyin() or emap_copyout()
			 */
			if (EMAP_FLAGS(sp) & EF_SCANCODE_RAW) {
				emap_iocnack(qp, mp, iocp, ENOTTY, -1);
				return (-1);
			}
			emap_do_iocdata(qp, mp, iocp);
			return (0);
		default:
			return (putnext(qp, mp)); /* nothing to process here */
		}
	}

	case M_DATA: {
		register emap_state_t *sp = (emap_state_t *) qp->q_ptr;

		if (EMAP_STATE(sp) != ES_NULL &&
		    !(EMAP_FLAGS(sp) & EF_SCANCODE_RAW))
			return (putq(qp, mp));
		return (putnext(qp, mp));	/* nothing to process here */
	}

	default:
		return (putnext(qp, mp));	/* pass it on */
	}
	/*NOTREACHED*/
}

/*
 * Service procedure for write from user end of stream (write queue).
 */
static int
emap_wsrv(queue_t *const qp)
{
	register mblk_t *mp;

	ASSERT(qp != NULL);

	while ((mp = getq(qp)) != NULL) {
		ASSERT(M_TYPE(mp) == M_IOCTL || M_TYPE(mp) == M_DATA);

		if (!canputnext(qp))
			return (putbq(qp, mp));	/* write side is blocked */

		switch (M_TYPE(mp)) {
		/*
		 * If you add any other message types to be processed here,
		 * make sure putq() is done on them in the put procedure.
		 */

		case M_IOCTL:
			emap_do_ioctl(qp, mp);
			break;

		case M_DATA: {
			register emap_state_t *sp = (emap_state_t *) qp->q_ptr;

			/*
			 * Although we have done the following test in the
			 * put routine, we should do it again, in case state
			 * has changed since then.
			 */
			if (EMAP_STATE(sp) != ES_NULL &&
			    !(EMAP_FLAGS(sp) & EF_SCANCODE_RAW))
				mp = emap_output_msg(qp, mp, sp);
			(void) putnext(qp, mp);
			break;
		}

		default:
			cmn_err(CE_WARN, "emap_wsrv: bad message type (%#x)\n",
				M_TYPE(mp));
			(void) putnext(qp, mp);		/* pass it on */
			break;
		}
	}
	return (0);
}

/*
 * Called from the write queue service procesure as the normal priority
 * M_IOCTL is received from the stream head.
 */
static void
emap_do_ioctl(queue_t *const qp, mblk_t *const mp)
{
	register emap_state_t *sp;
	register struct iocblk *iocp;

	ASSERT(qp != NULL);
	ASSERT(mp != NULL);

	iocp = (struct iocblk *) mp->b_rptr;
	sp = (emap_state_t *) qp->q_ptr;

	if (iocp->ioc_count != TRANSPARENT) {
		emap_iocnack(qp, mp, iocp, EINVAL, -1);
		return;
	}
	if (iocp->ioc_cmd != KDSKBMODE && EMAP_FLAGS(sp) & EF_SCANCODE_RAW) {
		emap_iocnack(qp, mp, iocp, ENOTTY, -1);
		return;
	}


	switch (iocp->ioc_cmd) {

	case LDSMAP:
		ASSERT(!(EMAP_FLAGS(sp) & EF_SCANCODE_RAW));

		if (EMAP_FLAGS(sp) & EF_GOT_LDSMAP_DATA) {
			int error;
			register mblk_t *tmp = unlinkb(mp);

			error = emap_setmap(tmp, sp);
			EMAP_FLAGS(sp) &= ~EF_GOT_LDSMAP_DATA;
			freemsg(tmp);
			if (error) {
				emap_iocnack(qp, mp, iocp, error, -1);
				return;
			}
			emap_iocack(qp, mp, iocp, 0);
		} else
			emap_copyin(qp, mp, EMAP_SIZE, sp);
		return;

	case LDGMAP: {
		register mblk_t *datamp;

		ASSERT(!(EMAP_FLAGS(sp) & EF_SCANCODE_RAW));

		if (EMAP_STATE(sp) == ES_NULL) {
			emap_iocnack(qp, mp, iocp, EINVAL, -1);
			return;
		}

		if ((datamp = allocb(EMAP_SIZE, BPRI_MED)) == NULL) {
			emap_iocnack(qp, mp, iocp, ENOSR, -1);
			return;
		}
		emap_getmap(datamp, sp);
		emap_copyout(qp, mp, datamp, EMAP_SIZE, sp);
		return;
	}

	case LDNMAP:
		ASSERT(!(EMAP_FLAGS(sp) & EF_SCANCODE_RAW));

		if (EMAP_STATE(sp) != ES_NULL) {
			if (qp->q_next != NULL)
				(void) putctl(qp->q_next, M_START);
			flushq(qp, FLUSHDATA);
			flushq(OTHERQ(qp), FLUSHDATA);

			/*
			 * Turn off keyboard extended mapping.
			 */
			EMAP_STATE(sp) = ES_NULL;
		}
		emap_iocack(qp, mp, iocp, 0);
		return;

	case LDDMAP:
		ASSERT(!(EMAP_FLAGS(sp) & EF_SCANCODE_RAW));

		if (EMAP_STATE(sp) == ES_NULL) {
			emap_iocnack(qp, mp, iocp, EINVAL, -1);
			return;
		}

		if (EMAP_STATE(sp) != ES_OFF) {
			if (qp->q_next != NULL)
				(void) putctl(qp->q_next, M_START);
			flushq(qp, FLUSHDATA);
			flushq(OTHERQ(qp), FLUSHDATA);
			/*
			 * Turn off keyboard extended mapping temprorily.
			 */
			EMAP_STATE(sp) = ES_OFF;
		}
		emap_iocack(qp, mp, iocp, 0);
		return;

	case LDEMAP:
		ASSERT(!(EMAP_FLAGS(sp) & EF_SCANCODE_RAW));

		if (EMAP_STATE(sp) == ES_NULL) {
			emap_iocnack(qp, mp, iocp, EINVAL, -1);
			return;
		}

		/*
		 * Start keyboard extended mapping.
		 */
		EMAP_STATE(sp) = ES_ON;
		emap_iocack(qp, mp, iocp, 0);
		return;

	case KDSKBMODE:

		/*
		 * Just intercept it.  i.e. Should not return or ack/nack,
		 * instead must putnext() under ANY conditions, even in
		 * error situations and let "target" module deal with it.
		 */
#ifdef _VPIX
		if (mp->b_cont == NULL || mp->b_cont->b_cont == NULL)
#else /* !_VPIX */
		if (mp->b_cont == NULL)
#endif /* !_VPIX */
			(void) putnext(qp, mp);	   /* error, but pass it on */

#ifdef _VPIX
		switch (*(int *) mp->b_cont->b_cont->b_rptr)
#else /* !_VPIX */
		switch (*(int *) mp->b_cont->b_rptr)
#endif /* !_VPIX */
		{

		case K_RAW:
			EMAP_FLAGS(sp) |= EF_SCANCODE_RAW;
			EMAP_SAVED_STATE(sp) = EMAP_STATE(sp);
			if (EMAP_STATE(sp) != ES_NULL) {
				if (qp->q_next != NULL)
					(void) putctl(qp->q_next, M_START);
				flushq(qp, FLUSHDATA);
				flushq(OTHERQ(qp), FLUSHDATA);

				/*
				 * Turn off keyboard extended mapping.
				 */
				EMAP_STATE(sp) = ES_NULL;

			}
			break;

		case K_XLATE:
			EMAP_FLAGS(sp) &= ~EF_SCANCODE_RAW;
			EMAP_STATE(sp) = EMAP_SAVED_STATE(sp);
			break;

		default:
			cmn_err(CE_WARN, "emap_do_ioctl: bad KDSKBMODE arg\n");
			break;
		}

		(void) putnext(qp, mp);		/* pass it on */
		break;

	default:
		(void) putnext(qp, mp);		/* pass it on */
		break;
	}
}

/*
 * Called from the write queue put procesure as the high priority
 * M_IOCDATA is received from the stream head.  Therefore, we SHOULD
 * NOT SPEND A LOT OF TIME IN HERE.  If there is a time consuming
 * task to be handled construct the message, put it on the write
 * queue to be processed by the write queue service procedure.
 */
static void
emap_do_iocdata(queue_t *const qp, mblk_t *const mp, struct iocblk *const iocp)
{
	register struct copyresp *csp;
	register emap_state_t *sp;

	ASSERT(qp != NULL);
	ASSERT(mp != NULL);
	ASSERT(iocp != NULL);

	sp = (emap_state_t *) qp->q_ptr;

	ASSERT(!(EMAP_FLAGS(sp) & EF_SCANCODE_RAW));

	csp = (struct copyresp *) iocp;

	switch (iocp->ioc_cmd) {

	case LDGMAP:
		if (csp->cp_rval) { 	/* already nak'ked for us */
			freemsg(mp);
			return;
		}
		emap_iocack(qp, mp, iocp, 0);
		return;

	case LDSMAP:
		ASSERT(!(EMAP_FLAGS(sp) & EF_GOT_LDSMAP_DATA));

		if (csp->cp_rval) {	 /* already nak'ked for us */
			freemsg(mp);
			return;
		}
		if (mp->b_cont != NULL) {
			register mblk_t *tmp;

			if ((tmp = msgpullup(mp->b_cont, EMAP_SIZE)) == NULL) {
				emap_iocnack(qp, mp, iocp, EINVAL, -1);
				return;
			}
			freemsg(mp->b_cont);
			mp->b_cont = tmp;
		}
		/*
		 * Defer processing it to emap_wsrv() and eventually
		 * to emap_do_ioctl().
		 */
		EMAP_FLAGS(sp) |= EF_GOT_LDSMAP_DATA;
		iocp->ioc_count = TRANSPARENT;
		M_TYPE(mp) = M_IOCTL;
		(void) putq(qp, mp);
		return;

	default:
		(void) putnext(qp, mp);	 /* not for us, pass it forward */
		break;
	}
}

static void
emap_copyin(queue_t *const qp, mblk_t *const mp, const int size,
	    const emap_state_t *const sp)
{
	register struct copyreq *cqp;

	ASSERT(qp != NULL);
	ASSERT(mp != NULL);
	ASSERT(mp->b_cont != NULL);
	ASSERT(!(EMAP_FLAGS(sp) & EF_SCANCODE_RAW));

	cqp = (struct copyreq *) mp->b_rptr;
	cqp->cq_addr = (caddr_t) *(long *) mp->b_cont->b_rptr;
	cqp->cq_size = size;
	cqp->cq_private = (mblk_t *) sp;
	cqp->cq_flag = 0;
	mp->b_wptr = mp->b_rptr + sizeof (struct copyreq);
	M_TYPE(mp) = M_COPYIN;
	freemsg(mp->b_cont);
	mp->b_cont = NULL;
	qreply(qp, mp);
}

static void
emap_copyout(queue_t *const qp, mblk_t *const mp, mblk_t *nmp,
    const int size, const emap_state_t *const sp)
{
	register struct copyreq *cqp;

	ASSERT(qp != NULL);
	ASSERT(mp != NULL);
	ASSERT(mp->b_cont != NULL);
	ASSERT(!(EMAP_FLAGS(sp) & EF_SCANCODE_RAW));

	cqp = (struct copyreq *) mp->b_rptr;
	cqp->cq_addr = (caddr_t) *(long *) mp->b_cont->b_rptr;
	cqp->cq_size = size;
	cqp->cq_private = (mblk_t *) sp;
	cqp->cq_flag = 0;
	mp->b_wptr = mp->b_rptr + sizeof (struct copyreq);
	M_TYPE(mp) = M_COPYOUT;

	freemsg(mp->b_cont);
	mp->b_cont = nmp;
	qreply(qp, mp);
}

static void
emap_iocack(queue_t *const qp, mblk_t *const mp,
	    struct iocblk *const iocp, const int rval)
{
	register mblk_t	*tmp;

	ASSERT(qp != NULL);
	ASSERT(mp != NULL);
	ASSERT(iocp != NULL);
	ASSERT(!(EMAP_FLAGS((emap_state_t *) qp->q_ptr) & EF_SCANCODE_RAW));

	M_TYPE(mp) = M_IOCACK;
	iocp->ioc_rval = rval;
	iocp->ioc_count = iocp->ioc_error = 0;
	if ((tmp = unlinkb(mp)) != NULL)
		freemsg(tmp);
	qreply(qp, mp);
}

static void
emap_iocnack(queue_t *const qp, mblk_t *const mp,
    struct iocblk *const iocp, const int error, const int rval)
{
	ASSERT(qp != NULL);
	ASSERT(mp != NULL);
	ASSERT(iocp != NULL);

	M_TYPE(mp) = M_IOCACK;
	iocp->ioc_rval = rval;
	iocp->ioc_error = error;
	qreply(qp, mp);
}

/*
 * Perform input keyboard extended mapping on a message placed
 * on the read queue.
 */
static mblk_t *
emap_input_msg(queue_t *const qp, register mblk_t *imp,
    register emap_state_t *sp)
{
	int ibsize;			/* incoming block size */
	register mblk_t *nexti = imp;
	register mblk_t *omp = imp;
	/*
	 * imp: 	head of incoming msgb's we're examining and
	 *		the current one	being processed.
	 *
	 * omp: 	head of outgoing msgb's we're constructing
	 *
	 * nexti:	ptr to 1 msgb next to the current on input
	 */

	ASSERT(qp != NULL);
	ASSERT(sp != NULL);
	ASSERT(EMAP_STATE(sp) != ES_NULL);
	ASSERT(!(EMAP_FLAGS(sp) & EF_SCANCODE_RAW));

	while (imp != NULL) {
		nexti = unlinkb(imp);
		/*
		 * If imp had only one msgb left, nexti is NULL now.
		 */
		ibsize = imp->b_wptr - imp->b_rptr;

		if (ibsize > 0) {
			ibsize = emap_mapin(imp, sp);
			/*
			 * imp is recycled.
			 */
			if (EMAP_ERROR(sp)) {
				emap_outchar(WR(qp), (unsigned char) CTRL('g'),
				    sp);
				/*
				 * Exceptionally ingnoring EMAP_ERROR(sp) != 0
				 * which could result from the call to
				 * emap_outchar() here.
				 */
				EMAP_ERROR(sp) = 0;
			}
		}
		if (imp != NULL)
			imp->b_cont = nexti;
		else
			cmn_err(CE_WARN, "emap_input_msg: corrupted "
			    "msgb resulted from emap_mapin\n");
		imp = nexti;
	}
	return (omp);
}

/*
 * Given a pointer to an input message block placed on the read queue,
 * maps its character contents in place and returns its new length.
 * The input string will not expand, it may even contract - 2 dead key
 * sequence or 3 compose key sequence map to a single output mapped
 * character.  So, we can recycle the input msgb and there is no fear
 * of running out of space in its data buffer.
 */
static int
emap_mapin(mblk_t *const ibp, emap_state_t *const sp)
{
	char error = 0;
	register int i;
	unsigned char c;		/* current character in input string */
	unsigned char mc;		/* mapped character */
	register unsigned char *icp;	/* input string */
	register unsigned char *ocp;	/* resultant ouput string */
	struct emap *emp;		/* ptr to control struct of mapping */
	emp_t mapp;			/* ptr to emapping table */
	register emip_t eip;		/* ptr to dead or compose indexes */
	register emop_t eop;		/* ptr to the resultant mapped struct */

	/*
	 * ibp should be a single msgb only
	 */
	ASSERT(sp != NULL);
	ASSERT(ibp != NULL);
	ASSERT(EMAP_STATE(sp) != ES_NULL);
	ASSERT(!(EMAP_FLAGS(sp) & EF_SCANCODE_RAW));

	ocp = icp = ibp->b_rptr;
	emp = EMAP_EMAP(sp);
	mapp = emp->e_map;
	while (icp < ibp->b_wptr) {
		c = *icp++;		/* grab a char from input string */
		mc = mapp->e_imap[c];	/* entry in the input map table */

		switch (EMAP_STATE(sp)) {

		case ES_OFF:
			if (c != mapp->e_toggle) {
				*ocp++ = c;
				continue;
			} else
				EMAP_STATE(sp) = ES_ON;
			break;

		case ES_ON:
			if ((mc != EMAPED) || (c == EMAPED)) {
				/*
				 * Substitute char with its mapped character
				 */
				*ocp++ = mc;
				continue;
			}
			/*
			 * e_imap[c] is NULL, i.e. either toggle, compose
			 * key sequence or dead key sequence.
			 */
			if (c == mapp->e_toggle) {
				EMAP_STATE(sp) = ES_OFF;
				break;
			}
			if (c == mapp->e_comp)
				/*
				 * This is the unique compose key
				 */
				EMAP_STATE(sp) = ES_COMP1;
			else {
				/*
				 * Dead key sequence, save the 1st one
				 * and set the state so that the next
				 * is checked to see if it is in the
				 * correct dead key sequence.
				 */
				EMAP_SAVED(sp) = c;
				EMAP_STATE(sp) = ES_DEAD;
			}
			break;

		case ES_COMP1:
			if (mc == EMAPED) {
				if (c == mapp->e_comp || c == mapp->e_toggle)
					/*
					 * Cannot press compose twice and
					 * cannot press toggle key
					 * in the compose key sequence
					 */
					++error;
				else {
					EMAP_SAVED(sp) = c;
					EMAP_STATE(sp) = ES_COMP2;
				}
			} else {
				EMAP_SAVED(sp) = mc;
				EMAP_STATE(sp) = ES_COMP2;
			}
			break;

		case ES_DEC:
			/*
			 * Numeric compose sequence
			 */
			if (!ISDIGIT(mc))
				++error;
			else
				*ocp++ = EMAP_SAVED(sp) + (mc - '0');
			EMAP_STATE(sp) = ES_ON;
			break;

		case ES_DEAD:
			if (mc == EMAPED) {
				/*
				 * The 2nd one in the dead key sequence
				 * cannot map to EMAPED - zero!
				 */
				++error;
				/*
				 * Discard the dead key and if it was compose
				 * key that was pressed, start taking in the
				 * compose sequence, otherwise, back to normal
				 * base mapping state.
				 */
				EMAP_STATE(sp) =
				    (c == mapp->e_comp) ? ES_COMP1 : ES_ON;
				break;
			}
			eip = mapp->e_dind;
			/*
			 * eip is now pointing to the dead key indexes
			 */
			i = emp->e_ndind;	/* # of dead indexes */
			c = EMAP_SAVED(sp);
			goto dcsearch;

		case ES_COMP2:
			if (mc == EMAPED) {
				if (c == mapp->e_comp || c == mapp->e_toggle) {
					/*
					 * Cannot press compose twice and
					 * cannot press toggle key
					 * in the compose key sequence
					 */
					++error;
					EMAP_STATE(sp) = ES_COMP1;
					break;
				}
				mc = c;
			}

			/*
			 * Let's check if this is a numeric compose sequence
			 */
			c = EMAP_SAVED(sp);
			if (ISDIGIT(c) && ISDIGIT(mc)) {
				EMAP_SAVED(sp) = (c - '0')*100 + (mc - '0')*10;
				EMAP_STATE(sp) = ES_DEC;
				break;
			}

			eip = (emip_t) ((unsigned char *) mapp + mapp->e_cind);
			/*
			 * eip is now pointing to the compose indexes.
			 */
			i = emp->e_ncind;	/* # of compose indexes */

dcsearch:
			/*
			 * eip is either pointing to the dead key indexes
			 * or to the compose indexes.
			 */
			while (--i >= 0) {
				if (eip->e_key == c)
					break;		/* found it! */
				++eip;
			}
			if (i >= 0) {		/* found it! */
				i = eip->e_ind;
				eop = (emop_t) ((unsigned char *) mapp +
						mapp->e_dctab) + i;
				i = (++eip)->e_ind - i;
				/*
				 * i is the index of the next one.
				 */
				c = mc;		/* char we are looking for */
				while (--i >= 0) {
					if (eop->e_key == c) {
						/*
						 * Found it!  Now put it in
						 * the output string.
						 */
						*ocp++ = eop->e_out;
						break;
					}
					++eop;
				}
			}
			if (i < 0)
				++error;	/* no match for the sequence */
			EMAP_STATE(sp) = ES_ON;
			break;

		}

	}

	EMAP_ERROR(sp) = error;
	ibp->b_wptr = ocp;
	return ((int) (ibp->b_wptr - ibp->b_rptr));
}

/*
 * Perform output keyboard extended mapping on a MESSAGE placed on the
 * write queue, accumulating and returning the outgoing characters in
 * a new message.
 */
static mblk_t *
emap_output_msg(queue_t *const qp, register mblk_t *imp,
    register emap_state_t *sp)
{
	int ibsize;			/* incoming block size */
	register mblk_t *nexti = imp;
	register mblk_t *omp = NULL;
	register mblk_t *nexto = NULL;
	/*
	 * imp: 	head of incoming msgb's we're examining and
	 *		the current one	being processed.
	 *
	 * omp: 	head of outgoing msgb's we're constructing
	 *
	 * nexti:	ptr to 1 msgb next to the current on input
	 *
	 * nexto:	ptr to 1 msgb next to the current on output
	 */

	ASSERT(qp != NULL);
	ASSERT(sp != NULL);
	ASSERT(EMAP_STATE(sp) != ES_NULL);
	ASSERT(!(EMAP_FLAGS(sp) & EF_SCANCODE_RAW));

	while (imp != NULL) {
		nexti = unlinkb(imp);
		/*
		 * If imp had only one msgb left, nexti is NULL now.
		 */
		ibsize = imp->b_wptr - imp->b_rptr;

		if (ibsize > 0) {
			imp = emap_output_blk(imp, ibsize, sp);
			/*
			 * If emap_output_blk() succeeds, imp is changed
			 * completely, it even points to a different msgb.
			 */
			if (EMAP_ERROR(sp))
				emap_outchar(WR(qp), (unsigned char) CTRL('g'),
				    sp);
				/*
				 * Exceptionally ingnoring EMAP_ERROR(sp) != 0
				 * which could result from the call to
				 * emap_outchar() here.
				 */
				EMAP_ERROR(sp) = 0;
		}
		if (omp == NULL)	/* the very 1st blk */
			nexto = omp = imp;
		else {
			ASSERT(nexto != NULL);
			nexto->b_cont =  imp;
			nexto = imp;
		}
		imp = nexti;
	}
	return (omp);
}

/*
 * Perform output keyboard extended mapping on a message BLOCK placed on the
 * write queue, accumulating and returning the outgoing characters in a new
 * message block.
 */
static mblk_t *
emap_output_blk(register mblk_t *ibp, const int ibsize,
    register emap_state_t *sp)
{
	mblk_t *obp;			/* resultant mapped output blk */
	register int obsize = 0;	/* total # of bytes in omp data blk */
	register int n;
	struct olen_cp_t {
		int len;		/* length of the outgoing map */
		unsigned char *cp;	/* outgoing map */
	} *olen_cp;

	ASSERT(sp != NULL);
	ASSERT(ibp != NULL);
	ASSERT(ibp->b_wptr - ibp->b_rptr == ibsize);
	ASSERT(ibsize > 0);
	ASSERT(EMAP_STATE(sp) != ES_NULL);
	ASSERT(!(EMAP_FLAGS(sp) & EF_SCANCODE_RAW));

	olen_cp = kmem_alloc(ibsize * sizeof (struct olen_cp_t), KM_SLEEP);
	/*
	 * Fill in the olen_cp array with addresses and lengths of the
	 * outgoing maps, and calculate the total required size of the
	 * outgoing blk in obsize.
	 */
	for (n = ibsize; --n >= 0; olen_cp++) {
		unsigned char c = *ibp->b_rptr;

		ibp->b_rptr++;
		olen_cp->cp = emap_mapout(c, &olen_cp->len, sp);
		obsize += olen_cp->len;
	}
	olen_cp -= ibsize;

	if ((obp = allocb(obsize, BPRI_MED)) == NULL) {
		EMAP_ERROR(sp) = ENOSR;
		kmem_free(olen_cp, ibsize * sizeof (struct olen_cp_t));
		cmn_err(CE_WARN, "emap_output_blk: out of msg blocks\n");
		return (ibp);	/* not allowed to return NULL */
	}
	freemsg(ibp);

	/*
	 * Fill in the outgoing blk with all the outgoing maps in the
	 * olen_cp array of pointers.
	 */

	for (n = ibsize; --n >= 0; olen_cp++) {
		while (olen_cp->len-- > 0)
			*obp->b_wptr++ = *olen_cp->cp++;
	}
	olen_cp -= ibsize;

	kmem_free(olen_cp, ibsize * sizeof (struct olen_cp_t));
	obp->b_cont = NULL;
	return (obp);
}

/*
 * Output keyboard extended mapping:  Given a character, return a pointer
 * to its output mapped character or string of characters.  Also, return
 * the length of the resultant character or string.
 */
static unsigned char *
emap_mapout(unsigned char c, int *const lenp, const emap_state_t *const sp)
{
	struct emap *emp;
	emp_t mapp;			/* ptr to emapping table */
	unsigned char *ecp;

	ASSERT(sp != NULL);
	ASSERT(EMAP_STATE(sp) != ES_NULL);
	ASSERT(!(EMAP_FLAGS(sp) & EF_SCANCODE_RAW));

	emp = EMAP_EMAP(sp);
	mapp = emp->e_map;
	ecp = &mapp->e_omap[c];
	if (*ecp == EMAPED) {	/* i.e. mapped to some string */
		register int i;
		unsigned char *esp;
		register emip_t eip;	/* ptr to indexes string indexes */

		eip = (emip_t) ((unsigned char *) mapp + mapp->e_sind);
		esp = (unsigned char *) mapp + mapp->e_stab;
		i = emp->e_nsind;	/* # of string indexes */
		while (--i > 0) {
			if (eip->e_key == c) {		/* found it! */
				i = eip->e_ind;
				/*
				 * i = offset of beginning of this string
				 * length is beginning of next string minus
				 * beginning of this one.
				 */
				*lenp = (++eip)->e_ind - i;
				return (esp + i);
			}
			++eip;
		}
	}
	*lenp = 1;	/* mapped to a single character: *ecp */
	return (ecp);
}

/*
 * Put a character on the write queue.
 */
static void
emap_outchar(queue_t *qp, const unsigned char c, emap_state_t *const sp)
{
	int len;
	register mblk_t *mp;
	register unsigned char *ocp;

	ASSERT(qp != NULL);
	ASSERT(sp != NULL);
	ASSERT(!(EMAP_FLAGS(sp) & EF_SCANCODE_RAW));

	ocp = emap_mapout(c, &len, sp);
	if ((mp = allocb(len, BPRI_MED)) == NULL) {
		EMAP_ERROR(sp) = ENOSR;
		cmn_err(CE_WARN, "emap_outchar: out of msg blocks\n");
		return;
	}

	while (len-- > 0)
		*mp->b_wptr++ = *ocp++;
	(void) putnext(qp, mp);
}

/*
 * Return the current keyboard extended map in the msgb.
 */
static void
emap_getmap(mblk_t *const mp, const emap_state_t *const sp)
{
	ASSERT(mp != NULL);
	ASSERT(sp != NULL);
	ASSERT(EMAP_STATE(sp) != ES_NULL);
	ASSERT(!(EMAP_FLAGS(sp) & EF_SCANCODE_RAW));

	bcopy((caddr_t) EMAP_MAP(sp), (caddr_t) mp->b_rptr, EMAP_SIZE);
	mp->b_wptr = mp->b_rptr + EMAP_SIZE;
}

/*
 * Set the new keyboard extended map.
 */
static int
emap_setmap(mblk_t *mp, emap_state_t *const sp)
{
	register int n = 0;
	register int len = 0;
	unsigned char map[EMAP_SIZE];
	register emp_t mapp = (emp_t) &map;	/* ptr to emapping table */

	ASSERT(mp != NULL);
	ASSERT(sp != NULL);
	ASSERT(!(EMAP_FLAGS(sp) & EF_SCANCODE_RAW));

	bzero((caddr_t) &map, EMAP_SIZE);
	do {
		if ((len = mp->b_wptr - mp->b_rptr) > EMAP_SIZE - n) {
			n += len;
			break;		/* EINVAL */
		}
		bcopy((caddr_t) mp->b_rptr, (caddr_t) &map[n], len);
		n += len;
		mp = mp->b_cont;
	} while (mp != NULL);

	if (n > EMAP_SIZE || n < sizeof (struct emtab))
		return (EINVAL);

	if (emap_chkmap((emp_t) &map))	/* validate user provided emap */
		return (EINVAL);

	/*
	 * Replace the current keyboard extended map and
	 * set the state structure accordingly.
	 */
	bcopy((caddr_t) &map, (caddr_t) EMAP_MAP(sp), EMAP_SIZE);
	EMAP_NDIND(sp) = (mapp->e_cind - E_DIND) / sizeof (struct emind);
	EMAP_NCIND(sp) = ((mapp->e_dctab - mapp->e_cind) /
	    sizeof (struct emind)) - 1;
	EMAP_NSIND(sp) = (mapp->e_stab - mapp->e_sind) / sizeof (struct emind);

	EMAP_STATE(sp) = ES_ON;	/* keyboard extended mapping enabled */
	return (0);
}

/*
 * Check the validity of an emap; return 0 if ok.
 * A completely consistent emap is a user/utility responsibility.
 * We just check for indices and offsets that would cause us to
 * stray outside the emap.
 */
static int
emap_chkmap(const emp_t mapp)
{
	register int n;
	register int ndind, ncind, ndcout, nsind, nschar;
	register emip_t eip;

	ASSERT(mapp != NULL);

	/*
	 * Check table offsets
	 */
	n = mapp->e_cind - E_DIND;
	ndind = n / sizeof (struct emind);
	if ((n < 0) || (n % sizeof (struct emind)) ||
	    (mapp->e_cind > (EMAP_SIZE - 2 * sizeof (struct emind))))
		return (-1);

	n = mapp->e_dctab - mapp->e_cind;
	ncind = n / sizeof (struct emind);
	if ((n < 0) || (n % sizeof (struct emind)) ||
	    (mapp->e_dctab > (EMAP_SIZE - sizeof (struct emind))))
		return (-1);

	n = mapp->e_sind - mapp->e_dctab;
	ndcout = n / sizeof (struct emout);
	if ((n < 0) || (n % sizeof (struct emout)) ||
	    (mapp->e_sind > EMAP_SIZE))
		return (-1);

	n = mapp->e_stab - mapp->e_sind;
	nsind = n / sizeof (struct emind);
	nschar = EMAP_SIZE - mapp->e_stab;
	if ((n < 0) || (n % sizeof (struct emind)) || (nschar < 0))
		return (-1);

	/*
	 * Check dead/compose indices
	 */
	eip = &mapp->e_dind[0];
	n = ndind + ncind;
	while (--n > 0) {
		if (eip[1].e_ind < eip[0].e_ind)
			return (-1);
		++eip;
	}
	if ((n == 0) && ((int) eip->e_ind > ndcout))
		return (-1);

	/*
	 * Check string indices
	 */
	eip = (emip_t) ((unsigned char *) mapp + mapp->e_sind);
	n = nsind;
	while (--n > 0) {
		if (eip[1].e_ind < eip[0].e_ind)
			return (-1);
		++eip;
	}
	if ((n == 0) && ((int) eip->e_ind > nschar))
		return (-1);

	/*
	 * looks like a usable map
	 */
	return (0);
}
