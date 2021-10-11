/*
 * Copyright (c) 1996, Sun Microsystems, Inc.
 * All Rights Reserved.
 */

#pragma	ident	"@(#)dptghd_queue.c	1.1	96/06/13 SMI"


#include <sys/types.h>
#include <sys/param.h>
#include <sys/debug.h>
#include "dptghd_queue.h"



void
QueueAdd( Que_t *qp, Qel_t *qelp, void *datap )
{
	/* init the queue element */
	qelp->qe_linkp = NULL;
	qelp->qe_datap = datap;

	if (!qp->qh_tailpp) {
		/* list is empty */
		qp->qh_headp = qelp;
	} else {
		/* add it to the tailend */
		*(qp->qh_tailpp) = qelp;
	}

	qp->qh_tailpp = &qelp->qe_linkp;
	return;
}


/*
 * QueueDelete()
 *
 *	Remove a specific entry from a singly-linked queue.
 *
 */

void
QueueDelete( Que_t *qp, Qel_t *qelp )
{
	Qel_t	**qpp = &qp->qh_headp;

	while (*qpp != NULL) {
		if (*qpp != qelp)
			continue;
		if ((*qpp = qelp->qe_linkp) == NULL)
			qp->qh_tailpp = NULL;
		return;
	}
	/* its not on this queue */
	return;
}



void *
QueueRemove( Que_t *qp )
{
	Qel_t	*qelp;

	/* pop one off the done queue */
	if ((qelp = qp->qh_headp) == NULL) {
		return (NULL);
	}

	/* if the queue is now empty fix the tail pointer */
	if ((qp->qh_headp = qelp->qe_linkp) == NULL)
		qp->qh_tailpp = NULL;

	qelp->qe_linkp = NULL;

	return (qelp->qe_datap);
}


void
L2_add( L2el_t *headp, L2el_t *elementp, void *private )
{

	ASSERT(headp != NULL && elementp != NULL);
	ASSERT(headp->l2_nextp != NULL);
	ASSERT(headp->l2_prevp != NULL);

	elementp->l2_private = private;

	elementp->l2_nextp = headp;
	elementp->l2_prevp = headp->l2_prevp;
	headp->l2_prevp->l2_nextp = elementp;
	headp->l2_prevp = elementp;

	return;
}

void
L2_delete( L2el_t *elementp )
{

	ASSERT(elementp != NULL);
	ASSERT(elementp->l2_nextp != NULL);
	ASSERT(elementp->l2_prevp != NULL);
	ASSERT(elementp->l2_nextp->l2_prevp == elementp);
	ASSERT(elementp->l2_prevp->l2_nextp == elementp);

	elementp->l2_prevp->l2_nextp = elementp->l2_nextp;
	elementp->l2_nextp->l2_prevp = elementp->l2_prevp;

	/* link it to itself in case someone does a double delete */
	elementp->l2_nextp = elementp;
	elementp->l2_prevp = elementp;

	return;
}


void *
L2_next( L2el_t *elementp )
{
	void	*datap;

	ASSERT(elementp != NULL);

	if (L2_EMPTY(elementp))
		return (NULL);
	return (elementp->l2_nextp->l2_private);
}

