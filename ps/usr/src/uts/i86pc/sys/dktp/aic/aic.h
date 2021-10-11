/*
 * Copyright (c) 1993-94 Sun Microsystems, Inc.
 */

#pragma ident   "@(#)aic.h 1.4     94/09/08 SMI"

/*
 *	Version 1.0 R1 dated Feb 4, 1994
 *	Version 1.0 R2 dated Feb 16,1994
 *		Removed the pkt timeout id from the per tgt info structure. 
 *	Version 1.1 dated Apr 9, 1994
 *		Added dma_lim structure for dma operations in aic structure.
 *	Also added a flag to indicate the compeltion of NOINTR pakcets in the
 *  aic strcture, required per controller.
 */

/*
 * This file is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify this file without charge, but are not authorized to
 * license or distribute it to anyone else except as part of a product
 * or program developed by the user.
 * 
 * THIS FILE IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 * 
 * This file is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 * 
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY THIS FILE
 * OR ANY PART THEREOF.
 * 
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even
 * if Sun has been advised of the possibility of such damages.
 * 
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 * 
 * @(#)aic.h 1.0 Feb 4, 1994 Copyright 1993 Sun Microsystems.
 */
#ifndef lint
static char sccsid_h[] = "@(#)aic.h 1.0 Feb 16, 1994 Copyright 1993 Sun Microsystems.";
#endif

#ifndef AIC_H
#define AIC_H

#ifdef	__cplusplus
extern "C" {
#endif

/**
#include <sys/sunddi.h>
#include <sys/scsi/impl/transport.h>
#include "aic6x60.h"
#include "scsi6x60.h"
**/
#include "scb6x60.h"

#define NTGTS	8		/* Maximum number of targets */
#define NLUNS	8		/* Maximum number of LUNs per target */
#define NUM_CTRLRS			2	/* No. of controllers supported. */
#define NUM_AIC6360_REGS	32	/* No. of registers in AIC6360 */
#define NOINTR_TIMEOUT	0x77

/* Definitions for setting in Completion routine */
#define ARQ_DONE_STATUS 0x3f
#define STAT_GOT_STATUS (STATE_XFERRED_DATA|STATE_GOT_BUS|STATE_GOT_TARGET| \
						STATE_SENT_CMD|STATE_GOT_STATUS)
#define ARQ_GOT_STATUS (STATE_XFERRED_DATA|STATE_GOT_BUS|STATE_GOT_TARGET| \
						STATE_SENT_CMD|STATE_GOT_STATUS|STATE_ARQ_DONE)


/* Sense Data length for Auto Request Sense */
#define AIC_SENSE_LEN		14

/* Sector size for data transfers */
#define SECTOR_SIZE			512
#define ILLEGAL_VALUE	-1
#define DEFAULT_BASE_ADDRESS	0x340	/* AHA-1522As Default I/O base addr */

/*
 * Information maintained about each target the HBA serves.
 */
#define AIC_MAX_DMA_SEGS	16

/* 
 * tgt_state bits defining information per target
 */
#define AIC_TGT_SYNC	0x01
#define	AIC_TGT_ARQ		0x02
#define AIC_TGT_DISCO	0x04
#define AIC_TGT_TAGQ	0x08

struct per_tgt_info {
	ddi_dma_lim_t dma_lim;
#if 0
	unsigned ptgt_arq : 1;		/* auto-request sense enable */
	unsigned ptgt_tagque : 1;	/* tagged queueing enable */
	unsigned ptgt_sync : 1;
	unsigned ptgt_discon : 1;
	unsigned ptgt_resv : 4;		
#endif
	unsigned int 	tgt_state;
	unsigned int 	total_sectors;	/* The total no of sectors */
	unsigned int 	disk_geometry;  /* The disk geometry 	*/
	kcondvar_t pt_cv;			/* Per target condition variable */
	/*	 Callback id 
	int		pt_cb_id;		
	*/
#ifdef DMA
	ddi_dma_cookie_t	*sg;
#endif
};

struct aic {
	scsi_hba_tran_t  	*aic_tran;	
	void	*aic_iblock;
	dev_info_t 		*aic_dip;
	HACB			*hacb;
	struct per_tgt_info	pt[NTGTS*NLUNS];	
	unsigned int num_targets_attached;
	opaque_t	aic_cbhdl;
};

/**
typedef struct _AICSCB {
	SCB aic_scb;
	INT timeout_id;
}AICSCB;
**/

struct 	aic_scsi_cmd {
	struct scsi_pkt	*cmd_pkt;
	ulong		cmd_flags;	/* flags from scsi_init_pkt */
	u_int		cmd_cflags;	/* private hba CFLAG flags */
	struct aic_scsi_cmd *cmd_linkp;	/* link ptr for completion pkts */
	ddi_dma_handle_t cmd_dmahandle;	/* dma handle 			*/
	union {
		ddi_dma_win_t	d_dmawin;	/* dma window		*/
		caddr_t		d_addr;		/* transfer address	*/
	} cm;
	ddi_dma_seg_t	cmd_dmaseg;
	opaque_t	cmd_private;
	u_char		cmd_cdblen;	/* length of cdb 		*/
	u_char		cmd_scblen;	/* length of scb 		*/
	u_char		cmd_privlen;	/* length of target private 	*/
	u_char		cmd_rqslen;	/* len of requested rqsense	*/
	long		cmd_totxfer;	/* total transfer for cmd	*/
/*	keep target private at the end for allocation			*/
	u_char		cmd_pkt_private[PKT_PRIV_LEN];
	INT aic_timeout_id;
	int		nointr_pkt_comp;	/* For NOINTR packets */
	int		pkt_cb_id;			/* Callback id */
	SCB scb;
};

/*
 * Handy constants
 */

/* For returns from xxxcap() functions */

#define FALSE		0
#define UNDEFINED	-1
 
/*
 * Handy macros 
 */

#define	SDEV2ADDR(devp) (&((devp)->sd_address))
#define	SDEV2TRAN(devp) ((devp)->sd_address.a_hba_tran)
#define	PKT2TRAN(pktp)	((pktp)->pkt_address.a_hba_tran)
#define	ADDR2TRAN(ap)	((ap)->a_hba_tran)

#define PKT2AIC(pktp)	(struct aic *)((PKT2TRAN((pktp)))->tran_hba_private)

#define SDEV2AIC(sd)	(struct aic *)((SDEV2TRAN((sd)))->tran_hba_private)

#define ADDR2AIC(ap)	(struct aic *)((ADDR2TRAN((ap)))->tran_hba_private)

/* Get target, lun from pktp */
#define PKTP2TARG(pktp) 	((pktp)->pkt_address.a_target)
#define PKTP2LUN(pktp) 		((pktp)->pkt_address.a_lun)

/* Get target, lun from aicp */
#define AICP2TARG(aicp)	(((struct scsi_address *)(aicp))->a_target)
#define AICP2LUN(aicp)	(((struct scsi_address *)(aicp))->a_lun)

/* Get aicp from pktp */
#define PKTP2AICP(pktp)	((struct aic *)((pktp)->pkt_address.a_hba_tran))

/* Get ptgtp from ap */
/*
#define AP2PTGTP(ap) 		(&(((struct aic *)(ap)->a_hba_tran)->pt[(ap)->a_target * NLUNS + (ap)->a_lun]))
*/
#define AP2PTGTP(ap) 		(&((ADDR2AIC((ap)))->pt[(ap)->a_target * NLUNS + (ap)->a_lun]))

/* Get ptgtp from pktp */
#define PKTP2PTGTP(pktp) 	AP2PTGTP(&((pktp)->pkt_address))

/* Get aic_scsi_cmd from the pktp */
#define PKT2CMD(pktp)		(struct aic_scsi_cmd *)((pktp)->pkt_ha_private)


/***********************************************************************
 		 	MACROS USED IN "HIM6X60CompleteSCB()" Routine
 **********************************************************************/

#define SET_STATE(DATA) \
	if (scb->scbStatus & SCB_SENSE_DATA_VALID) \
		((struct scsi_arq_status *)pktp->pkt_scbp)->sts_rqpkt_state = DATA;\
	else \
		pktp->pkt_state = DATA

#define SET_REASON(DATA) \
	if (scb->scbStatus & SCB_SENSE_DATA_VALID) \
		((struct scsi_arq_status *)pktp->pkt_scbp)->sts_rqpkt_reason = DATA;\
	else \
		pktp->pkt_reason = DATA

#define SET_STAT(DATA) \
	if (scb->scbStatus & SCB_SENSE_DATA_VALID) \
		((struct scsi_arq_status *)pktp->pkt_scbp)->sts_rqpkt_statistics=DATA;\
	else \
		pktp->pkt_statistics = DATA

#define PKT_SCBP ((struct scsi_arq_status *)pktp->pkt_scbp)

/**********************************************************************/

#ifdef	__cplusplus
}
#endif

#endif  /* AIC_H */
