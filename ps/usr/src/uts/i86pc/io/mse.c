/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)mse.c	1.3	96/05/29 SMI"

#include "sys/param.h"
#include "sys/types.h"
#include "sys/signal.h"
#include "sys/errno.h"
#include "sys/file.h"
#include "sys/termio.h"
#include "sys/stream.h"
#include "sys/stropts.h"
#include "sys/strtty.h"
#include "sys/debug.h"
#include "sys/cred.h"
#include "sys/proc.h"
#include "sys/ws/chan.h"
#include "sys/mouse.h"
#include "sys/mse.h"
#include "sys/cmn_err.h"
#include "sys/ddi.h"

#define MSE_DEBUG 1
static int mse_debug = 0;

#ifndef BUILD_STATIC
/*
 * This is the loadable module wrapper.
 */
#include <sys/modctl.h>

extern struct mod_ops mod_miscops;


/*
 * Module linkage information for the kernel.
 */

static struct modlmisc modlmisc = {
	&mod_miscops, 	/* Type of module.  */
	"Mouse driver subroutines"
};

static struct modlinkage modlinkage = {
	MODREV_1,
	(void *)&modlmisc, 
	NULL
};

/*
 * This is the driver initialization routine.
 */
int
_init(void)
{
	return (mod_install(&modlinkage));
}


#ifndef SUNDEV
int
_fini(void)
{
	return (EBUSY);
}
#else
int
_fini(void)
{
	return (mod_remove(&modlinkage));
}


extern struct modctl *mod_getctl();
extern char *kobj_getmodname();

#endif

int
_info(struct modinfo *modinfop)
{
	return (mod_info(&modlinkage, modinfop));
}

#endif	/* !BUILD_STATIC */


/* lfh
extern int wakeup();
*/

void
mse_iocack(queue_t *qp, mblk_t *mp, struct iocblk *iocp, int rval)
{
	mp->b_datap->db_type = M_IOCACK;
	iocp->ioc_rval = rval;
	iocp->ioc_count = iocp->ioc_error = 0;
	qreply(qp,mp);
}

void
mse_iocnack(queue_t *qp, mblk_t *mp, struct iocblk *iocp, int error, int rval)
{
	mp->b_datap->db_type = M_IOCNAK;
	iocp->ioc_rval = rval;
	iocp->ioc_error = error;
	qreply(qp,mp);
}

void
mse_copyout(queue_t *qp, register mblk_t *mp, register mblk_t *nmp, uint size,
		 				unsigned long state)
{
	register struct copyreq *cqp;
	struct strmseinfo *cp;
	struct msecopy	*copyp;

#ifdef DEBUG
	cmn_err(CE_NOTE,"In mse_copyout");
#endif 
	cp = (struct strmseinfo *) qp->q_ptr;
	copyp = &cp->copystate;
	copyp->state = state;
	cqp = (struct copyreq *) mp->b_rptr;
	cqp->cq_size = size;
	cqp->cq_addr = (caddr_t) * (long *) mp->b_cont->b_rptr;
	cqp->cq_flag = 0;
	cqp->cq_private = (mblk_t *)copyp;

	mp->b_wptr = mp->b_rptr + sizeof(struct copyreq);
	mp->b_datap->db_type = M_COPYOUT;

	if (mp->b_cont) freemsg(mp->b_cont);
	mp->b_cont = nmp;

	qreply(qp, mp);
#ifdef DEBUG
	cmn_err(CE_NOTE,"leaving mse_copyout");
#endif 
}


void
mse_copyin(queue_t *qp, register mblk_t *mp, int size, unsigned long state)
{
	register struct copyreq *cqp;
	struct msecopy *copyp;
	struct strmseinfo *cp;

#ifdef DEBUG
	cmn_err(CE_NOTE,"in mse_copyin");
#endif 
	cp = (struct strmseinfo *) qp->q_ptr;
	copyp = &cp->copystate;

	copyp->state = state;
	cqp = (struct copyreq *) mp->b_rptr;
	cqp->cq_size = size;
	cqp->cq_addr = (caddr_t) * (long *) mp->b_cont->b_rptr;
	cqp->cq_flag = 0;
	cqp->cq_private = (mblk_t *)copyp;

	if (mp->b_cont)
		 freemsg(mp->b_cont);

	mp->b_datap->db_type = M_COPYIN;
	mp->b_wptr = mp->b_rptr + sizeof(struct copyreq);

	qreply(qp, mp);
#ifdef DEBUG
	cmn_err(CE_NOTE,"leaving mse_copyin");
#endif 
}

void
mseproc(struct strmseinfo *qp)
{
	register mblk_t 	*bp;
	register mblk_t 	*mp;
	struct ch_protocol	*protop;
	struct mse_event 	*minfo;

#ifdef DEBUG1
	cmn_err(CE_NOTE,"In mseproc");
#endif
/* If no change, don't load a new event */
	if (qp->x | qp->y){
#ifdef MSE_DEBUG
		if (mse_debug)
			printf("got mouse motion x %x  y %x\n", qp->x, qp->y);
#endif
		qp->type = MSE_MOTION;
	}
	else if (qp->button != qp->old_buttons){
#ifdef MSE_DEBUG
		printf("got mouse button \n");
#endif
		qp->type = MSE_BUTTON;
	}
	else
		return;

	qp->mseinfo.status = (~qp->button & 7) | ((qp->button ^ qp->old_buttons) << 3) | (qp->mseinfo.status & BUTCHNGMASK) | (qp->mseinfo.status & MOVEMENT);

	if (qp->type == MSE_MOTION) {
		register int sum;

        qp->mseinfo.status |= MOVEMENT;

		/*
		** See sys/mouse.h for UPPERLIM = 127 and LOWERLIM = -128
		*/

		sum = qp->mseinfo.xmotion + qp->x;

		if (sum > UPPERLIM)
            qp->mseinfo.xmotion = UPPERLIM;
		else if (sum < LOWERLIM)
            qp->mseinfo.xmotion = LOWERLIM;
		else
            qp->mseinfo.xmotion = (char)sum;

		sum = qp->mseinfo.ymotion + qp->y;

		if (sum > UPPERLIM)
            qp->mseinfo.ymotion = UPPERLIM;
		else if (sum < LOWERLIM)
            qp->mseinfo.ymotion = LOWERLIM;
		else
            qp->mseinfo.ymotion = (char)sum;
	}
			/* Note the button state */
	qp->old_buttons = qp->button;
	if((bp = allocb(sizeof(struct ch_protocol),BPRI_MED)) == NULL){ 
		return;
	}
	bp->b_datap->db_type = M_PROTO;
	bp->b_wptr += sizeof(struct ch_protocol);
	protop = (struct ch_protocol *) bp->b_rptr;
	protop->chp_type = CH_DATA;
	protop->chp_stype = CH_MSE;
	drv_getparm(LBOLT,(unsigned long *)&protop->chp_tstmp);
	if((mp = allocb(sizeof(struct mse_event),BPRI_MED)) == NULL){ 
		freemsg(bp);
		return;
	}
	bp->b_cont = mp;
	minfo = (struct mse_event *)mp->b_rptr;
	minfo->type = qp->type;	
	minfo->code = qp->button;	
	minfo->x = qp->x;	
	minfo->y = qp->y;	
	mp->b_wptr += sizeof(struct mse_event);
	putnext(qp->rqp, bp);
}
