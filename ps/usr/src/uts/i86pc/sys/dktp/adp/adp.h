/*
 * Copyright (c) 1995 Sun Microsystems, Inc.
 * All Rights Reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */

#ifndef _SYS_DKTP_ADP_H
#define _SYS_DKTP_ADP_H

#pragma ident	"@(#)adp.h	1.5	95/05/17 SMI"

/* #ifdef	__cplusplus
extern "C" {
#endif */

/* One per target-lun nexus	*/
struct adp_unit {
	ddi_dma_lim_t au_lim ;
	unsigned au_arq : 1 ;		/* auto-request sense enable */
	unsigned au_tagque : 1 ;	/* tagged queueing enable */
	unsigned au_gt1gig : 1 ;	/* disk > 1 Gibabyte */
	unsigned au_resv : 5 ;		/* pad rest of bits */
	unsigned long au_total_sectors;
};

/*	move from adp structure to struct cfp		*/
#define ADP_BLKP(X) (((struct adp *)(X))->a_blkp)

/* One per target-lun nexus	*/
struct adp {
	scsi_hba_tran_t	*a_tran;
	struct cfp 	*a_blkp;
	struct adp_unit	*a_unitp;
};

#define DEFAULT_NUM_SCBS	24

#define ADAPTEC_PCI_ID		0x9004
#define ADP_7871_PCI_ID		0x7178
#define ADP_7870_PCI_ID		0x7078
#define ADP_7278_PCI_ID		0x7278
#define ADP_SAMSUNG_PCI_ID	0x5078
#define ADP_PHYMAX_DMASEGS	255	/* phy max of Scatter/Gather seg*/
#define ADP_SENSE_LEN     	0x18   	/* AUTO-Request sense size in bytes*/
#define ADP_MAX_CDB_LEN		12	/* Standard SCSI CDB length	*/

/* Adapted 294x host adapter error codes */
#define ADP_NO_STATUS		0x00	/* No adapter status available */
#define ADP_ABT_HOST		0x04	/* Command aborted by host */
#define ADP_ABT_HA		0x05	/* Command aborted by host adapter */
#define ADP_SEL_TO		0x11	/* Selection timeout */
#define ADP_DU_DO		0x12	/* Data overrun/underrun error */
#define ADP_BUS_FREE		0x13	/* Unexpected bus free */
#define ADP_PHASE_ERR		0x14	/* Target bus phase sequence error */
#define ADP_INV_LINK		0x17	/* Invalid SCSI linking operation */
#define ADP_SNS_FAIL		0x1b	/* Auto-request sense failed */
#define ADP_TAG_REJ		0x1c	/* Tagged Queuing rejected by target */
#define ADP_HW_ERROR		0x20	/* Host adpater hardware error */
#define ADP_ABT_FAIL		0x21	/* Target did'nt respond to ATN (RESET)*/
#define ADP_RST_HA		0x22	/* SCSI bus reset by host adapter */
#define ADP_RST_OTHER		0x23	/* SCSI bus reset by other device */
#define ADP_NOAVL_INDEX 	0x30	/* No available index ?		*/

#define ADP_UNKNOWN_ERROR       0x30

#define ADP_KVTOP(vaddr) 	(HBA_KVTOP((vaddr), adp_pgshf, adp_pgmsk))
#define ADP_DIP(adp)		(((adp)->a_blkp)->ab_dip)

#define TRAN2ADP(hba)		((struct adp *)(hba)->tran_tgt_private)
#define TRAN2BLK(tran) 	((struct cfp *)((tran)->tran_hba_private))

#define SDEV2BLK(sd)	  	(TRAN2BLK(SDEV2TRAN(sd)))
#define SDEV2ADP(sd)	  	(TRAN2ADP(SDEV2TRAN(sd)))

#define ADP2BLK(adp)		((struct cfp *)(adp)->a_blkp)

#define ADDR2ADPP(ap)	  	(TRAN2ADP(ADDR2TRAN(ap)))
#define ADDR2BLK(ap)  		(TRAN2BLK(ADDR2TRAN(ap))) 
#define	TRAN2ADPUNITP(tran)	((TRAN2ADP(tran))->a_unitp)
#define	ADDR2ADPUNITP(pktp)	(TRAN2ADPUNITP(ADDR2TRAN(pktp)))
#define	PKT2ADPUNITP(pktp)	(TRAN2ADPUNITP(PKT2TRAN(pktp)))
#define PKT2ADPP(pktp)	  	(TRAN2ADP(PKT2TRAN(pktp)))
#define PKT2BLK(pktp) 		(TRAN2BLK(PKT2TRAN(pktp)))

#define	SCBP(pkt)	((struct scsi_status *)(pkt)->pkt_scbp)
#define	SCBP_C(pkt)	((*(pkt)->pkt_scbp) & STATUS_MASK)

#ifdef	__cplusplus
}
#endif

#endif  /* _SYS_DKTP_ADP_H */
