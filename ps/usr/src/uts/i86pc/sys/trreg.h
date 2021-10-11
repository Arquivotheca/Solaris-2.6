/*
 *	trreg.h -- Hardware register and values for the
 *		       IBM Token Ring 16/4 adapter.
 *
 *	Copyrighted as an unpublished work.
 *	(c) Copyright 1993,1996 Sun Microsystems, Inc.
 *	All rights reserved.
 */

#ifndef	_TRREG_H
#define	_TRREG_H

#pragma ident	"@(#)trreg.h	1.8	96/06/07 SMI"

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * The size of the receive or xmit buffer has to be a multiple of 8
 * and the board has 6 bytes of overhead. The number of receive buffers
 * configured is a minimum asked from the board, if there is more room
 * on the board, we will have more buffers, however, if there are not as
 * many as we have asked for the DIR_OPEN_ADP command will fail.
 * We ask for at least as many buffers that could hold two of the
 * longest packet we expect to receive.
 */

/* Board buffer sizes and related defines */
#define	TR_XMT_OVHD	6	/* 6 bytes of XMIT buffer unavailable */
#define	TR_MAXSAP	20	/* Only 20 protocols. Can change via config */
#define	TR_RCVBUF_LEN	256

#define	TR_SMALL_DHB	1600	/* DHB size for 8k Ram Size */
#define	TR_MED_DHB	4464	/* Max DHB size for 4Mbps Ram Size */
#define	TR_LRG_DHB	8928    /* Max DHB size for 16Mbps Ram Size & 32k */
#define	TR_XLRG_DHB	17960	/* Max DHB size for 16Mbps Ram Size & 64k */

/*
 * definitions for module_info
 */
#define	TRIDNUM		0x8022
#define	TR_HIWATER	89800	/* high water mark for flow control */
#define	TR_LOWATER	35920	/* low water mark for flow control */
#define	TR_DEFMAX	4096	/* default max packet size */

/*
 * IBM 16/4 Adapter system request block
 * After the adapter has been successfully opened this
 * is a block of 28 bytes.
 */
#define	SRB_SIZE	28
struct srb {
	uchar_t	srb[SRB_SIZE];
};

#define	SRB_INIT2	2

/*
 * IBM 16/4 Adapter system status block
 * After the adapter has been successfully opened this
 * is a block of 20 bytes.
 */
#define	SSB_SIZE	20
struct ssb {
	uchar_t	ssb[SSB_SIZE];
};

/*
 * IBM 16/4 Adapter request block
 * After the adapter has been successfully opened this
 * is a block of 28 bytes.
 */
#define	ARB_SIZE	28
struct arb {
	uchar_t	arb[ARB_SIZE];
};

/*
 * IBM 16/4 Adapter status block
 * After the adapter has been successfully opened this
 * is a block of 12 bytes.
 */
#define	ASB_SIZE	12
struct asb {
	uchar_t	asb[ASB_SIZE];
};

/*
 * parameters for the IBM Token Ring 16/4 adapter
 */

#define	TR_INIT_TIME 4	/* Reasonable waiting period for board initialization */

/* Max number of xmits we allow to queue before we start holding up xmitters. */
#define	TRMAXQDPKTS 7

/*
 * PIO (Programmed I/O Area)    0xa20-0xa27
 *		for PC Bus Only 0x2f0-0x2f7
 */

#define	GIE_BASE	0x2f0	/* the Global Interrupt Enable base value */
#define	PIO_SETUP1	0x0	/* SETUP 1 READ for primary board */
#define	PIO_RST_LCH	0x1	/* Reset Latch */
#define	PIO_SETUP2	0x2	/* SETUP 2 READ Only for Micro Channel */
#define	PIO_RST_RLS	0x2	/* Reset release */
#define	PIO_INT_ENB	0x3	/* Interrupt enable */

#define	TR_IRQ_MASK	0x3	/* the IRQ MASK in the value read in SETUP 1 */
#define	TR_IRQ2	0x0	/* IRQ 2 */
#define	TR_IRQ3	0x1	/* IRQ 3 */
#define	TR_IRQ6	0x2	/* IRQ 6 */
#define	TR_IRQ7	0x3	/* IRQ 7 */

/*
 * the board has only two possible programmed I/O addresses,
 * one if its a primary adapter and a different one if its a
 * secondary adapter
 */
#define	PRIMARY_PIO	(uchar_t *) 0xa20
#define	SECONDARY_PIO	(uchar_t *) 0xa24

/*
 * the bits 2-7 read in SETUP 1 specify the bits 18-13 of the mmio address
 * in the case of the boards with PC/IO bit 19 is always 1
 */
#define	MMIO_MASK	0xFC	/* the MMIO address bits mask */
#define	MMIO_ADDR(x)	((paddr_t) ((0x100 | ((x) & MMIO_MASK)) << 11))
#define	MMIO_SIZE	(8*1024)	/* 8K MMIO */

/*
 * the BIOS/MMIO layout
 *
 * These are the address offset of the registers in the MMIO domain
 * in the attachment control area
 * The following are the address of the even registers
 */

#define	RRR	0x1e00		/* shared Ram Relocation Register */
#define	RRR_ODD	0x1e01		/* odd byte of shared Ram Relocation Register */
#define	WRBR	0x1e02		/* Write Region Base Register */
#define	WWOR	0x1e04		/* Write Window Open Register */
#define	WWCR	0x1e06		/* Write Window Close Register */
#define	ISRP	0x1e08		/* Interrupt Status Register PC */
#define	ISRA	0x1e0a		/* Interrupr Status Register Adapter */
#define	TCR	0x1e0c		/* Timer Control Register */
#define	TVR	0x1e0e		/* Timer Value Register */
#define	SRPR	0x1e18		/* Share Ram Paging Register */

/*
 * the following command bits are OR'ed with the address of the
 * registers, and a write is performed with to the new address to
 * perform an OR or and AND to that register.  To read or write a
 * new value the original addresses with a read or write is used
 */
#define	MMIO_AND	0x20
#define	MMIO_OR		0x40

#define	MMIO_SETBIT(ofst, val)	\
	(trdp->trd_mmio->mmio[ofst | MMIO_OR] = (uchar_t)val)
#define	MMIO_RESETBIT(ofst, val) \
	(trdp->trd_mmio->mmio[ofst | MMIO_AND] = (uchar_t)~(val))

/*
 * This structure is a dummy used to map in the mmio/bios region with
 * ddi_map_regs.
 */
struct trmmio {
	uchar_t	mmio[MMIO_SIZE];
};

/*
 * The bits 1-7 of RRR even register are used to set the shared RAM
 * starting address bits 13-19.
 * The bit 2 and 3 of the RRR odd is used to read the size of the
 * Shared RAM
 */
#define	SRAM_ADDR_SHIFT	12	/* the shift value for the even register */
#define	SRAM_ADDR_MASK	0xFE	/* drops the low bit when setting address */
#define	SRAM_SET(x) ((x>>SRAM_ADDR_SHIFT) & SRAM_ADDR_MASK)

#define	RRR_O_SMASK	0x0c
#define	RRR_O_8K	0x00
#define	RRR_O_16K	0x04
#define	RRR_O_32K	0x08
#define	RRR_O_64K	0x0c

/*
 * This structure is a dummy used to map in the shared RAM
 *  region with ddi_map_regs.
 */
#define	SRAM_MAXSIZE	0x10000
struct trram {
	uchar_t	ram[SRAM_MAXSIZE];
};

/*
 * The offset of the information in AIP
 */
#define	AIP_AREA	0x1f00	/* offset of the AIP Area in the Rom */
#define	AIP_AEA_ADDR	0x1f00	/* The adapter encoded address */
#define	AEA_NIBBLE_CNT	12	/* only the even bytes to 0x1f16 */

#define	AIP_CH_ID_ADDR	0x1F30	/* Channel Identifier */
#define	AIP_CH_ID_CNT	24

#define	AIP_CHKSM1_ADDR	0x1f60	/* checksum area one */
#define	AIP_CHKSM2_ADDR	0x1ff0	/* checksum area two */

#define	AIP_ADP_TYPE	0x1FA0	/* Adapter Type */
#define	AIP_AUTO	0xC	/* Auto 16/4 Token Ring (ISA) */

#define	AIP_SUP_INT	0x1FBA	/* Supported Interrupt */
#define	AIP_INT		0xE

/*
 * The ISRA is a pair of registers used for interrupting the
 * adapter as well as interrogating its status.  The even register is
 * for interrogation and the odd register is for interrupting the
 * adapter.
 *
 * ISRA (Interrupt Status Register Adapter) even register
 * read only to the host
 */
#define	ISRA_E_PARERR	0x80	/* parity error on adapter's internal bus */
#define	ISRA_E_TIMER	0x40	/* at least one of the TCR-odd timers expired */
#define	ISRA_E_FAULT	0x20	/* Access interrupt */
#define	ISRA_E_DEADMAN	0x10	/* expiration of deadman timer */
#define	ISRA_E_MCHCK	0x08	/* adapter machine check */
				/* 0x4 is reserved */
#define	ISRA_E_HWIDIS	0x02	/* disable hardware interrupt  (Parity) */
#define	ISRA_E_SWIDIS	0x01	/* Disable software interrupt */

/*
 * ISRA odd register
 * can be used for interrupting the adapter
 */
				/* 0x80 (bit seven) is reserved */
#define	ISRA_O_BFF_REQ	0x40	/* bridge frame forward request */
#define	ISRA_O_SRB_CMD	0x20	/* command in SRB */
#define	ISRA_O_ASB_RES	0x10	/* response in ASB */
#define	ISRA_O_SRBFR_R	0x08	/* SRB free request */
#define	ISRA_O_ASBFR_R	0x04	/* ASB free request */
#define	ISRA_O_ARB_FREE	0x02	/* ARB is free */
#define	ISRA_O_SSB_FREE	0x01	/* SSB is free */

/*
 * The ISRP (Interrupt Status Register PC system ) pair is used by
 * the adapter to interrupt the host.
 *
 * ISRP even
 */

#define	ISRP_E_SC	0x80	/* interrupt steering control */
#define	ISRP_E_IENB	0x40	/* interrupt enable */
				/* 0x20 is reserved */
#define	ISRP_E_TIMER	0x10	/* TVR-even has expired */
#define	ISRP_E_ERR	0x08	/* MCHCK or deadman, or timer overrun */
#define	ISRP_E_FAULT	0x04	/* bad access by the host */
#define	ISRP_E_INTBLK	0x02	/* interrupt block bit */

/*
 * ISRP odd
 */
				/* 0x80 (bit seven) is reserved */
#define	ISRP_O_CHCK	0x40	/* adapter check */
#define	ISRP_O_SRB_RES	0x20	/* SRB response */
#define	ISRP_O_ASB_FREE	0x10	/* ASB is free */
#define	ISRP_O_ARB_CMD	0x08	/* command in ARB */
#define	ISRP_O_SSB_RES	0x04	/* response in SSB */
#define	ISRP_O_BFF_DONE	0x02	/* Bridge Frame Forward done */
				/* 0x1 is reserved */

#define	ISRP_O_INTR	0x7e	/* All valid interrupt bits */

/*
 * the adapter direct command codes
 */
#define	NO_CMD		0xff	/* for when the stream goes away */
#define	DIR_INTERRUPT	0x0	/* force an adapter interrupt */
#define	DIR_MOD_PARAMS	0x1	/* modify open parameters */
#define	DIR_RES_PARAMS	0x2	/* looks to be the same as MOD_PARAMS */
#define	DIR_OPEN_ADP	0x3	/* open the adapter */
#define	DIR_CLOSE_ADP	0x4	/* close the adapter */
#define	DIR_GRP_ADDR	0x6	/* set group addr */
#define	DIR_FUNC_ADDR	0x7	/* set functional addr */
#define	DIR_READ_LOG	0x8	/* read a reset the error statistics */

/* before issuing a command the retcode should be set to 0xfe */
#define	NULL_RC		0xfe
#define	TMP_RC		0xff
#define	OK_RC		0x0
#define	XMT_ERR_RC	0x22
#define	CMD_CNCL_RC	0x7

/* the generic structure used for putting commands in SRB */
struct dir_cmd {
	uchar_t	dc_cmd;		/* command code */
	uchar_t	dc_res;		/* reserved */
	uchar_t	dc_retcode;	/* return code */
};

/* structure for DIR_MOD_PARAMS (DIR.MODIFY.OPEN.PARMS) */
struct dir_mod_params {
	uchar_t	mp_cmd;		/* command code */
	uchar_t	mp_res1;	/* reserved */
	uchar_t	mp_retcode;	/* return code */
	uchar_t	mp_res2;	/* reserved */
	ushort_t	mp_openops;	/* new options */
};

/* for DIR_OPEN_ADP (DIR.OPEN.ADAPTER) */
struct dir_open_adp {
	uchar_t	oa_cmd;		/* command code */
	uchar_t	oa_res1[7];	/* reserved */
	ushort_t	oa_openops;	/* open options */
	uchar_t	oa_node[6];	/* this adapter ring address */
	uchar_t	oa_group[4];	/* group address */
	uchar_t	oa_func[4];	/* functional address */
	ushort_t	oa_num_rcv_buf;	/* number of receive buffers */
	ushort_t	oa_rcv_buf_len;	/* length of receive buffers */
	ushort_t	oa_dhb_len;	/* lenght of transmit buffers */
	uchar_t	oa_num_dhb;	/* number of DHBs */
	uchar_t	oa_res2;	/* reserved */
	uchar_t	oa_dlc_max_sap;	/* max number of saps */
	uchar_t	oa_dlc_max_sta;	/* max number of link stations */
	uchar_t	oa_dlc_max_gsap; /* max number of group saps */
	uchar_t	oa_dlc_max_gmem; /* max members per group saps */
	uchar_t	oa_dlc_T1_1;	/* DLC timer T1 interval group 1 */
	uchar_t	oa_dlc_T2_1;	/* DLC timer T2 interval group 1 */
	uchar_t	oa_dlc_TI_1;	/* DLC timer Ti interval group 1 */
	uchar_t	oa_dlc_T1_2;	/* DLC timer T1 interval group 2 */
	uchar_t	oa_dlc_T2_2;	/* DLC timer T2 interval group 2 */
	uchar_t	oa_dlc_TI_2;	/* DLC timer Ti interval group 2 */
	uchar_t	oa_prod_id[18];	/* product id */
};

/* defines for dir_open_adp.oa_openops */
#define	OO_WRAP		0x8000		/* wrap data, loopback */
#define	OO_DIS_HARDERR	0x4000		/* disable hard error */
#define	OO_DIS_SOFTERR	0x2000		/* disable soft error */
#define	OO_PASS_ADPMAC	0x1000		/* pass all MAC frames */
#define	OO_PASS_ATTMAC	0x0800		/* pass all attention MAC frames */
#define	OO_CONTENDER	0x0100		/* participate in "claim token" */
#define	OO_PASS_BEACMAC	0x0080		/* pass all beacon MAC frames */
#define	OO_RPL		0x0020		/* remote program load */
#define	OO_NOETR	0x0010		/* no early token release for 16Mbps */

#define	TR_OPEN_OPTS	0x00		/* no option selected */

#define	TR_OPEN_MAX_RETRY	5	/* no option selected */

/* the adapter response to OPEN */
struct dir_open_res {
	uchar_t	or_cmd;		/* command code */
	uchar_t	or_res;		/* reserved */
	uchar_t	or_retcode;	/* return code */
	uchar_t	or_res2[3];	/* reserved */
	ushort_t	or_errcode;	/* error code */
	ushort_t	or_asb_addr;	/* ASB offset */
	ushort_t	or_srb_addr;	/* SRB offset */
	ushort_t	or_arb_addr;	/* ARB offset */
	ushort_t	or_ssb_addr;	/* SSB offset */
};

/* for DIR_READ_LOG */
struct dir_read_log {
	uchar_t	rl_cmd;		/* command code */
	uchar_t	rl_res1;	/* reserved */
	uchar_t	rl_retcode;	/* return code */
	uchar_t	rl_res2[3];	/* reserved */
	uchar_t	rl_line_err;	/* line error */
	uchar_t	rl_int_err;	/* internal error */
	uchar_t	rl_burst_err;	/* burst error */
	uchar_t	rl_ac_err;	/* A/C error */
	uchar_t	rl_abort_del;	/* abort delimiter */
	uchar_t	rl_res3;	/* reserved */
	uchar_t	rl_lost_frame;	/* lost frame */
	uchar_t	rl_rcv_cong;	/* receive congestion count */
	uchar_t	rl_fr_cp_err;	/* frame copied error */
	uchar_t	rl_freq_err;	/* frequency error */
	uchar_t	rl_tok_err;	/* token error */
	uchar_t	rl_res4[3];
};

/* for DIR_GRP_ADDR and DIR_FUNC_ADDR */
struct dir_grp_addr {
	uchar_t	ga_cmd;		/* command */
	uchar_t	ga_res1;	/* reserved */
	uchar_t	ga_retcode;	/* return code */
	uchar_t	ga_res2[3];	/* reserved */
	uchar_t	ga_addr[4];	/* group or functional addr */
};

/*
 * The commands which use the DLC layer
 */
#define	DLC_CLOSE_SAP	0x16	/* close a SAP */
#define	DLC_CLOSE_STN	0x1a	/* close a link station */
#define	DLC_CON_STN	0x1b	/* initiate a SABME_UA */
#define	DLC_FLOW_CNTL	0x1d	/* control the flow over a link station */
#define	DLC_MODIFY	0x1c	/* modify a sap value or params of a link */
#define	DLC_OPEN_SAP	0x15	/* open a SAP */
#define	DLC_OPEN_STN	0x19	/* open a link station */
#define	DLC_REALLOC	0x17	/* reallocate a link station */
#define	DLC_RESET	0x14	/* reset saps or link stations */
#define	DLC_STAT	0x1e	/* read stats for a specified link station */

/* for DLC_CLOSE_SAP */
struct dlc_close_sap {
	uchar_t	cs_cmd;		/* command */
	uchar_t	cs_res1;	/* reserved */
	uchar_t	cs_retcode;	/* return code */
	uchar_t	cs_res2;	/* reserved */
	ushort_t	cs_stn_id;	/* sap id to close */
};

/* for DLC_CLOSE_STATION */
struct dlc_close_stn {
	uchar_t	cs_cmd;		/* command */
	uchar_t	cs_cmdcor;	/* command correlate */
	uchar_t	cs_retcode;	/* return code */
	uchar_t	cs_res1;	/* reserved */
	ushort_t	cs_stn_id;	/* station id to close */
};

/* for DLC_CON_STN */
struct dlc_con_stn {
	uchar_t	cn_cmd;		/* command */
	uchar_t	cn_cmdcor;	/* command correlate */
	uchar_t	cn_retcode;	/* return code */
	uchar_t	cn_res2;	/* reserved */
	ushort_t	cn_stn_id;	/* station id */
	uchar_t	cn_route[18];	/* routing info */
};

/* for DLC_FLOW_CNTL */
struct dlc_flow_cntl {
	uchar_t	cf_cmd;		/* command */
	uchar_t	cf_res1;	/* reserved */
	uchar_t	cf_retcode;	/* return code */
	uchar_t	cf_res2;	/* reserved */
	ushort_t	cf_stn_id;	/* station id */
	uchar_t	cf_flow_opt;	/* flow options */
};

/* for DLC_MODIFY */
struct dlc_modify {
	uchar_t	dm_cmd;		/* command */
	uchar_t	dm_res1;	/* reserved */
	uchar_t	dm_retcode;	/* return code */
	uchar_t	dm_res2;	/* reserved */
	ushort_t	dm_stn_id;	/* station id */
	uchar_t	dm_t1;		/* T1 value */
	uchar_t	dm_t2;		/* T2 value */
	uchar_t	dm_ti;		/* Ti value */
	uchar_t	dm_maxout;	/* max xmit w/o ack */
	uchar_t	dm_maxin;	/* max rcv w/o ack */
	uchar_t	dm_maxout_incr;	/* dynamic window incr */
	uchar_t	dm_max_retry;	/* N2 value */
	uchar_t	dm_acc_pri;	/* new access priority */
	uchar_t	dm_gsap_num;	/* number of following group saps */
	uchar_t	dm_gsaps[13];	/* GSAP list */
};

/* for DLC_OPEN_SAP */
struct dlc_open_sap {
	uchar_t	os_cmd;		/* command */
	uchar_t	os_res1;	/* reserved */
	uchar_t	os_retcode;	/* return code */
	uchar_t	os_res2;	/* reserved */
	ushort_t	os_stn_id;	/* station id */
	uchar_t	os_t1;		/* T1 value */
	uchar_t	os_t2;		/* T2 value */
	uchar_t	os_ti;		/* Ti value */
	uchar_t	os_maxout;	/* max xmit w/o ack */
	uchar_t	os_maxin;	/* max rcv w/o ack */
	uchar_t	os_maxout_incr;	/* dynamic window incr */
	uchar_t	os_max_retry;	/* N2 value */
	uchar_t	os_gsap_max;	/* max numbers saps for a group sap */
	ushort_t	os_max_ifield;	/* max rcv info field length */
	uchar_t	os_sap;		/* SAP value */
	uchar_t	os_sap_opts;	/* SAP options */
	uchar_t	os_stn_cnt;	/* staion count for this sap */
	uchar_t	os_gsap_num;	/* number of following group saps */
	uchar_t	os_gsaps[8];	/* GSAP list */
};

/* defines for dlc_open_sap, dlc_close_sap return codes */
#define	OS_OK		0x00	/* open successful */
#define	OS_INVCMD	0x01	/* invalid cmd */
#define	OS_NOTOPEN	0x04	/* adapter not open */
#define	OS_BADOPTION	0x06	/* bad option sent */
#define	OS_PERM		0x08	/* unauthorized access */
#define	OS_BADID	0x40	/* bad station id for close */
#define	OS_TOOBIG	0x42	/* param too big */
#define	OS_BADSAP	0x43	/* bad sap or already in use */
#define	OS_NOGROUP	0x45	/* no such group */
#define	OS_NOMEM	0x46	/* out of resources */
#define	OS_LINKS	0x47	/* open links to station */
#define	OS_NOTLAST	0x48	/* not the last sap in the group */
#define	OS_GROUPFULL	0x49	/* No more space in group sap */
#define	OS_NOTDONE	0x4c	/* commands outstanding */

/* defines for dlc_open_sap.os_sap_opts */
#define	OS_PRI_MASK	0xe0	/* xmit priority of this sap */
#define	OS_XID		0x08	/* XID handled by the board */
#define	OS_INDV_SAP	0x04	/* individual sap */
#define	OS_GRP_OPT	0x02	/* group sap */
#define	OS_GRP_MEM	0x01	/* member of a group sap */

/* for DLC_OPEN_STN */
struct dlc_open_stn {
	uchar_t	ot_cmd;		/* command */
	uchar_t	ot_res1;	/* reserved */
	uchar_t	ot_retcode;	/* return code */
	uchar_t	ot_res2;	/* reserved */
	ushort_t	ot_stn_id;	/* sap id */
	uchar_t	ot_t1;		/* T1 value */
	uchar_t	ot_t2;		/* T2 value */
	uchar_t	ot_ti;		/* Ti value */
	uchar_t	ot_maxout;	/* max xmit w/o ack */
	uchar_t	ot_maxin;	/* max rcv w/o ack */
	uchar_t	ot_maxout_incr;	/* dynamic window incr */
	uchar_t	ot_max_retry;	/* N2 value */
	uchar_t	ot_rsap;	/* the remote RSAP value */
	ushort_t	ot_max_ifield;	/* max rcv info field length */
	uchar_t	ot_stn_opts;	/* SAP options */
	uchar_t	ot_res3;	/* reserved */
	uchar_t	ot_raddr[6];	/* ring address of the remote station */
};

/* for DLC_REALLOC */
struct dlc_realloc {
	uchar_t	ra_cmd;		/* command */
	uchar_t	ra_res1;	/* reserved */
	uchar_t	ra_retcode;	/* return code */
	uchar_t	ra_res2;	/* reserved */
	ushort_t	ra_stn_id;	/* station id */
	uchar_t	ra_opts;	/* add/subtract option */
	uchar_t	ra_stn_cnt;	/* number of link station to move */
	uchar_t	ra_adp_cnt;	/* number of links set by adapter */
	uchar_t	ra_sap_cnt;	/* number of links for sap set by adapter */
};

/* for DLC_RESET */
struct dlc_reset {
	uchar_t	rs_cmd;		/* command */
	uchar_t	rs_res1;	/* reserved */
	uchar_t	rs_retcode;	/* return code */
	uchar_t	rs_res2;	/* reserved */
	ushort_t	rs_stn_id;	/* station id */
};

/* for DLC_STAT */
struct dlc_stat {
	uchar_t	st_cmd;		/* command */
	uchar_t	st_res1;	/* reserved */
	uchar_t	st_retcode;	/* return code */
	uchar_t	st_res2;	/* reserved */
	ushort_t	st_stn_id;	/* station id */
	ushort_t	st_cntr_addr;	/* offset to the statistics */
	ushort_t	st_hdr_addr;	/* offset to the LAN header */
	uchar_t	st_hdr_len;	/* the LAN header length */
	uchar_t	st_opts;	/* options */
};


/*
 * the transmit commands
 */

#define	XMT_DIR_FRAME	0x0a
#define	XMT_I_FRAME	0x0b
#define	XMT_UI_FRAME	0x0d
#define	XMT_XID_CMD	0x0e
#define	XMT_XRES_FIN	0x0f	/* XID response final */
#define	XMT_XRES_NFIN	0x10	/* XID response not final */
#define	XMT_TEST_CMD	0x11

/* generic xmit structure */
struct xmt_cmd {
	uchar_t	xc_cmd;		/* command */
	uchar_t	xc_cmdcor;	/* command correlate */
	uchar_t	xc_retcode;	/* return code */
	uchar_t	xc_res2;	/* reserved */
	ushort_t	xc_stn_id;	/* station id */
	uchar_t	xr_fserror;	/* FS byte error code */
};

/*
 * the adapter to PC commands
 */

#define	ADP_DLC_STAT	0x83
#define	ADP_RCV_DATA	0x81
#define	ADP_RING_CHNG	0x84
#define	ADP_XMT_REQ	0x82

/* for ADP_DLC_STAT (DLC.STATUS) */
struct adp_dlc_stat {
	uchar_t	as_cmd;		/* command */
	uchar_t	as_res1[3];	/* reserved */
	ushort_t	as_stn_id;	/* id of sap of station */
	ushort_t	as_status;	/* DLC status indicator */
	uchar_t	as_frmr_data[5]; /* data sent or rcvd whit frmr response */
	uchar_t	as_acc_pri;	/* new addess priority for SAP or Station */
	uchar_t	as_raddr[6];	/* remote addre ring address */
	uchar_t	as_rsap;	/* remote SAP's value */
};

/* defines for adp_dlc_stat.as_status */
#define	AS_LINK_LOST	0x8000	/* link lost */
#define	AS_DISC_RCV	0x4000	/* DM or DISC rcvd, or DISC ack'ed */
#define	AS_FRMR_RCV	0x2000	/* FRMR received */
#define	AS_FRMR_SENT	0x1000	/* FRMR sent */
#define	AS_SABME_RCV	0x0800	/* SABME for for an open link station rcv'ed */
#define	AS_SABME_ORCV	0x0400	/* SABME rcved, link station opened */
#define	AS_ENT_BUSY	0x0200	/* remote station entered local busy */
#define	AS_EXT_BUSY	0x0100	/* remote station exited local busy */
#define	AS_TI_EXP	0x0080	/* Ti timer has expired */
#define	AS_DLC_CNT	0x0040	/* DLC counter overflow */
#define	AS_ACC_PRI	0x0020	/* Access priority reduced */

/* for ADP_RCV_DATA */
struct adp_rcv_data {
	uchar_t	rd_cmd;		/* command */
	uchar_t	rd_res1[3];	/* reserved */
	ushort_t	rd_stn_id;	/* id of sap of station */
	ushort_t	rd_rcv_buf;	/* offset to the receive buffer */
	uchar_t	rd_lan_hdr_len;	/* LAN hdr length */
	uchar_t	rd_dlc_hdr_len;	/* DLC hdr length */
	ushort_t	rd_frame_len;	/* lenght of the entire frame */
	uchar_t	rd_ncb_type;	/* category of the message received */
};

/* defines for adp_rcv_data.rd_ncb_type */
#define	NT_MAC		0x02
#define	NT_I		0x04
#define	NT_UI		0x06
#define	NT_XID_POLL	0x08
#define	NT_XID_NPOLL	0x0a
#define	NT_XID_FIN	0x0c
#define	NT_XID_NFIN	0x0e
#define	NT_TEST_FIN	0x10
#define	NT_TEST_NFIN	0x12

/* response to a rcv_data */
struct rcv_data_res {
	uchar_t	dr_cmd;		/* command */
	uchar_t	dr_res1;	/* reserved */
	uchar_t	dr_retcode;	/* return code */
	uchar_t	dr_res2;	/* reserved */
	ushort_t	dr_stn_id;	/* id of sap of station */
	ushort_t	dr_rcv_buf;	/* offset to the receive buffer */
};

/* the receive buffer format */
struct rcv_buf {
	ushort_t	rb_res1;	/* reserved */
	ushort_t	rb_next;	/* ofset of the next buffer */
	uchar_t	rb_res2;	/* reserved */
	uchar_t	rb_rcv_fs;	/* FS/Address match (last buffer only) */
	ushort_t	rb_buf_len;	/* buffer length */
	uchar_t	rb_fr_data[1];	/* the data */
};

/* for ADP_RING_CHNG (RING.STATUS.CHANGE) */
struct adp_ring_chng {
	uchar_t	rc_cmd;		/* command */
	uchar_t	rc_res[5];	/* reserved */
	ushort_t	rc_status;	/* current status */
};

/* for ADP_XMT_REQ (TRANSMIT.DATA.REQUEST) */
struct adp_xmt_req {
	uchar_t	ar_cmd;		/* command */
	uchar_t	ar_cmdcor;	/* command correlate */
	ushort_t	ar_res1;	/* return code */
	ushort_t	ar_stn_id;	/* station id */
	ushort_t	ar_dhb_addr;	/* offset of the DHB */
};

/* response to ADP_XMT_REQ */
struct adp_xmt_res {
	uchar_t	xr_cmd;		/* command */
	uchar_t	xr_cmdcor;	/* command correlate */
	uchar_t	xr_retcode;	/* return code */
	uchar_t	xr_res2;	/* reserved */
	ushort_t	xr_stn_id;	/* station id */
	ushort_t	xr_fr_len;	/* frame length */
	uchar_t	xr_hdr_len;	/* LAN header length */
	uchar_t	xr_rsap;	/* remote SAP value */
};

/*
 * FC field defines
 */
#define	FC_MAC	0x00		/* MAC frame */
#define	FC_LLC	0x40		/* LLC frame */

/*
 * Microchannel specific definitions
 */

#define	TR_MAXSLOTS	8	/* maximum number of slots to probe */

/* POS register info */
#define	TRPOS_SYSENAB	0x94
#define	TRPOS_ADAP_ENAB 0x96
#define	TRPOS_SETUP	0x08
#define	TRPOS_DISABLE	0x00
#define	TRPOS_REG_BASE	0x100

/* Board IDs */
#define	TRPOSID_TRA	0xE000	/* Token Ring Adapter/A */
#define	TRPOSID_TR4_16A 0xE001	/* Token Ring 16/4 Adapter/A */

/* POS register definitions */
#define	TRPOS_REG2	2
#define	TRPOS_ENABLED		0x1	/* card is enabled */

#define	TRPOS_REG3	3
#define	TRPOS_IRQ_LSB		0x80
#define	TRPOS_SECONDARY		0x01

#define	TRPOS_REG4	4
#define	TRPOS_IRQ_MSB		0x1
#define	TRPOS_MMIO_MASK		0xFE
#define	TRPOS_MMIO_SHIFT	12
#define	TRPOS_MMIO_ADDR(r)	((r[TRPOS_REG4] & TRPOS_MMIO_MASK)\
				    << TRPOS_MMIO_SHIFT)
#define	TRPOS_RAM_ADDR(r)	((r[TRPOS_REG2] & TRPOS_MMIO_MASK)\
				    << TRPOS_MMIO_SHIFT)
#define	TRPOS_RAM_SIZE(r)	(8192<<((r[TRPOS_REG3]>>2)&0x3))


#ifdef	__cplusplus
}
#endif

#endif	/* _TRREG_H */
