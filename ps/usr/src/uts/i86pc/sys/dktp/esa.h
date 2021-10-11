/*
 * Copyrighted as an unpublished work.
 * (c) Copyright Sun Microsystems, Inc. 1994
 * All rights reserved.
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

#ifndef _SYS_DKTP_ESA_H
#define _SYS_DKTP_ESA_H

#pragma ident	"@(#)esa.h 1.2	94/07/09 SMI"

/* #ifdef	__cplusplus
extern "C" {
#endif */


/* This is the per card (7770) structure				*/
struct esa_card {
	struct him_config_block *ec_blkp;
	unsigned int ec_ioaddr ;
	kmutex_t ec_mutex ;		/* Mutex protecting the card	*/
	u_int ec_intr_idx ;
	u_int ec_flags ;
	void *ec_iblock;			/* ddi_iblock cookie ptr */
	struct esa_card *ec_next;
	struct him_data_block *ec_datap ;	/* One per card */
	struct him_config_block *ec_Ablkp ;	/* One per card */
	unsigned long ec_him_data_paddr;	/* physical address	*/
	int	ec_him_data_size;		/* size in bytes	*/
	u_char ec_child ;
};

/* One per target-lun nexus	*/
struct esa_unit {
	ddi_dma_lim_t eu_lim ;
	unsigned eu_arq : 1 ;		/* auto-request sense enable */
	unsigned eu_tagque : 1 ;	/* tagged queueing enable */
	unsigned eu_gt1gig : 1 ;	/* disk > 1 Gibabyte */
	unsigned eu_resv : 5 ;		/* pad rest of bits */
	unsigned long eu_total_sectors;
};

/*	move from esa structure to him_config_block		*/
#define ESA_BLKP(X) (((struct esa *)(X))->e_blkp)

/* One per target-lun nexus	*/
struct esa {
	scsi_hba_tran_t			*e_tran;
	struct him_config_block 	*e_blkp;
	struct esa_unit			*e_unitp;
};

#define ESA_PHYMAX_DMASEGS	255	/* phy max of Scatter/Gather seg*/
#define ESA_SENSE_LEN     	0x18   	/* AUTO-Request sense size in bytes*/
#define ESA_MAX_CDB_LEN		12	/* Standard SCSI CDB length	*/

/* Adapted 274x host adapter error codes */
#define ESA_NO_STATUS		0x00	/* No adapter status available */
#define ESA_ABT_HOST		0x04	/* Command aborted by host */
#define ESA_ABT_HA		0x05	/* Command aborted by host adapter */
#define ESA_SEL_TO		0x11	/* Selection timeout */
#define ESA_DU_DO		0x12	/* Data overrun/underrun error */
#define ESA_BUS_FREE		0x13	/* Unexpected bus free */
#define ESA_PHASE_ERR		0x14	/* Target bus phase sequence error */
#define ESA_INV_LINK		0x17	/* Invalid SCSI linking operation */
#define ESA_SNS_FAIL		0x1b	/* Auto-request sense failed */
#define ESA_TAG_REJ		0x1c	/* Tagged Queuing rejected by target */
#define ESA_HW_ERROR		0x20	/* Host adpater hardware error */
#define ESA_ABT_FAIL		0x21	/* Target did'nt respond to ATN (RESET)*/
#define ESA_RST_HA		0x22	/* SCSI bus reset by host adapter */
#define ESA_RST_OTHER		0x23	/* SCSI bus reset by other device */

#define ESA_UNKNOWN_ERROR       0x24

#define ESA_KVTOP(vaddr) 	(HBA_KVTOP((vaddr), esa_pgshf, esa_pgmsk))
#define ESA_DIP(esa)		(((esa)->e_blkp)->eb_dip)

#define TRAN2ESA(hba)		((struct esa *)(hba)->tran_tgt_private)
#define TRAN2BLK(tran) 	((struct him_config_block *)((tran)->tran_hba_private))

#define SDEV2BLK(sd)	  	(TRAN2BLK(SDEV2TRAN(sd)))
#define SDEV2ESA(sd)	  	(TRAN2ESA(SDEV2TRAN(sd)))

#define BLK2CARD(blk)		((struct esa_card *)(blk)->eb_cardp)

#define TRAN2CARD(tran)		(BLK2CARD(TRAN2BLK(tran)))

#define ESA2BLK(esa)		((struct him_config_block *)(esa)->e_blkp)
#define ESA2CARD(esa)		(BLK2CARD(ESA2BLK(esa)))

#define ADDR2ESAP(ap)	  	(TRAN2ESA(ADDR2TRAN(ap)))
#define ADDR2BLK(ap)  		(TRAN2BLK(ADDR2TRAN(ap))) 
#define	TRAN2ESAUNITP(tran)	((TRAN2ESA(tran))->e_unitp)
#define	ADDR2ESAUNITP(pktp)	(TRAN2ESAUNITP(ADDR2TRAN(pktp)))
#define	PKT2ESAUNITP(pktp)	(TRAN2ESAUNITP(PKT2TRAN(pktp)))
#define PKT2ESAP(pktp)	  	(TRAN2ESA(PKT2TRAN(pktp)))
#define PKT2BLK(pktp) 		(TRAN2BLK(PKT2TRAN(pktp)))

#define	SCBP(pkt)	((struct scsi_status *)(pkt)->pkt_scbp)
#define	SCBP_C(pkt)	((*(pkt)->pkt_scbp) & STATUS_MASK)

/* esa_card ec_flags 							*/
#define ESA_GT1GIG	0x0002
#define ESA_DOS_BIOS	0x0004

#ifdef	__cplusplus
}
#endif

#endif  /* _SYS_DKTP_ESA_H */
