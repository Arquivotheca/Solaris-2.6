/*
 * Copyright (c) 1994 by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)queue.c	1.3	94/06/30 SMI"

#include <sys/dktp/ncrs/ncr.h>


/* add nptp to the front of queue */
void
ncr_addfq(	ncr_t	*ncrp,
		npt_t	*nptp )
{
	/* See if it's already in the queue */
	if (nptp->nt_linkp != NULL ||  nptp == ncrp->n_backp
				   ||  nptp == ncrp->n_forwp)
		cmn_err(CE_PANIC, "ncrs: ncr_addfq: queue botch\n");

	if ((nptp->nt_linkp = ncrp->n_forwp) == NULL)
		ncrp->n_backp = nptp;
	ncrp->n_forwp = nptp;
	return;
}


/* add nptp to the back of queue */
void
ncr_addbq(	ncr_t	*ncrp,
		npt_t	*nptp )
{
	nptp->nt_linkp = NULL;

	if (ncrp->n_forwp == NULL)
		ncrp->n_forwp = nptp;
	else
		ncrp->n_backp->nt_linkp = nptp;

	ncrp->n_backp = nptp;
	return;
}


/*
 * remove current target from front of hba's queue
 */
npt_t *
ncr_rmq( ncr_t *ncrp )
{
	npt_t	*nptp = ncrp->n_forwp;

	if (nptp != NULL) {
		if ((ncrp->n_forwp = nptp->nt_linkp) == NULL)
			ncrp->n_backp = NULL;
		nptp->nt_linkp = NULL;
	}
	return (nptp);
}


/*
 * remove specified target from the middle of the hba's queue
 */
void
ncr_delq(	ncr_t	*ncrp,
		npt_t	*nptp )
{
	npt_t	*prevp = ncrp->n_forwp;

	if (prevp == nptp) {
		if ((ncrp->n_forwp = nptp->nt_linkp) == NULL)
			ncrp->n_backp = NULL;

		nptp->nt_linkp = NULL;
		return;
	}
	while (prevp != NULL) {
		if (prevp->nt_linkp == nptp) {
			if ((prevp->nt_linkp = nptp->nt_linkp) == NULL)
				ncrp->n_backp = prevp;
			nptp->nt_linkp = NULL;
			return;
		}
		prevp = prevp->nt_linkp;
	}
	cmn_err(CE_PANIC, "ncrs: ncr_rmq: queue botch\n");
}

/*
 * These two routines manipulate the queue of commands that
 * are waiting for their completion routines to be called.
 * The queue is usually in FIFO order but on an MP system
 * it's possible for the completion routines to get out
 * of order. If that's a problem you need to add a global
 * mutex around the code that calls the completion routine
 * in the interrupt handler.
 */
void
ncr_doneq_add(	ncr_t	*ncrp,
		nccb_t	*nccbp )
{
	NDBG19(("ncr_doneq_add: ncrp=0x%x nccbp=0x%x\n", ncrp, nccbp));
	nccbp->nc_linkp = NULL;
	*ncrp->n_donetail = nccbp;
	ncrp->n_donetail = &nccbp->nc_linkp;
	return;
}

nccb_t	*
ncr_doneq_rm( ncr_t *ncrp )
{
	nccb_t	*nccbp;

	/* pop one off the done queue */
	if ((nccbp = ncrp->n_doneq) != NULL) {
		/* if the queue is now empty fix the tail pointer */
		if ((ncrp->n_doneq = nccbp->nc_linkp) == NULL)
			ncrp->n_donetail = &ncrp->n_doneq;
		nccbp->nc_linkp = NULL;
	}
	return (nccbp);
}

/*
 * These routines manipulate the target's queue of pending requests 
 */
void
ncr_waitq_add(	npt_t	*nptp,
		nccb_t	*nccbp )
{
	NDBG19(("ncr_waitq_add: nptp=0x%x nccbp=0x%x\n", nptp, nccbp));
	nccbp->nc_queued = TRUE;
	nccbp->nc_linkp = NULL;
	*(nptp->nt_waitqtail) = nccbp;
	nptp->nt_waitqtail = &nccbp->nc_linkp;
	return;
}

void
ncr_waitq_add_lifo(	npt_t	*nptp,
			nccb_t	*nccbp )
{
	NDBG19(("ncr_waitq_add: nptp=0x%x nccbp=0x%x\n", nptp, nccbp));
	nccbp->nc_queued = TRUE;
	if ((nccbp->nc_linkp = nptp->nt_waitq) == NULL) {
		nptp->nt_waitqtail = &nccbp->nc_linkp;
	}
	nptp->nt_waitq = nccbp;
	return;
}

nccb_t	*
ncr_waitq_rm( npt_t *nptp )
{
	nccb_t	*nccbp;

	/* pop one off the wait queue */
	if ((nccbp = nptp->nt_waitq) != NULL) {
		/* if the queue is now empty fix the tail pointer */
		if ((nptp->nt_waitq = nccbp->nc_linkp) == NULL)
			nptp->nt_waitqtail = &nptp->nt_waitq;
		nccbp->nc_linkp = NULL;
		nccbp->nc_queued = FALSE;
	}

	NDBG19(("ncr_waitq_rm: nptp=0x%x nccbp=0x%x\n", nptp, nccbp));

	return (nccbp);
}


/*
 * remove specified target from the middle of the hba's queue
 */
void
ncr_waitq_delete(	npt_t	*nptp,
			nccb_t	*nccbp )
{
	nccb_t	*prevp = nptp->nt_waitq;

	if (prevp == nccbp) {
		if ((nptp->nt_waitq = nccbp->nc_linkp) == NULL)
			nptp->nt_waitqtail = &nptp->nt_waitq;

		nccbp->nc_linkp = NULL;
		nccbp->nc_queued = FALSE;
		NDBG19(("ncr_waitq_delete: nptp=0x%x nccbp=0x%x\n"
				, nptp, nccbp));
		return;
	}
	while (prevp != NULL) {
		if (prevp->nc_linkp == nccbp) {
			if ((prevp->nc_linkp = nccbp->nc_linkp) == NULL)
				nptp->nt_waitqtail = &nptp->nt_waitq;

			nccbp->nc_linkp = NULL;
			nccbp->nc_queued = FALSE;
			NDBG19(("ncr_waitq_delete: nptp=0x%x nccbp=0x%x\n"
					, nptp, nccbp));
			return;
		}
		prevp = prevp->nc_linkp;
	}
	cmn_err(CE_PANIC, "ncrs: ncr_waitq_delete: queue botch\n");
}
