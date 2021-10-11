/*
 * Copyright (c) 1996, Sun Microsystems, Inc.
 * All Rights Reserved.
 */

#ifndef _DPTGHD_H
#define	_DPTGHD_H

#pragma	ident	"@(#)dptghd.h	1.1	96/06/13 SMI"

#ifdef	__cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include <sys/conf.h>
#include <sys/kmem.h>
#include <sys/ddi.h>
#include <sys/sunddi.h>
#include <sys/debug.h>
#include <sys/scsi/scsi.h>

#include "dptghd_queue.h"		/* queue and list structures */
#include "dptghd_scsi.h"
#include "dptghd_debug.h"

#ifndef	TRUE
#define	TRUE	1
#endif

#ifndef	FALSE
#define FALSE	0
#endif

/*
 * values for cmd_state:
 */

typedef enum {
	GCMD_STATE_IDLE = 0,
	GCMD_STATE_WAITQ,
	GCMD_STATE_ACTIVE,
	GCMD_STATE_DONEQ,
	GCMD_STATE_ABORTING_CMD,
	GCMD_STATE_ABORTING_DEV,
	GCMD_STATE_RESETTING_DEV,
	GCMD_STATE_RESETTING_BUS,
	GCMD_STATE_HUNG,
	GCMD_NSTATES
} cmdstate_t;

/*
 * action codes for the HBA timeout function
 */

typedef enum {
	GACTION_EARLY_TIMEOUT = 0,	/* timed-out before started */ 
	GACTION_EARLY_ABORT,		/* scsi_abort() before started */ 
	GACTION_ABORT_CMD,		/* abort a specific request */
	GACTION_ABORT_DEV,		/* abort everything on specifici dev */
	GACTION_RESET_TARGET,		/* reset a specific dev */
	GACTION_RESET_BUS,		/* reset the whole bus */
	GACTION_INCOMPLETE		/* giving up on incomplete request */
} gact_t;


/*
 * the common portion of the Command Control Block
 */

typedef struct ghd_cmd {
	Qel_t		 cmd_q;		/* queue for done or active CCBs*/
	cmdstate_t	 cmd_state;	/* request's current state */

	L2el_t		 cmd_timer_link; /* ccb timer doubly linked list */
	ulong		 cmd_start_time;/* lbolt at start of request */
	ulong		 cmd_timeout;	/* how long to wait */

	opaque_t	*cmd_private;	/* used by the HBA driver */
	void		*cmd_pktp;	/* request packet */
	struct scsi_address *cmd_ap;	/* dev address for this request */

	ddi_dma_handle_t cmd_dma_handle;
	ddi_dma_win_t	 cmd_dmawin;
	ddi_dma_seg_t	 cmd_dmaseg;
	int		 cmd_dma_flags;
	long		 cmd_totxfer;
} gcmd_t;


/*
 * CMD/CCB timer config structure - one per HBA driver module
 */
typedef struct tmr_conf {
	kmutex_t	t_mutex;	/* mutex to protect t_ccc_listp */
	int		t_timeout_id;	/* handle for timeout() function */
	long		t_ticks;	/* periodic timeout in clock ticks */
	int		t_refs;		/* reference count */
	struct cmd_ctl	*t_ccc_listp;	/* control struct list, one per HBA */
} tmr_t;



/*
 * CMD/CCB timer control structure - one per HBA instance (per board)
 */
typedef struct cmd_ctl {
	struct cmd_ctl	*ccc_nextp;	/* list of control structs */
	struct tmr_conf	*ccc_tmrp;	/* back ptr to config struct */

	kmutex_t ccc_activel_mutex;	/* mutex to protect list */
	L2el_t	 ccc_activel;		/* list of active CMD/CCBs */

	dev_info_t *ccc_hba_dip;
	ddi_iblock_cookie_t ccc_iblock;
	ddi_softintr_t  ccc_soft_id;	/* ID for timeout softintr */

	kmutex_t ccc_hba_mutex;		/* mutex for HBA soft-state */
	int	 ccc_hba_pollmode;	/* FLAG_NOINTR mode active? */

	kmutex_t ccc_waitq_mutex;
	Que_t	 ccc_waitq;		/* cmd_t's waiting to be started */

	kmutex_t ccc_doneq_mutex;
	Que_t	 ccc_doneq; 		/* completed cmd_t's */

	void	*ccc_hba_handle;
	gcmd_t	*(*ccc_ccballoc)();	/* alloc/init gcmd and ccb */
	void	(*ccc_ccbfree)();
	void	(*ccc_sg_func)();
	int	(*ccc_hba_start)(void *handle, gcmd_t *);
	void	(*ccc_process_intr)(void *handle, void *intr_status);
	int	(*ccc_get_status)(void *handle, void *intr_status);
	void	(*ccc_timeout_func)(void *handle, gcmd_t *cmdp, 
				    struct scsi_address *ap, gact_t action);
} ccc_t;


/* ******************************************************************* */

#include "dptghd_scsa.h"

/*
 * GHD Entry Points
 */
void	 dptghd_complete(ccc_t *cccp, gcmd_t *cmdp);
void	 dptghd_doneq_put(ccc_t *cccp, gcmd_t *cmdp);
gcmd_t	*dptghd_doneq_tryget(ccc_t *cccp);

int	 dptghd_intr(ccc_t *cccp, void *status);
int	 dptghd_register(ccc_t *, dev_info_t *, int, char *name,
			void *hba_handle,
			gcmd_t *(*ccc_ccballoc)(struct scsi_address *,
				struct scsi_pkt *, void *, int,
				int, int, int),
			void (*ccc_ccbfree)(struct scsi_address *,
				struct scsi_pkt *),
			void (*ccc_sg_func)(struct scsi_pkt *, gcmd_t *,
				ddi_dma_cookie_t *, int, int, void *),
			int   (*hba_start)(void *, gcmd_t *),
			u_int (*int_handler)(caddr_t),
			int   (*get_status)(void *, void *),
			void  (*process_intr)(void *, void *),
			void  (*timeout_func)(void *, gcmd_t *,
				struct scsi_address *ap, gact_t),
			tmr_t *tmrp);
void	 dptghd_unregister(ccc_t *cccp);

int	 dptghd_transport(ccc_t *cccp, gcmd_t *cmdp, struct scsi_address *ap,
			ulong timeout, int polled, void *intr_status);

int	 dptghd_tran_abort(ccc_t *cccp, gcmd_t *cmdp, struct scsi_address *ap,
				void *intr_status);
int	 dptghd_tran_abort_lun(ccc_t *cccp, struct scsi_address *ap,
				void *intr_status);
int	 dptghd_tran_reset_target(ccc_t *cccp, struct scsi_address *ap,
				void *intr_status);
int	 dptghd_tran_reset_bus(ccc_t *cccp, void *intr_status);

void	 dptghd_waitq_delete(ccc_t *cccp, gcmd_t *cmdp);
gcmd_t	*dptghd_waitq_get(ccc_t *cccp);
void	 dptghd_waitq_put(ccc_t *cccp, gcmd_t *cmdp);



/*
 * GHD CMD/CCB timer Entry points
 */

int	dptghd_timer_attach(ccc_t *cccp, tmr_t *tmrp,
	  void (*timeout_func)(void *handle, gcmd_t *, struct scsi_address *,
				gact_t));
void	dptghd_timer_detach(ccc_t *cccp);
void	dptghd_timer_fini(tmr_t *tmrp);
void	dptghd_timer_init(tmr_t *tmrp, char *name, long ticks);
cmdstate_t dptghd_timer_newstate(ccc_t *cccp, gcmd_t *cmdp,
				struct scsi_address *ap, gact_t action);
void	dptghd_timer_poll(ccc_t *cccp);
void	dptghd_timer_start(ccc_t *cccp, gcmd_t *cmdp, cmdstate_t next_state,
			long cmd_timeout);
void	dptghd_timer_stop(ccc_t *cccp , gcmd_t *cmdp);


/* ******************************************************************* */

#define GHD_INLINE	1

#if defined(GHD_INLINE)
#define	DPTGHD_DONEQ_PROCESS(cccp) DPTGHD_DONEQ_PROCESS_INLINE(cccp)
#else
#define	DPTGHD_DONEQ_PROCESS(cccp) dptghd_doneq_process(cccp)
#endif

#define	DPTGHD_DONEQ_PROCESS_INLINE(cccp)				\
	{								\
		struct scsi_pkt	*pktp;					\
		gcmd_t		*cmdp;					\
									\
		while (cmdp = dptghd_doneq_tryget(cccp)) 	{	\
			cmdp->cmd_state = GCMD_STATE_IDLE;		\
			pktp = GCMDP2PKTP(cmdp);			\
			ASSERT(pktp != NULL);				\
			ASSERT(pktp->pkt_comp != NULL);			\
			ASSERT(!(pktp->pkt_flags & FLAG_NOINTR));	\
			(*pktp->pkt_comp)(pktp);			\
		}							\
	}


/* ******************************************************************* */


/*
 * These are shortcut macros for linkages setup by GHD
 */
#define	GCMDP2PKTP(gcmdp)	(gcmdp)->cmd_pktp
#define PKTP2GCMDP(pktp)	((gcmd_t *)(pktp)->pkt_ha_private)


/* These are shortcut macros for linkages setup by SCSA */

/*
 * (scsi_address *) to (scsi_hba_tran *)
 */
#define	ADDR2TRAN(ap)	(ap)->a_hba_tran

/*
 * (scsi_device *) to (scsi_address *)
 */
#define	SDEV2ADDR(sdp) &(sdp)->sd_address

/*
 * (scsi_device *) to (scsi_hba_tran *)
 */
#define	SDEV2TRAN(sdp) ADDR2TRAN(SDEV2ADDR(sdp))

/*
 * (scsi_pkt *) to (scsi_hba_tran *)
 */
#define	PKT2TRAN(pktp)	ADDR2TRAN(&(pktp)->pkt_address)

/*
 * (scsi_hba_tran) to (per-target-soft-state *)
 */
#define	TRAN2TARGET(tranp)	(tranp)->tran_tgt_private

/*
 * (scsi_device *) to (per-target-soft-state *)
 */
#define	SDEV2TARGET(sd)		(TRAN2TARGET(SDEV2TRAN(sd)))

/*
 * (scsi_pkt *) to (per-target-soft-state *)
 */
#define	PKT2TARGET(pktp)	(TRAN2TARGET(PKT2TRAN(pktp)))


/*
 * (scsi_hba_tran *) to (per-HBA-soft-state *)
 */
#define	TRAN2HBA(tranp)		(tranp)->tran_hba_private


/*
 * (scsi_device *) to (per-HBA-soft-state *)
 */
#define	SDEV2HBA(sd)		TRAN2HBA(SDEV2TRAN(sd))


/* ******************************************************************* */


#ifdef	__cplusplus
}
#endif

#endif  /* _DPTGHD_H */
