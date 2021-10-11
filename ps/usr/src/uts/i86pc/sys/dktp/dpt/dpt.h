/*
 * Copyright (c) 1995-96, by Sun Microsystems, Inc.  All Rights Reserved.
 */

#ifndef _SYS_DKTP_DPT_H
#define	_SYS_DKTP_DPT_H

#pragma ident	"@(#)dpt.h	1.23	96/08/29 SMI"

#ifdef	__cplusplus
extern "C" {
#endif

#define	HBA_INTPROP(devi, pname, pval, plen) \
	(ddi_prop_op(DDI_DEV_T_NONE, (devi), PROP_LEN_AND_VAL_BUF, \
		DDI_PROP_DONTPASS, (pname), (caddr_t)(pval), (plen)))

#define	HBA_KVTOP(vaddr, shf, msk) \
		((paddr_t)(hat_getkpfnum((caddr_t)(vaddr)) << (shf)) | \
			    ((paddr_t)(vaddr) & (msk)))

#define	DPT_KVTOP(vaddr) (HBA_KVTOP((vaddr), dpt_pgshf, dpt_pgmsk))

#define	PRF		prom_printf

#define	DPT_PHYMAX_DMASEGS	64	/* phy max Scatter/Gather seg	*/
#define	DPT_MAX_DMA_SEGS	32	/* Max used Scatter/Gather seg	*/
#define	DPT_SENSE_LEN		0x18	/* AUTO-Request sense size	*/

/* dpt host adapter error codes from location dpt_stat.sp_hastat */
#define	DPT_OK			0x0
#define	DPT_SELTO		0x1
#define	DPT_CMDTO		0x2
#define	DPT_BUSRST		0x3
#define	DPT_POWUP		0x4
#define	DPT_PHASERR		0x5
#define	DPT_BUSFREE		0x6
#define	DPT_BUSPARITY		0x7
#define	DPT_BUSHUNG		0x8
#define	DPT_MSGREJECT		0x9
#define	DPT_RSTSTUCK		0x0a
#define	DPT_REQSENFAIL		0x0b
#define	DPT_HAPARITY		0x0c
#define	DPT_CPABORTNOTACTIVE	0x0d
#define	DPT_CPBUSABORT		0x0e
#define	DPT_CPRESETNOTACTIVE	0x0f
#define	DPT_CPRESETONBUS	0x10
#define	DPT_CSRAMECC		0x11	/* Controller RAM ECC Error	*/
#define	DPT_CSPCIPE		0x12	/* PCI Parity Error:data or cmd	*/
#define	DPT_CSPCIRMA		0x13	/* PCI Received Master Abort	*/
#define	DPT_CSPCIRTA		0x14	/* PCI Received Target Abort	*/
#define	DPT_CSPCISTA		0x15	/* PCI Signaled Target Abort	*/

#ifdef  DPT_DEBUG
#define	DPT_UNKNOWN_ERROR	0x16
#endif

/* Scatter Gather - definitions and list structure.			*/
struct dpt_sg {
	paddr_t data_addr;
	ulong	data_len;
};

/* EATA Command Packet control bits structure definition - 		*/
/* this controlls the data direction, cdb interpret, and other misc 	*/
/* controll bits.							*/
struct cp_bits {
	unchar SCSI_Reset:1;	/* Cause a SCSI Bus reset on the cmd	*/
	unchar HBA_Init:1;	/* Cause Controller to reInitialize	*/
	unchar Auto_Req_Sen:1;	/* Do Auto Request Sense on errors	*/
	unchar Scatter_Gather:1; /* Data Ptr points to a SG Packet	*/
	unchar Resrvd:1;	/* RFU					*/
	unchar Interpret:1;	/* Interpret the SCSI cdb of own use	*/
	unchar DataOut:1;	/* Data Out phase with command		*/
	unchar DataIn:1;	/* Data In phase with command		*/
};

/*
 * The DPT 20xx Host Adapter Command Control Block (CCB)
 */

typedef struct dpt_ccb {
	/* Begin EATA Command Packet Portion 				*/
	union {
		struct cp_bits b_bit;
		u_char b_byte;
	} ccb_option;
	u_char  ccb_senselen;	/* AutoRequestSense Data length.	*/
	u_char  ccb_resv[5];
	u_char  ccb_id;		/* Target SCSI ID, if no interpret.   	*/
	u_char  ccb_msg0;	/* Identify and Disconnect Message.   	*/
	u_char  ccb_msg1;
	u_char  ccb_msg2;
	u_char  ccb_msg3;
	u_char  ccb_cdb[12];	/* SCSI cdb for command.		*/
	u_char	ccb_datalen[4];	/* Data length in bytes for command.  	*/
	struct  dpt_ccb *ccb_vp; /* Command Packet Virtual Pointer.	*/
	u_char 	ccb_datap[4];   /* Data Physical Address for command. 	*/
	u_char 	ccb_statp[4];   /* Status Packet Physical Address.    	*/
	u_char 	ccb_sensep[4];  /* AutoRequestSense data Phy Address. 	*/
	/* End EATA Command Packet Portion 				*/

	u_char  ccb_ctlrstat;   /* Ctlr Status after ccb complete	*/
	u_char  ccb_scsistat;   /* SCSI Status after ccb complete	*/

	u_char	ccb_cdblen;	/* cdb length				*/
	u_char  ccb_scatgath_siz;

	struct	scsi_arq_status ccb_sense; /* Auto_Request sense blk	*/
	struct  dpt_sg ccb_sg_list[DPT_MAX_DMA_SEGS]; /* scatter/gather	*/
	struct  dpt_gcmd_wrapper *ccb_ownerp; /* ptr to the scsi_cmd	*/
	paddr_t ccb_paddr;	/* ccb physical address.		*/
	ushort	ccb_ioaddr;	/* hba io base address for usr commands */
}ccb_t;

#define	ccb_optbit	ccb_option.b_bit
#define	ccb_optbyte	ccb_option.b_byte

/*
 * Wrapper for HBA per-packet driver-private data area so that the
 * gcmd_t struture is linkted to the IOBP-alloced ccb_t
 */

typedef struct dpt_gcmd_wrapper {
	ccb_t	*dw_ccbp;
	gcmd_t	 dw_gcmd;
} dwrap_t;

#define	GCMDP2DWRAP(gcmdp)	((dwrap_t *)(gcmdp)->cmd_private)
#define	GCMDP2CCBP(gcmdp)	GCMDP2DWRAP(gcmdp)->dw_ccbp
#define	PKTP2CCBP(pktp)		GCMDP2CCBP(PKTP2GCMDP(pktp))

#define	CCBP2GCMDP(ccbp)	(&(ccbp)->ccb_ownerp->dw_gcmd)


/* StatusPacket data structure - this structure is returned by the 	*/
/* controller upon command completion. It contains status, message info */
/* and pointer to the initiating command ccb (virtual).			*/
struct 	dpt_stat {
	u_char	sp_hastat;		/* Controller Status message.	*/
	u_char	sp_scsi_stat;		/* SCSI Bus Status message.	*/
	u_char	reserved[2];
	u_char	sp_inv_residue[4];	/* how much was not xferred	*/
	struct	dpt_ccb *sp_vp;		/* Command pkt Virtual Pointer.	*/
	u_char	sp_ID_Message;
	u_char	sp_Que_Message;
	u_char	sp_Tag_Message;
	u_char	sp_Messages[9];
};

struct  dpt_blk {
	struct dpt_stat 	db_stat; /* hardware completion struct 	*/
	paddr_t			db_stat_paddr; /* its phy addr		*/

	kmutex_t 		db_mutex;	/* overall driver mutex */
	kmutex_t 		db_rmutex;	/* pkt resource mutex */

	ccc_t			db_ccc;		/* CCB timer control */
	Que_t			db_doneq;
	kmutex_t 		db_doneq_mutex;

	dev_info_t 		*db_dip;
	ddi_iblock_cookie_t	db_iblock;

	ushort			db_ioaddr;
	ushort  		db_scatgath_siz;

	u_char			db_targetid;
	u_char			db_intr;
	u_char			db_dmachan;
	u_char			db_numdev;


	struct	dpt_ccb 	*db_ccbp;
	ddi_dma_lim_t		*db_limp;
	ushort  		db_q_siz;
	ushort  		db_child;
	u_char			db_max_target;
	u_char			db_max_lun;
#ifdef DPT_DEBUG
	int			db_ccb_cnt;
	struct	dpt_ccb		*db_ccboutp;
#endif
};

struct dpt_unit {
	ddi_dma_lim_t	du_lim;
	unsigned du_arq : 1;		/* auto-request sense enable	*/
	unsigned du_tagque : 1;		/* tagged queueing enable	*/
	unsigned du_resv : 6;
	int	du_total_sectors;	/* total capacity in sectors	*/
};


#define	DPT_DIP(dpt)		((dpt)->d_blkp)->db_dip

#define	TRAN2DPTP(tranp)	((struct dpt *)(TRAN2TARGET(tranp)))
#define	TRAN2DPTBLKP(tranp)	TRAN2DPTP(tranp)->d_blkp
#define	TRAN2DPTUNITP(tranp)	TRAN2DPTP(tranp)->d_unitp

#define	PKT2DPT(pktp)		TRAN2DPTP(PKT2TRAN(pktp))
#define	PKT2DPTUNITP(pktp)	TRAN2DPTUNITP(PKT2TRAN(pktp))
#define	PKT2DPTBLKP(pktp)	TRAN2DPTBLKP(PKT2TRAN(pktp))

#define	ADDR2DPTUNITP(ap)	TRAN2DPTUNITP(ADDR2TRAN(ap))
#define	ADDR2DPTBLKP(ap)	TRAN2DPTBLKP(ADDR2TRAN(ap))

#define	SDEV2DPTP(sd)		TRAN2DPTP(SDEV2TRAN(sd))


/* used to get dpt_blk from dpt struct from upper layers */
#define	DPT_BLKP(X) (((struct dpt *)(X))->d_blkp)
struct dpt {
	struct scsi_hba_tran	*d_tran;
	struct dpt_blk		*d_blkp;
	struct dpt_unit		*d_unitp;
};

#define	DPT_MAX_CTRLS		18
typedef struct {
	u_short dc_number;
	u_short dc_addr[DPT_MAX_CTRLS];
	struct dpt_blk  *dc_blkp[DPT_MAX_CTRLS];
} dpt_controllers_t;

#define	HBA_BUS_TYPE_LENGTH 32	/* Length For Bus Type Field		*/

/* ReadConfig data structure - this structure contains the EATA Configuration */
struct ReadConfig {
	unchar ConfigLength[4];	/* Len in bytes after this field.	*/
	unchar EATAsignature[4];
	unchar EATAversion;

	unchar OverLapCmds:1;	/* TRUE if overlapped cmds supported	*/
	unchar TargetMode:1;	/* TRUE if target mode supported	*/
	unchar TrunNotNec:1;
	unchar MoreSupported:1;
	unchar DMAsupported:1;	/* TRUE if DMA Mode Supported		*/
	unchar DMAChannelValid:1; /* TRUE if DMA Channel is Valid	*/
	unchar ATAdevice:1;
	unchar HBAvalid:1;	/* TRUE if HBA field is valid		*/

	unchar PadLength[2];
	unchar HBA[4];
	unchar CPlength[4];	/* Command Packet Length		*/
	unchar SPlength[4];	/* Status Packet Length			*/
	unchar QueueSize[2];	/* Controller Que depth			*/
	unchar Reserved[2];	/* Reserved Field			*/
	unchar SG_Size[2];

	unchar IRQ_Number:4;	/* IRQ Ctlr is on ... ie 14,15,12	*/
	unchar IRQ_Trigger:1;	/* 0 =Edge Trigger, 1 =Level Trigger	*/
	unchar Secondary:1;	/* TRUE if ctlr not parked on 0x1F0	*/
	unchar DMA_Channel:2;	/* DMA Channel used if PM2011		*/

	unchar Reserved0;	/* Reserved Field			*/

	unchar	Disable:1;	/* Secondary I/O Address Disabled	*/
	unchar ForceAddr:1;	/* PCI Forced To An EISA/ISA Address	*/
	unchar	Reserved1:6;	/* Reserved Field			*/

	unchar	MaxScsiID:5;	/* Maximun SCSI Target ID Supported	*/
	unchar	MaxChannel:3;	/* Maximum Channel Number Supported	*/

	unchar	MaxLUN;		/* Maximun LUN Supported		*/

	unchar	Reserved2:6;	/* Reserved Field			*/
	unchar	PCIbus:1;	/* PCI Adapter Flag			*/
	unchar	EISAbus:1;	/* EISA Adapter				*/
	unchar	RaidNum;	/* RAID HBA Number For Stripping	*/
	unchar	Reserved3;	/* Reserved Field			*/
};

/* Controller IO Register Offset Definitions				*/
#define	HA_COMMAND	0x07	/* Command register			*/
#define	HA_STATUS	0x07	/* Status register			*/
#define	HA_DMA_BASE	0x02	/* LSB for DMA Physical Address 	*/
#define	HA_ERROR	0x01	/* Error register			*/
#define	HA_DATA		0x00	/* Data In/Out register			*/
#define	HA_AUX_STATUS   0x08	/* Auxillary Status Reg on 2012		*/
#define	HA_IMMED_MOD	0x05	/* Immediate Modifier Register		*/
#define	HA_IMMED_FUNC	0x06	/* Immediate Function Register		*/

/* for EATA immediate commands */
#define	HA_GEN_CODE		0x06
#define	HA_GEN_ABORT_TARGET	0x05
#define	HA_GEN_ABORT_LUN	0x04

#define	HA_AUX_BUSY	0x01	/* Aux Reg Busy bit.			*/
#define	HA_AUX_INTR	0x02	/* Aux Reg Interrupt Pending.		*/

#define	HA_ST_ERROR	0x01	/* HA_STATUS register bit defs		*/
#define	HA_ST_INDEX	0x02
#define	HA_ST_CORRCTD   0x04
#define	HA_ST_DRQ	0x08
#define	HA_ST_SEEK_COMP 0x10
#define	HA_ST_WRT_FLT   0x20
#define	HA_ST_READY	0x40
#define	HA_ST_BUSY	0x80

#define	HA_ST_DATA_RDY  HA_ST_SEEK_COMP + HA_ST_READY + HA_ST_DRQ

#define	HA_SELTO	0x01

/* Controller Command Definitions					*/
#define	CP_READ_CFG_PIO 0xF0
#define	CP_PIO_CMD	0xF2
#define	CP_TRUCATE_CMD  0xF4
#define	CP_READ_CFG_DMA 0xFD
#define	CP_SET_CFG_DMA  0xFE
#define	CP_EATA_RESET   0xF9
#define	CP_DMA_CMD	0xFF
#define	ECS_EMULATE_SEN 0xD4
#define	CP_EATA_IMMED   0xFA

/* EATA Immediate Sub Functions						*/
#define	CP_EI_ABORT_MSG		0x00
#define	CP_EI_RESET_MSG		0x01
#define	CP_EI_RESET_BUS		0x02
#define	CP_EI_ABORT_CP		0x03
#define	CP_EI_INTERRUPTS	0x04
#define	CP_EI_MOD_ENABLE	0x00
#define	CP_EI_MOD_DISABLE	0x01
#define	CP_EI_RESET_BUSES	0x09


/* EATA Command/Status Packet Definitions				*/
#define	HA_DATA_IN	0x80
#define	HA_DATA_OUT	0x40
#define	HA_CP_QUICK	0x10
#define	HA_SCATTER	0x08
#define	HA_AUTO_REQSEN	0x04
#define	HA_HBA_INIT	0x02
#define	HA_SCSI_RESET	0x01

#define	HA_STATUS_MASK  0x7F
#define	HA_IDENTIFY_MSG 0x80
#define	HA_DISCO_RICO   0x40

#define	FLUSH_CACHE_CMD		0x35		 /* Synchronize Cache Command */

#define	SCBP(pkt)	((struct scsi_status *)(pkt)->pkt_scbp)
#define	SCBP_C(pkt)	((*(pkt)->pkt_scbp) & STATUS_MASK)

#define	PCI_DPT_VEND_ID	0x1044		/* DPT Vendor ID   */
#define	PCI_DPT_DEV_ID	0xa400		/* DPT Device ID   */
#define	MAX_PCI_BUS	1
#define	MAX_PCI_DEVICE	32

#define	DPT_PCI_ADAPTER		0
#define	DPT_EISA_ADAPTER	1
#define	DPT_ISA_ADAPTER		2

#ifdef	__cplusplus
}
#endif

#endif /* _SYS_DKTP_DPT_H */
