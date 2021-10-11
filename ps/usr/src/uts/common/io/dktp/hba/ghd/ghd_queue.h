/*
 * Copyright (c) 1996, Sun Microsystems, Inc.
 * All Rights Reserved.
 */

#ifndef _GHD_QUEUE_H
#define	_GHD_QUEUE_H
#pragma	ident	"@(#)ghd_queue.h	1.1	96/07/30 SMI"
#ifdef	__cplusplus
extern "C" {
#endif


/*
 *  A queue of singly linked elements
 */

typedef struct queue_element {
	struct queue_element	*qe_linkp;
	void			*qe_datap;
} Qel_t;

#define	QEL_INIT(qelp)	((qelp)->qe_linkp = NULL, (qelp)->qe_datap = 0)

typedef struct queue_head {
	Qel_t	 *qh_headp;
	Qel_t	**qh_tailpp;
} Que_t;

#define	QUEUE_INIT(QP)	(((QP)->qh_headp = NULL), ((QP)->qh_tailpp = NULL))
#define	QEMPTY(QP)	((QP)->qh_headp == NULL)

void	 QueueAdd(Que_t *qp, Qel_t *qelp, void *datap);
void	 QueueDelete(Que_t *qp, Qel_t *qelp);
void	*QueueRemove(Que_t *qp);


/*
 * A list of doubly linked elements
 */

typedef struct L2el {
	struct	L2el	*l2_nextp;
	struct	L2el	*l2_prevp;
	void		*l2_private;
} L2el_t;

#define	L2_INIT(headp)	\
	(((headp)->l2_nextp = (headp)), ((headp)->l2_prevp = (headp)))

#define L2_EMPTY(headp) ((headp)->l2_nextp == (headp))

void	L2_add( L2el_t *headp, L2el_t *elementp, void *private );
void	L2_delete( L2el_t *elementp );
void	*L2_next( L2el_t *elementp );


#ifdef	__cplusplus
}
#endif
#endif  /* _GHD_QUEUE_H */
