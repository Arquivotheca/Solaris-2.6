/*
 * Copyright (c) 1996, Sun Microsystems, Inc.
 * All Rights Reserved.
 */

#pragma	ident	"@(#)dptghd_timer.c	1.1	96/06/13 SMI"

#include <sys/types.h>
#include <sys/conf.h>
#include <sys/ddi.h>
#include <sys/sunddi.h>
#include <sys/ksynch.h>
#include <sys/scsi/conf/autoconf.h>

#include "dptghd.h"

/*
 * Local functions
 */

static	gcmd_t	*dptghd_timeout_get(ccc_t *cccp);
static	int	 dptghd_timeout_loop(ccc_t *cccp);
static	u_int	 dptghd_timeout_softintr(caddr_t arg);
static	void	 dptghd_timeout(caddr_t arg);
static	void	 dptghd_timeout_disable(tmr_t *tmrp);
static	void	 dptghd_timeout_enable(tmr_t *tmrp);

/*
 * Local data
 */
static	kmutex_t tglobal_mutex;

/* table of timeouts for abort processing steps */
cmdstate_t dptghd_timeout_table[GCMD_NSTATES];

/* This table indirectly initializes the ghd_timeout_table */
struct {
	int		valid;
	cmdstate_t	state;
	long		value;
} dptghd_time_inits[] = {
	{ TRUE, GCMD_STATE_ABORTING_CMD, 3 },
	{ TRUE, GCMD_STATE_ABORTING_DEV, 3 },
	{ TRUE, GCMD_STATE_RESETTING_DEV, 5 },
	{ TRUE, GCMD_STATE_RESETTING_BUS, 10 },
	{ TRUE, GCMD_STATE_HUNG, 60},
	{ FALSE, 0, 0 },	/* spare entry */
	{ FALSE, 0, 0 },	/* spare entry */
	{ FALSE, 0, 0 },	/* spare entry */
	{ FALSE, 0, 0 },	/* spare entry */
	{ FALSE, 0, 0 }		/* spare entry */
};
int	dptghd_ntime_inits = sizeof dptghd_time_inits
				/ sizeof dptghd_time_inits[0];

#ifdef ___notyet___

#include <sys/modctl.h>
extern struct mod_ops mod_miscops;
static struct modlmisc modlmisc = {
	&mod_miscops,	/* Type of module */
	"CCB Timeout Utility Routines"
};
static struct modlinkage modlinkage = {
	MODREV_1, (void *)&modlmisc, NULL
};

/*
 * If this is a loadable module then there's a single CCB timer configure
 * structure for all HBA drivers (rather than one per HBA driver).
 */
static	tmr_t	tmr_conf;

_init()
{
	dptghd_timer_init(&tmr_conf, "CCB timer", 0);
	return (mod_install(&modlinkage));
}

_fini()
{
	dptghd_timer_fini(&tmr_conf);
	return (modremove(&modlinkage));
}
_info(modinfop)
struct modinfo *modinfop;
{
	return (mod_info(&modlinkage, modinfop));
}

#endif /* ___notyet___ */



/*
 *
 * dptghd_timeout_loop()
 *
 *	Check the CCB timer value for every active CCB for this
 * HBA driver instance.
 *
 *	This function is called both by the ghd_timeout() interrupt 
 * handler when called via the timer callout, and by ghd_timer_poll()
 * while procesing "polled" (FLAG_NOINTR) requests. 
 *
 * 	The ccc_activel_mutex is held while a CCB list is being scanned.
 * This prevents the HBA driver's transport or interrupt functions
 * from changing the active CCB list. But we wake up very infrequently
 * and do as little as possible so it shouldn't affect performance.
 *	
 */

static int
dptghd_timeout_loop( ccc_t *cccp )
{
	int	 got_any = FALSE;
	gcmd_t	*gcmdp;
	ulong	 lbolt;

	drv_getparm(LBOLT, &lbolt);

	mutex_enter(&cccp->ccc_activel_mutex);
	gcmdp = (gcmd_t *)L2_next(&cccp->ccc_activel);
	while (gcmdp) {
		/*
		 * check to see if this one has timed out
		 */
		if ((gcmdp->cmd_timeout > 0)
		&&  (lbolt - gcmdp->cmd_start_time >= gcmdp->cmd_timeout)) {
			got_any = TRUE;
		}
		gcmdp = (gcmd_t *)L2_next(&gcmdp->cmd_timer_link);
	}
	mutex_exit(&cccp->ccc_activel_mutex);
	return (got_any);
}

/*
 *
 * dptghd_timeout()
 *
 *	Called every t_ticks ticks to scan the CCB timer lists
 *
 *	The t_mutex mutex is held the entire time this routine is active.
 *	It protects the list of ccc_t's.
 *
 *	The list of cmd_t's is protected by the ccc_activel_mutex mutex
 *	in the ghd_timeout_loop() routine.
 *
 *
	+------------+
	|   tmr_t    |----+
	+------------+    |
			  |
			  V
			  +---------+
			  |  ccc_t  |----+
			  +---------+    |
			  |		 V
			  |		 +--------+   +--------+
			  |		 | gcmd_t |-->| gcmd_t |--> ...
			  |		 +--------+   +--------+
			  V
			  +---------+
			  |  ccc_t  |----+
			  +---------+    |
			  |		 V
			  |		 +--------+
			  |		 | gcmd_t |
			  V		 +--------+
			  ...
 *
 *
 *
 */

static void
dptghd_timeout( caddr_t arg )
{
	tmr_t	*tmrp = (tmr_t *)arg;
	ccc_t	*cccp;

	/*
	 * Each HBA driver instance has a separate CCB timer list.
	 * Just exit if there are no more active timeout lists
	 * to process.
	 */
	mutex_enter(&tmrp->t_mutex);
	if ((cccp = tmrp->t_ccc_listp) == NULL) {
		mutex_exit(&tmrp->t_mutex);
		return;
	}

	do {
		/*
		 * If any active CCBs on this HBA have timed out then kick
		 * off the HBA driver's softintr handler to do the timeout
		 * processing
		 */
		if (dptghd_timeout_loop(cccp))
			ddi_trigger_softintr(cccp->ccc_soft_id);
	} while ((cccp = cccp->ccc_nextp) != NULL);

	/* re-establish the timeout callback */
	tmrp->t_timeout_id = timeout(dptghd_timeout, (caddr_t)tmrp,
						tmrp->t_ticks);
	mutex_exit(&tmrp->t_mutex);
	return;
}


/*
 *
 * dptghd_timer_newstate()
 *
 *	The HBA mutex is held by my caller.
 *
 */

cmdstate_t
dptghd_timer_newstate(	ccc_t	*cccp,
			gcmd_t	*gcmdp,
			struct scsi_address *ap,
			gact_t	 action )
{
	void	(*func)(void *, gcmd_t *, struct scsi_address *, gact_t);
	void	*hba_handle;
	cmdstate_t next_state;

	ASSERT(mutex_owned(&cccp->ccc_hba_mutex));

	func = cccp->ccc_timeout_func;
	hba_handle = cccp->ccc_hba_handle;

	switch (action) {
	case GACTION_EARLY_ABORT:
	case GACTION_EARLY_TIMEOUT:
		/* done before it started */
		dptghd_waitq_delete(cccp, gcmdp);
		(*func)(hba_handle, gcmdp, NULL, action);

		/* all done */
		gcmdp->cmd_state = GCMD_STATE_DONEQ;
		dptghd_doneq_put(cccp, gcmdp);
		next_state = GCMD_STATE_DONEQ;
		break;

	case GACTION_ABORT_CMD:
		GDBG_WARN(("dptghd_timer_newstate: scsi_abort request, "
			   "h=0x%x g=0x%x ap=0x%x\n", hba_handle, gcmdp, ap));
		(*func)(hba_handle, gcmdp, ap, action);
		next_state = GCMD_STATE_ABORTING_CMD;
		break;

	case GACTION_ABORT_DEV:
		GDBG_WARN(("dptghd_timer_newstate: scsi_abort device, "
			   "h=0x%x g=0x%x ap=0x%x\n", hba_handle, gcmdp, ap));
		(*func)(hba_handle, gcmdp, ap, action);
		next_state = GCMD_STATE_ABORTING_DEV;
		break;

	case GACTION_RESET_TARGET:
		GDBG_WARN(("dptghd_timer_newstate: scsi_reset target, "
			   "h=0x%x g=0x%x ap=0x%x\n", hba_handle, gcmdp, ap));
		(*func)(hba_handle, gcmdp, ap, action);
		next_state = GCMD_STATE_RESETTING_DEV;
		break;

	case GACTION_RESET_BUS:
		GDBG_WARN(("dptghd_timer_newstate: scsi_reset bus, "
			   "h=0x%x g=0x%x ap=0x%x\n", hba_handle, gcmdp, ap));
		(*func)(hba_handle, gcmdp, ap, action);
		next_state = GCMD_STATE_RESETTING_BUS;
		break;

	case GACTION_INCOMPLETE:
	default:
		GDBG_ERROR(("dptghd_timer_newstate: HBA bus reset failed "
				 "cccp=0x%x gcmdp=0x%x ap=0x%x\n",
				 cccp, gcmdp, ap));
		/*
		 * When all else fails, punt.
		 *
		 * We're in big trouble if we get to this point.
		 * Maybe we should try to re-initialize the HBA.
		 */
		(*func)(hba_handle, gcmdp, ap, action);
		next_state = GCMD_STATE_HUNG;
	}

	return (next_state);
}


/*
 *
 * dptghd_timeout_softintr()
 *
 *	This interrupt is scheduled if a particular HBA instance's
 *	CCB timer list has a timed out CCB.
 *
 *	Find the timed out CCB and then call the HBA driver's timeout
 *	function.
 *
 *	In order to avoid race conditions all processing must be done
 *	while holding the HBA instance's mutex. If the mutex wasn't
 *	held the HBA driver's hardware interrupt routine could be
 *	triggered and it might try to remove a CCB from the list at
 *	same time as were trying to abort it.
 *
 */

static u_int
dptghd_timeout_softintr( caddr_t arg )
{
	ccc_t	*cccp = (ccc_t *)arg;

	/* grab this HBA instance's mutex */
	mutex_enter(&cccp->ccc_hba_mutex);

	GDBG_INTR(("dptghd_timeout_softintr: cccp=0x%x\n", cccp));

	/* timeout each expired CCB */
	dptghd_timer_poll(cccp);

	mutex_exit(&cccp->ccc_hba_mutex);

	/*
	 * Now run the pkt completion routines of anything that finished
	 */
	DPTGHD_DONEQ_PROCESS(cccp);

	return (DDI_INTR_UNCLAIMED);
}


/*
 * dptghd_timer_poll()
 *
 * This function steps a packet to the next action in the recovery
 * procedure.
 *
 * The caller must be  already holding the HBA mutex and take care of
 * running the pkt completion functions.
 *
 */

void
dptghd_timer_poll( ccc_t *cccp )
{
	gcmd_t	*gcmdp;
	gact_t	 action;
	cmdstate_t next_state;
	long	 new_timeout;

	ASSERT(mutex_owned(&cccp->ccc_hba_mutex));

	/* abort each expired CCB */
	while (gcmdp = dptghd_timeout_get(cccp)) {

		switch (gcmdp->cmd_state) {
		case GCMD_STATE_IDLE:
		case GCMD_STATE_DONEQ:
		default:
			/* not supposed to happen */
			GDBG_ERROR(("dptghd_timer_poll: invalid state %d\n",
				    gcmdp->cmd_state));
			return;

		case GCMD_STATE_WAITQ:
			action = GACTION_EARLY_TIMEOUT;
			break;

		case GCMD_STATE_ACTIVE:
			action = GACTION_ABORT_CMD;
			break;

		case GCMD_STATE_ABORTING_CMD:
			action = GACTION_ABORT_DEV;
			break;

		case GCMD_STATE_ABORTING_DEV:
			action = GACTION_RESET_TARGET;
			break;

		case GCMD_STATE_RESETTING_DEV:
			action = GACTION_RESET_BUS;
			break;

		case GCMD_STATE_RESETTING_BUS:
			action = GACTION_INCOMPLETE;
			break;

		case GCMD_STATE_HUNG:
			action = GACTION_INCOMPLETE;
			break;
		}

		next_state = dptghd_timer_newstate(cccp, gcmdp, gcmdp->cmd_ap,
						action);
		
		/*
		 * Restart the timer for the request using the
		 * timeout value from dptghd_timeout_table[].
		 */
		if (new_timeout = dptghd_timeout_table[next_state]) {
			dptghd_timer_start(cccp, gcmdp, next_state,
					new_timeout);
		} 
	}
	return;
}




/*
 *
 * dptghd_timeout_get()
 *
 *	Remove the first expired CCB from a particular timer list.
 *
 */

static gcmd_t *
dptghd_timeout_get( ccc_t *cccp )
{
	gcmd_t	*gcmdp;
	ulong	lbolt;

	ASSERT(mutex_owned(&cccp->ccc_hba_mutex));

	drv_getparm(LBOLT, &lbolt);

	mutex_enter(&cccp->ccc_activel_mutex);
	gcmdp = (gcmd_t *)L2_next(&cccp->ccc_activel);
	while (gcmdp != NULL) {
		if (lbolt - gcmdp->cmd_start_time >= gcmdp->cmd_timeout)
			goto expired;
		gcmdp = (gcmd_t *)L2_next(&gcmdp->cmd_timer_link);
	}
	mutex_exit(&cccp->ccc_activel_mutex);
	return (NULL);

expired:
	/* unlink if from the CCB timer list */
	L2_delete(&gcmdp->cmd_timer_link);
	mutex_exit(&cccp->ccc_activel_mutex);
	return (gcmdp);
}


/*
 *
 * dptghd_timeout_enable()
 *
 *	Only start a single timeout callback for each HBA driver
 *	regardless of the number of boards it supports.
 *
 */

static void
dptghd_timeout_enable( tmr_t *tmrp )
{
	mutex_enter(&tglobal_mutex);
	if (tmrp->t_refs++ == 0)  {
		/* establish the timeout callback */
		tmrp->t_timeout_id = timeout(dptghd_timeout, (caddr_t)tmrp,
							tmrp->t_ticks);
	}
	mutex_exit(&tglobal_mutex);
	return;
}

static void
dptghd_timeout_disable( tmr_t *tmrp )
{
	ASSERT(tmrp != NULL);
	ASSERT(tmrp->t_ccc_listp == NULL);

	mutex_enter(&tglobal_mutex);
	if (tmrp->t_refs-- <= 1)
		untimeout(tmrp->t_timeout_id);
	mutex_exit(&tglobal_mutex);
	return;
}

/* ************************************************************************ */

	/* these are the externally callable routines */

/*
 *
 * dptghd_timer_init()
 *
 *
 */

void
dptghd_timer_init( tmr_t *tmrp, char *name, long ticks )
{
	int	indx;

	mutex_init(&tglobal_mutex, "DPTGHD timer global mutex", MUTEX_DRIVER,
			(void *)NULL);
	mutex_init(&tmrp->t_mutex, name, MUTEX_DRIVER, (void *)NULL);

	/*
	 *determine default timeout value
	 */
	if (ticks == 0)
		ticks = scsi_watchdog_tick * drv_usectohz(1000000);
	tmrp->t_ticks = ticks;


	/*
	 * Initialize the table of abort timer values using an
	 * indirect lookup table so that this code isn't dependant
	 * on the cmdstate_t enum values or order.
	 */
	for(indx = 0; indx < dptghd_ntime_inits; indx++) {
		int	state;
		ulong	value;

		if (!dptghd_time_inits[indx].valid)
			continue;
		state = dptghd_time_inits[indx].state;
		value = dptghd_time_inits[indx].value;
		dptghd_timeout_table[state] = value;
	}
	return;
}


void
dptghd_timer_fini( tmr_t *tmrp )
{
	mutex_destroy(&tmrp->t_mutex);
	mutex_destroy(&tglobal_mutex);
	return;
}



int
dptghd_timer_attach(	ccc_t	*cccp,
			tmr_t	*tmrp,
			void	(*timeout_func)(void *handle, gcmd_t *,
						struct scsi_address *, gact_t) )
{
	ddi_iblock_cookie_t iblock;

	if (ddi_add_softintr(cccp->ccc_hba_dip, DDI_SOFTINT_LOW,
				&cccp->ccc_soft_id, &iblock, NULL,
				dptghd_timeout_softintr, (caddr_t)cccp)
	!=  DDI_SUCCESS) {
		return (FALSE);
	}

	/* init the per HBA-instance control fields */
	mutex_init(&cccp->ccc_activel_mutex, "CCB list mutex", MUTEX_DRIVER,
		iblock);
	L2_INIT(&cccp->ccc_activel);
	cccp->ccc_timeout_func = timeout_func;

	/* stick this HBA's control structure on the master list */
	mutex_enter(&tmrp->t_mutex);

	cccp->ccc_nextp = tmrp->t_ccc_listp;
	tmrp->t_ccc_listp = cccp;
	mutex_exit(&tmrp->t_mutex);

	/*
	 * The enable and disable routines use a separate mutex than
	 * t_mutex which is used by the timeout callback function.
	 * This is to avoid a deadlock when calling untimeout() from
	 * the disable routine.
	 */
	dptghd_timeout_enable(tmrp);

	return (TRUE);
}


/*
 *
 * dptghd_detach()
 *
 *	clean up for a detaching HBA instance
 *
 */

void
dptghd_timer_detach( ccc_t *cccp )
{
	tmr_t	*tmrp = cccp->ccc_tmrp;
	ccc_t	**prevpp;

	/* make certain the CCB list is empty */
	ASSERT(cccp->ccc_activel.l2_nextp == &cccp->ccc_activel);
	ASSERT(cccp->ccc_activel.l2_nextp == cccp->ccc_activel.l2_prevp);

	mutex_enter(&tmrp->t_mutex);

	prevpp = &tmrp->t_ccc_listp;
	ASSERT(*prevpp != NULL);

	/* run down the linked list to find the entry that preceeds this one */
	do {
		if (*prevpp == cccp)
			goto remove_it;
		prevpp = &(*prevpp)->ccc_nextp;
	} while (*prevpp != NULL);

	/* fell off the end of the list */
	GDBG_ERROR(("dptghd_timer_detach: corrupt list, cccp=0x%x\n", cccp));

remove_it:
	*prevpp = cccp->ccc_nextp;
	mutex_exit(&tmrp->t_mutex);
	mutex_destroy(&cccp->ccc_activel_mutex);

	ddi_remove_softintr(cccp->ccc_soft_id);

	dptghd_timeout_disable(tmrp);

	return;
}

/*
 *
 * dptghd_timer_start()
 *
 *	Add a CCB to the CCB timer list.
 */

void
dptghd_timer_start(	ccc_t	*cccp,
			gcmd_t	*gcmdp,
			cmdstate_t next_state,
			long	 cmd_timeout )
{
	ulong	lbolt;

	drv_getparm(LBOLT, &lbolt);
	mutex_enter(&cccp->ccc_activel_mutex);

	/* initialize this CCB's timer */
	gcmdp->cmd_state = next_state;
	gcmdp->cmd_start_time = lbolt;
	gcmdp->cmd_timeout = (cmd_timeout * HZ);


	/* add it to the list */
	L2_add(&cccp->ccc_activel, &gcmdp->cmd_timer_link, gcmdp);
	mutex_exit(&cccp->ccc_activel_mutex);
	return;
}


/*
 *
 * dptghd_timer_stop()
 *
 *	Remove a completed CCB from the CCB timer list.
 *
 */

void
dptghd_timer_stop( ccc_t *cccp , gcmd_t *gcmdp )
{

	mutex_enter(&cccp->ccc_activel_mutex);
	L2_delete(&gcmdp->cmd_timer_link);
	mutex_exit(&cccp->ccc_activel_mutex);
	return;
}
