/*
 * Copyright (c) 1996, Sun Microsystems, Inc.
 * All Rights Reserved.
 */

#pragma	ident	"@(#)dptghd.c	1.1	96/06/13 SMI"

#include <sys/types.h>
#include <sys/kmem.h>
#include <sys/debug.h>
#include <sys/scsi/scsi.h>

#include "dptghd.h"

/* ghd_poll() function codes: */
typedef enum {
	GHD_POLL_REQUEST,	/* wait for a specific request */
	GHD_POLL_DEVICE,	/* wait for a specific device to idle */
	GHD_POLL_ALL		/* wait for the whole bus to idle */
} gpoll_t;

/*
 * Local functions:
 */
static	gcmd_t	*dptghd_doneq_get(ccc_t *cccp);
static	void	 dptghd_doneq_process(ccc_t *cccp);
static	void	 dptghd_doneq_pollmode_enter(ccc_t *cccp);
static	void	 dptghd_doneq_pollmode_exit(ccc_t *cccp);

static	u_int	 dptghd_dummy_intr(caddr_t arg);
static	int	 dptghd_poll(ccc_t *cccp, gpoll_t polltype, ulong polltime,
			  gcmd_t *poll_gcmdp, struct scsi_address *ap,
			  void *intr_status);


void
dptghd_complete( ccc_t *cccp, gcmd_t *gcmdp )
{

	ASSERT(mutex_owned(&cccp->ccc_hba_mutex));

	/* the request is now complete */
	gcmdp->cmd_state = GCMD_STATE_DONEQ;

	/* stop the packet timer */
	dptghd_timer_stop(cccp, gcmdp);

	/* and schedule the pkt completion callback */
	dptghd_doneq_put(cccp, gcmdp);
	return;
}

void
dptghd_doneq_put( ccc_t *cccp, gcmd_t *gcmdp )
{
	kmutex_t *doneq_mutexp = &cccp->ccc_doneq_mutex;

	mutex_enter(doneq_mutexp);
	QueueAdd(&cccp->ccc_doneq, &gcmdp->cmd_q, gcmdp);
	mutex_exit(doneq_mutexp);

	gcmdp->cmd_state = GCMD_STATE_DONEQ;
	return;
}

static gcmd_t	*
dptghd_doneq_get( ccc_t *cccp )
{
	kmutex_t *doneq_mutexp = &cccp->ccc_doneq_mutex;
	gcmd_t	 *gcmdp;

	mutex_enter(doneq_mutexp);
	gcmdp = QueueRemove(&cccp->ccc_doneq);
	mutex_exit(doneq_mutexp);
	return (gcmdp);
}

gcmd_t	*
dptghd_doneq_tryget( ccc_t *cccp )
{
	kmutex_t *doneq_mutexp = &cccp->ccc_doneq_mutex;
	gcmd_t	 *gcmdp;

	mutex_enter(doneq_mutexp);
	if (cccp->ccc_hba_pollmode)
		gcmdp = NULL;
	else
		gcmdp = QueueRemove(&cccp->ccc_doneq);
	mutex_exit(doneq_mutexp);
	return (gcmdp);
}

static void
dptghd_doneq_pollmode_enter( ccc_t *cccp )
{
	kmutex_t *doneq_mutexp = &cccp->ccc_doneq_mutex;
	gcmd_t	 *gcmdp;

	mutex_enter(doneq_mutexp);
	cccp->ccc_hba_pollmode = TRUE;
	mutex_exit(doneq_mutexp);
	return;
}


static void
dptghd_doneq_pollmode_exit( ccc_t *cccp )
{
	kmutex_t *doneq_mutexp = &cccp->ccc_doneq_mutex;
	gcmd_t	 *gcmdp;

	mutex_enter(doneq_mutexp);
	cccp->ccc_hba_pollmode = FALSE;
	mutex_exit(doneq_mutexp);
	return;
}


/* ***************************************************************** */

/*
 *
 * dptghd_doneq_process()
 *
 *	see ghd.h for the macro expansion
 *
 *
 * 	Need to drop the HBA mutex while running completion
 * routines because the HBA driver can be re-entered by the
 * completion routine via the HBA transport entry point.
 *
 *	The doneq is protected by a separate mutex than the
 * HBA mutex in order to avoid mutex contention on MP systems.
 *
 */

void
dptghd_doneq_process( ccc_t *cccp )
{
	DPTGHD_DONEQ_PROCESS_INLINE(cccp);
}

/* ***************************************************************** */



void
dptghd_waitq_delete( ccc_t *cccp, gcmd_t *gcmdp )
{
	kmutex_t *waitq_mutexp = &cccp->ccc_waitq_mutex;

	mutex_enter(waitq_mutexp);
	QueueDelete(&cccp->ccc_waitq, &gcmdp->cmd_q);
	mutex_exit(waitq_mutexp);
	return;
}

gcmd_t	*
dptghd_waitq_get( ccc_t *cccp )
{
	kmutex_t *waitq_mutexp = &cccp->ccc_waitq_mutex;
	gcmd_t	 *gcmdp;

	mutex_enter(waitq_mutexp);
	gcmdp = QueueRemove(&cccp->ccc_waitq);
	mutex_exit(waitq_mutexp);
	return (gcmdp);
}

void
dptghd_waitq_put( ccc_t *cccp, gcmd_t *gcmdp )
{
	kmutex_t *waitq_mutexp = &cccp->ccc_waitq_mutex;

	mutex_enter(waitq_mutexp);
	QueueAdd(&cccp->ccc_waitq, &gcmdp->cmd_q, gcmdp);
	mutex_exit(waitq_mutexp);
	return;
}




/*
 * Autovector Interrupt Entry Point
 *
 *	Dummy return to be used before mutexes has been initialized
 *	guard against interrupts from drivers sharing the same irq line
 */

/*ARGSUSED*/
static u_int
dptghd_dummy_intr( caddr_t arg )
{
	return (DDI_INTR_UNCLAIMED);
}


/*
 * dptghd_intr_init()
 *
 *	Do the usual interrupt handler setup stuff. Note the doneq has
 * to be protected by a mutex but that mutex isn't initialized here.
 * That way the caller can decide whether to use separate HBA and doneq
 * mutexes or a single mutex
 *
 */


int
dptghd_register(ccc_t	*cccp,
		dev_info_t *dip,
		int	 inumber,
		char	*mutex_name,
		void	*hba_handle,
		gcmd_t	*(*ccballoc)(struct scsi_address *, struct scsi_pkt *,
					void *, int, int, int, int),
		void	(*ccbfree)(struct scsi_address *, struct scsi_pkt *),
		void	(*sg_func)(struct scsi_pkt *, gcmd_t *,
					ddi_dma_cookie_t *, int, int, void *),
		int	(*hba_start)(void *, gcmd_t *),
		u_int	(*int_handler)(caddr_t),
		int	(*get_status)(void *, void *),
		void	(*process_intr)(void *, void *),
		void	(*timeout_func)(void *,gcmd_t *, struct scsi_address *,
					gact_t),
		tmr_t	*tmrp )
{

	/*
	 *	Establish initial dummy interrupt handler
	 *	get iblock cookie to initialize mutexes used in the
	 *	real interrupt handler
	 */
	if (ddi_add_intr(dip, inumber, &cccp->ccc_iblock, NULL,
			 dptghd_dummy_intr, hba_handle) != DDI_SUCCESS) {
		return (FALSE);
	}
	mutex_init(&cccp->ccc_hba_mutex, mutex_name, MUTEX_DRIVER,
			cccp->ccc_iblock);
	ddi_remove_intr(dip, inumber, cccp->ccc_iblock);

	/* Establish real interrupt handler */
	if (ddi_add_intr(dip, inumber, &cccp->ccc_iblock, NULL,
			 int_handler, (caddr_t)hba_handle) != DDI_SUCCESS) {
		mutex_destroy(&cccp->ccc_hba_mutex);
		return (FALSE);
	}

	/*
	 * Use a separate mutex for the wait and done queues
	 */
	mutex_init(&cccp->ccc_waitq_mutex, "Wait-Queue Mutex",
		   MUTEX_DRIVER, cccp->ccc_iblock);

	mutex_init(&cccp->ccc_doneq_mutex, "Done-Queue Mutex",
		   MUTEX_DRIVER, cccp->ccc_iblock);

	cccp->ccc_hba_dip = dip;
	cccp->ccc_ccballoc = ccballoc;
	cccp->ccc_ccbfree = ccbfree;
	cccp->ccc_sg_func = sg_func;
	cccp->ccc_hba_start = hba_start;
	cccp->ccc_process_intr = process_intr;
	cccp->ccc_get_status = get_status;
	cccp->ccc_hba_handle = hba_handle;

	dptghd_timer_attach(cccp, tmrp, timeout_func);
	dptghd_doneq_pollmode_exit(cccp);
	return (TRUE);
}


void
dptghd_unregister( ccc_t *cccp )
{
	dptghd_timer_detach(cccp);

	ddi_remove_intr(cccp->ccc_hba_dip, 0, cccp->ccc_iblock);
	mutex_destroy(&cccp->ccc_hba_mutex);
	mutex_destroy(&cccp->ccc_waitq_mutex);
	mutex_destroy(&cccp->ccc_doneq_mutex);
}



int
dptghd_intr( ccc_t *cccp, void *intr_status )
{
	kmutex_t *hba_mutexp = &cccp->ccc_hba_mutex;


	mutex_enter(hba_mutexp);

	GDBG_INTR(("dptghd_intr(): cccp=0x%x status=0x%x\n",
			cccp, intr_status));

	if (!(*cccp->ccc_get_status)(cccp->ccc_hba_handle, intr_status)) {
		mutex_exit(hba_mutexp);

		/*
		 * The get_status() could possibly change
		 * the doneq??? Shouldn't happen but try doing
		 * the completion callbacks here just to be safe.
		 */
		DPTGHD_DONEQ_PROCESS(cccp);
		return (DDI_INTR_UNCLAIMED);
	}

	for (;;) {
		/* process the interrupt status */
		(*cccp->ccc_process_intr)(cccp->ccc_hba_handle, intr_status);

		/* loop if new interrupt status was stacked by hardware */
		if (!(*cccp->ccc_get_status)(cccp->ccc_hba_handle, intr_status))
			break;
	}

	GDBG_INTR(("dptghd_intr(): done cccp=0x%x status=0x%x\n",
		   cccp, intr_status));

	mutex_exit(hba_mutexp);
	DPTGHD_DONEQ_PROCESS(cccp);

	return (DDI_INTR_CLAIMED);
}

static int
dptghd_poll(	ccc_t	*cccp,
		gpoll_t	 polltype,
		ulong	 polltime,
		gcmd_t	*poll_gcmdp,
		struct scsi_address *ap,
		void	*intr_status )
{
	kmutex_t *hba_mutexp = &cccp->ccc_hba_mutex;
	gcmd_t	 *gcmdp;
	Que_t	  gcmd_hold_queue;
	int	  got_it = FALSE;
	ulong	  start_lbolt;
	ulong	  current_lbolt;


	ASSERT(mutex_owned(hba_mutexp));
	QUEUE_INIT(&gcmd_hold_queue);

	/* Que hora es? */
	drv_getparm(LBOLT, &start_lbolt);

	/* unqueue and save all CMD/CCBs until I find the right one */
	while (!got_it) {

		/* Give up yet? */
		drv_getparm(LBOLT, &current_lbolt);
		if (polltime && (current_lbolt - start_lbolt >= polltime))
			break;

		/*
		 * delay 1 msec each time around the loop (this is an
		 * arbitrary delay value, any value should work) except
		 * zero because some devices don't like being polled too
		 * fast and it saturates the bus on an MP system.
		 */
		drv_usecwait(1000);

		/*
		 * check for any new device status
		 */
		if ((*cccp->ccc_get_status)(cccp->ccc_hba_handle, intr_status))
			(*cccp->ccc_process_intr)(cccp->ccc_hba_handle,
							intr_status);

		/*
		 * Process any timed-out requests. This will modify
		 * the cccp->ccc_activel list which we may check below
		 * if we're doing the GHD_POLL_DEVICE function.
		 */
		dptghd_timer_poll(cccp);


		/*
		 * Unqueue all the completed requests, look for mine
		 */
		while (gcmdp = dptghd_doneq_get(cccp)) {
			/*
			 * If we got one and it's my request, then
			 * we're done. 
			 */
			if (gcmdp == poll_gcmdp) {
				poll_gcmdp->cmd_state = GCMD_STATE_IDLE;
				got_it = TRUE;
				continue;
			}
			/* fifo queue the other cmds on my local list */
			QueueAdd(&gcmd_hold_queue, &gcmdp->cmd_q, gcmdp);
		}


		/*
		 * Check whether we're done yet. 
		 */
		switch (polltype) {
		case GHD_POLL_DEVICE:
			/*
			 * wait for everything queued on a specific device
			 */
			got_it = TRUE;
			mutex_enter(&cccp->ccc_activel_mutex);
			gcmdp = (gcmd_t *)L2_next(&cccp->ccc_activel);
			while (gcmdp) {
				if (ap->a_target == gcmdp->cmd_ap->a_target
				&&  ap->a_lun == gcmdp->cmd_ap->a_lun) {
					got_it = FALSE;
					break;
				}
				gcmdp = (gcmd_t *)
					L2_next(&gcmdp->cmd_timer_link);
			}
			mutex_exit(&cccp->ccc_activel_mutex);

		case GHD_POLL_ALL:
			/* 
			 * if waiting for all outstanding requests and
			 * if active list is now empty then exit
			 */
			if (L2_EMPTY(&cccp->ccc_activel))
				got_it = TRUE;
			break;
		}
	}

	if (QEMPTY(&gcmd_hold_queue)) {
		return (got_it);
	}

	/* 
	 * copy the local gcmd_hold_queue back to the doneq so
	 * that the order of completion callbacks is preserved
	 */
	while (gcmdp = (gcmd_t *)QueueRemove(&gcmd_hold_queue))
	 	dptghd_doneq_put(cccp, gcmdp);

	return (got_it);
}


/*
 * dptghd_tran_abort()
 *
 *	Abort specific command on a target.
 *
 */

int
dptghd_tran_abort(	ccc_t	*cccp,
			gcmd_t	*gcmdp,
			struct scsi_address *ap,
			void	*intr_status )
{
	gact_t	 action;
	int	 rc;

	/*
	 * call the driver's abort_cmd function
	 */

	mutex_enter(&cccp->ccc_hba_mutex);
	dptghd_doneq_pollmode_enter(cccp);

	switch (gcmdp->cmd_state) {
	case GCMD_STATE_WAITQ:
		/* not yet started */
		action = GACTION_EARLY_ABORT;
		break;

	case GCMD_STATE_ACTIVE:
		/* in progress */
		action = GACTION_ABORT_CMD;
		break;

	default:
		/* everything else, probably already being aborted */
		rc = FALSE;
		goto exit;
	}

	/* send out the abort command */
	dptghd_timer_newstate(cccp, gcmdp, ap, action);

	/* wait upto 2 seconds for the abort to complete */
	if (rc = dptghd_poll(cccp, GHD_POLL_REQUEST, 5, gcmdp,
							ap, intr_status)) {
		gcmdp->cmd_state = GCMD_STATE_DONEQ;
		dptghd_doneq_put(cccp, gcmdp);
	}



exit:
	dptghd_doneq_pollmode_exit(cccp);
	mutex_exit(&cccp->ccc_hba_mutex);

	DPTGHD_DONEQ_PROCESS(cccp);

	return (rc);
}


/*
 * dptghd_tran_abort_lun()
 *
 *	Abort all commands on a specific target.
 *
 */

int
dptghd_tran_abort_lun(	ccc_t	*cccp,
			struct scsi_address *ap,
			void	*intr_status )
{
	gact_t	 action;
	int	 rc;


	/*
	 * call the HBA driver's abort_device function
	 */

	mutex_enter(&cccp->ccc_hba_mutex);
	dptghd_doneq_pollmode_enter(cccp);

	/* send out the abort device request */
	dptghd_timer_newstate(cccp, NULL, ap, GACTION_ABORT_DEV);

	/* wait upto 5 seconds for the device to go idle */
	rc = dptghd_poll(cccp, GHD_POLL_DEVICE, 5, NULL, ap, intr_status);

	dptghd_doneq_pollmode_exit(cccp);
	mutex_exit(&cccp->ccc_hba_mutex);

	DPTGHD_DONEQ_PROCESS(cccp);

	return (rc);

}



/*
 * dptghd_tran_reset_target()
 *
 *	reset the target device
 *
 *
 */

int
dptghd_tran_reset_target(	ccc_t	*cccp,
				struct scsi_address *ap,
				void	*intr_status )
{
	int rc = TRUE;


	mutex_enter(&cccp->ccc_hba_mutex);
	dptghd_doneq_pollmode_enter(cccp);

	/* send out the abort device request */
	dptghd_timer_newstate(cccp, NULL, ap, GACTION_RESET_TARGET);

	/* wait upto 5 seconds for the device to reset */
	rc = dptghd_poll(cccp, GHD_POLL_DEVICE, 5, NULL, ap, intr_status);

	dptghd_doneq_pollmode_exit(cccp);
	mutex_exit(&cccp->ccc_hba_mutex);

	DPTGHD_DONEQ_PROCESS(cccp);
	return (rc);
}



/*
 * dptghd_tran_reset()
 *
 *	reset the scsi bus
 *
 */

int
dptghd_tran_reset_bus( ccc_t *cccp, void *intr_status )
{
	int	rc;

	mutex_enter(&cccp->ccc_hba_mutex);
	dptghd_doneq_pollmode_enter(cccp);

	/* send out the abort device request */
	dptghd_timer_newstate(cccp, NULL, NULL, GACTION_RESET_BUS);

	/*
	 * Wait for all active requests on this HBA to complete
	 */
	rc = dptghd_poll(cccp, GHD_POLL_ALL, 5, NULL, NULL, intr_status);


	dptghd_doneq_pollmode_exit(cccp);
	mutex_exit(&cccp->ccc_hba_mutex);

	DPTGHD_DONEQ_PROCESS(cccp);
	return (rc);
}

int
dptghd_transport(	ccc_t	*cccp,
			gcmd_t	*gcmdp,
			struct scsi_address *ap,
			ulong	 timeout,
			int	 polled,
			void	*intr_status )
{
	/*
	 * Save the scsi_address in case I later need to process a
	 * scsi_abort(ap, NULL) request.
	 */
	gcmdp->cmd_ap = ap;

	mutex_enter(&cccp->ccc_hba_mutex);

	GDBG_START(("dptghd_start(): done cccp=0x%x gcmdp=0x%x ap=0x%x\n",
		    cccp, gcmdp, ap));

	if (polled) {
		/* lock the doneq so I can scan for this request */
		dptghd_doneq_pollmode_enter(cccp);
	}

	if ((*cccp->ccc_hba_start)(cccp->ccc_hba_handle, gcmdp) 
			!= TRAN_ACCEPT) {
		if (polled) {
			/* unlock the doneq */
			dptghd_doneq_pollmode_exit(cccp);
		}
		mutex_exit(&cccp->ccc_hba_mutex);

		/* Do the completion callbacks now */
		DPTGHD_DONEQ_PROCESS(cccp);
		return (TRAN_BUSY);
	}

	/*
	 * Add this request to the packet timer list and start its
	 * abort timer.
	 */
	dptghd_timer_start(cccp, gcmdp, GCMD_STATE_ACTIVE, timeout);


	/*
	 * If FLAG_NOINTR, then wait until the request completes or times out
	 * before returning to caller
	 */
	if (!polled) {
		/* don't wait */
		mutex_exit(&cccp->ccc_hba_mutex);

		/* Do the completion callbacks now */
		DPTGHD_DONEQ_PROCESS(cccp);
		return (TRAN_ACCEPT);
	}

	/*
	 * Wait until the request completes
	 */
	(void)dptghd_poll(cccp, GHD_POLL_REQUEST, 0, gcmdp, ap, intr_status);
	gcmdp->cmd_state = GCMD_STATE_IDLE;

	dptghd_doneq_pollmode_exit(cccp);
	mutex_exit(&cccp->ccc_hba_mutex);

	/* Do the completion callbacks now */
	DPTGHD_DONEQ_PROCESS(cccp);
	return (TRAN_ACCEPT);
}
