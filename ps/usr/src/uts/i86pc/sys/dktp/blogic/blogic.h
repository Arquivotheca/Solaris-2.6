/*
*  blogic.h
*
*  BusLogic Device Driver Header file for SunSoft Solaris x86
*
*  Copyright (c) 1995, BusLogic, Inc.
*  Copyright (c) 1995, Sun Microsystems, Inc.
*  All rights reserved.
*/

#ident "@(#)blogic.h	1.5 95/06/07"

#ifndef _SYS_DKTP_BLOGIC_H
#define _SYS_DKTP_BLOGIC_H

#ifdef	__cplusplus
extern "C" {
#endif

#define BLOGIC_KVTOP(vaddr) (HBA_KVTOP((vaddr), blogic_pgshf, blogic_pgmsk))

#define BLOGIC_MAX_DMA_SEGS	32      /* Max Scatter/Gather segments	*/
#define BLOGIC_MAX_NUM_MBOX	40      /* Max mailbox's/ccb's		*/
#define BLOGIC_MIN_NUM_MBOX	16      /* Min mailbox's/ccb's		*/
#define BLOGIC_MAX_RETRY	4       /* Max times to retry cmd	*/
#define BLOGIC_SENSE_LEN	14 	/* Max sense bytes 		*/
#define BLOGIC_TIMEOUT_SECS	15	/* Default timeout for cmd	*/
#define BLOGIC_RETRY_MAX	4	/* Default retry cnt for cmd	*/
#define BLOGIC_RESET_MAX	2	/* Default reset cnt for reset	*/
#define BLOGIC_MAX_TARG_NARROW	8	/* Max # targs for narrow scsi	*/
#define BLOGIC_MAX_TARG_WIDE	16	/* Max # targs for wide scsi	*/
#define BLOGIC_MAX_LUN_NARROW	8	/* Max # luns for narrow scsi	*/
#define BLOGIC_MAX_LUN_WIDE	64	/* Max # luns for wide scsi	*/

/*
*  Scatter/Gather DMA Segment Descriptor
*/
struct blogic_dma_seg {
	ulong   data_len;     /* segment length 			*/
	ulong   data_ptr;     /* segment address 			*/
};

/*
*  The BusLogic Host Adapter Command Control Block (CCB)
*/
#pragma pack(1)
struct blogic_ccb {
	unchar  ccb_op;         /* CCB Operation Code 			*/
	unchar 	ccb_datadir;	/* +1 Data direction			*/
	unchar  ccb_cdblen;     /* +2 Length of SCSI CDB		*/
	unchar	ccb_senselen;	/* +3 max. bytes of sense to xfer	*/
	ulong   ccb_datalen;	/* +4 Data Length (lsb - msb) 		*/
	ulong   ccb_datap;	/* +8 Data (buffer) pointer (lsb-msb)	*/
	ushort	ccb_res;	/* +12,13 reserved			*/
	unchar  ccb_hastat;     /* +14 Host Adapter status 		*/
	unchar  ccb_tarstat;    /* +15 Target Status 			*/
	unchar	ccb_tf_tid;	/* +16 target ID 			*/
	unchar	ccb_tf_lun;	/* +17 LUN and Tag bits			*/
	unchar  ccb_cdb[HBA_MAX_CDB_LEN]; /* +18 -> 29 */
	unchar	ccb_control;
	unchar  ccb_linkid;	/* Command Link ID 			*/
	ulong	ccb_link;	/* Link Pointer (lsb - msb) 		*/
	ulong	ccb_sensep;	/* Sense pointer (lsb - msb)		*/

	paddr_t			ccb_paddr;	/* physical address	*/
	struct	blogic_ccb	*ccb_forw;	/* forward link pointers*/
	struct  scsi_cmd	*ccb_ownerp;	/* owner pointer	*/
	struct blogic_dma_seg	*ccb_sg_list;	/* ptr to sg list	*/
	struct scsi_arq_status	ccb_sense;	/* Auto_Request sense blk */
	time_t			ccb_starttime;
	unchar	ccb_retrycnt;		/* # times cmd has been retried	*/
	unchar	ccb_abortcnt;		/* # times cmd has been aborted	*/
	unchar	ccb_flag;
	int			ccb_timeout_id;
#ifdef BLOGIC_DEBUG
	unchar	ccb_dbg_idx;
	unchar	ccb_dbg_fill[3];
#endif
	unchar	ccb_end;
};
#pragma pack()


/*
*  ccb status code
*/
#define CCB_RSCACHE	0x0001		/* request sense data valid	 */
#define CCB_PGVALID	0x0002		/* ccb within a page		 */

/*
*  Direction bits for ccb_datadir
*/
#define CTD_MASK        0x18    /* The 2 direction bits */
#define CTD_IN          0x8     /* The ccb_tf_in bit */
#define CTD_OUT         0x10    /* The ccb_tf_out bit */

/*
*  ccb_op values
*/
#define COP_COMMAND     0       /* Normal SCSI Command 			*/
#define COP_SCATGATH    2       /* SCSI Command with Scatter/Gather 	*/
#define COP_CMD_RESID   3       /* Command with residual		*/
#define COP_SG_RESID    4       /* Command with Scatter/Gather resid	*/	
#define COP_RESET       0x81    /* Bus Device Reset (Aborts all 	*/

/*
*  LUN byte definitions for Narrow SCSI
*  Datadir byte definitions for Wide SCSI
*/
#define QUEUEHEAD       0x60            /* Head of Queue tag */
#define ORDERED         0xA0            /* Ordered Queue tag */
#define SIMPLE          0x20            /* Simple Queue tag  */

/*
*  ccb_flag defines
*/
#define ACTIVE		0x01
#define TIMEDOUT	0x02

struct  blogic_blk {
	kmutex_t		bb_mutex;
	dev_info_t		*bb_dip;
	void			*bb_iblock;
	ushort			bb_base_ioaddr;
	ushort			bb_datacmd_ioaddr;
	ushort			bb_intr_ioaddr;
	unchar			bb_boardtype;
 	unchar			bb_flag;
	unchar			bb_targetid;
   	unchar			bb_bdid;
	unchar			bb_intr;
	unchar			bb_dmachan;
	unchar			bb_dmaspeed;
	unchar			bb_buson;
	unchar			bb_busoff;
	unchar			bb_fwrev;
	unchar			bb_fwver;
	unchar			bb_active_cnt;
	unchar			bb_numdev;
   	unchar			bb_num_mbox;
	unchar			bb_reset_cnt;
	unchar			bb_reset_max;
	unchar			bb_retry_max;
	int			bb_timeout_id;
	long			bb_timeout_ticks;
	unchar			bb_dev_type[BLOGIC_MAX_TARG_WIDE];

	struct mbox_entry	*bb_CurrMboxOut;
	struct mbox_entry	*bb_FirstMboxOut;
	struct mbox_entry	*bb_LastMboxOut;
	struct mbox_entry	*bb_CurrMboxIn; 
	struct mbox_entry	*bb_FirstMboxIn;
	struct mbox_entry	*bb_LastMboxIn;

	uint			bb_actm_stat;
	struct scsi_inquiry	*bb_inqp;
	caddr_t			bb_ccblist;
	paddr_t			bb_pccblist;
	struct blogic_ccb	*bb_ccb_freelist;
	caddr_t			bb_dmalist;
	ushort  		bb_child;
	short  			bb_resv;
	opaque_t		bb_cbthdl;
	int			bb_dma_reqsize;
	ddi_dma_lim_t 		*bb_dmalim_p;
	caddr_t 		bb_blogicpkt;
	int 			blogic_cb_id;
	int			bb_intr_idx;
	unchar			*bb_buf;
	unchar			bb_max_targs;
	unchar			bb_max_luns;
};
#define bb_ctrl_ioaddr bb_base_ioaddr
#define bb_stat_ioaddr bb_base_ioaddr

#define ISA_HBA		0
#define EISA_HBA 	1
#define MCA_HBA		2
#define PCI_HBA		3

/*
*  bb_flag definitions
*/
#define TAG_Q_SUPPORTED 0x01
#define TAG_Q_OFF	0x02
#define VAR_XLAT_SCHEME 0x04
#define INSTALLED	0x08
#define WIDE_SCSI	0x10

/*
*  bb_dev_type definitions
*/
#define TAPE_DEVICE		0x01
#define SLOW_DEVICE		0x02
#define RESET_CONDITION		0x80

/*
*  Macros to convert ccb virtual to physical addr and vice versa.
*/
#define BLOGIC_CCB_KVTOPHYS(B, CCB)	\
	(paddr_t) ((paddr_t)(B->bb_pccblist) + \
	(paddr_t) ((caddr_t) CCB - B->bb_ccblist))

#define BLOGIC_CCB_PHYSTOKV(B, PCCB)	\
	(caddr_t) (B->bb_ccblist + \
	((paddr_t) PCCB - (paddr_t) (B->bb_pccblist)))


struct blogic_unit {
	ddi_dma_lim_t	au_lim;
	unsigned 	au_arq : 1;	/* auto-request sense enable	*/
	unsigned 	au_tagque : 1;	/* tagged queueing enable	*/
	unsigned 	au_resv : 6;
	ulong		au_capacity;	/* capacity in total sectors */
};

#define	BLOGIC_DIP(blogic)		(((blogic)->a_blkp)->bb_dip)

#define	TRAN2HBA(hba)		((struct blogic *)(hba)->tran_hba_private)
#define	SDEV2HBA(sd)		(TRAN2HBA(SDEV2TRAN(sd)))

#define	TRAN2BLOGIC(hba)		((struct blogic *)(hba)->tran_tgt_private)
#define	TRAN2BLOGICBLKP(hba)	((TRAN2BLOGIC(hba))->a_blkp)
#define	TRAN2BLOGICUNITP(hba)	((TRAN2BLOGIC(hba))->a_unitp)
#define	SDEV2BLOGIC(sd)		(TRAN2BLOGIC(SDEV2TRAN(sd)))
#define	PKT2BLOGICUNITP(pktp)	(TRAN2BLOGICUNITP(PKT2TRAN(pktp)))
#define	PKT2BLOGICBLKP(pktp)	(TRAN2BLOGICBLKP(PKT2TRAN(pktp)))
#define	ADDR2BLOGICUNITP(ap)	(TRAN2BLOGICUNITP(ADDR2TRAN(ap)))
#define	ADDR2BLOGICBLKP(ap)	(TRAN2BLOGICBLKP(ADDR2TRAN(ap)))
#define	BLOGIC_BLKP(X) 		(((struct blogic *)(X))->a_blkp)

struct blogic {
	scsi_hba_tran_t		*a_tran;
	struct blogic_blk		*a_blkp;
	struct blogic_unit		*a_unitp;
};

/*
*  Adapter I/O ports.  These are offsets from ioaddr.
*/
#define BLOGICCTL		0	/* Host Adapter Control Port (WRITE) 	*/
#define BLOGICSTAT		0	/* Status Port (READ) 			*/
#define BLOGICDATACMD	1	/* Command (WRITE) and Data (READ/WRITE) Port */
#define BLOGICINTFLGS	2	/* Interrupt Flags Port (READ) 		*/
#define BLOGICBIOS	3	/* Extended Status Port (BIOS stuff)	*/

/*
* Bit definitions for base port (write direction):
*/
#define CTL_HRST        0x80    /* Hard Reset of Host Adapter 		*/
#define CTL_SRST        0x40    /* Soft Reset 				*/
#define CTL_IRST        0x20    /* Interrupt Reset (clears blogicINTFLGS) 	*/
#define CTL_SCRST       0x10    /* Reset SCSI Bus 			*/

/*
* Bit definitions for base port (read direction):
*/
#define STAT_STST       0x80    /* Self-test in progress 		*/
#define STAT_DIAGF      0x40    /* Internal Diagnostic Failure 		*/
#define STAT_INIT       0x20    /* Mailbox Init required 		*/
#define STAT_IDLE       0x10    /* Adapter is Idle 			*/
#define STAT_CDF        0x08    /* BLOGICDATACMD (outgoing) is full 	*/
#define STAT_DF         0x04    /* BLOGICDATACMD (incoming) is full 	*/
#define STAT_INVDCMD    0x01    /* Invalid host adapter command 	*/
#define STAT_MASK       0xfd    /* mask for valid status bits 		*/

/*
* Bit definitions for blogic interrupt status port (base +2):
*/
#define INT_ANY         0x80    /* Any interrupt (set when bits 0-3 valid) */
#define INT_SCRD        0x08    /* SCSI reset detected 			*/
#define INT_HACC        0x04    /* Host Adapter command complete 	*/
#define INT_MBOE        0x02    /* MailBox (outgoing) Empty 		*/
#define INT_MBIF        0x01    /* MailBox (incoming) Mail Ready	*/

#define CMD_XMASK       0x80    /* Mask to test for extended command */

/*
*  BusLogic Host Adapter Command Opcodes:
*
*  NOTE: for all multi-byte values sent in BLOGICDATACMD,
*  MSB is sent FIRST
*/

#define CMD_NOP         0x00    /* No operation, sets INT_HACC 		*/
#define CMD_MBXINIT     0x01    /* Initialize Mailboxes 		*/
				/*   ARGS: count, 1-255 valid 		*/
				/*         mbox addr (3 bytes) 		*/
#define CMD_DOSCSI      0x02    /* Start SCSI (Scan outgoing mailboxes) */
#define CMD_ATBIOS      0x03    /* Start AT BIOS command 		*/
#define CMD_ADINQ       0x04    /* Adapter Inquiry 			*/
				/*   RETURNS: 4 bytes of firmware info 	*/
#define CMD_MBOE_CTL    0x05    /* Enable/Disable INT_MBOE interrupt 	*/
				/*   ARG: 0 - Disable, 1 - Enable 	*/
#define CMD_SELTO_CTL   0x06    /* Set SCSI Selection Time Out 		*/
				/*   ARGS: 0 - TO off, 1 - TO on	*/
				/*         Reserved (0)			*/
				/*   Time-out value (in ms, 2 bytes)	*/
#define CMD_BONTIME     0x07    /* Set Bus-ON Time 			*/
				/*   ARG: time in microsec, 2-15 valid 	*/
#define CMD_BOFFTIME    0x08    /* Set Bus-OFF Time 			*/
				/*   ARG: time in microsec, 0-250 valid	*/
#define CMD_XFERSPEED   0x09    /* Set AT bus burst rate (MB/sec) 	*/
				/*   ARG: 0 - 5MB, 1 - 6.7 MB, 		*/
				/*   2 - 8MB, 3 - 10MB 			*/
				/*   4 - 5.7MB (1542 ONLY!) 		*/
#define CMD_INSTDEV     0x0a    /* Return Installed Devices 		*/
				/*   RETURNS: 8 bytes, one per target	*/
				/*   ID, start with 0.  Bits set	*/
				/*   (bitpos=LUN) in each indicate Unit */
				/*   Ready				*/
#define CMD_CONFIG      0x0b    /* Return Configuration Data 		*/
				/*   RETURNS: 3 bytes, which are: 	*/
				/*   byte 0: DMA Channel, bit encoded as*/
  #define CFG_DMA_CH5	0x20	/*              Channel 5 		*/
  #define CFG_DMA_CH6	0x40	/*              Channel 6 		*/
  #define CFG_CMD_CH7	0x80	/*              Channel 7 		*/
  #define CFG_DMA_MASK	0xe0	/*              (mask for above) 	*/
				/*   byte 1: Interrupt Channel, 	*/
  #define CFG_INT_CH9	0x01	/*              Channel 9 		*/
  #define CFG_INT_CH10	0x02	/*              Channel 10 		*/
  #define CFG_INT_CH11	0x04	/*              Channel 11 		*/
  #define CFG_INT_CH12	0x08	/*              Channel 12 		*/
  #define CFG_INT_CH14	0x20	/*              Channel 14 		*/
  #define CFG_INT_CH15	0x40	/*              Channel 15 		*/
  #define CFG_INT_MASK	0x6f	/*              (mask for above) 	*/
				/*   byte 2: HBA SCSI ID, in binary	*/
#define CFG_ID_MASK     0x03    /*              (mask for above) 	*/
#define CMD_WTFIFO      0x1c    /* Write Adapter FIFO Buffer 		*/
#define CMD_RDFIFO      0x1d    /* Read Adapter FIFO Buffer 		*/
#define CMD_ECHO        0x1f    /* Echo Data 				*/
				/*   ARG: one byte of test data 	*/
				/*   RETURNS: the same byte (hopefully)	*/
#define CMD_XMBXINIT	0x81	/* Extended Initialize Mailboxes 	*/
				/* ARGS: count, 1-255 valid 		*/
				/*       mbox addr (long) 		*/
#define CMD_XINQSETUP	0x8d	/* Return extended setup data		*/
#define CMD_XSTRICT_RND_RBN	0x8f	/* Notify f/w that driver will	*/
				/*   perform strict round-robin scheme	*/
#define CMD_DISAB_ISA_PORT 0x95	/* Disable ISA-Compatible I/O port range*/
#define CMD_ENAB_64_LUN	0x96	/* Enable 16 Targ/64 Lun support	*/

/*
*  The Mail Box Structure.
*/
struct  mbox_entry {
	ulong	mbx_ccb_addr;	/* blogic-style CCB address	 	*/
	unchar	mbx_hbastat;
	unchar	mbx_scsistat;
	unchar	mbx_reserved;
	unchar  mbx_cmdstat;	/* Command/Status byte (below)		*/
};

/*
*  Command codes for mbx_cmdstat:
*/
#define MBX_FREE        0       /* Available mailbox slot 		*/
#define MBX_CMD_START   1       /* Start SCSI command described by CCB 	*/
#define MBX_CMD_ABORT   2       /* Abort SCSI command described by CCB 	*/
#define MBX_STAT_DONE   1       /* CCB completed without error 		*/
#define MBX_STAT_ABORT  2       /* CCB was aborted by host 		*/
#define MBX_STAT_CCBNF  3       /* CCB for ABORT request not found 	*/
#define MBX_STAT_ERROR  4       /* CCB completed with error 		*/

#define NO_RECORD_OF_CCB 0xFF

/*
*  ccb_hastat values:
*/
#define HS_OK           0x00    /* No host adapter detected error 	*/
#define HS_SELTO        0x11    /* Selection Time Out 			*/
#define HS_DATARUN      0x12    /* Data over/under-run 			*/
#define HS_BADFREE      0x13    /* Unexpected Bus Free 			*/
#define HS_BUSPHASE     0x14    /* Target bus phase sequence failure 	*/
#define HS_BADCCB       0x16    /* invalid ccb operation code		*/
#define HS_BADLINK      0x17    /* linked CCB with diff. lun		*/
#define HS_BADTARGDIR   0x18    /* invalid target direction from host	*/
#define HS_DUPCCB	0x19	/* duplicate ccb			*/
#define HS_BADSEG	0x1a	/* invalid ccb or segment list param	*/
#define HS_UNKNOWN_ERR  0x01

/*
*  blogic command structure.  For each command, we have flags, a number
*  of data bytes output, and a number of data bytes input.
*  This table is used by blogic_docmd.
*/
struct blogic_cmd {
	unchar  ac_flags;       /* flag bits (below) 			*/
	unchar  ac_args;        /* # of argument bytes 			*/
	unchar  ac_vals;        /* number of value bytes 		*/
};

#define BLOGIC_VENDOR_ID	0x104b

/*
*  Macros to grab bus, device, function, and chan numbers
*  from the blogic.conf file.
*/
#define PCI_CONF_BUSNO(x)	(((x) & 0xFF00) >> 8)
#define PCI_CONF_DEVNO(x)	(((x) & 0x00F8) >> 3)
#define PCI_CONF_FUNCNO(x)	(((x) & 0x0007))

#ifdef	__cplusplus
}
#endif

#endif  /* _SYS_DKTP_BLOGIC_H */
