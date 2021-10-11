/*
 * Copyright (c) 1992-1995 Sun Microsystems, Inc.  All Rights Reserved.
 */

#ifndef	_SYS_DKTP_MCIS_H
#define	_SYS_DKTP_MCIS_H

#pragma ident	"@(#)mcis.h	1.11	95/04/12 SMI"

#ifdef	__cplusplus
extern "C" {
#endif

#define	MCIS_KVTOP(vaddr) (HBA_KVTOP((vaddr), mcis_pgshf, mcis_pgmsk))

#define	MCIS_MAX_DMA_SEGS	16	/* Max Scatter/Gather segments	*/
#define	MCIS_MAXLD		0x10
#define	MCIS_MAXINITLD		0x06
#define	MCIS_MAX_PHYSDEV	64
#define	MCIS_MAX_PREMAP		7

/* a Scatter/Gather DMA Segment Descriptor */
struct mcis_dma_seg {
	ulong	data_ptr;	/* segment address */
	ulong 	data_len;	/* segment length */
};

#pragma pack(1)

struct	mcis_tsb {	/* termination status block			*/
	ushort		t_status;	/* end status word		*/
	ushort		t_rtycnt;	/* retry count			*/
	uint		t_resid;	/* residual byte count		*/
	paddr_t		t_datapt;	/* scatter gather list addr	*/
	ushort		t_targlen;	/* target status length followed */
	unsigned	t_vres2    : 1;	/* vendor unique bit		*/
	unsigned	t_targstat : 4; /* target status code		*/
	unsigned	t_vres1    : 3;	/* vendor unique bit		*/
	unchar		t_hastat;	/* hba status			*/
	unchar		t_targerr;	/* target error code		*/
	unchar		t_haerr;	/* hba error code		*/
	ushort		t_res;		/* reserved			*/
	unchar		t_crate;	/* cache hit ratio		*/
	unchar		t_cstat;	/* cache status			*/
	paddr_t		t_mscb;		/* last processed scb addr	*/
};

/*
 * The MCIS Host Adapter Command Control Block (CCB)
 */

struct mcis_ccb {
	unsigned	ccb_cmdop:6;
	unsigned	ccb_ns:   1;
	unsigned	ccb_nd:   1;
	unchar		ccb_cmdsup;
	unchar		ccb_ch;
	union {
		struct {
			unsigned	ccb_res1: 	1;
			unsigned	ccb_nobuff: 	1;
			unsigned	ccb_ss: 	1;
			unsigned	ccb_res2: 	1;
			unsigned	ccb_scatgath: 	1;
			unsigned	ccb_retry: 	1;
			unsigned	ccb_es: 	1;
			unsigned	ccb_read: 	1;
		} ccb_opfld;
		unchar	ccb_op;
	} ccb_ops;
	daddr_t		ccb_baddr;	/* logical block address	*/
	paddr_t		ccb_datap;	/* system address		*/
	uint		ccb_datalen;	/* system buffer count		*/
	paddr_t 	ccb_tsbp;	/* tsb address			*/
	paddr_t 	ccb_link;	/* link pointer			*/
	unchar		ccb_cdb[12];	/* scsi cmd descriptor block	*/

	struct  mcis_dma_seg ccb_sg_list[MCIS_MAX_DMA_SEGS]; /* SG segs	*/
	struct mcis_tsb ccb_tsb;	/* termination status block	*/
	union {
		char    *ccb_scratch;  	/* spare buffer space, if needed */
		struct  scsi_cmd *ccb_ownerp;	/* ccb pointer		 */
	} cc;
	paddr_t	ccb_paddr;		/* physical address		*/
	unchar	ccb_status;		/* status byte			*/
	unchar	ccb_cdblen;		/* Length of SCSI CDB		*/
	unchar	ccb_flag;
	uint	ccb_scsi_op0;
	uint	ccb_scsi_op1;
};

#define	ccb_scratch	cc.ccb_scratch
#define	ccb_ownerp	cc.ccb_ownerp

struct	mcis_rctl {
	unsigned	rc_eintr : 1;	/* enable interrupt		*/
	unsigned	rc_edma  : 1;	/* enable dma			*/
	unsigned	rc_res   : 5;	/* reserved			*/
	unsigned	rc_reset : 1;	/* hw reset			*/
};

struct	mcis_rintr {
	unsigned	ri_ldevid: 4;	/* logical device id		*/
	unsigned	ri_code  : 4;	/* interrupt id	code		*/
};

struct mcis_rstat {
	unsigned	rs_busy	: 1;	/* busy */
	unsigned	rs_intrhere : 1; /* interrupt request active	*/
	unsigned	rs_cmd0	: 1;	/* command interface reg empty	*/
	unsigned	rs_cmdfull: 1;	/* command interface reg full	*/
	unsigned	rs_resv	: 4;	/* reserved			*/
};

#define	MCIS_rctlp(X)	((struct mcis_rctl *)(X))
#define	MCIS_rintrp(X)	((struct mcis_rintr *)(X))
#define	MCIS_rstatp(X)	((struct mcis_rstat *)(X))

struct	mcis_pos {
	ushort		p_hbaid;	/* hba id			*/
	unsigned	p3_chan   : 4;	/* pos reg 3 - arbitration level */
	unsigned	p3_fair   : 1;	/* pos reg 3 - fairness		*/
	unsigned	p3_targid : 3;	/* pos reg 3 - hba target id	*/
	unchar		p2_ehba   : 1;	/* pos reg 2 - hba enable	*/
	unchar		p2_ioaddr : 3;	/* pos reg 2 - ioaddr		*/
	unchar		p2_romseg : 4;	/* pos reg 2 - rom addr		*/
	unchar		p_intr;		/* interrupt level		*/
	unchar		p_pos4;		/* pos reg 4			*/
	unsigned	p_rev	: 12;	/* revision level		*/
	unsigned	p_slotsz: 4;	/* 16 or 32 slot size		*/
	unchar		p_luncnt;	/* # of lun per target		*/
	unchar		p_targcnt;	/* # of targets			*/
	unchar		p_pacing;	/* dma pacing factor		*/
	unchar		p_ldcnt;	/* number of logical device	*/
	unchar		p_tmeoi;	/* time from eoi to intr off	*/
	unchar		p_tmreset;	/* time from reset to busy off	*/
	ushort		p_cache;	/* cache status			*/
	ushort		p_retry;	/* retry status			*/
};

struct	mcis_asgncmd {
	ushort		a_cmdop;	/* command opcode		*/
	uint		a_ld   : 4;	/* logical device		*/
	uint		a_targ : 3;	/* target number		*/
	uint		a_rm   : 1;	/* remove assignment		*/
	uint		a_lunn : 3;	/* lun number			*/
	uint		a_resv : 5;	/* reserved			*/
};

struct	mcis_assign {
	union {
		struct	mcis_asgncmd	a_blk;
		uint			a_cmd;
	} mc_ac;
};

#pragma pack()

struct	mcis_ldevmap	{
	struct mcis_ldevmap	*ld_avfw;	/* avail forward	*/
	struct mcis_ldevmap	*ld_avbk;	/* avail backward	*/
	struct mcis		*ld_mcisp;	/* target struct	*/
	struct mcis_assign	ld_cmd;		/* ld assign command	*/
	struct scsi_cmd	*ld_cmdp;	/* scsi packet		*/
	uint			ld_status;	/* ownership flags	*/
	int			ld_busy;
};

struct  mcis_blk {
	kmutex_t 		mb_mutex;
	dev_info_t 		*mb_dip;
	ddi_iblock_cookie_t	mb_iblock;

	ushort			mb_ioaddr;

	unchar			mb_intr;
	unchar			mb_flag;
	unchar			mb_targetid; /* hba target */
	unchar			mb_boottarg; /* boot device target */

	unchar			mb_ldcnt;    /* total logical devices */
	unchar			mb_numdev;   /* total physical devices */
	unchar			mb_intr_code;
	unchar			mb_intr_dev;

	struct	scsi_inquiry	*mb_inqp;

	ushort  		mb_child;
	short  			mb_resv;
	opaque_t		mb_cbthdl;
	struct	mcis_ldevmap	mb_ldmhd;
	struct	mcis_ldevmap	mb_ldm[MCIS_MAXLD];
};

struct mcis_unit {
	ddi_dma_lim_t	mu_lim;
	unsigned mu_disk   : 1;		/* is a disk */
	unsigned mu_resv   : 7;
	unsigned long mu_tot_sects;
};

struct mcis_capacity {
	unsigned long mc_last_block;
	unsigned long mc_block_len;
};


#define	MCIS_DIP(mcis)		(((mcis)->m_blkp)->mb_dip)

#define	TRAN2HBA(hba)		((struct mcis *)(hba)->tran_hba_private)
#define	SDEV2HBA(sd)		(TRAN2HBA(SDEV2TRAN(sd)))

#define	TRAN2MCIS(hba)		((struct mcis *)(hba)->tran_tgt_private)
#define	TRAN2MCISBLKP(hba)	((TRAN2MCIS(hba))->m_blkp)
#define	TRAN2MCISUNITP(hba)	((TRAN2MCIS(hba))->m_unitp)
#define	SDEV2MCIS(sd)		(TRAN2MCIS(SDEV2TRAN(sd)))
#define	PKT2MCIS(pktp)		(TRAN2MCIS(PKT2TRAN(pktp)))
#define	PKT2MCISUNITP(pktp)	(TRAN2MCISUNITP(PKT2TRAN(pktp)))
#define	PKT2MCISBLKP(pktp)	(TRAN2MCISBLKP(PKT2TRAN(pktp)))
#define	ADDR2MCISUNITP(ap)	(TRAN2MCISUNITP(ADDR2TRAN(ap)))
#define	ADDR2MCISBLKP(ap)	(TRAN2MCISBLKP(ADDR2TRAN(ap)))

#define	MCIS_BLKP(X)		(((struct mcis *)(X))->m_blkp)

struct mcis {
	scsi_hba_tran_t		*m_tran;
	struct mcis_blk		*m_blkp;
	struct mcis_unit	*m_unitp;
	struct mcis_ldevmap	*m_ldp;
};

/* remove these if not using IBM reads/writes */
#define	set_blkcnt(X, Y) \
{ (X)->ccb_cdb[0] = ((Y) & 0xff); (X)->ccb_cdb[1] = (((Y) >> 8) & 0xff); }
#define	set_blklen(X, Y) \
{ (X)->ccb_cdb[2] = ((Y) & 0xff); (X)->ccb_cdb[3] = (((Y) >> 8) & 0xff); }

/*	define for mb_flag and ccb_flag					*/
#define	MCIS_CACHE	0x0001		/* hardware caching enabled 	*/

/*	defines for ld_status flag					*/
#define	MSCB_OWNLD	0x0001		/* own's a logical device 	*/
#define	MSCB_WAITLD	0x0002		/* waiting for a ld assignment	*/
#define	MSCB_GOTLD	0x0004		/* got a ld assignment		*/
#define	MSCB_INITLD	0x0008		/* ld assignment valid at init	*/

#define	MSCB_opfld	ccb_ops.ccb_opfld
#define	MSCB_op		ccb_ops.ccb_op


#define	TERR_SELTO	0x10		/* selection timeout		*/
#define	TERR_BUSPHASE	0x13		/* bus phase error		*/

#define	MCIS_CMD_port(X) ((X))
#define	MCIS_CMD	0x0
#define	MCIS_ATTN	0x4
#define	MCIS_CTRL	0x5
#define	MCIS_INTR	0x6
#define	MCIS_STAT	0x7

/*	attention register						*/
#define	ISATTN_ICMD	0x10	/* start immediate command		*/
#define	ISATTN_SCB	0x30	/* start ccb 				*/
#define	ISATTN_LSCB	0x40	/* start long ccb 			*/
#define	ISATTN_EOI	0xe0	/* end of interrupt			*/

#define	MCIS_SENDEOI(X, LD)	\
	(outb((X)+MCIS_ATTN, ISATTN_EOI | (LD)))

/*	basic control register						*/
#define	ISCTRL_RESET	0x80	/* hardware reset			*/
#define	ISCTRL_EDMA	0x02	/* dma enable				*/
#define	ISCTRL_EINTR	0x01	/* enable interrupt			*/

/*	basic status register						*/
#define	ISSTAT_CMDFULL	0x8	/* command interface reg full		*/
#define	ISSTAT_CMD0	0x4	/* command interface reg empty		*/
#define	ISSTAT_INTRHERE 0x2	/* interrupt request active		*/
#define	ISSTAT_BUSY	0x1	/* busy					*/

#define	MCIS_QBUSYWAIT(X)	\
	mcis_qwait((X), ISSTAT_BUSY, 0, ISSTAT_BUSY)

#define	MCIS_QINTRWAIT(X)	\
	mcis_qwait((X), ISSTAT_INTRHERE, ISSTAT_INTRHERE, 0)

#define	MCIS_BUSYWAIT(X)	\
	mcis_wait((X), ISSTAT_BUSY, 0, ISSTAT_BUSY)

#define	MCIS_INTRWAIT(X)	\
	mcis_wait((X), ISSTAT_INTRHERE, ISSTAT_INTRHERE, 0)

#define	MCIS_CMDOUTWAIT(X)	\
	mcis_wait((X), ISSTAT_CMD0|ISSTAT_BUSY, ISSTAT_CMD0, ISSTAT_BUSY)


/*	interrupt id							*/
#define	ISINTR_SCB_OK	0x1	/* ccb completed successfully		*/
#define	ISINTR_SCB_OK2	0x5	/* ccb completed ok after retry 	*/
#define	ISINTR_HBA_FAIL	0x7	/* hba hw failed			*/
#define	ISINTR_ICMD_OK	0xa	/* immediate command completed ok	*/
#define	ISINTR_CMD_FAIL	0xc	/* cmd completed with failure		*/
#define	ISINTR_CMD_INV	0xe	/* invalid command			*/
#define	ISINTR_SEQ_ERR	0xf	/* sw sequencing error			*/
#define	ISINTR_UNKNOWN_ERR 0x2	/* for string conversion 		*/

/*	command opcode							*/
#define	ISCMD_ABORT	0x0f	/* command abort			*/
#define	ISCMD_ASSIGN	0x0e	/* logical device assignment		*/
#define	ISCMD_DEVINQ	0x0b	/* device inquiry			*/
#define	ISCMD_DMACTRL	0x0d	/* dma pacing control			*/
#define	ISCMD_HBACTRL	0x0c	/* feature control			*/
#define	ISCMD_FMTPREP	0x17	/* format prepare			*/
#define	ISCMD_FMTUNIT	0x16	/* format unit				*/
#define	ISCMD_CMDSTAT	0x07	/* get command complete status		*/
#define	ISCMD_GETPOS	0x0a	/* get pos and adapter information	*/
#define	ISCMD_READ	0x01	/* read data				*/
#define	ISCMD_READCAP	0x09	/* read capacity			*/
#define	ISCMD_PREREAD	0x31	/* read prefetch			*/
#define	ISCMD_READVFY	0x03	/* read verify				*/
#define	ISCMD_REASSIGN	0x18	/* reassign block			*/
#define	ISCMD_REQSEN	0x08	/* request sense			*/
#define	ISCMD_RESET	0x00	/* scsi (soft) reset			*/
#define	ISCMD_SNDSCSI	0x1f	/* send scsi command			*/
#define	ISCMD_WRITE	0x02	/* write data				*/
#define	ISCMD_WRITEVFY	0x03	/* write verify				*/

#define	ISCMD_ICMD	0x04	/* supplement code			*/
#define	ISCMD_SCB	0x1c	/* start ccb 				*/
#define	ISCMD_LSCB	0x24	/* start long ccb 			*/

/* 	values for tsbp->t_haerr					*/
#define	MCIS_HAERR_BADCCB  	0x01    /* hba rejected ccb 		*/
#define	MCIS_HAERR_BADCMD  	0x03    /* command not supported 	*/
#define	MCIS_HAERR_SYSABORT  	0x04    /* host aborted cmd 		*/
#define	MCIS_HAERR_BADLD  	0x0a    /* logical device not mapped 	*/
#define	MCIS_HAERR_END  	0x0b    /* max block addr exceeded	*/
#define	MCIS_HAERR_END16	0x0c    /* max block addr < 16 bit card	*/
#define	MCIS_HAERR_BADDEV  	0x13    /* invalid device for command	*/
#define	MCIS_HAERR_TIMEOUT  	0x21    /* timeout 			*/
#define	MCIS_HAERR_DMA  	0x22    /* hba dma err			*/
#define	MCIS_HAERR_BUF  	0x23    /* hba buffer err		*/
#define	MCIS_HAERR_ABORT  	0x24    /* hba aborted cmd 		*/
#define	MCIS_HAERR_ERR  	0x80    /* hba microprocessor error 	*/


/* 	values for tsbp->t_targerr					*/
#define	MCIS_TAERR_BUSRESET  	0x01    /* scsi bus reset  		*/
#define	MCIS_TAERR_SELTO  	0x10    /* scsi selection time out 	*/

#define	LD_CMD	ld_cmd.mc_ac.a_cmd
#define	LD_CB	ld_cmd.mc_ac.a_blk

#define	MCIS_HBALD	0x0f

#define	b_inqdout		b_boottarg
#define	ldmhd_fw(X)		((X)->mb_ldmhd.ld_avfw)
#define	ldmhd_bk(X)		((X)->mb_ldmhd.ld_avbk)


#ifdef	__cplusplus
}
#endif

#endif	/* _SYS_DKTP_MCIS_H */
