/*
 * Copyright (c) 1992-96, by Sun Microsystems, Inc.  All Rights Reserved.
 */

#ifndef	_SYS_DKTP_AHA_H
#define	_SYS_DKTP_AHA_H

#pragma ident	"@(#)aha.h	1.19	96/08/29 SMI"


#ifdef	__cplusplus
extern "C" {
#endif

#define	AHA_KVTOP(vaddr) (HBA_KVTOP((vaddr), aha_pgshf, aha_pgmsk))

/*
 * The Adaptec Host Adapter Command Control Block (CCB)
 */

#define	AHA_MAX_DMA_SEGS	17	/* Max Scatter/Gather segments	*/
#define	AHA_17_DMA_SEGS		17	/* for a/b controllers		*/
#define	AHA_255_DMA_SEGS	255	/* for C or greater controllers	*/
#define	AHA_SENSE_LEN		14 	/* max sense bytes 		*/

/* a Scatter/Gather DMA Segment Descriptor */
struct aha_dma_seg {
	unchar  data_len[3];	/* segment length 			*/
	unchar  data_ptr[3];	/* segment address 			*/
};

#pragma pack(1)
struct aha_ccb {
	unchar  ccb_op;		/* CCB Operation Code 			*/
	unchar 	ccb_tf_lun : 3;	/* LUN on target device 		*/
	unchar 	ccb_tf_in  : 1;	/* 'incoming' xfer, length is checked 	*/
	unchar 	ccb_tf_out : 1;	/* 'outgoing' xfer, length is checked 	*/
	unchar 	ccb_tf_tid : 3;	/* Target ID 				*/
	unchar  ccb_cdblen;	/* Length of SCSI CDB			*/
	unchar	ccb_senselen;	/* max. bytes of sense to xfer 		*/
	unchar  ccb_datalen[3];	/* Data Length (msb, mid, lsb) 		*/
	unchar  ccb_datap[3];	/* Data (buffer) ptr (msb, mid, lsb)	*/
	unchar  ccb_link[3];	/* Link Pointer (msb, mid, lsb) 	*/
	unchar  ccb_linkid;	/* Command Link ID 			*/
	unchar  ccb_hastat;	/* Host Adapter status 			*/
	unchar  ccb_tarstat;	/* Target Status 			*/
	unchar  ccb_reserved2[2];
/*
 * The following allows space for MAX bytes of SCSI CDB, followed
 * by 14 bytes of sense data acquired by the AHA in the event of an
 * error.  The beginning of the sense data will be:
 *             &aha_ccb.ccb_cdb[aha_ccb.ccb_cdblen]
 */
	unchar  ccb_cdb[HBA_MAX_CDB_LEN+AHA_SENSE_LEN];
	struct  aha_dma_seg *ccb_sg_list;	/* SG segs	*/
	union {
		char    *ccb_scratch;  	/* spare buffer space, if needed */
		struct  scsi_cmd *ccb_ownerp;	/* owner pointer */
	} cc;
	paddr_t	ccb_paddr;			/* physical address */
	struct	aha_ccb	*ccb_forw;		/* forward link pointers */
	struct	aha_ccb	*ccb_back;		/* backward link pointers */
	ushort	ccb_status;			/* status word */
	ushort  ccb_resv;
	struct scsi_arq_status ccb_sense;	/* Auto_Request sense blk */
};

#define	ccb_scratch	cc.ccb_scratch
#define	ccb_ownerp	cc.ccb_ownerp

#pragma pack()

/*	ccb status code							 */
#define	CCB_PGVALID	0x0001		/* ccb within a page		 */

/* Direction bits for ccb_targ: */
#define	CTD_MASK	0x18	/* The 2 direction bits */
#define	CTD_IN		0x8	/* The ccb_tf_in bit */
#define	CTD_OUT		0x10	/* The ccb_tf_out bit */

/*
 * ccb_op values:
 */
#define	COP_COMMAND	0	/* Normal SCSI Command 			*/
#define	COP_SCATGATH	2	/* SCSI Command with Scatter/Gather 	*/
#define	COP_CMD_RESID	3	/* Command with residual		*/
#define	COP_SG_RESID	4	/* Command with Scatter/Gather resid	*/
#define	COP_RESET	0x81	/* Bus Device Reset (Aborts all 	*/

#define	AHA_CFG_64HD    	0x0001	/* 64-head BIOS   		*/
#define	AHA_CFG_SCATGATH 	0x0002	/* scatter/gather feature	*/
#define	AHA_CFG_SCATGATH 	0x0002	/* scatter/gather feature	*/
#define	AHA_CFG_EXTBIOS		0x0004  /* extended bios enabled	*/
#define	AHA_EXTBIOS_ENABLED_FLAG 0x0008	/* return code from EXTBIOS	*/
#define	AHA_CFG_C		0x0010	/* board is a 154C/1542C type of   */
					/* board, note this type of board  */
					/* supports the EXTBIOS & MBXUNLCK */
					/* commands			   */

struct  aha_blk {
	kmutex_t ab_mutex;
	dev_info_t *ab_dip;
	ddi_iblock_cookie_t	ab_iblock;
	ushort	ab_ioaddr;
	unchar 	ab_flag;
	unchar	ab_targetid;

	unchar	ab_bdid;
	unchar	ab_intr;
	unchar	ab_dmachan;
	unchar	ab_dmaspeed;
	unchar	ab_buson;
	unchar	ab_busoff;

	char	ab_numdev;
	char	ab_num_mbox;
	short	ab_mbout_idx;
	short	ab_mbin_idx;
	int	ab_mbx_allocmem;
	int	ab_mbout_miss;
	int	ab_mbin_miss;

	unchar	*ab_bufp;
	struct  mbox_entry	*ab_mboutp;
	struct  mbox_entry	*ab_mbinp;
	uint	ab_actm_stat;
	struct  aha_ccb		*ab_actm_addrp;

	struct	scsi_inquiry	*ab_inqp;

	int	ab_ccb_cnt;
	struct	aha_ccb 	*ab_ccbp;
	struct	aha_ccb 	*ab_ccboutp;
	ushort  		ab_child;
	short  			ab_resv;
	opaque_t		ab_cbthdl;
	int			ab_dma_reqsize;
	unchar			ab_slot;
	ddi_dma_lim_t		*ab_dma_lim;	/* actuall limit structure */
						/* will be determined at   */
						/* runtime based on the	   */
						/* board type		   */
};

struct aha_unit {
	ddi_dma_lim_t	au_lim;
	unsigned au_arq		: 1;	/* auto-request sense enable	*/
	unsigned au_tagque	: 1;	/* tagged queueing enable	*/
	unsigned au_resv	: 6;
	unsigned long au_total_sectors; /* total sector count		*/

};


#define	AHA_DIP(aha)		(((aha)->a_blkp)->ab_dip)

#define	TRAN2HBA(hba)		((struct aha *)(hba)->tran_hba_private)
#define	SDEV2HBA(sd)		(TRAN2HBA(SDEV2TRAN(sd)))

#define	TRAN2AHA(hba)		((struct aha *)(hba)->tran_tgt_private)
#define	TRAN2AHABLKP(hba)	((TRAN2AHA(hba))->a_blkp)
#define	TRAN2AHAUNITP(hba)	((TRAN2AHA(hba))->a_unitp)
#define	SDEV2AHA(sd)		(TRAN2AHA(SDEV2TRAN(sd)))
#define	PKT2AHAUNITP(pktp)	(TRAN2AHAUNITP(PKT2TRAN(pktp)))
#define	PKT2AHABLKP(pktp)	(TRAN2AHABLKP(PKT2TRAN(pktp)))
#define	ADDR2AHAUNITP(ap)	(TRAN2AHAUNITP(ADDR2TRAN(ap)))
#define	ADDR2AHABLKP(ap)	(TRAN2AHABLKP(ADDR2TRAN(ap)))


#define	AHA_BLKP(X) (((struct aha *)(X))->a_blkp)
struct aha {
	scsi_hba_tran_t		*a_tran;
	struct aha_blk		*a_blkp;
	struct aha_unit		*a_unitp;
};



/*
 * Adapter I/O ports.  These are offsets from ioaddr.
 */

#define	AHACTL		0	/* Host Adapter Control Port (WRITE) 	*/
#define	AHASTAT		0	/* Status Port (READ) 			*/
#define	AHADATACMD	1	/* Command (WRITE) and Data (READ/WRITE) Port */
#define	AHAINTFLGS	2	/* Interrupt Flags Port (READ) 		*/

/*
 * Bit definitions for AHACTL port:
 */

#define	CTL_HRST	0x80	/* Hard Reset of Host Adapter 		*/
#define	CTL_SRST	0x40	/* Soft Reset 				*/
#define	CTL_IRST	0x20	/* Interrupt Reset (clears AHAINTFLGS) 	*/
#define	CTL_SCRST	0x10	/* Reset SCSI Bus 			*/

/*
 * Bit definitions for AHASTAT port:
 */

#define	STAT_STST	0x80	/* Self-test in progress 		*/
#define	STAT_DIAGF	0x40	/* Internal Diagnostic Failure 		*/
#define	STAT_INIT	0x20	/* Mailbox Init required 		*/
#define	STAT_IDLE	0x10	/* Adapter is Idle 			*/
#define	STAT_CDF	0x08	/* AHADATACMD (outgoing) is full 	*/
#define	STAT_DF		0x04	/* AHADATACMD (incoming) is full 	*/
#define	STAT_INVDCMD	0x01	/* Invalid host adapter command 	*/
#define	STAT_MASK	0xfd	/* mask for valid status bits 		*/

/*
 * Bit definitions for AHAINTFLGS port:
 */

#define	INT_ANY		0x80	/* Any interrupt (set when bits 0-3 valid) */
#define	INT_SCRD	0x08	/* SCSI reset detected 			*/
#define	INT_HACC	0x04	/* Host Adapter command complete 	*/
#define	INT_MBOE	0x02	/* MailBox (outgoing) Empty 		*/
#define	INT_MBIF	0x01	/* MailBox (incoming) Mail Ready	*/

/*
 * AHA-Host Adapter Command Opcodes:
 * NOTE: for all multi-byte values sent in AHADATACMD, MSB is sent FIRST
 */

#define	CMD_NOP		0x00	/* No operation, sets INT_HACC 		*/

#define	CMD_MBXINIT	0x01	/* Initialize Mailboxes 		*/
				/* ARGS: count, 1-255 valid 		*/
				/*    mbox addr (3 bytes) 		*/

#define	CMD_DOSCSI	0x02	/* Start SCSI (Scan outgoing mailboxes) */

#define	CMD_ATBIOS	0x03	/* Start AT BIOS command 		*/

#define	CMD_ADINQ	0x04	/* Adapter Inquiry 			*/
				/* RETURNS: 4 bytes of firmware info 	*/

#define	CMD_MBOE_CTL	0x05	/* Enable/Disable INT_MBOE interrupt 	*/
				/* ARG: 0 - Disable, 1 - Enable 	*/

#define	CMD_SELTO_CTL	0x06	/* Set SCSI Selection Time Out 		*/
				/* ARGS: 0 - TO off, 1 - TO on 		*/
				/*    Reserved (0) 			*/
				/*    Time-out value (in ms, 2 bytes)	*/

#define	CMD_BONTIME	0x07	/* Set Bus-ON Time 			*/
				/* ARG: time in microsec, 2-15 valid 	*/

#define	CMD_BOFFTIME	0x08	/* Set Bus-OFF Time 			*/
				/* ARG: time in microsec, 0-250 valid 	*/

#define	CMD_XFERSPEED	0x09	/* Set AT bus burst rate (MB/sec) 	*/
				/* ARG: 0 - 5MB, 1 - 6.7 MB, 		*/
				/* 2 - 8MB, 3 - 10MB 			*/
				/* 4 - 5.7MB (1542 ONLY!) 		*/

#define	CMD_INSTDEV	0x0a	/* Return Installed Devices 		*/
				/* RETURNS: 8 bytes, one per target ID, start */
				/*    with 0.  Bits set (bitpos=LUN) in */
				/*    each indicate Unit Ready 		*/

#define	CMD_CONFIG	0x0b	/* Return Configuration Data 		*/
				/* RETURNS: 3 bytes, which are: 	*/
				/*  byte 0: DMA Channel, bit encoded as */
#define	CFG_DMA_CH0	0x01	/*		Channel 0 		*/
#define	CFG_DMA_CH5	0x20	/*		Channel 5 		*/
#define	CFG_DMA_CH6	0x40	/*		Channel 6 		*/
#define	CFG_CMD_CH7	0x80	/*		Channel 7 		*/
#define	CFG_DMA_MASK	0xe1	/*		(mask for above) 	*/
				/*  byte 1: Interrupt Channel, 		*/
#define	CFG_INT_CH9	0x01	/*		Channel 9 		*/
#define	CFG_INT_CH10	0x02	/*		Channel 10 		*/
#define	CFG_INT_CH11	0x04	/*		Channel 11 		*/
#define	CFG_INT_CH12	0x08	/*		Channel 12 		*/
#define	CFG_INT_CH14	0x20	/*		Channel 14 		*/
#define	CFG_INT_CH15	0x40	/*		Channel 15 		*/
#define	CFG_INT_MASK	0x6f	/*		(mask for above) 	*/
				/*  byte 2: HBA SCSI ID, in binary 	*/
#define	CFG_ID_MASK	0x03	/*		(mask for above) 	*/

#define	CMD_WTFIFO	0x1c	/* Write Adapter FIFO Buffer 		*/

#define	CMD_RDFIFO	0x1d	/* Read Adapter FIFO Buffer 		*/

#define	CMD_ECHO	0x1f	/* Echo Data 				*/
				/* ARG: one byte of test data 		*/
				/* RETURNS: the same byte (hopefully) 	*/
#define	CMD_EXTBIOS	0x28	/* command that fetches EXTENDED BIOS   */
				/* information from the borad.		*/
#define	CMD_MBXUNLK	0x29	/* Unlocks locked mailboxes (this   	*/
				/* determination is made by CMD_EXTBIOS */

/*
 * The Mail Box Structure.
 */

struct  mbox_entry {
	unchar  mbx_cmdstat;	/* Command/Status byte (below) 	*/
	unchar  mbx_ccb_addr[3];	/* AHA-style CCB address 	*/
	};

/*
 * Command codes for mbx_cmdstat:
 */

#define	MBX_FREE	0	/* Available mailbox slot 		*/

#define	MBX_CMD_START	1	/* Start SCSI command described by CCB 	*/
#define	MBX_CMD_ABORT	2	/* Abort SCSI command described by CCB 	*/

#define	MBX_STAT_DONE	1	/* CCB completed without error 		*/
#define	MBX_STAT_ABORT	2	/* CCB was aborted by host 		*/
#define	MBX_STAT_CCBNF	3	/* CCB for ABORT request not found 	*/
#define	MBX_STAT_ERROR	4	/* CCB completed with error 		*/

/*
 * ccb_hastat values:
 */
#define	HS_OK		0x00	/* No host adapter detected error 	*/
#define	HS_SELTO	0x11	/* Selection Time Out 			*/
#define	HS_DATARUN	0x12	/* Data over/under-run 			*/
#define	HS_BADFREE	0x13	/* Unexpected Bus Free 			*/
#define	HS_BUSPHASE	0x14	/* Target bus phase sequence failure 	*/
#define	HS_BADCCB	0x16	/* invalid ccb operation code		*/
#define	HS_BADLINK	0x17	/* linked CCB with diff. lun		*/
#define	HS_BADTARGDIR	0x18	/* invalid target direction from host	*/
#define	HS_DUPCCB	0x19	/* duplicate ccb			*/
#define	HS_BADSEG	0x1a	/* invalid ccb or segment list param	*/
#define	HS_UNKNOWN_ERR	0x01

/*
 * Micro Channel POS data
 */
#define	AHA_MAX_MC_SLOTS	8	/* Max Mirco Channel slots	*/
#define	MC_SLOT_SELECT		0x96	/* POS/slot select register */
#define	SLOT_ENABLE		0x8	/* enable access to a given slot */
#define	BIOS_DISABLE    	0x80	/* disable onboard BIOS */
#define	INTR_DISABLE    	0x07	/* disable h/w interrupt channel */
#define	AHA1640_SIGNATURE 	0x0F1F	/* characteristic board ID */
#define	BT646_SIGNATURE   	0x0708	/* characteristic board ID */
#define	AHA_POSADDR3_MASK	0x0c7	/* read ioaddr from POS reg 3 */
#define	AHA_POS4IRQ_MASK	0x03	/* read IRQ from POS reg 4 */
#define	AHA_IOADDR_130		0x01	/* pos reg 3 io address 0x130 */
#define	AHA_IOADDR_134		0x41	/* pos reg 3 io address 0x134 */
#define	AHA_IOADDR_230		0x02	/* pos reg 3 io address 0x230 */
#define	AHA_IOADDR_234		0x42	/* pos reg 3 io address 0x234 */
#define	AHA_IOADDR_330		0x03	/* pos reg 3 io address 0x330 */
#define	AHA_IOADDR_334		0x43	/* pos reg 3 io address 0x334 */
#define	AHA_MAX_NUMIOADDR 	6	/* # of possible io port addresses */
#define	AHA_CODE_TOIRQ		8	/* add 8 to POS4 to get IRQ	*/
#define	AHA_1640DEFAULT_IRQ	0x3	/* IRQ 11 */

#define	POS0	0x100	/* POS register definitions */
#define	POS1	0x101
#define	POS2	0x102
#define	POS3	0x103
#define	POS4	0x104
/*
 * AHA command structure.  For each command, we have flags, a number
 * of data bytes output, and a number of data bytes input.
 * This table is used by aha_docmd.
 */
struct  aha_cmd {
	unchar	ac_flags;	/* flag bits (below) 			*/
	unchar	ac_args;	/* # of argument bytes 			*/
	unchar	ac_vals;	/* number of value bytes 		*/
};

struct aha_addr_code {	/* 1640 codes vs io port addresses */
	ushort ac_ioaddr;
	ushort ac_code;
};


#ifdef	__cplusplus
}
#endif

#endif	/* _SYS_DKTP_AHA_H */
