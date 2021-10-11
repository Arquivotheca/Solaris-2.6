/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 */

#ifndef _SYS_DKTP_NCRS_NCR_H
#define	_SYS_DKTP_NCRS_NCR_H

#pragma	ident	"@(#)ncr.h	1.9	95/12/12 SMI"

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * #define	NCR_DEBUG
 */
#if defined(__ppc)
#define	printf	prom_printf		/* vla fornow */
#endif

#include <sys/types.h>
#include <sys/ddidmareq.h>
#include <sys/modctl.h>
#include <sys/sunddi.h>
#include <sys/dktp/hba.h>
#include <sys/scsi/scsi.h>
#include <sys/scsi/impl/transport.h>

typedef	unsigned char bool_t;
typedef	ulong	ioadr_t;

#include <sys/dktp/ncrs/interrupt.h>
#include <sys/dktp/ncrs/debug.h>

#include <sys/pci.h>

#ifdef  PCI_DDI_EMULATION
#define ddi_io_getb(a, b)			inb(b)
#define ddi_io_getw(a, b)			inw(b)
#define ddi_io_getl(a, b)			inl(b)
#define ddi_io_putb(a, b, c)			outb(b, c)
#define ddi_io_putw(a, b, c)			outw(b, c)
#define ddi_io_putl(a, b, c)			outl(b, c)
#define ddi_regs_map_setup(a, b, c, d, e, f)	DDI_SUCCESS
#define ddi_regs_map_free(a)
#endif

#define	NCR_PCI_RNUMBER		1
#define	NCR_EISA_RNUMBER	0

/*
 * If your HBA supports DMA or bus-mastering, you may have your own
 * scatter-gather list for physically non-contiguous memory in one
 * I/O operation; if so, there's probably a size for that list.
 * It must be placed in the ddi_dma_lim_t structure, so that the system
 * DMA-support routines can use it to break up the I/O request, so we
 * define it here.
 */

#define	NCR_MAX_DMA_SEGS	17

/*
 * Scatter-gather list structure defined by HBA hardware, for
 * loop in ncr_dmaget()
 */

typedef	struct NcrTableIndirect {	/* Table Indirect entries */
	ulong   count;		/* 24 bit count */
	paddr_t address;	/* 32 bit physical address */
} ncrti_t;

typedef	struct DataPointers {
	ncrti_t	nd_data[NCR_MAX_DMA_SEGS];	/* Pointers to data buffers */
	unchar	nd_left;		/* number of entries left to process */
	unchar	nd_num;			/* # of filled entries in S/G table */
	ushort	nd_pad;			/* pad to 8 byte multiple */
} ntd_t;



/*
 * Information setup to describe a request
 */
typedef	struct	ncr_ccb {
	struct scsi_cmd	 nc_cmd;	/* common packet wrapper */
	union scsi_cdb	 nc_cdb;	/* scsi command description block */
	struct ncr_ccb	*nc_linkp;	/* doneq linked list ptr */

	ncrti_t	nc_sg[NCR_MAX_DMA_SEGS]; /* scatter/gather structure */
	unchar	nc_num;			/* number of S/G list entries */
	unchar	nc_cdblen;		/* actual length of cdb */
	unchar	nc_type;		/* type of request */
	bool_t	nc_queued;		/* TRUE if on waitq */
} nccb_t;


/*
 * Information maintained about each (target,lun) the HBA serves.
 */

typedef struct ncr_unit {
	/* DSA reg points here when this target is active */
	/* Table Indirect pointers for NCR SCRIPTS */
	struct	{
		unchar	nt_pad0;	/* currently unused */
		unchar	nt_sxfer;	/* SCSI transfer/period parms */
		unchar	nt_sdid;	/* SCSI destination ID for SELECT */
		unchar	nt_scntl3;	/* currently unused */
	} nt_selectparm;

	ncrti_t	nt_sendmsg;		/* pointer to sendmsg buffer */
	ncrti_t	nt_rcvmsg;		/* pointer to msginbuf */
	ncrti_t	nt_cmd;			/* pointer to cdb buffer */
	ncrti_t	nt_status;		/* pointer to status buffer */
	ncrti_t	nt_extmsg;		/* pointer to extended message buffer */
	ncrti_t	nt_syncin;		/* pointer to sync in buffer */
	ncrti_t	nt_syncout;		/* pointer to sync out buffer */
	ncrti_t	nt_errmsg;		/* pointer to message reject buffer */
	ntd_t	nt_curdp;		/* current S/G data pointers */
	ntd_t	nt_savedp;		/* saved S/G data pointers */

	nccb_t	*nt_nccbp;		/* ccb for active request */
	nccb_t	*nt_waitq;		/* queue of pending requests */
	nccb_t	**nt_waitqtail;		/* queue tail ptr */

	struct	ncr_unit *nt_linkp;	/* wait queue link pointer */
	paddr_t	nt_dsa_physaddr;	/* physical addr of table indirects */

	/* these are the buffers the HBA actually does i/o to/from */
	union scsi_cdb	nt_cdb;		/* scsi command description block */
	/* keep these two together so HBA can transmit in single move */
	struct {
		unchar	ntu_id[1];	/* 1 byte identify msg */
		unchar	ntu_sync[5];	/* sdtr outbound message */
	} nt_id;
#define	nt_identify	nt_id.ntu_id
#define	nt_syncobuf	nt_id.ntu_sync
#define	nt_abortmsg	nt_id.ntu_sync[0]
	unchar	nt_msginbuf[1];		/* first byte of message in */
	unchar	nt_extmsgbuf[1];	/* length of extended message */
	unchar	nt_syncibuf[3];		/* room for sdtr inbound message */
	unchar	nt_statbuf[1];		/* status byte */
	unchar	nt_errmsgbuf[1];	/* error message for target */

	ddi_dma_lim_t	nt_dma_lim;	/* per-target for sector size */
	unchar	nt_state;		/* current state of this target */
	unchar	nt_type;		/* type of request */

	bool_t	nt_goterror;		/* true if error occurred */
	unchar	nt_dma_status;		/* copy of DMA error bits */
	unchar	nt_scsi_status0;	/* copy of SCSI bus error bits */
	unchar	nt_scsi_status1;	/* copy of SCSI bus error bits */

	ushort	nt_target;		/* target number */
	ushort	nt_lun;			/* logical unit number */
	bool_t	nt_fastscsi;		/* true if > 5MB/sec, tp < 200ns */
	unchar	nt_sscfX10;		/* sync i/o clock divisor */

	unsigned 	nt_arq : 1;	/* auto-request sense enable */
	unsigned	nt_tagque : 1;	/* tagged queueing enable */
	unsigned	nt_resv : 6;

	ulong	nt_total_sectors;	/* total # of sectors on device */
} npt_t;

/*
 * The states a HBA to (target, lun) nexus can have are:
 */
#define	NPT_STATE_DONE		((unchar)0) /* processing complete */
#define	NPT_STATE_IDLE		((unchar)1) /* HBA is waiting for work */
#define	NPT_STATE_WAIT		((unchar)2) /* HBA is waiting for reselect */
#define	NPT_STATE_QUEUED	((unchar)3) /* target request is queued */
#define	NPT_STATE_DISCONNECTED	((unchar)4) /* disconnctd, wait for reconnect */
#define	NPT_STATE_ACTIVE	((unchar)5) /* this target is the active one */

/*
 * types of ccb requests stored in nc_type
 */
#define	NRQ_NORMAL_CMD		((unchar)0)	/* normal command */
#define	NRQ_ABORT		((unchar)1)	/* Abort message */
#define	NRQ_ABORT_TAG		((unchar)2)	/* Abort Tag message */
#define	NRQ_DEV_RESET		((unchar)3)	/* Bus Device Reset message */

/*
 * macro to get offset of npt_t members for compiling into the SCRIPT
 */
#define	NTOFFSET(label) ((long)&(((npt_t *)0)->label))

typedef	unchar	bus_t;
#define	BUS_TYPE_EISA		((bus_t)1)
#define	BUS_TYPE_PCI		((bus_t)2)

typedef struct ncr_blk {
	kmutex_t	 n_mutex;
	ddi_iblock_cookie_t n_iblock;
	dev_info_t	*n_dip;

	struct ncrops	*n_ops;		/* ptr to NCR SIOP ops table */
	npt_t	*n_pt[ NTARGETS * NLUNS_PER_TARGET ];
					/* ptrs to target DSA structures */

	caddr_t	 n_ptsave;		/* save ptr to per target buffer */
	size_t	 n_ptsize;		/* save the size of the buffer */

	npt_t	*n_current;		/* ptr to active target's DSA */
	npt_t	*n_forwp;		/* ptr to front of the wait queue */
	npt_t	*n_backp;		/* ptr to back of the wait queue */
	npt_t	*n_hbap;		/* the HBA's target struct */
	nccb_t	*n_doneq;		/* queue of completed commands */
	nccb_t	**n_donetail;		/* queue tail ptr */

	ddi_acc_handle_t	n_handle;
	int     *n_regp;		/* this hba's regprop */
	int     n_reglen;		/* this hba's regprop reglen */

	int	n_reg;
	u_int	n_inumber;		/* interrupts property index */
	unchar	n_irq;			/* interrupt request line */

	int	n_sclock;		/* hba's SCLK freq. in MHz */
	int	n_speriod;		/* hba's SCLK period, in nsec. */
	unchar	n_syncstate[NTARGETS];	/* sync i/o state flags */
	int	n_minperiod[NTARGETS];	/* minimum sync i/o period per target */
	unchar	n_scntl3;		/* 53c8xx hba's core clock divisor */

	unchar	n_initiatorid;		/* this hba's target number and ... */
	bool_t	n_nodisconnect[NTARGETS]; /* disable disconnects on target */
	ushort	n_idmask;		/* ... its corresponding bit mask */
	unchar	n_iden_msg[1];		/* buffer for identify messages */
	unchar	n_disc_num;		/* number of disconnected devs */
	unchar	n_state;		/* the HBA's current state */
	bool_t	n_is710;		/* TRUE if HBA is a 53c710 */
	opaque_t n_cbthdl;		/* callback thread */

	bus_t   n_bustype;		/* bustype */
	unchar  n_compaq;		/* compaq */
	unchar  n_geomtype;		/* compaq geometry type */
} ncr_t;

#define	NSTATE_IDLE		((unchar)0) /* HBA is idle */
#define	NSTATE_ACTIVE		((unchar)1) /* HBA is processing a target */
#define	NSTATE_WAIT_RESEL	((unchar)2) /* HBA is waiting for reselect */

/*
 * states of the hba while negotiating synchronous i/o with a target
 */
#define	NSYNCSTATE(ncrp, nptp)	(ncrp)->n_syncstate[(nptp)->nt_target]
#define	NSYNC_SDTR_NOTDONE	((unchar)0) /* SDTR negotiation needed */
#define	NSYNC_SDTR_SENT		((unchar)1) /* waiting for target SDTR msg */
#define	NSYNC_SDTR_RCVD		((unchar)2) /* target waiting for SDTR msg */
#define	NSYNC_SDTR_REJECT	((unchar)3) /* send Message Reject to target */
#define	NSYNC_SDTR_DONE		((unchar)4) /* final state */


/*
 * action codes for interrupt decode routines in interrupt.c
 */
#define	NACTION_DONE		0x01	/* target request is done */
#define	NACTION_ERR		0x02	/* target's request got error */
#define	NACTION_DO_BUS_RESET	0x04	/* reset the SCSI bus */
#define	NACTION_SAVE_BCNT	0x08	/* save scatter/gather byte ptr */
#define	NACTION_SIOP_HALT	0x10	/* halt the current HBA program */
#define	NACTION_SIOP_REINIT	0x20	/* totally reinitialize the HBA */
#define	NACTION_SDTR		0x40	/* got SDTR interrupt */
#define	NACTION_SDTR_OUT	0x80	/* send SDTR message */
#define	NACTION_ACK		0x100	/* ack the last byte and continue */
#define	NACTION_CHK_INTCODE	0x200	/* SCRIPTS software interrupt */
#define	NACTION_INITIATOR_ERROR	0x400	/* send IDE message and then continue */
#define	NACTION_MSG_PARITY	0x800	/* send MPE error and then continue */
#define	NACTION_MSG_REJECT	0x1000	/* send MR and then continue */
#define	NACTION_BUS_FREE	0x2000	/* reselect error caused disconnect */
#define	NACTION_ABORT		0x4000	/* abort an invalid reconnect */
#define	NACTION_GOT_BUS_RESET	0x8000	/* detected a scsi bus reset */


/*
 * This structure defines which bits in which registers must be saved
 * during a chip reset. The saved bits were established by the
 * HBA's POST BIOS and are very hardware dependent.
 */
typedef	struct	regsave710 {
	unchar	nr_reg;		/* the register number */
	unchar	nr_bits;	/* the bit mask */
} nrs_t;


#define	ncr_save_regs(ncrp, np, rp, n) \
		ncr_saverestore((ncrp), (np), (rp), (n), TRUE)

#define	ncr_restore_regs(ncrp, np, rp, n) \
		ncr_saverestore((ncrp), (np), (rp), (n), FALSE)

/*
 * Handy constants
 */

/* For returns from xxxcap() functions */

#define	FALSE		0
#define	TRUE		1
#define	UNDEFINED	-1

/*
 * Handy macros
 */
#if defined(__ppc)
#define	NCR_KVTOP(vaddr) \
		CPUPHYS_TO_IOPHYS( \
		((paddr_t)(hat_getkpfnum((caddr_t)(vaddr)) << (PAGESHIFT)) | \
			    ((paddr_t)(vaddr) & (PAGEOFFSET))))
#else
#define NCR_KVTOP(vaddr) (HBA_KVTOP((vaddr), PAGESHIFT, PAGEOFFSET))
#endif


/* clear and set bits in an i/o register */
#define	ClrSetBits(ncrp, reg, clr, set) \
	ddi_io_putb((ncrp)->n_handle, (ncrp)->n_reg + (reg), \
		(ddi_io_getb((ncrp)->n_handle, (ncrp)->n_reg + (reg)) \
			& ~(clr)) | (set))

/* Make certain the buffer doesn't cross a page boundary */
#define	PageAlignPtr(ptr, size)	\
	(caddr_t)(btop((unsigned)(ptr)) != btop((unsigned)(ptr) + (size)) ? \
	ptob(btop((unsigned)(ptr)) + 1) : (unsigned)(ptr))

#define	NCR_DIP(ncr)		(((ncr)->n_blkp)->n_dip)

#define	TRAN2NCR(hba)		((struct ncr *)(hba)->tran_hba_private)
#define	TRAN2NCRBLKP(hba)	((TRAN2NCR(hba))->n_blkp)
#define	TRAN2NCRUNITP(hba)	((TRAN2NCR(hba))->n_unitp)
#define	SDEV2NCR(sd)		(TRAN2NCR(SDEV2TRAN(sd)))
#define	PKT2NCRBLKP(pktp)	(TRAN2NCRBLKP(PKT2TRAN(pktp)))
#define	PKT2NCRUNITP(pktp)	NTL2UNITP(PKT2NCRBLKP(pktp), \
					(pktp)->pkt_address.a_target, \
					(pktp)->pkt_address.a_lun)
#define	ADDR2NCRBLKP(ap)	(TRAN2NCRBLKP(ADDR2TRAN(ap)))
#define	ADDR2NCRUNITP(ap)	NTL2UNITP(ADDR2NCRBLKP(ap), \
					(ap)->a_target, (ap)->a_lun)
#define	NTL2UNITP(ncr_blkp, targ, lun)	((ncr_blkp)->n_pt[TL2INDEX(targ, lun)])

/* Map (target, lun) to n_pt array index */
#define	TL2INDEX(target, lun)	((target) * NTARGETS + (lun))

#define	SCMDP2NCCBP(cmdp)	((nccb_t *)((cmdp)->cmd_private))
#define	PKTP2NCCBP(pktp)	SCMDP2NCCBP(SCMD_PKTP(pktp))
#define	NCCBP2SCMDP(nccbp)	(&(nccbp)->nc_cmd)
#define	NCCBP2PKTP(nccbp)	(&NCCBP2SCMDP(nccbp)->cmd_pkt)

#define	NCR_BLKP(x)	(((struct ncr *)(x))->n_blkp)

typedef struct ncr {
	scsi_hba_tran_t		*n_tran;
	struct ncr_blk		*n_blkp;
	struct ncr_unit		**n_unitp;
} np_t;

/*
 * Debugging stuff
 */
#define	Byte0(x)		(x&0xff)
#define	Byte1(x)		((x>>8)&0xff)
#define	Byte2(x)		((x>>16)&0xff)
#define	Byte3(x)		((x>>24)&0xff)

/*
 * include all of the function prototype declarations
 */
#include <sys/dktp/ncrs/ncrops.h>
#include <sys/dktp/ncrs/ncrdefs.h>
#include <sys/dktp/ncrs/script.h>


#ifdef	__cplusplus
}
#endif
#endif  /* _SYS_DKTP_NCRS_NCR_H */
