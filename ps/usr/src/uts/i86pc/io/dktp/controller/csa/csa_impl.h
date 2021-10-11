/*
 * Copyright (c) 1995, Sun Microsystems, Inc.
 * All Rights Reserved.
 */

#ifndef _CSA_IMPL_H
#define	_CSA_IMPL_H

#pragma	ident	"@(#)csa_impl.h	1.3	95/06/05 SMI"

#ifdef	__cplusplus
extern "C" {
#endif

#include <sys/types.h>

#include "ccb.h"

/*
 * misc defines
 */
#define	CSA_MAX_LDRIVES	8	/* hardware limitation */

#define	CSA_MAX_SG	17	/* Max scatter/gather descriptors per command.
				 * The hardware supports upto 255. This is
				 * purely arbitrary, but I think we should
				 * allow at least 17 descriptors so that
				 * we can always do at least 64k in a
				 * single command.
				 */

/*
 * just to be safe don't use tag ID 0 and tag ID 255
 */
#define	CSA_MAX_CMDS	254	/* max commands per controller */

#define	CSA_KVTOP(vaddr)						\
( (paddr_t)(((ulong)(hat_getkpfnum((caddr_t)(vaddr)) << MMU_PAGESHIFT)) \
	|   ((ulong)(vaddr) & MMU_PAGEOFFSET)) )


/* ******************************************************************** */
/*  DATA STRUCTURES							*/
/* ******************************************************************** */

/*
 * Command List Header
 */
typedef struct command_list_header {
	unchar	ldrive;		/* 0-255 logical drive number */
	unchar	priority;	/* 0-127 prioritize disk requests */
	ushort	control;	/* command list header control flags */
} chdr_t;

/*
 * command list header control flags
 *   bit  state  function
 *  ---------------------------------
 *     0    0 =  notify host on completion of list
 *          1 =  notify host on completion of every request
 *     1    0 =  error notification on completion of list
 *          1 =  error notification on completion of request
 *     2    0 =  no abort on error
 *          1 =  abort on error
 *     3    0 =  order requests
 *          1 =  do not reorder requests
 *  15-4    0    reserved
 */
#define	CTRL_REQNOTIFY	0x0001
#define	CTRL_ERRREQUEST	0x0002
#define	CTRL_ERRABORT	0x0004
#define	CTRL_NOREORDER	0x0008


/*
 * Request Block
 */
typedef struct request_block_header {
	ushort	rh_next;	/* offset to next request block */
	unchar	rh_cmd;		/* request function command */
	unchar	rh_status;	/* return code error status */
	ulong	rh_blk_num;	/* block number on logical disk */
	ushort	rh_blk_cnt;	/* block count on logical disk */
	unchar	rh_cnt1;	/* scatter/gather descriptor count #1 */
	unchar	rh_cnt2;	/* scatter/gather descriptor count #2 */
} rhdr_t;

/*
 * Request Block status codes
 */
#define	RB_LIST_DONE	0x01	/* status reg: list done */
#define	RB_RECOV_ERR	0x02	/* status reg: non-fatal error */
#define	RB_FATAL_ERR	0x04	/* status reg: fatal error */
#define	RB_ABORT_ERR	0x08	/* status reg: aborted */
#define	RB_REQ_ERR	0x10	/* invalid request blk */
#define	RB_CMDLIST_ERR	0x20	/* status reg: invalid cmd list */
#define	RB_ANY_ERR	0x3E	/* any error(aborted,fatal,nonfatal) */
#define	RB_ANY_REQ_ERR	0x1E	/* any request error */

typedef	struct scatter_gather {
	ulong	sg_size;	/* buffer size */
	ulong	sg_addr;	/* buffer physical address */
} sg_t;

typedef struct request_block {
	rhdr_t	rb_hdr;			/* request block header */
	sg_t	rb_sg[CSA_MAX_SG];	/* scatter/gather descriptor array */
} rblk_t;



/*
 * Compaq Drive Array Controller Command List
 */
typedef struct cmd_list {
	chdr_t		 cl_hdr;	/* command list header */
	rblk_t		 cl_req;	/* request block */
	struct csa_ccb	*cl_csa_ccbp;	/* ptr to ccb for this command */
	ushort		 pad;
} cmd_t;



/*
 * the csa driver's Command Control Block
 */
typedef struct csa_ccb {
	struct cmpkt	csa_ccb_cmpkt;	/* the dadk common packet */
	ccb_t		csa_ccb_common;	/* the "common" portion of the ccb */
	cmd_t		*csa_ccb_clp;	/* ptr to command list for this ccb */
	paddr_t		csa_ccb_paddr;	/* phys addr of *csa_ccb_clp */
	unchar		csa_ccb_tagid;	/* unique tag id for this ccb */
	char		csa_ccb_status;	/* driver's ccb queue status flag */
	short		pad;
} csa_ccb_t;

/* ccb queue status */
#define	CCB_CFREE	'F'	/* ccb buffer is free */
#define	CCB_CBUSY	'B'	/* ccb buffer is busy */
#define	CCB_CDONE	'-'	/* ccb buffer is done */
#define	CCB_CQUEUED	'W'	/* ccb buffer on wait queue */
#define	CCB_CSENT	'S'	/* ccb buffer sent to controller */


#ifdef CSA_DEBUG
#define	CSA_SET_STATE(cbp, ccbp, state)				\
	(((cbp)->cb_status[(ccbp)->csa_ccb_tagid] = (state)),	\
	 ((ccbp)->csa_ccb_status = (state)))
#else
#define	CSA_SET_STATE(cbp, ccbp, state)				\
	 ((ccbp)->csa_ccb_status = (state))
#endif

#define	CSA_CCB_ST_FREE(cbp, ccbp)	CSA_SET_STATE(cbp, ccbp, CCB_CFREE)
#define	CSA_CCB_ST_BUSY(cbp, ccbp)	CSA_SET_STATE(cbp, ccbp, CCB_CBUSY)
#define	CSA_CCB_ST_DONE(cbp, ccbp)	CSA_SET_STATE(cbp, ccbp, CCB_CDONE)
#define	CSA_CCB_ST_QUEUED(cbp, ccbp)	CSA_SET_STATE(cbp, ccbp, CCB_CQUEUED)
#define	CSA_CCB_ST_SENT(cbp, ccbp)	CSA_SET_STATE(cbp, ccbp, CCB_CSENT)


/* ********************************************************************** */

/*
 * call-able functions
 */

Bool_t		 csa_bmic_mode(ushort ioaddr);
struct ccb	*csa_ccballoc(opaque_t arg);
void		 csa_ccbfree(opaque_t arg, struct ccb *ccbp);
Bool_t		 csa_ccbinit(struct csa_blk *csa_blkp);
void		 csa_dev_init(struct csa_blk *csa_blkp, ushort ioaddr);
void		 csa_dev_fini(struct csa_blk *csa_blkp, ushort ioaddr);
Bool_t		 csa_get_irq(ushort ioaddr, unchar *irqp);
Bool_t		 csa_get_ldgeom(csa_t *csap, struct tgdk_geom *tg);
int		 csa_inquiry(dev_info_t *mdip, dev_info_t *cdip,
			struct csa *csap);
void		 csa_process_intr(opaque_t arg1, opaque_t arg2);
int		 csa_intr_status(opaque_t arg1, opaque_t *arg2);
int		 csa_send_readwrite(struct csa *csap, struct cmpkt *pktp,
			struct csa_blk *csa_blkp, struct ccb *ccbp);
void		 csa_sg_func(struct cmpkt *pktp, struct ccb *ccbp,
			ddi_dma_cookie_t *cookiep, int segno, opaque_t arg);
Bool_t		 csa_pollret(csa_blk_t *csa_blkp, ccb_t *ccbp);
int		 csa_flush_cache(dev_info_t *dip, ddi_reset_cmd_t cmd);

/* ********************************************************************** */

/*
 * Interrupt status for Command List Complete Channel
 */
struct intr_info {
	paddr_t	 cl_paddr;
	ushort	 offset;
	unchar	 status;
	unchar	 tagid;
};

#ifdef	__cplusplus
}
#endif

#endif /* _CSA_IMPL_H */
