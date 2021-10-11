/*
 * Copyright (c) 1995, Sun Microsystems, Inc.
 * All Rights Reserved.
 */

#pragma	ident	"@(#)delivery.c	1.1	95/01/28 SMI"

/*
 * These routines implement the IBM SCB Move Mode Pipes. There
 * are two major functions, enqueue on the outbound pipe, and
 * dequeue from the inbound pipe. Refer to IBM document 85F1678:
 *
 *		SCB Architecture Supplement to
 *		  the IBM Personal System/2
 *	    Hardware Interface Technical Reference
 *			Architectures.
 *
 */


#include <sys/scsi/scsi.h>
#include <sys/dktp/hba.h>
#include <sys/varargs.h>

#include "corv.h"
#include "delivery.h"

void	corv_err(char *fmt, ... );
#ifdef  CORV_DEBUG
extern	ulong	corv_debug;
#endif

/* delivery service routines - physical level */
static	void	delivery_push(caddr_t src, caddr_t dst, size_t length);
static	void	delivery_pull(caddr_t src, caddr_t dst, size_t length);
static	void	delivery_signal(ushort ioaddr);

static	void	enq_state_change(PIPEDS *pipep, ushort ioaddr);
static	void	deq_state_change(PIPEDS *pipep, ushort ioaddr);

/* 
 * Wrap Element 
 */ 
static ELE_HDR wrap_elem = { 
	WRAP_EL_LEN, 	/* length		*/
	0, 		/* type			*/
	0, 		/* reserved		*/
	WRAP_EVENT, 	/* function code	*/
	OFF, 		/* expedite		*/
	0, 		/* fixed		*/
	NO_CHAIN,	/* chaining		*/
	OFF, 		/* supperss exception	*/
	EVENT,		/* element id		*/
	0,		/* dest id		*/
	0,		/* source id		*/
	0L		/* corrid 		*/
};


			

/*
 *	Physical level interface macros
 */
static void
delivery_push(	caddr_t	src,
		caddr_t	dst,
		size_t	bcount )
{
	bcopy(src, dst, bcount);
}

static void
delivery_pull(	caddr_t	src,
		caddr_t	dst,
		size_t	bcount )
{
	bcopy(src, dst, bcount);
}

static void
delivery_signal(ushort ioaddr)
{
	outb(ioaddr + CORV_ATTN, MOVE_MODE_SIGNAL);
}
/*****************************************************************************
 *	MOVE MODE DELIVERY SERVICE ROUTINES
 ****************************************************************************/

/*
 *   This service routine is called to place elements into a pipe.
 *   If the destination is an entity in this unit, pass the element
 *   directly to the driver's element handler.  If not, place the
 *   element into the pipe of the appropriate unit.
 */

int
enq_element(	PIPEDS	*pipep,
		ELE_HDR	*ce_ptr,
		ushort	 ioaddr )
{
	caddr_t	wel;			/* working element pointer      */
	ushort	sur_start_elem;		/* surrogate start of elements  */
	SDS     sur_deq_stat;		/* surrogate dequeue status     */


	if (pipep->enq_ctrl.es.flg.full == ON) {
		DBG_DENQERR((" Pipe Full condtion\n"));
		return (DS_PIPE_FULL);
	}

	/*
	 * get surrogate state information
	 */
	sur_start_elem = *(pipep->ob_pipe.sse);
	sur_deq_stat.fld = pipep->ob_pipe.sds->fld;

	/*
	 * update local end of free pointer (enqueue tail) by tracking
	 * the start of element pointer (dequeue head).
	 */

	if (pipep->enq_ctrl.ef > sur_start_elem
	|| (pipep->enq_ctrl.ef == sur_start_elem
	&&  pipep->enq_ctrl.es.flg.wrap == sur_deq_stat.flg.wrap)) {

		pipep->enq_ctrl.ef = pipep->enq_ctrl.top;

	} else {
		pipep->enq_ctrl.ef = sur_start_elem;

#if 0
/* ??? this isn't in the tech ref man, it looks wrong to me ??? */
		if (pipep->enq_ctrl.ef == pipep->enq_ctrl.top) {

			/* !!! reset ef !!! */
			pipep->enq_ctrl.ef = 0;
		}
#endif
	}

	/* set working element pointer to start of free space. */
	wel = pipep->enq_ctrl.base + pipep->enq_ctrl.sf;

	/*
	 * see if enough room in the pipe to hold this element.
	 * if so, push it into the pipe, update the enqueue state
	 * and indicate an element has been queued.
	 */

	if (ce_ptr->length <= (pipep->enq_ctrl.ef - pipep->enq_ctrl.sf)) {

		/* element fits - push it into pipe and update control areas */
		delivery_push((caddr_t)ce_ptr, wel, ce_ptr->length);

		/* update start of free      */
		pipep->enq_ctrl.sf += ce_ptr->length;

		/* turn on queued bit        */
		pipep->enq_ctrl.es.flg.queued = ON;

		enq_state_change(pipep, ioaddr);
		return (DS_SUCCESS);

	}
	/* element dosen't fit, check if pipe needs to wrap to top of pipe */
	if (pipep->enq_ctrl.ef == pipep->enq_ctrl.top) {
		/* push a wrap element */
		delivery_push((caddr_t)&wrap_elem, wel, WRAP_EL_LEN);
		DBG_DENQ(("enq_element: wrap \n"));

		/* update local enqueue control areas
		 * mark wrap element start
		 */
		pipep->enq_ctrl.we = pipep->enq_ctrl.sf;

		/* new end of free space     */
		pipep->enq_ctrl.ef = sur_start_elem;

		/* reset start of free space */
		pipep->enq_ctrl.sf = SCB_BASE_OFF;

		/* toggle wrap flag          */
		pipep->enq_ctrl.es.flg.wrap ^= ON;

		/* Pipe has wrapped, see if element will fit now */
		if (ce_ptr->length <=
			(pipep->enq_ctrl.ef - pipep->enq_ctrl.sf)) {

			/* fits, update working pointer to start of pipe */
			wel = pipep->enq_ctrl.base + pipep->enq_ctrl.sf;

			/* copy element into pipe and update control areas */
			delivery_push((caddr_t)ce_ptr, wel, ce_ptr->length);

			/* update start of free */
			pipep->enq_ctrl.sf += ce_ptr->length;

			/* turn on queued bit   */
			pipep->enq_ctrl.es.flg.queued = ON;

			enq_state_change(pipep, ioaddr);
			return (DS_SUCCESS);
		}
		/*
		 * No, element doesn't fit, even
		 * after wrapping
		 */

		/*
		 * indicate full condition
		 */
		pipep->enq_ctrl.es.flg.full = ON;

		DBG_DENQERR(("enq_element: 1. pipe full on\n"));
		DBG_DENQERR(("enq_element: enq just Wrapped\n"));

		/* PipeFull */
		enq_state_change(pipep, ioaddr);
		return (DS_PIPE_FULL);
	}

	/*
	 * We've caught up with beginning of a previously
	 * wrapped pipe, the pipe is full.
	 */

	/*
	 * indicate full condition
	 */
	pipep->enq_ctrl.es.flg.full = ON;

	DBG_DENQERR(("enq_element: 2. pipe full on\n"));
	DBG_DENQERR(("enq_element: enq - previously Wrapped\n"));

	enq_state_change(pipep, ioaddr);

	return (DS_PIPE_FULL);
}

/*
 *   This routine determines if a signal must be sent to a unit as a
 *   result of an enqueue operation.  If yes, send one.  If not, simply
 *   return.  In either case, update the surrogate enqueue control areas.
 */
static void
enq_state_change(	PIPEDS	*pipep,
			ushort	 ioaddr )
{
	ulong	send_signal;		/* send signal flag              */
	SDS	sur_deq_stat;		/* copy surrogate dequeue status */


	/*
	 * copy local enqueue status to surrogate enqueue status
	 * and local start-of-free to surrogate start-of-free
	 */
 	pipep->ob_pipe.ses->fld = pipep->enq_ctrl.es.fld ;
  	*(pipep->ob_pipe.ssf) = pipep->enq_ctrl.sf;

	/* Get surrogate dequeue status */
	sur_deq_stat.fld = pipep->ob_pipe.sds->fld ;


	/* check signalling conditions */
	if (((pipep->enq_ctrl.es.flg.queued) && (pipep->sys_cfg & SIG_ENQ))
	||   (pipep->enq_ctrl.es.flg.queued && sur_deq_stat.flg.empty
					    && (pipep->sys_cfg & SIG_NOT_EMPTY))
	||   (pipep->enq_ctrl.es.flg.full && (pipep->sys_cfg & SIG_FULL))) {
		/* send_signal */
		/* set dequeue flag in signalling control area */
		pipep->adp_sca->deq = SIGNAL_FLAG_ON;
		delivery_signal(ioaddr);
	}

	/* reset local enqueue state information */
	pipep->enq_ctrl.es.flg.empty  = OFF;
	pipep->enq_ctrl.es.flg.full   = OFF;
	pipep->enq_ctrl.es.flg.queued = OFF;

	return;
}

/*
 *   This service routine is called to remove elements from the inbound pipe.
 * Assumptions:
 *   The receive callback function should copy the element if it needs
 *   to keep it around.
 */

void
deq_element(	PIPEDS *pipep,
		ushort ioaddr,
		void (*func)(void *arg, ELE_HDR *ep),
		void *arg)
{
	ELE_HDR	*wel;		/* working element pointer          */
	ushort	sur_start_free; /* copy of surrogate start of free  */
	SES	sur_enq_stat;   /* copy of surrogate enqueue status */
	ushort	ele_length;   	/* length of element */

	/* Get surrogate start of free and enqueue status */
	DBG_DDEQ(("...DEQ..."));

	sur_start_free = *(pipep->ib_pipe.ssf);
	sur_enq_stat.fld = pipep->ib_pipe.ses->fld;
	
	/* If we think the pipe is full, re-read the surrogate values again  */
	/* to ensure there is no timing window.                              */

	if (pipep->deq_ctrl.se == sur_start_free
	&&  pipep->deq_ctrl.ds.flg.wrap != sur_enq_stat.flg.wrap) {

   		sur_start_free = *(pipep->ib_pipe.ssf);
   		sur_enq_stat.fld = pipep->ib_pipe.ses->fld;
	}
	
	/* check for elements in the pipe */

	if (pipep->deq_ctrl.se == sur_start_free
	&&  pipep->deq_ctrl.ds.flg.wrap == sur_enq_stat.flg.wrap) {

/* ??? setting empty here shouldn't be necessary ??? */
		/* set empty flag */
     		pipep->deq_ctrl.ds.flg.empty = ON;	

		/* no elements in the pipe, signal state change and return */
     		deq_state_change(pipep, ioaddr);

		DBG_DDEQ(("---------------PIPE EMPTY ----------------\n"));
		return;
	}

	do {
		/* update end of elements with start-of-free */
		pipep->deq_ctrl.ee = sur_start_free;

		/* get pointer to next element in the pipe */
		wel = (ELE_HDR *) ((unchar *) pipep->deq_ctrl.base +
			pipep->deq_ctrl.se);

		/* check for wrap element */

		if (wel->opcode == WRAP_EVENT) {
			DBG_DDEQ(("WRP  \n"));

			/* reset start of elements */
			pipep->deq_ctrl.se = SCB_BASE_OFF;

			/* set element dequeued bit */
			pipep->deq_ctrl.ds.flg.dequeued = ON;

			/* toggle wrap flag */
			pipep->deq_ctrl.ds.flg.wrap ^= ON;

		} else {

			/* callback receive function */
			(func)(arg, wel);

			/* update local dequeue control info */
			pipep->deq_ctrl.se += wel->length;
			pipep->deq_ctrl.ds.flg.dequeued = ON;

			/* call state change, indicating element removed */
			deq_state_change(pipep, ioaddr);
		}

	} while((pipep->deq_ctrl.se != pipep->deq_ctrl.ee) ||
		(pipep->deq_ctrl.ds.flg.wrap != sur_enq_stat.flg.wrap));


	pipep->deq_ctrl.ds.flg.empty = ON;

	/* call state change, indicating element removed */
     	deq_state_change(pipep, ioaddr);

	return;
}

/*
 *   This routine determines if a signal must be sent to a unit as
 *   a result of removing an element from a pipe.  If so, send one.
 *   If not, simply return.  In either case update the surrogate
 *   dequeue control areas.
 */
static void
deq_state_change(	PIPEDS		*pipep,
			ushort		 ioaddr )
{
	SES   	sur_enq_stat;	/* copy of status                */

	
	/* Get surrogate enqueue status */
	sur_enq_stat.fld = pipep->ib_pipe.ses->fld;

	/* Copy local dequeue status to surrogate dequeue status */
 	pipep->ib_pipe.sds->fld = pipep->deq_ctrl.ds.fld ;

	/* Copy start of elements to surrogate start-of-elements */
  	*(pipep->ib_pipe.sse) = pipep->deq_ctrl.se;

	/* check signalling conditions */
  	if ((pipep->deq_ctrl.ds.flg.dequeued && (pipep->sys_cfg & SIG_DEQ))
	||  (sur_enq_stat.flg.full && (pipep->sys_cfg & SIG_NOT_FULL))
	||  (pipep->deq_ctrl.ds.flg.empty && (pipep->sys_cfg & SIG_EMPTY))) {

		/* send signal */
		DBG_DDEQ(("-------deq_state_ SEND SIGNAL-----\n"));

		pipep->adp_sca->enq = SIGNAL_FLAG_ON;
		delivery_signal(ioaddr);
	}

	/* reset local state information */
	pipep->deq_ctrl.ds.flg.empty = OFF;
	pipep->deq_ctrl.ds.flg.full = OFF;
	pipep->deq_ctrl.ds.flg.dequeued = OFF;

	return;
}
