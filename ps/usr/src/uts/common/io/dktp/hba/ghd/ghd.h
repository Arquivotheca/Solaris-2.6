/*
 * Copyright (c) 1996, Sun Microsystems, Inc.
 * All Rights Reserved.
 */

#ifndef _GHD_H
#define	_GHD_H

#pragma	ident	"@(#)ghd.h	1.1	96/07/30 SMI"

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

#include "ghd_queue.h"		/* queue and list structures */
#include "ghd_scsi.h"
#include "ghd_debug.h"

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
	Qel_t		 cmd_q;		/* link for for done/active CCB Qs */
	cmdstate_t	 cmd_state;	/* request's current state */

	L2el_t		 cmd_timer_link;/* ccb timer doubly linked list */
	ulong		 cmd_start_time;/* lbolt at start of request */
	ulong		 cmd_timeout;	/* how long to wait */

	opaque_t	 cmd_private;	/* used by the HBA driver */
	void		*cmd_pktp;	/* request packet */
	void		*cmd_tgtp;	/* dev address for this request */

	ddi_dma_handle_t cmd_dma_handle;
	ddi_dma_win_t	 cmd_dmawin;
	ddi_dma_seg_t	 cmd_dmaseg;
	int		 cmd_dma_flags;
	long		 cmd_totxfer;
	long		 cmd_resid;
} gcmd_t;


/*
 * Initialize the gcmd_t structure
 */

#define	GHD_GCMD_INIT(gcmdp, cmdp, tgtp)	\
(	QEL_INIT(&(gcmdp)->cmd_q),		\
	L2_INIT(&(gcmdp)->cmd_timer_link),	\
	(gcmdp)->cmd_private = (cmdp),		\
	(gcmdp)->cmd_tgtp = (tgtp)		\
)


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
	char		*ccc_label;	/* name of this HBA driver */

	kmutex_t ccc_activel_mutex;	/* mutex to protect list ... */
	L2el_t	 ccc_activel;		/* ... list of active CMD/CCBs */

	dev_info_t *ccc_hba_dip;
	ddi_iblock_cookie_t ccc_iblock;
	ddi_softintr_t  ccc_soft_id;	/* ID for timeout softintr */

	kmutex_t ccc_hba_mutex;		/* mutex for HBA soft-state */
	int	 ccc_hba_pollmode;	/* FLAG_NOINTR mode active? */

	kmutex_t ccc_waitq_mutex;	/* mutex to protect Q ... */
	Que_t	 ccc_waitq;		/* ... cmd_t's waiting to be started */

	kmutex_t ccc_doneq_mutex;
	Que_t	 ccc_doneq; 		/* completed cmd_t's */

	void	*ccc_hba_handle;
	gcmd_t	*(*ccc_ccballoc)();	/* alloc/init gcmd and ccb */
	void	(*ccc_ccbfree)();
	void	(*ccc_sg_func)();
	int	(*ccc_hba_start)(void *handle, gcmd_t *);
	void	(*ccc_process_intr)(void *handle, void *intr_status);
	int	(*ccc_get_status)(void *handle, void *intr_status);
	int	(*ccc_timeout_func)(void *handle, gcmd_t *cmdp, void *tgtp,
				    gact_t action);
} ccc_t;


/* ******************************************************************* */

#include "ghd_scsa.h"
#include "ghd_dma.h"

/*
 * GHD Entry Points
 */
void	 ghd_complete(ccc_t *cccp, gcmd_t *cmdp);
void	 ghd_doneq_process(ccc_t *cccp);
void	 ghd_doneq_put(ccc_t *cccp, gcmd_t *cmdp);
gcmd_t	*ghd_doneq_tryget(ccc_t *cccp);

int	 ghd_intr(ccc_t *cccp, void *status);
int	 ghd_register(char *, ccc_t *, dev_info_t *, int, void *hba_handle,
			gcmd_t *(*ccc_ccballoc)(void *, void *, int, int,
						int, int),
			void (*ccc_ccbfree)(void *),
			void (*ccc_sg_func)(gcmd_t *, ddi_dma_cookie_t *,
					    int, int),
			int  (*hba_start)(void *, gcmd_t *),
			u_int (*int_handler)(caddr_t),
			int  (*get_status)(void *, void *),
			void (*process_intr)(void *, void *),
			int  (*timeout_func)(void *, gcmd_t *, void *, gact_t),
			tmr_t *tmrp);
void	 ghd_unregister(ccc_t *cccp);

int	 ghd_transport(ccc_t *cccp, gcmd_t *cmdp, void *tgtp, ulong timeout,
			int polled, void *intr_status);

int	 ghd_tran_abort(ccc_t *cccp, gcmd_t *cmdp, void *tgtp,
				void *intr_status);
int	 ghd_tran_abort_lun(ccc_t *cccp, void *tgtp, void *intr_status);
int	 ghd_tran_reset_target(ccc_t *cccp, void *tgtp, void *intr_status);
int	 ghd_tran_reset_bus(ccc_t *cccp, void *tgtp, void *intr_status);

void	 ghd_waitq_delete(ccc_t *cccp, gcmd_t *cmdp);
gcmd_t	*ghd_waitq_get(ccc_t *cccp);
void	 ghd_waitq_put(ccc_t *cccp, gcmd_t *cmdp);



/*
 * GHD CMD/CCB timer Entry points
 */

int	ghd_timer_attach(ccc_t *cccp, tmr_t *tmrp,
	  int (*timeout_func)(void *handle, gcmd_t *, void *, gact_t));
void	ghd_timer_detach(ccc_t *cccp);
void	ghd_timer_fini(tmr_t *tmrp);
void	ghd_timer_init(tmr_t *tmrp, char *name, long ticks);
void	ghd_timer_newstate(ccc_t *cccp, gcmd_t *cmdp, void *tgtp,
			   gact_t action);
void	ghd_timer_poll(ccc_t *cccp);
void	ghd_timer_start(ccc_t *cccp, gcmd_t *cmdp, long cmd_timeout);
void	ghd_timer_stop(ccc_t *cccp , gcmd_t *cmdp);


/* ******************************************************************* */

#define GHD_INLINE	1

#if defined(GHD_INLINE)
#define	GHD_DONEQ_PROCESS(cccp) GHD_DONEQ_PROCESS_INLINE(cccp)
#else
#define	GHD_DONEQ_PROCESS(cccp) ghd_doneq_process(cccp)
#endif

#define	GHD_DONEQ_PROCESS_INLINE(cccp)				\
	{								\
		struct scsi_pkt	*pktp;					\
		gcmd_t		*cmdp;					\
									\
		while (cmdp = ghd_doneq_tryget(cccp)) 	{	\
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
#define	GCMDP2TGTP(gcmdp)	(gcmdp)->cmd_tgtp
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
#define	PKTP2TRAN(pktp)	ADDR2TRAN(&(pktp)->pkt_address)

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
#define	PKTP2TARGET(pktp)	(TRAN2TARGET(PKTP2TRAN(pktp)))


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

#endif  /* _GHD_H */
