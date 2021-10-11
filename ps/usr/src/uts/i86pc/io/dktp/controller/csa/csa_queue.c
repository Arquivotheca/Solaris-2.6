/*
 * Copyright (c) 1995, Sun Microsystems, Inc.
 * All Rights Reserved.
 */

#pragma	ident	"@(#)csa_queue.c	1.2	95/05/31 SMI"

#include <sys/types.h>
#include <sys/param.h>
#if defined(CSA_DEBUG) && defined(DEBUG)
#include <sys/ksynch.h>
#endif

#include "csa_queue.h"



void
QueueAdd(
	Que_t	*qp,
	Qel_t	*qelp,
	void	*datap
)
{
	if (!qp->qh_tailpp) {
		/* first time, initialize the queue header */
		qp->qh_headp = NULL;
		qp->qh_tailpp = &qp->qh_headp;
	}

	/* init the queue element */
	qelp->qe_linkp = NULL;
	qelp->qe_datap = datap;

	/* add it to the tailend */
	*(qp->qh_tailpp) = qelp;
	qp->qh_tailpp = &qelp->qe_linkp;

#ifdef CSA_DEBUG
	qp->qh_add++;
#endif

	return;
}



void *
QueueRemove(Que_t *qp)
{
	Qel_t	*qelp;

	/* pop one off the done queue */
	if ((qelp = qp->qh_headp) == NULL) {
		return (NULL);
	}

	/* if the queue is now empty fix the tail pointer */
	if ((qp->qh_headp = qelp->qe_linkp) == NULL)
		qp->qh_tailpp = &qp->qh_headp;
	qelp->qe_linkp = NULL;

#ifdef CSA_DEBUG
	qp->qh_rm++;
#endif

	return (qelp->qe_datap);
}
