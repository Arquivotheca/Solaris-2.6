/*
 * Copyright (c) 1995-1996 Sun Microsystems, Inc.
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

#pragma ident	"@(#)adp.h	1.7	96/07/30 SMI"

/* #ifdef	__cplusplus
extern "C" {
#endif */


typedef struct adp {
	cfp_struct	 ab_cfp;	/* HIM adapter configuration struct */

	ccc_t		 ab_ccc;	/* the GHD control stucture */

	dev_info_t	*ab_dip;
	unsigned int	 ab_ioaddr;
	u_int 		 ab_intr_idx;
	unsigned char 	 ab_flag;
} adp_t;

/* ab_flag defines for Solaris in adp_block */
#define	ADP_FLAG_GT1GIG		0x01	/* disk > 1 Gibabyte */
#define	ADP_FLAG_NOBIOS		0x02	/* no INT 13 BIOS active */

#define	ADPP2CFPP(adpp)		(&(adpp)->ab_cfp)
#define CFPP2ADPP(cfpp)		((adp_t *)((cfpp)->Cf_OSspecific))

/*
 * One per target-lun nexus
 */
typedef struct adp_tgt {
	ddi_dma_lim_t au_lim;
	uint	au_arq : 1;		/* auto-request sense enable */
	uint	au_tagque : 1;		/* tagged queueing enable */
	uint	au_resv : 6;		/* pad rest of bits */
	u_short	au_target;		/* the encoded target/LUN address */
	u_char	au_lun;
	ulong	au_total_sectors;

	scsi_hba_tran_t	*au_tran;
	adp_t 	*au_adpp;
} adptgt_t;



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
#define TGTP2DIP(tgtp)		(((tgtp)->a_hbap)->ab_dip)

#define TRAN2TGTP(hba)		((adptgt_t *)(hba)->tran_tgt_private)
#define TRAN2ADPP(tran) 	((adp_t *)((tran)->tran_hba_private))

#define SDEV2TGTP(sd)  		(TRAN2TGTP(SDEV2TRAN(sd)))
#define SDEV2ADPP(sd)	  	(TRAN2ADPP(SDEV2TRAN(sd)))

#define TGTP2ADPP(adp)		((adp_t *)(tgtp)->au_adpp)

#define ADDR2TGTP(ap)  		(TRAN2TGTP(ADDR2TRAN(ap)))
#define ADDR2ADPP(ap)  		(TRAN2ADPP(ADDR2TRAN(ap))) 
#define PKTP2TGTP(pktp)		(TRAN2TGTP(PKTP2TRAN(pktp)))
#define PKTP2ADPP(pktp)		(TRAN2ADPP(PKTP2TRAN(pktp)))

#define	SCBP(pkt)	((struct scsi_status *)(pkt)->pkt_scbp)
#define	SCBP_C(pkt)	((*(pkt)->pkt_scbp) & STATUS_MASK)

#ifdef	__cplusplus
}
#endif

#endif  /* _SYS_DKTP_ADP_H */
