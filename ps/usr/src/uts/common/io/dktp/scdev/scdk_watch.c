/*
 * Copyright (c) 1996, by Sun Microsystems, Inc.
 * All rights reserved.
 */

#pragma ident	"@(#)scdk_watch.c	1.12	96/09/12 SMI"

/*
 * generic scsi device watch
 */

#if DEBUG || lint
#define	SWDEBUG
#endif

/*
 * debug goodies
 */
#ifdef SWDEBUG
static int swdebug = 0;
#define	DEBUGGING	((scsi_options & SCSI_DEBUG_TGT) && scdk_debug > 1)
#define	SW_DEBUG	if (swdebug == 1) scsi_log
#define	SW_DEBUG2	if (swdebug > 1) scsi_log
#else	/* SWDEBUG */
#define	swdebug		(0)
#define	DEBUGGING	(0)
#define	SW_DEBUG	if (0) scsi_log
#define	SW_DEBUG2	if (0) scsi_log
#endif



/*
 * Includes, Declarations and Local Data
 */

#include <sys/scsi/scsi.h>
#include <sys/dktp/scdkwatch.h>
#include <sys/var.h>
#include <sys/proc.h>
#include <sys/thread.h>
#include  <sys/buf.h>
#include  <sys/debug.h>

#include <sys/vtoc.h>
#include <sys/dkio.h>
#include <sys/cdio.h>
#include <sys/file.h>

#include <sys/dktp/sctarget.h>
#include <sys/dktp/objmgr.h>
#include <sys/dktp/flowctrl.h>
#include <sys/dktp/tgcom.h>
#include <sys/dktp/tgdk.h>
#include <sys/dktp/tgcd.h>
#include <sys/dktp/tgpassthru.h>
#include <sys/dktp/bbh.h>
#include <sys/dktp/scdk.h>

#ifdef TRACE
#include <sys/vtrace.h>
#endif	/* TRACE */

char *sw_label = "scdk-watch";

/*
 * all info resides in the scdk watch structure
 *
 * the monitoring is performed by one separate thread which works
 * from a linked list of scdk_watch_request packets
 */
static struct scdk_watch {
	kthread_t		*sw_thread;	/* the watch thread	*/
	kmutex_t		sw_mutex;	/* mutex protecting list */
						/* and this structure */
	kcondvar_t		sw_cv;		/* cv for waking up thread */
	struct scdk_watch_request *sw_head;	/* head of linked list	*/
						/* of request structures */
	u_char			 sw_state;	/* for suspend-resume */
} sw;

/* ALL MEMBERS PROTECTED BY scdk_watch::sw_mutex */

/*
 * Values for sw_state
 */
#define	SW_RUNNING		0
#define	SW_SUSPEND_REQUESTED	1
#define	SW_SUSPENDED		2

struct scdk_watch_request {
	struct scdk_watch_request *swr_next;	/* linked request list	*/
	struct scdk_watch_request *swr_prev;
	int			swr_interval;	/* interval between TURs */
	int			swr_timeout;	/* count down		*/
	u_char			swr_busy;	/* TUR in progress	*/
	u_char			swr_what;	/* watch or stop	*/
	u_char			swr_sense_length; /* required sense length */
	struct scsi_pkt		*swr_pkt;	/* TUR pkt itself	*/
	struct scsi_pkt		*swr_rqpkt;	/* request sense pkt	*/
	struct buf		*swr_bp;	/* bp for TUR data */
	struct buf		*swr_rqbp;	/* bp for request sense data */
	int			(*swr_callback)(); /* callback to driver */
	caddr_t			swr_callback_arg;
};

/* VARIABLES PROTECTED BY "Unshared data": scdk_watch_request:: */

/*
 * values for sw_what
 */
#define	SWR_WATCH	0	/* device watch */
#define	SWR_STOP	1	/* stop monitoring and destroy swr */

opaque_t scdk_watch_request_submit(struct scsi_device *devp,
	int interval, int sense_length, int (*callback)(), caddr_t cb_arg);
static void scdk_watch_request_destroy(struct scdk_watch_request *swr);
void scdk_watch_request_terminate(opaque_t token);
static void scdk_watch_thread();
static void scdk_watch_request_intr(struct scsi_pkt *pkt);
void scdk_watch_resume();
void scdk_watch_suspend();

/*
 * setup, called from _init(), the thread is created when we need it
 * and exits when there is nothing to do anymore and everything has been
 * cleaned up (ie. resources deallocated)
 */
void
scdk_watch_init()
{
/* NO OTHER THREADS ARE RUNNING */
	mutex_init(&sw.sw_mutex, "scdk_watch_mutex", MUTEX_DRIVER, NULL);
	cv_init(&sw.sw_cv, "scdk_watch_cv", CV_DRIVER, NULL);
	sw.sw_state = SW_RUNNING;
}

/*
 * cleaning up, called from _fini()
 */
void
scdk_watch_fini()
{
/* NO OTHER THREADS ARE RUNNING */
	/*
	 * hope and pray that the thread has exited
	 */
	ASSERT(sw.sw_thread == 0);
	mutex_destroy(&sw.sw_mutex);
	cv_destroy(&sw.sw_cv);
}

/*
 * allocate an swr (scsi watch request structure) and initialize pkts
 */
#define	ROUTE		&devp->sd_address
#define	TUR_TIMEOUT	5
#define	RQS_TIMEOUT	5

opaque_t
scdk_watch_request_submit(
	struct scsi_device	*devp,
	int			interval,
	int			sense_length,
	int			(*callback)(),	/* callback function */
	caddr_t			cb_arg)		/* device number */
{
	register struct scdk_watch_request	*swr = NULL;
	register struct scdk_watch_request	*sswr, *p;
	struct buf				*bp = NULL;
	struct buf				*rqbp = NULL;
	struct scsi_pkt				*rqpkt = NULL;
	struct scsi_pkt				*pkt = NULL;
	struct target_private			*tp = NULL;

	SW_DEBUG((dev_info_t *)NULL, sw_label, SCSI_DEBUG,
		"scdk_watch_request_submit: Entering ...\n");

	mutex_enter(&sw.sw_mutex);
	if (sw.sw_thread == 0) {
		register kthread_t	*t;

		t = thread_create((caddr_t)NULL, 0, scdk_watch_thread,
			(caddr_t)0, 0, &p0, TS_RUN, v.v_maxsyspri - 2);
		if (t == NULL) {
			mutex_exit(&sw.sw_mutex);
			return (NULL);
		} else {
			sw.sw_thread = t;
		}
	}

	for (p = sw.sw_head; p != NULL; p = p->swr_next) {
		if ((p->swr_callback_arg == cb_arg) &&
			(p->swr_callback == callback))
			break;
	}

	/* update time interval for an existing request */
	if (p) {
		p->swr_timeout = p->swr_interval = drv_usectohz(interval);
		p->swr_what = SWR_WATCH;
		cv_signal(&sw.sw_cv);
		mutex_exit(&sw.sw_mutex);
		return ((opaque_t)p);
	}
	mutex_exit(&sw.sw_mutex);

	/*
	 * allocate space for scdk_watch_request
	 */
	swr = (struct scdk_watch_request *)
		kmem_zalloc(sizeof (struct scdk_watch_request), KM_SLEEP);

	/*
	 * x86 kludge: allocate a target_private structure to avoid
	 * panics in scdk.c
	 */
	tp = (struct target_private *)
		kmem_zalloc(sizeof (struct target_private), KM_SLEEP);
	tp->x_bp = (struct buf *)swr;		/* save address of swr */
	tp->x_sdevp = (opaque_t)cb_arg;		/* setup scdkp */
	tp->x_callback = (void (*)())nulldev;	/* setup callback */

	/*
	 * allocate request sense bp and pkt and make cmd
	 * we shouldn't really need it if ARQ is enabled but it is useful
	 * if the ARQ failed.
	 */
	rqbp = scsi_alloc_consistent_buf(ROUTE, NULL,
		sense_length, B_READ, SLEEP_FUNC, NULL);

	rqpkt = scsi_init_pkt(ROUTE, (struct scsi_pkt *)NULL,
		rqbp, CDB_GROUP0, 1, 0, PKT_CONSISTENT, SLEEP_FUNC, NULL);

	makecom_g0(rqpkt, devp, FLAG_NOPARITY,
		SCMD_REQUEST_SENSE, 0, SENSE_LENGTH);

	rqpkt->pkt_private = (opaque_t)tp;
	rqpkt->pkt_time = RQS_TIMEOUT;
	rqpkt->pkt_comp = scdk_watch_request_intr;
	rqpkt->pkt_flags |= FLAG_HEAD;
	rqbp->av_back = (struct buf *)rqpkt;

	/*
	 * create TUR pkt
	 */
	bp = scsi_alloc_consistent_buf(ROUTE, NULL,
	    0, B_READ, SLEEP_FUNC, NULL);

	pkt = scsi_init_pkt(ROUTE, (struct scsi_pkt *)NULL, bp,
		CDB_GROUP0, sizeof (struct scsi_arq_status),
		0, 0, SLEEP_FUNC, NULL);

	makecom_g0(pkt, devp, 0, SCMD_TEST_UNIT_READY, 0, 0);

	pkt->pkt_private = (opaque_t)tp;
	pkt->pkt_time = TUR_TIMEOUT;
	pkt->pkt_comp = scdk_watch_request_intr;
	bp->av_back = (struct buf *)pkt;
	if (scsi_ifgetcap(&pkt->pkt_address, "tagged-qing", 1) == 1) {
		pkt->pkt_flags |= FLAG_STAG;
	}

	/*
	 * set the allocated resources in swr
	 */
	swr->swr_rqbp = rqbp;
	swr->swr_bp = bp;
	swr->swr_rqpkt = rqpkt;
	swr->swr_pkt = pkt;
	swr->swr_timeout = swr->swr_interval = drv_usectohz(interval);
	swr->swr_callback = callback;
	swr->swr_callback_arg = cb_arg;
	swr->swr_what = SWR_WATCH;
	swr->swr_sense_length = (u_char)sense_length;

	/*
	 * add to the list and wake up the thread
	 */
	mutex_enter(&sw.sw_mutex);
	swr->swr_next = sw.sw_head;
	swr->swr_prev = NULL;
	if (sw.sw_head) {
		sw.sw_head->swr_prev = swr;
	}
	sw.sw_head = swr;

	/*
	 * reset all timeouts, so all requests are in sync again
	 * XXX there is a small window where the watch thread releases
	 * the mutex so that could upset the resyncing
	 */
	sswr = swr;
	while (sswr) {
		sswr->swr_timeout = swr->swr_interval;
		sswr = sswr->swr_next;
	}
	cv_signal(&sw.sw_cv);
	mutex_exit(&sw.sw_mutex);
	return ((opaque_t)swr);
}


/*
 * called by (eg. pwr management) to resume the scdk_watch_thread
 */
/*ARGSUSED*/
void
scdk_watch_resume()
{
	/*
	 * Change the state to SW_RUNNING and wake up the scdk_watch_thread
	 */
	SW_DEBUG(0, sw_label, SCSI_DEBUG, "scdk_watch_resume:\n");
	mutex_enter(&sw.sw_mutex);
	sw.sw_state = SW_RUNNING;
	cv_signal(&sw.sw_cv);
	mutex_exit(&sw.sw_mutex);
}


/*
 * called by clients (eg. pwr management) to suspend the scdk_watch_thread
 */
/*ARGSUSED*/
void
scdk_watch_suspend()
{
	struct scdk_watch_request	*swr;
	u_long	now;
	u_long halfsec_delay = drv_usectohz(500000);
	u_char	outstanding_cmds = 1;

	SW_DEBUG(0, sw_label, SCSI_DEBUG, "scdk_watch_suspend:\n");

	mutex_enter(&sw.sw_mutex);
	if (sw.sw_head) {
		/*
		 * Set the state to SW_SUSPEND_REQUESTED and wait for ALL
		 * outstanding TUR requests to complete before returning.
		 */
		sw.sw_state = SW_SUSPEND_REQUESTED;

		while (outstanding_cmds) {

			SW_DEBUG(0, sw_label, SCSI_DEBUG,
				"checking for outstanding cmds\n");
			outstanding_cmds = 0;
			for (swr = sw.sw_head; swr; swr = swr->swr_next) {
				if (swr->swr_busy) {
					outstanding_cmds = 1;
					break;
				}
			}
			if (outstanding_cmds) {

				/*
				 * XXX: Assumes that this thread can rerun
				 * till all outstanding cmds are complete
				 */
				(void) drv_getparm(LBOLT, &now);
				(void) cv_timedwait(&sw.sw_cv, &sw.sw_mutex,
					now + halfsec_delay);
			}
		}

		/*
		 * There are no outstanding cmds now - so set state to
		 * SW_SUSPENDED
		 */
	}

	sw.sw_state = SW_SUSPENDED;
	mutex_exit(&sw.sw_mutex);

}

/*
 * destroy swr, called for watch thread
 */
static void
scdk_watch_request_destroy(
	struct scdk_watch_request	*swr)
{
	ASSERT(MUTEX_HELD(&sw.sw_mutex));
	ASSERT(swr->swr_busy == 0);

	SW_DEBUG((dev_info_t *)NULL, sw_label, SCSI_DEBUG,
		"scdk_watch_request_destroy: Entering ...\n");

	/*
	 * remove swr from linked list and destroy pkts
	 */
	if (swr->swr_prev) {
		swr->swr_prev->swr_next = swr->swr_next;
	}
	if (swr->swr_next) {
		swr->swr_next->swr_prev = swr->swr_prev;
	}
	if (sw.sw_head == swr) {
		sw.sw_head = swr->swr_next;
	}

	/* x86: free target_private structure */
	kmem_free((caddr_t)swr->swr_rqpkt->pkt_private,
	    sizeof (struct target_private));
	scsi_destroy_pkt(swr->swr_rqpkt);
	scsi_free_consistent_buf(swr->swr_rqbp);
	scsi_destroy_pkt(swr->swr_pkt);
	scsi_free_consistent_buf(swr->swr_bp);
	kmem_free((caddr_t)swr, sizeof (struct scdk_watch_request));
}

/*
 * scdk_watch_request_terminate()
 * called by requestor to indicate to the watch thread to
 * destroy this request
 */
void
scdk_watch_request_terminate(
	opaque_t	token)
{
	register struct scdk_watch_request	*swr =
		(struct scdk_watch_request *)token;
	register struct scdk_watch_request	*sswr;

	/*
	 * indicate to watch thread to clean up this swr
	 * We don't do it here to avoid screwing up the linked list
	 * while the watch thread is walking thru it.
	 */
	SW_DEBUG((dev_info_t *)NULL, sw_label, SCSI_DEBUG,
		"scdk_watch_request_terminate: Entering(%x) ...\n", swr);
	mutex_enter(&sw.sw_mutex);

	/*
	 * check if it is still in the list
	 */
	sswr = sw.sw_head;
	while (sswr) {
		if (sswr == swr) {
			swr->swr_what = SWR_STOP;
			break;
		}
		sswr = sswr->swr_next;
	}
	mutex_exit(&sw.sw_mutex);
}

/*
 * the scsi watch thread:
 * it either wakes up if there is work to do or if the cv_timeait
 * timed out
 * normally, it wakes up every <delay> seconds and checks the list.
 * the interval is not very accurate if the cv was signalled but that
 * really doesn't matter much
 * it is more important that we fire off all TURs simulataneously so
 * we don't have to wake up frequently
 */
static void
scdk_watch_thread()
{
	struct scdk_watch_request	*swr, *next;
	u_long				now;
	long				last_delay = 0;
	long				next_delay = 0;
	long				onesec = drv_usectohz(1000000);
	long				exit_delay = 60 * onesec;
	struct scdk 			*scdkp = NULL;

	SW_DEBUG((dev_info_t *)NULL, sw_label, SCSI_DEBUG,
		"scdk_watch_thread: Entering ...\n");

	/*
	 * grab the mutex and wait for work
	 */
	mutex_enter(&sw.sw_mutex);
	if (sw.sw_head == NULL) {
		cv_wait(&sw.sw_cv, &sw.sw_mutex);
	}

	/*
	 * now loop forever for work; if queue is empty exit
	 */
	for (;;) {
		swr = sw.sw_head;
		while (swr) {

			/*
			 * If state is not running, wait for scdk_watch_resume
			 * to signal restart
			 */
			if (sw.sw_state != SW_RUNNING) {
				SW_DEBUG(0, sw_label, SCSI_DEBUG,
					"scdk_watch_thread suspended\n");
				cv_wait(&sw.sw_cv, &sw.sw_mutex);
			}

			if (next_delay == 0) {
				next_delay = swr->swr_timeout;
			} else {
				next_delay = min(swr->swr_timeout, next_delay);
			}

			swr->swr_timeout -= last_delay;
			next = swr->swr_next;

			SW_DEBUG((dev_info_t *)NULL, sw_label, SCSI_DEBUG,
				"scdk_watch_thread: "
				"swr(%x),what=%x,timeout=%x,"
				"interval=%x,delay=%x\n",
				swr, swr->swr_what, swr->swr_timeout,
				swr->swr_interval, last_delay);

			switch (swr->swr_what) {
			case SWR_STOP:
				if (swr->swr_busy == 0) {
					scdk_watch_request_destroy(swr);
				}
				break;

			default:
				if (swr->swr_timeout <= 0 &&
					!swr->swr_busy) {
					swr->swr_busy = 1;

					/*
					 * submit the cmd and let the completion
					 * function handle the result
					 * release the mutex (good practice)
					 * this should be safe even if the list
					 * is changing
					 */
					mutex_exit(&sw.sw_mutex);
					SW_DEBUG((dev_info_t *)NULL,
						sw_label, SCSI_DEBUG,
						"scdk_watch_thread: "
						"Starting TUR\n");
					swr->swr_bp->b_flags = B_READ;
					scdkp = (struct scdk *)
						swr->swr_callback_arg;
					if (scdkp)
						FLC_ENQUE(scdkp->scd_flcobjp,
						    swr->swr_bp);
					else {
						/*
						 * try again later
						 */
						swr->swr_busy = 0;
						SW_DEBUG((dev_info_t *)NULL,
							sw_label, SCSI_DEBUG,
							"scdk_watch_thread: "
							"Transport Failed\n");
					}
					mutex_enter(&sw.sw_mutex);
					swr->swr_timeout = swr->swr_interval;
				}
				break;
			}
			swr = next;
		}

		/*
		 * delay using cv_timedwait; we return when
		 * signalled or timed out
		 */
		if (sw.sw_head != NULL) {
			if (next_delay <= 0) {
				next_delay = onesec;
			}
		} else {
			next_delay = exit_delay;
		}
		(void) drv_getparm(LBOLT, &now);

		/*
		 * if we return from cv_timedwait because we were
		 * signalled, the delay is not accurate but that doesn't
		 * really matter
		 */
		(void) cv_timedwait(&sw.sw_cv, &sw.sw_mutex, now + next_delay);
		last_delay = next_delay;
		next_delay = 0;

		/*
		 * is there still work to do?
		 */
		if (sw.sw_head == NULL) {
			break;
		}
	}

	/*
	 * no more work to do, reset sw_thread and exit
	 */
	sw.sw_thread = 0;
	mutex_exit(&sw.sw_mutex);
	SW_DEBUG((dev_info_t *)NULL, sw_label, SCSI_DEBUG,
		"scdk_watch_thread: Exiting ...\n");
}

/*
 * callback completion function for scsi watch pkt
 */
#define	SCBP(pkt)	((struct scsi_status *)(pkt)->pkt_scbp)
#define	SCBP_C(pkt)	((*(pkt)->pkt_scbp) & STATUS_MASK)

static void
scdk_watch_request_intr(
	struct scsi_pkt	*pkt)
{
	struct scdk_watch_result	result;
	struct scdk_watch_request	*swr = (struct scdk_watch_request *)
			(((struct target_private *)pkt->pkt_private)->x_bp);
	struct scsi_status		*rqstatusp;
	struct scsi_extended_sense	*rqsensep = NULL;
	int				amt = 0;
	struct scdk 			*scdkp = NULL;

	SW_DEBUG((dev_info_t *)NULL, sw_label, SCSI_DEBUG,
		"scdk_watch_intr: Entering ...\n");

	/*
	 * first check if it is the TUR or RQS pkt
	 */
	if (pkt == swr->swr_pkt) {
		scdkp = (struct scdk *)swr->swr_callback_arg;
		if (scdkp)
			FLC_DEQUE(scdkp->scd_flcobjp, swr->swr_bp);
		if (pkt->pkt_reason == CMD_CMPLT &&
			SCBP_C(pkt) != STATUS_GOOD &&
			SCBP_C(pkt) != STATUS_RESERVATION_CONFLICT) {
			if (SCBP(pkt)->sts_chk &&
				((pkt->pkt_state & STATE_ARQ_DONE) == 0)) {

				/*
				 * submit the request sense pkt
				 */
				SW_DEBUG((dev_info_t *)NULL,
					sw_label, SCSI_DEBUG,
					"scdk_watch_intr: "
					"Submitting a Request Sense "
					"Packet\n");
				swr->swr_rqbp->b_flags = B_READ;
				if (scdkp)
					FLC_ENQUE(scdkp->scd_flcobjp,
					    swr->swr_rqbp);
				else {
					/*
					 * just give up and try again later
					 */
					SW_DEBUG((dev_info_t *)NULL,
						sw_label, SCSI_DEBUG,
						"scdk_watch_intr: "
						"Request Sense "
						"Transport Failed\n");
					goto done;
				}

				/*
				 * wait for rqsense to complete
				 */
				return;

			} else if (SCBP(pkt)->sts_chk) {

				/*
				 * check the autorequest sense data
				 */
				struct scsi_arq_status	*arqstat =
				    (struct scsi_arq_status *)pkt->pkt_scbp;

				rqstatusp = &arqstat->sts_rqpkt_status;
				rqsensep = &arqstat->sts_sensedata;
				amt = swr->swr_sense_length -
					arqstat->sts_rqpkt_resid;
				SW_DEBUG((dev_info_t *)NULL,
					sw_label, SCSI_DEBUG,
					"scdk_watch_intr: "
					"Auto Request Sense, amt=%x\n", amt);
			}
		}

	} else if (pkt == swr->swr_rqpkt) {

		/*
		 * check the request sense data
		 */
		scdkp = (struct scdk *)swr->swr_callback_arg;
		if (scdkp)
			FLC_DEQUE(scdkp->scd_flcobjp, swr->swr_rqbp);
		rqstatusp = (struct scsi_status *)pkt->pkt_scbp;
		rqsensep = (struct scsi_extended_sense *)
			swr->swr_rqbp->b_un.b_addr;
		amt = swr->swr_sense_length - pkt->pkt_resid;
		SW_DEBUG((dev_info_t *)NULL, sw_label, SCSI_DEBUG,
		"scdk_watch_intr: "
		"Request Sense Completed, amt=%x\n", amt);
	} else {

		/*
		 * should not reach here!!!
		 */
		scsi_log((dev_info_t *)NULL, sw_label, CE_PANIC,
			"scdk_watch_intr: Bad Packet(%x)", pkt);
	}

	if (rqsensep) {

		/*
		 * check rqsense status and data
		 */
		if (rqstatusp->sts_busy || rqstatusp->sts_chk) {

			/*
			 * try again later
			 */
			SW_DEBUG((dev_info_t *)NULL, sw_label, SCSI_DEBUG,
				"scdk_watch_intr: "
				"Auto Request Sense Failed - "
				"Busy or Check Condition\n");
			goto done;
		}

		SW_DEBUG((dev_info_t *)NULL, sw_label, SCSI_DEBUG,
			"scdk_watch_intr: "
			"es_key=%x, adq=%x, amt=%x\n",
			rqsensep->es_key, rqsensep->es_add_code, amt);
	}

	/*
	 * callback to target driver to do the real work
	 */
	result.statusp = SCBP(swr->swr_pkt);
	result.sensep = rqsensep;
	result.actual_sense_length = (u_char) amt;
	result.pkt = swr->swr_pkt;

	if ((*swr->swr_callback)(swr->swr_callback_arg, &result)) {
		swr->swr_what = SWR_STOP;
	}

done:
	swr->swr_busy = 0;
}
