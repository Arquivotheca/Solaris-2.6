/*
 * Copyright (c) 1992 Sun Microsystems, Inc.  All Rights Reserved.
 */

#ifndef _SYS_DKTP_EHA_H
#define	_SYS_DKTP_EHA_H

#pragma ident	"@(#)eha.h	1.12	94/09/29 SMI"

#ifdef	__cplusplus
extern "C" {
#endif

#define	EHA_KVTOP(vaddr) (HBA_KVTOP((vaddr), eha_pgshf, eha_pgmsk))

#define	EHA_PHYMAX_DMASEGS 128	/* physical max number of s/g elements	*/
#define	EHA_MAX_DMA_SEGS 32	/* max number of s/g elements 		*/
#define	EHA_MAX_CCBS_OUT 64
#define	EHA_SENSE_LEN	 14 	/* max #  of sense bytes 		*/
#define	EHA_MAX_SG_SIZ	4194304 /* max size of one scatter gather entry */
/*
 * Note that scatter gather (max size of 4 meg * max number of elements of
 * 128) is greater than max total transfer size of 16 megabytes
 * currently this is set down to 256K in upper level code
 */

#define	EHA_MBO_LSB	0xCD0	/* host adapter mailbox out i/o ports */
#define	EHA_MBI_LSB	0xCD8	/* host adapter mailbox in i/o ports */

struct eha_stat		{		/* 1740 status block		*/
	unsigned short	status_word;	/* flags for the completed ccb 	*/
	u_char		ctlr_status;	/* host adapter status		*/
	u_char		target_status;	/* target status		*/
	ulong		resid_count;	/* residual byte count		*/
	ulong		resid_addr;	/* residual buffer address	*/
	unsigned short	add_status_len;	/* additional status length	*/
	u_char 		sense_xfred;    /* sense bytes actually xfered	*/
	u_char		res0[10];	/* 10 reserved bytes		*/
	unsigned short	target_mode_status; /* target mode status word	*/
	u_char		target_cmd[6];	/* target mode CDB		*/
	u_char		es_resv[2];	/* padd for alignment		*/
};

/* status block field definitions				*/

/* status word definitions					*/

#define	DON		0x1	/* command done -  no error 	*/
#define	DU		0x2	/* data underrun		*/
#define	SEL		0x4	/* host adapter selected	*/
#define	QF		0x8	/* host adapter queue full	*/
#define	SC		0x10	/* specification check		*/
#define	DO		0x20	/* data overrun			*/
#define	STATUSCH	0x40	/* chaining halted		*/
#define	INT		0x80	/* interrupt issued for CCB	*/
#define	ASA		0x100	/* additional status available	*/
#define	SNS		0x200	/* sense information stored	*/
#define	INI		0x800	/* initialization required	*/
#define	ME		0x1000	/* major error occurred		*/
#define	ECA		0x4000	/* ext contingent alligience 	*/

/* Host status definitions					*/

#define	HOST_NO_STATUS  0x00	/* no adapter status available	*/
#define	HOST_CMD_ABORTED 0x04	/* command aborted by host	*/
#define	H_A_CMD_ABORTED 0x05	/* command aborted by hba 	*/
#define	HOST_NO_FW	0x08	/* firmware not downloaded	*/
#define	HOST_NOT_INITTED 0x09	/* SCSI subsystem not initialized */
#define	HOST_BAD_TARGET 0x0A	/* target not assigned 		*/
#define	HOST_SEL_TO	0x11	/* selection timeout		*/
#define	HOST_DU_DO	0x12	/* Data overrun or underrun	*/
#define	HOST_BUS_FREE	0x13	/* unexpected bus free		*/
#define	HOST_PHASE_ERR	0x14	/* target bus phase seq error	*/
#define	HOST_BAD_OPCODE	0x16	/* invalid operation code	*/
#define	HOST_BAD_PARAM	0x18	/* invalid control blk parameter */
#define	HOST_DUPLICATE	0x19	/* duplicate target ctl blk rec */
#define	HOST_BAD_SG_LIST 0x1A   /* invalid Scatter/Gather list	*/
#define	HOST_BAD_REQSENSE 0x1B	/* request sense command failed */
#define	HOST_HW_ERROR	0x20	/* Host Adapter hardware error  */
#define	HOST_ATTN_FAILED 0x21	/* target didn't respond to attn */
#define	HOST_SCSI_RST1	 0x22	/* SCSI bus reset by host adapter */
#define	HOST_SCSI_RST2	 0x23	/* SCSI bus reset by other device */
#define	HOST_BAD_CHKSUM	0x80	/* program checksum failure	*/

struct sg_element {
	ulong	data_ptr;	/* 32 bit data pointer		*/
	ulong	data_len;	/* 32 bit data length		*/
};

struct eha_ccb {			/* command control block	*/
	unsigned short	ccb_cmdword;	/* command word			*/
	unsigned short	ccb_flag1;
	unsigned short	ccb_flag2;
	unsigned short	ccb_res0;
	paddr_t		ccb_datap;	/* data or s/g list pointer	*/
	ulong		ccb_datalen;	/* data or s/g list length	*/
	paddr_t 	ccb_statp;	/* status block pointer		*/
	ulong		ccb_chainp;	/* chain address pointer	*/
	ulong		ccb_res1;
	paddr_t    	ccb_sensep;	/* sense information pointer	*/
	unchar		ccb_senselen;	/* max. bytes of sense to xfer 	*/
	unchar		ccb_cdblen;	/* length in bytes of SCSI CDB	*/
	unsigned short	ccb_chksum;	/* chksum of data to be xfered	*/
	unchar		ccb_cdb[HBA_MAX_CDB_LEN]; /* CDB bytes 0-11	*/
	/* end of 174x ccb */

	struct eha_stat ccb_stat; 	/* hardware completion struct   */
	struct scsi_arq_status ccb_sense; /* Auto_Request sense blk	*/
	struct sg_element ccb_sg_list[EHA_MAX_DMA_SEGS]; /* sg list	*/
	union {
		char    *b_scratch;  	/* spare buffer space		*/
		struct scsi_cmd *b_ownerp; /* owner pointer- to 	*/
					/* point back to the packet	*/
	} cc;
	paddr_t		ccb_paddr; 	/* ccb physical address.	*/
	struct eha_ccb	*ccb_forw;
	u_char 		ccb_target;	/* scsi target id (to ATTN reg)	*/
	u_char 		ccb_resv;
};

#define	ccb_scratch	cc.b_scratch
#define	ccb_ownerp	cc.b_ownerp

/* command word operation codes						*/
#define	NOOP_CMD		0x0	/* no operation			*/
#define	DO_SCSI_CMD		0x1	/* initiator SCSI command	*/
#define	DIAG_CMD		0x5	/* run diagnostic command	*/
#define	INIT_SUBSYS_CMD		0x6	/* initialize SCSI subsystem	*/
#define	READ_AENINFO_CMD 	0x7	/* read async event notification */
#define	READ_SENSE_CMD		0x8	/* read sense information	*/
#define	DOWNLOAD_FW_CMD		0x9	/* download firmware		*/
#define	HA_INQ_CMD		0xA	/* read host adapter inquiry dat */
#define	DO_TARGET_CMD		0x10	/* target SCSI command		*/

/* flag word 1 bit definitions						*/
#define	CNE			0x1	/* chain no error		*/
#define	DI			0x80	/* disable interrupt		*/
#define	SES			0x400	/* suppress error on underrun	*/
#define	SG			0x1000	/* scatter-gather		*/
#define	DSBLK			0x4000	/* disable status block		*/
#define	ARS			0x8000	/* automatic request sense	*/

/* flag word 2 bit definitions 						*/
#define	TAG			0x8	/* tagged queueing		*/
#define	FLAG2ND			0x40	/* no disconnect		*/
#define	DAT			0x100	/* data transfer 		*/
#define	DIR			0x200	/* direction of transfer 	*/
#define	ST			0x400	/* no transfer to host memory	*/
#define	CHK			0x800	/* calc checksum on data	*/
#define	REC			0x4000	/* error recovery procedures	*/

/* bits 2-0 contain the LUN, bits 4&5 contain the tag type 		*/
/* tag type definitions - bits 4&5 of flag word 2 			*/
#define	HEAD_OF_QUEUE		0x10	/* head of queue		*/
#define	ORDERED_QUEUE_TAG	0x20	/* ordered queue tag		*/
#define	SIMPLE_QUEUE_TAG	0x00	/* simple queue tag		*/

/* Adapter immediate commands						*/
#define	IMMED_RESET		0x80	/* reset device command		*/
#define	IMMED_RESUME		0x90	/* resume command		*/

/* adapter port definitions						*/
#define	HID0		0xC80	/* byte 0 of expansion board ID 	*/
#define	HID1		0xC81	/* byte 1 of expansion board ID 	*/
#define	HID2		0xC82	/* byte 2 of expansion board ID 	*/
#define	HID3		0xC83	/* byte 3 of expansion board ID 	*/

#define	EBCTRL		0xC84	/* expansion board control reg		*/

#define	CDEN		0x1	/* host adapter enabled/disabled 	*/
#define	HAERR		0x2	/* host adapter fatal error		*/
#define	ERRST		0x4	/* reset HAERR and CDEN			*/

#define	EHA_MODE	0xCC0   /* adapter mode, port addresses 	*/
#define	EHA_MODE_OFFSET 0x40

#define	INTERFACE_1740 	0x80	/* set->1740 mode, cleared->1540 mode 	*/
#define	CONFIGURE	0x40	/* set->H.A. to program EEPROM  	*/
#define	ISA_ADDR	0x7	/* bits 2-0 = ISA port addr range 	*/

#define	EHA_INTDEF	0xCC2	/* interrupt definition			*/
#define	EHA_INTBITS	0x7	/* bits 2-0 select int channel		*/
#define	EHA_INT2IRQ	0x09	/* to get irq				*/

#define	EHA_SCSIDEF	0xCC3	/* SCSI definition			*/
#define	SCSIDEF_OFFSET  0x43
#define	SCSI_ID_BITS	0x7	/* bits 2-0 are HBA SCSI id		*/

#define	EHA_ATTENTION	0xCD4 	/* attention reg			*/

#define	IMMEDIATE_CMD	0x10	/* adapter immediate command		*/
#define	CCB_START	0x40	/* start CCB				*/
#define	CCB_ABORT	0x50	/* abort CCB				*/
/* bits 3-0 contain the target SCSI ID					*/

#define	EHA_CONTROL	0xCD5	/* Group 2 control definitions  	*/

#define	HARD_RESET	0x80	/* adapter reset with diags		*/
#define	CLEAR_INT	0x40	/* clears adapter interrupt		*/
#define	SET_HOST_RDY	0x20	/* sets Host ready status bit		*/

#define	EHA_INTERRUPT	0xCD6	/* Interrupt Status reg			*/

#define	CCB_DONE	0x10 	/* CCB completed successfully		*/
#define	CCB_RETRIED	0x50 	/* CCB completed ok after retry 	*/
#define	HBA_HW_FAILED	0x70	/* host adapter h.w. failure		*/
#define	IMMEDIATE_DONE	0xA0	/* Immediate cmd done w/success 	*/
#define	CCB_DONE_ERROR	0xC0	/* ccb complete with error		*/
#define	ASYNC_EVENT	0xD0	/* asynchronous event notification 	*/
#define	IMMEDIATE_DONE_ERR 0xE0	/* immediate command done with err 	*/

#define	TARGET_BITS	0x0F	/* bits 3-0 contain the target ID 	*/
#define	INTSTAT_BITS 	0xF0 	/* Interrupt status bits 0-4		*/


#define	EHA_STATUS1	0xCD7	/* Group 2 Status reg			*/

#define	MBO_EMPTY	0x4	/* mailbox out empty			*/
#define	INT_PENDING	0x2	/* interrupt pending			*/
#define	EHA_BUSY	0x1	/* busy - set by write to attn. 	*/

#define	EHA_STATUS2	0xCDC	/* Group 2 Status 2 reg			*/

#define	HOST_RDY	0x1	/* Host ready				*/

/* reset status returned in MBI_LSB after diagnostics are complete 	*/

#define	NO_ERR		0x0	/* no error				*/
#define	ROM_BAD		0x1	/* uP ROM test failure			*/
#define	RAM_BAD		0x2	/* RAM test failure			*/
#define	POWERDEV_ERR 	0x3	/* Power protection device err		*/
#define	INTERNAL_FAILURE 0x4	/* uP internal peripheral failure 	*/
#define	BUFFER_BAD	0x5	/* buffer control chip failure		*/
#define	SYS_IF_BAD	0x7	/* System interface control chip failure */
#define	SCSI_IF_BAD	0x8	/* SCSI interface chip failure		*/

/* soft reset status							*/
#define	HW_BAD		0x7	/* hardware failure			*/

#define	EHA_INQUIRY_DATA_LEN	0x100

struct eha_blk {
	kmutex_t 		eb_mutex;
	dev_info_t 		*eb_dip;
	ddi_iblock_cookie_t	eb_iblock;

	unchar			eb_flag;
	unchar			eb_targetid;
	unchar			eb_intr;
	char			eb_numdev;

	ushort			eb_ioaddr;
	ushort			eb_mbi_lsb;
	ushort			eb_mbo_lsb;
	ushort			eb_attention;
	ushort			eb_scsidef;
	ushort			eb_status1;
	ushort			eb_intrport;
	ushort			eb_control;

	struct scsi_inquiry	*eb_inqp;

	int	eb_ccb_cnt;
	struct eha_ccb		*eb_ccbp;
	struct eha_ccb		*eb_ccboutp;
	struct eha_ccb		*eb_last;
	ushort  		eb_child;
	ushort  		eb_q_siz; /* max ccb's for tag-queueing	*/
	opaque_t		eb_cbthdl;
};

struct eha_unit {
	ddi_dma_lim_t	eu_lim;
	unsigned eu_arq : 1;		/* auto-request sense enable	*/
	unsigned eu_tagque : 1;		/* tagged queueing enable	*/
	unsigned eu_resv : 6;
};

struct eha {
	scsi_hba_tran_t		*e_tran;
	struct eha_blk		*e_blkp;
	struct eha_unit		*e_unitp;
};

#define	EHA_DIP(eha)		(((eha)->e_blkp)->eb_dip)

#define	TRAN2HBA(hba)		((struct eha *)(hba)->tran_hba_private)
#define	SDEV2HBA(sd)		(TRAN2HBA(SDEV2TRAN(sd)))

#define	TRAN2EHA(hba)		((struct eha *)(hba)->tran_tgt_private)
#define	TRAN2EHABLKP(hba)	((TRAN2EHA(hba))->e_blkp)
#define	TRAN2EHAUNITP(hba)	((TRAN2EHA(hba))->e_unitp)
#define	SDEV2EHA(sd)		(TRAN2EHA(SDEV2TRAN(sd)))
#define	PKT2EHAUNITP(pktp)	(TRAN2EHAUNITP(PKT2TRAN(pktp)))
#define	PKT2EHABLKP(pktp)	(TRAN2EHABLKP(PKT2TRAN(pktp)))
#define	ADDR2EHAUNITP(ap)	(TRAN2EHAUNITP(ADDR2TRAN(ap)))
#define	ADDR2EHABLKP(ap)	(TRAN2EHABLKP(ADDR2TRAN(ap)))

#define	EHA_BLKP(X) (((struct eha *)(X))->e_blkp)


#define	EHA_ENABLE_INTR(ioaddr) \
	outb((ioaddr)+EHA_INTDEF, inb((ioaddr)+EHA_INTDEF) | 0x10);
#define	EHA_DISABLE_INTR(ioaddr) \
	outb((ioaddr)+EHA_INTDEF, inb((ioaddr)+EHA_INTDEF) & ~0x10);

#ifdef	__cplusplus
}
#endif

#endif	/* _SYS_DKTP_EHA_H */
