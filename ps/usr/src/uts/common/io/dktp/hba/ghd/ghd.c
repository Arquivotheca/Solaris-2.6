/*
 * Copyright (c) 1996, Sun Microsystems, Inc.
 * All Rights Reserved.
 */

#pragma	ident	"@(#)ghd.c	1.1	96/07/30 SMI"

#include <sys/types.h>
#include <sys/kmem.h>
#include <sys/debug.h>
#include <sys/scsi/scsi.h>

#include "ghd.h"

/* ghd_poll() function codes: */
typedef enum {
	GHD_POLL_REQUEST,	/* wait for a specific request */
	GHD_POLL_DEVICE,	/* wait for a specific device to idle */
	GHD_POLL_ALL		/* wait for the whole bus to idle */
} gpoll_t;

/*
 * Local functions:
 */
static	gcmd_t	*ghd_doneq_get(ccc_t *cccp);
static	void	 ghd_doneq_pollmode_enter(ccc_t *cccp);
static	void	 ghd_doneq_pollmode_exit(ccc_t *cccp);

static	u_int	 ghd_dummy_intr(caddr_t arg);
static	int	 ghd_poll(ccc_t *cccp, gpoll_t polltype, ulong polltime,
			  gcmd_t *poll_gcmdp, void *tgtp, void *intr_status);


/*
 * Local configuration variables
 */

ulong	ghd_tran_abort_timeout = 5;
ulong	ghd_tran_abort_lun_timeout = 5;
ulong	ghd_tran_reset_target_timeout = 5;
ulong	ghd_tran_reset_bus_timeout = 5;

/*
 * ghd_complete:
 *
 *	The HBA driver calls this entry point when it's completely
 *	done processing a request.
 *
 */

void
ghd_complete( ccc_t *cccp, gcmd_t *gcmdp )
{

	ASSERT(mutex_owned(&cccp->ccc_hba_mutex));

	/* the request is now complete, so stop the packet timer */
	gcmdp->cmd_state = GCMD_STATE_DONEQ;
	ghd_timer_stop(cccp, gcmdp);

	/* and schedule the pkt completion callback */
	ghd_doneq_put(cccp, gcmdp);
	return;
}

void
ghd_doneq_put( ccc_t *cccp, gcmd_t *gcmdp )
{
	kmutex_t *doneq_mutexp = &cccp->ccc_doneq_mutex;

	mutex_enter(doneq_mutexp);
	QueueAdd(&cccp->ccc_doneq, &gcmdp->cmd_q, gcmdp);
	mutex_exit(doneq_mutexp);

	gcmdp->cmd_state = GCMD_STATE_DONEQ;
	return;
}

static gcmd_t	*
ghd_doneq_get( ccc_t *cccp )
{
	kmutex_t *doneq_mutexp = &cccp->ccc_doneq_mutex;
	gcmd_t	 *gcmdp;

	mutex_enter(doneq_mutexp);
	gcmdp = QueueRemove(&cccp->ccc_doneq);
	mutex_exit(doneq_mutexp);
	return (gcmdp);
}

gcmd_t	*
ghd_doneq_tryget( ccc_t *cccp )
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
ghd_doneq_pollmode_enter( ccc_t *cccp )
{
	kmutex_t *doneq_mutexp = &cccp->ccc_doneq_mutex;

	mutex_enter(doneq_mutexp);
	cccp->ccc_hba_pollmode = TRUE;
	mutex_exit(doneq_mutexp);
	return;
}


static void
ghd_doneq_pollmode_exit( ccc_t *cccp )
{
	kmutex_t *doneq_mutexp = &cccp->ccc_doneq_mutex;

	mutex_enter(doneq_mutexp);
	cccp->ccc_hba_pollmode = FALSE;
	mutex_exit(doneq_mutexp);
	return;
}


/* ***************************************************************** */

/*
 *
 * ghd_doneq_process()
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
ghd_doneq_process( ccc_t *cccp )
{
	GHD_DONEQ_PROCESS_INLINE(cccp);
}

/* ***************************************************************** */



void
ghd_waitq_delete( ccc_t *cccp, gcmd_t *gcmdp )
{
	kmutex_t *waitq_mutexp = &cccp->ccc_waitq_mutex;

	mutex_enter(waitq_mutexp);
	QueueDelete(&cccp->ccc_waitq, &gcmdp->cmd_q);
	mutex_exit(waitq_mutexp);
	return;
}

gcmd_t	*
ghd_waitq_get( ccc_t *cccp )
{
	kmutex_t *waitq_mutexp = &cccp->ccc_waitq_mutex;
	gcmd_t	 *gcmdp;

	mutex_enter(waitq_mutexp);
	gcmdp = QueueRemove(&cccp->ccc_waitq);
	mutex_exit(waitq_mutexp);
	return (gcmdp);
}

void
ghd_waitq_put( ccc_t *cccp, gcmd_t *gcmdp )
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
ghd_dummy_intr( caddr_t arg )
{
	return (DDI_INTR_UNCLAIMED);
}


/*
 * ghd_intr_init()
 *
 *	Do the usual interrupt handler setup stuff. Note the doneq has
 * to be protected by a mutex but that mutex isn't initialized here.
 * That way the caller can decide whether to use separate HBA and doneq
 * mutexes or a single mutex
 *
 */


int
ghd_register(	char	*labelp,
		ccc_t	*cccp,
		dev_info_t *dip,
		int	 inumber,
		void	*hba_handle,
		gcmd_t	*(*ccballoc)(void *, void *, int, int, int, int),
		void	(*ccbfree)(void *),
		void	(*sg_func)(gcmd_t *, ddi_dma_cookie_t *, int, int),
		int	(*hba_start)(void *, gcmd_t *),
		u_int	(*int_handler)(caddr_t),
		int	(*get_status)(void *, void *),
		void	(*process_intr)(void *, void *),
		int	(*timeout_func)(void *, gcmd_t *, void *, gact_t),
		tmr_t	*tmrp )
{

	cccp->ccc_label = labelp;

	/*
	 *	Establish initial dummy interrupt handler
	 *	get iblock cookie to initialize mutexes used in the
	 *	real interrupt handler
	 */
	if (ddi_add_intr(dip, inumber, &cccp->ccc_iblock, NULL,
			 ghd_dummy_intr, hba_handle) != DDI_SUCCESS) {
		return (FALSE);
	}
	mutex_init(&cccp->ccc_hba_mutex, labelp, MUTEX_DRIVER,
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

	ghd_timer_attach(cccp, tmrp, timeout_func);
	ghd_doneq_pollmode_exit(cccp);
	return (TRUE);
}


void
ghd_unregister( ccc_t *cccp )
{
	ghd_timer_detach(cccp);

	ddi_remove_intr(cccp->ccc_hba_dip, 0, cccp->ccc_iblock);
	mutex_destroy(&cccp->ccc_hba_mutex);
	mutex_destroy(&cccp->ccc_waitq_mutex);
	mutex_destroy(&cccp->ccc_doneq_mutex);
}



int
ghd_intr( ccc_t *cccp, void *intr_status )
{
	kmutex_t *hba_mutexp = &cccp->ccc_hba_mutex;


	mutex_enter(hba_mutexp);

	GDBG_INTR(("ghd_intr(): cccp=0x%x status=0x%x\n",
			cccp, intr_status));

	if (!(*cccp->ccc_get_status)(cccp->ccc_hba_handle, intr_status)) {
		mutex_exit(hba_mutexp);

		/*
		 * The get_status() could possibly change
		 * the doneq??? Shouldn't happen but try doing
		 * the completion callbacks here just to be safe.
		 */
		GHD_DONEQ_PROCESS(cccp);
		return (DDI_INTR_UNCLAIMED);
	}

	for (;;) {
		/* process the interrupt status */
		(*cccp->ccc_process_intr)(cccp->ccc_hba_handle, intr_status);

		/* loop if new interrupt status was stacked by hardware */
		if (!(*cccp->ccc_get_status)(cccp->ccc_hba_handle, intr_status))
			break;
	}

	GDBG_INTR(("ghd_intr(): done cccp=0x%x status=0x%x\n",
		   cccp, intr_status));

	mutex_exit(hba_mutexp);
	GHD_DONEQ_PROCESS(cccp);

	return (DDI_INTR_CLAIMED);
}

static int
ghd_poll(	ccc_t	*cccp,
		gpoll_t	 polltype,
		ulong	 polltime,
		gcmd_t	*poll_gcmdp,
		void	*tgtp,
		void	*intr_status )
{
	gcmd_t	 *gcmdp;
	Que_t	  gcmd_hold_queue;
	int	  got_it = FALSE;
	ulong	  start_lbolt;
	ulong	  current_lbolt;


	ASSERT(mutex_owned(&cccp->ccc_hba_mutex));
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
		ghd_timer_poll(cccp);


		/*
		 * Unqueue all the completed requests, look for mine
		 */
		while (gcmdp = ghd_doneq_get(cccp)) {
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
				if (tgtp == gcmdp->cmd_tgtp) {
					got_it = FALSE;
					break;
				}
				gcmdp = (gcmd_t *)
					L2_next(&gcmdp->cmd_timer_link);
			}
			mutex_exit(&cccp->ccc_activel_mutex);
			break;

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
	 	ghd_doneq_put(cccp, gcmdp);

	return (got_it);
}


/*
 * ghd_tran_abort()
 *
 *	Abort specific command on a target.
 *
 */

int
ghd_tran_abort(	ccc_t	*cccp,
		gcmd_t	*gcmdp,
		void	*tgtp,
		void	*intr_status )
{
	gact_t	 action;
	int	 rc;

	/*
	 * call the driver's abort_cmd function
	 */

	mutex_enter(&cccp->ccc_hba_mutex);
	ghd_doneq_pollmode_enter(cccp);

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

	/* stop the timer and remove it from the active list */
	ghd_timer_stop(cccp, gcmdp);

	/* start a new timer and send out the abort command */
	ghd_timer_newstate(cccp, gcmdp, tgtp, action);

	/* wait for the abort to complete */
	if (rc = ghd_poll(cccp, GHD_POLL_REQUEST, ghd_tran_abort_timeout,
			  gcmdp, tgtp, intr_status)) {
		gcmdp->cmd_state = GCMD_STATE_DONEQ;
		ghd_doneq_put(cccp, gcmdp);
	}



exit:
	ghd_doneq_pollmode_exit(cccp);
	mutex_exit(&cccp->ccc_hba_mutex);

	GHD_DONEQ_PROCESS(cccp);

	return (rc);
}


/*
 * ghd_tran_abort_lun()
 *
 *	Abort all commands on a specific target.
 *
 */

int
ghd_tran_abort_lun(	ccc_t	*cccp,
			void	*tgtp,
			void	*intr_status )
{
	int	 rc;

	/*
	 * call the HBA driver's abort_device function
	 */

	mutex_enter(&cccp->ccc_hba_mutex);
	ghd_doneq_pollmode_enter(cccp);

	/* send out the abort device request */
	ghd_timer_newstate(cccp, NULL, tgtp, GACTION_ABORT_DEV);

	/* wait for the device to go idle */
	rc = ghd_poll(cccp, GHD_POLL_DEVICE, ghd_tran_abort_lun_timeout,
		      NULL, tgtp, intr_status);

	ghd_doneq_pollmode_exit(cccp);
	mutex_exit(&cccp->ccc_hba_mutex);

	GHD_DONEQ_PROCESS(cccp);

	return (rc);

}



/*
 * ghd_tran_reset_target()
 *
 *	reset the target device
 *
 *
 */

int
ghd_tran_reset_target(	ccc_t	*cccp,
			void	*tgtp,
			void	*intr_status )
{
	int rc = TRUE;


	mutex_enter(&cccp->ccc_hba_mutex);
	ghd_doneq_pollmode_enter(cccp);

	/* send out the device reset request */
	ghd_timer_newstate(cccp, NULL, tgtp, GACTION_RESET_TARGET);

	/* wait for the device to reset */
	rc = ghd_poll(cccp, GHD_POLL_DEVICE, ghd_tran_reset_target_timeout,
		      NULL, tgtp, intr_status);

	ghd_doneq_pollmode_exit(cccp);
	mutex_exit(&cccp->ccc_hba_mutex);

	GHD_DONEQ_PROCESS(cccp);
	return (rc);
}



/*
 * ghd_tran_reset()
 *
 *	reset the scsi bus
 *
 */

int
ghd_tran_reset_bus(	ccc_t	*cccp,
			void	*tgtp,
			void	*intr_status )
{
	int	rc;

	mutex_enter(&cccp->ccc_hba_mutex);
	ghd_doneq_pollmode_enter(cccp);

	/* send out the bus reset request */
	ghd_timer_newstate(cccp, NULL, tgtp, GACTION_RESET_BUS);

	/*
	 * Wait for all active requests on this HBA to complete
	 */
	rc = ghd_poll(cccp, GHD_POLL_ALL, ghd_tran_reset_bus_timeout,
		      NULL, NULL, intr_status);


	ghd_doneq_pollmode_exit(cccp);
	mutex_exit(&cccp->ccc_hba_mutex);

	GHD_DONEQ_PROCESS(cccp);
	return (rc);
}

int
ghd_transport(	ccc_t	*cccp,
		gcmd_t	*gcmdp,
		void	*tgtp,
		ulong	 timeout,
		int	 polled,
		void	*intr_status )
{
	/*
	 * Save the target identifier in case we need to abort
	 * everything on a target. 
	 */
	gcmdp->cmd_tgtp = tgtp;

	mutex_enter(&cccp->ccc_hba_mutex);

	GDBG_START(("ghd_transport(): done cccp=0x%x gcmdp=0x%x tgtp=0x%x\n",
		    cccp, gcmdp, tgtp));

	if (polled) {
		/* lock the doneq so I can scan for this request */
		ghd_doneq_pollmode_enter(cccp);
	}

	/*
	 * Add this request to the packet timer list and start its
	 * abort timer.
	 */
	gcmdp->cmd_state = GCMD_STATE_ACTIVE;
	ghd_timer_start(cccp, gcmdp, timeout);

	/*
	 * Start up the I/O request
	 */
	if ((*cccp->ccc_hba_start)(cccp->ccc_hba_handle, gcmdp) 
			!= TRAN_ACCEPT) {
		/* it failed before it started, stop the timer */
		gcmdp->cmd_state = GCMD_STATE_IDLE;
		ghd_timer_stop(cccp, gcmdp);

		if (polled) {
			/* unlock the doneq */
			ghd_doneq_pollmode_exit(cccp);
		}
		mutex_exit(&cccp->ccc_hba_mutex);

		/* Do the completion callbacks now */
		GHD_DONEQ_PROCESS(cccp);
		return (TRAN_BUSY);
	}


	/*
	 * If FLAG_NOINTR, then wait until the request completes or times out
	 * before returning to caller
	 */
	if (polled) {
		/*
		 * Wait until the request completes
		 */
		(void)ghd_poll(cccp, GHD_POLL_REQUEST, 0, gcmdp, tgtp,
				intr_status);
		ghd_doneq_pollmode_exit(cccp);
		ASSERT(gcmdp->cmd_state == GCMD_STATE_IDLE);
	}

	/* Drop the HBA mutex and then do the completion callbacks */
	mutex_exit(&cccp->ccc_hba_mutex);
	GHD_DONEQ_PROCESS(cccp);
	return (TRAN_ACCEPT);
}

#ifdef GHD_DEBUG
static void
ghd_dump_ccc( ccc_t *cccp )
{

	prom_printf("activel 0x%x waitq 0x%x doneq 0x%x\n",
		&cccp->ccc_activel, &cccp->ccc_waitq, &cccp->ccc_doneq);
	return;
}
#endif
