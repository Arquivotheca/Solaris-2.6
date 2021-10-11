/*
 * Copyright (c) 1995, Sun Microsystems, Inc.
 * All Rights Reserved.
 */

#ifndef _CSA_CSA_QUEUE_H
#define	_CSA_CSA_QUEUE_H

#pragma	ident	"@(#)csa_queue.h	1.2	95/05/31 SMI"

#ifdef	__cplusplus
extern "C" {
#endif

/*
 *  command list queue
 */

#define	QEMPTY(QP)	((QP)->qh_headp == NULL)
#define	QUEUE_INIT(QP)	((QP)->qh_headp = NULL, (QP)->qh_tailpp = NULL)

typedef struct queue_element {
	struct queue_element	*qe_linkp;
	void			*qe_datap;
} Qel_t;

typedef struct queue_head {
	Qel_t	 *qh_headp;
	Qel_t	**qh_tailpp;
#ifdef CSA_DEBUG
	ulong	  qh_add;
	ulong	  qh_rm;
#endif
} Que_t;

void	 QueueAdd(Que_t *qp, Qel_t *qelp, void *datap);
void	*QueueRemove(Que_t *qp);

#ifdef	__cplusplus
}
#endif

#endif  /* _CSA_CSA_QUEUE_H */
