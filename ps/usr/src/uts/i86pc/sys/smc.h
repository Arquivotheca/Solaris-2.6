/* Copyright (c) 1992, 1996  Sun Microsystems, Inc.		*/
/* All Rights Reserved.						*/
/*								*/
/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF		*/
/*	UNIX System Laboratories, Inc.				*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * Module: WD8003
 * Project: System V ViaNet
 *
 *		Copyright (c) 1987 by Western Digital Corporation.
 *		All rights reserved.  Contains confidential information and
 *		trade secrets proprietary to
 *			Western Digital Corporation
 *			2445 McCabe Way
 *			Irvine, California 92714
 */

#ifndef	_SYS_SMC_H
#define	_SYS_SMC_H

#pragma ident	"@(#)smc.h	1.18	96/10/01 SMI"

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * Configuration options for WD 8003
 */

#define	WDDEBUG 0

/* ddi property macro */
#define	WD_PROP(devi, pname, pval, plen) \
	(ddi_prop_op(DDI_DEV_T_NONE, (devi), PROP_LEN_AND_VAL_BUF, \
		DDI_PROP_DONTPASS, (pname), (caddr_t)(pval), (plen)))

#define	WD16BITBRD(boardtype) \
	((boardtype & (BOARD_16BIT | SLOT_16BIT)) == (BOARD_16BIT | SLOT_16BIT))
/*
 * When using WD_PROP calls use the following defines so that the compiler
 * catches any spelling mistakes.
 */
#define	SmcIntr		"intr"
#define	SmcReg		"reg"
#define	SmcConfig	"config"

#define	WDVPKTSZ	(3*256)
#define	WDHIWAT		(32*WDVPKTSZ)
#define	WDLOWAT		(8*WDVPKTSZ)
#define	WDMAXPKT	1500
#define	WDMAXPKTLLC	1497	/* max packet when using LLC1 */
#define	WDMINSEND	60	/* 64 - 4 bytes CRC */
#define	MAXMULTI	16	/* max number of multicast addresses */
#define	WDADDRL		(sizeof (u_short) + ETHERADDRL)

struct wddladdr {
	struct	ether_addr	dl_phys;
	u_short	dl_sap;
};

struct wdmaddr {
	int count;		/* # of streams using this multicast address */
	unsigned char filterbit; /* the hashed value of entry */
	unsigned char entry[6];	/* multicast addresses are 6 bytes */
};

struct wddev {
	unsigned short	wd_flags;	/* flags to indicate status */
	unsigned short	wd_type;	/* LLC/Ether */
	queue_t		*wd_qptr;	/* points to queue associated with */
					/* open device */
	unsigned short	wd_mask;	/* mask for ether type or LLC SAPs */
	unsigned short	wd_state;	/* state variable for DL_INFO */
	unsigned short	wd_sap;		/* sap or ethertype depending */
					/* on wd_type */
	unsigned short	wd_no;		/* index number from front of array */
	struct wdparam	*wd_macpar;	/* board specific parameters */
	struct wdstat	*wd_stats;	/* driver and board statistics */
	unsigned short	wd_snap[3];	/* for SNAP and other extensions */
	unsigned short	wd_sdu;		/* max packet size */
	unsigned short	wd_rws;		/* receive window size - for LLC2 */
	unsigned short	wd_sws;		/* send window size - for LLC2 */
	unsigned short	wd_rseq;	/* receive sequence number - for LLC2 */
	unsigned short	wd_sseq;	/* send sequence number - for LLC2 */
	struct wdmaddr	wd_maddrs[MAXMULTI];	/* multicast addresses for */
						/* this stream */
	int wd_mcount;			/* number of multicast addresses */
					/* for this stream */
	char wd_kstatinit;
};

/* status bits */
#define	WDS_OPEN	0x01	/* minor device is opened */
#define	WDS_PROM_PHY	0x02	/* promiscuous mode enabled for physical */
#define	WDS_PROM	WDS_PROM_PHY
#define	WDS_XWAIT	0x04	/* waiting to be rescheduled */
#define	WDS_RAW		0x08	/* stream is in RAW mode */
#define	WDS_PROM_SAP	0x10	/* promiscuous mode enabled for sap level */
#define	WDS_PROM_MLT	0x20	/* promiscuous mode enabled for multicast */
#define	WDS_SU		0x80	/* opened by privileged user */
#define	WDS_RWAIT	0x100	/* waiting for read reschedule */
#define	WDS_AUTO_XID	0x200	/* automatically respond to XID packets */
#define	WDS_AUTO_TEST	0x400	/* automatically respond to TEST packets */
#define	WDS_FAST	0x800	/* M_DATA fastpath */
/* link protocol type */
/* defined in lihdr.h */

union crc_reg {			/* structure for software crc */
	unsigned int value;
	struct {
		unsigned a0	:1;
		unsigned a1	:1;
		unsigned a2	:1;
		unsigned a3	:1;
		unsigned a4	:1;
		unsigned a5	:1;
		unsigned a6	:1;
		unsigned a7	:1;
		unsigned a8	:1;
		unsigned a9	:1;
		unsigned a10	:1;
		unsigned a11	:1;
		unsigned a12	:1;
		unsigned a13	:1;
		unsigned a14	:1;
		unsigned a15	:1;
		unsigned a16	:1;
		unsigned a17	:1;
		unsigned a18	:1;
		unsigned a19	:1;
		unsigned a20	:1;
		unsigned a21	:1;
		unsigned a22	:1;
		unsigned a23	:1;
		unsigned a24	:1;
		unsigned a25	:1;
		unsigned a26	:1;
		unsigned a27	:1;
		unsigned a28	:1;
		unsigned a29	:1;
		unsigned a30	:1;
		unsigned a31	:1;
	} bits;
};

struct wdparam {
	short	wd_index;	/* board index */
	short	wd_int;		/* interrupt level */
	short	wd_ioaddr;	/* I/O port for device */
	caddr_t	wd_base;	/* address of board's memory */
	long	wd_memsize;	/* memory size */
	caddr_t	wd_rambase;	/* pointer to mapped version */
	int	wd_boardtype;	/* Starlan = 2; Ethernet = 3 */
	int	wd_noboard;	/* board present flag */
	int	wd_init;	/* board status */
	int	wd_str_open;	/* number of streams open */
	short	wd_major;	/* major device number */
	short	wd_minors;	/* number of minor devices allowed */
	short	wd_bflags;	/* flags used for various config options */
	long	wd_nextq;	/* next queue to be scheduled */
	int	wd_txbuf_busy;	/* flag for transmission buffer */
	int	wd_txbuf_timeout;   /* watchdog timeout value */
	unchar	wd_nxtpkt;	/* page # of next packet to remove */
	long	wd_ncount;	/* count of bufcalls */
	long	wd_proms;	/* number of promiscuous streams */
	long	wd_devmode;	/* device mode (e.g. PROM) */
	short	wd_firstd;	/* first minor device for this major */
	dev_info_t	*wd_devi;	/* associated dev_info */
	unchar	wd_macaddr[6]; 	/* machine address */
	int	wd_multicnt;	/* number of defined multicast addresses */
	kmutex_t wd_intrlock;	/* hardware mutex */
	struct wdmaddr *wd_multip; /* last referenced multicast address */
	struct wdmaddr *wd_multiaddrs;	/* array of multicast addresses */
	unchar	wd_memenable;	/* image for 16-bit cards for MEM-16 enable */
	unchar	wd_memdisable;	/* image for 16-bit cards for MEM-16 disable */
};

/* board config (bflags) */
#define	WDF_MEM16OK   0x01	/* safe to leave board in MEM16 mode and not */
				/* disable interrupts on the wdbcopy */
#define	WDF_NODISABLE 0x02	/* is it safe to not disable interrupts */
				/* during MEM16 operation */
#define	WDF_UNSAFE8BIT 0X04	/* is it unsafe to ever switch to 8 bit mode */
				/* bug 1159363 */

/* Debug Flags and other info */

#define	WDSYSCALL	0x01	/* trace syscall functions */
#define	WDPUTPROC	0x02	/* trace put procedures */
#define	WDSRVPROC	0x04	/* trace service procedures */
#define	WDRECV		0x08	/* trace receive processing */
#define	WDRCVERR	0x10	/* trace receive errors */
#define	WDDLPRIM	0x20	/* trace DL primitives */
#define	WDINFODMP	0x40	/* dump info req data */
#define	WDDLPRIMERR	0x80	/* mostly llccmds errors */
#define	WDDLSTATE	0x100	/* print state chages */
#define	WDTRACE		0x200	/* trace loops */
#define	WDINTR		0x400	/* trace interrupt processing */
#define	WDBOARD		0x800	/* trace access to the board */
#define	WDLLC1		0x1000	/* trace llc1 processing */
#define	WDSEND		0x2000	/* trace sending */
#define	WDBUFFER	0x4000	/* trace buffer/canput fails */
#define	WDSCHED		0x8000	/* trace scheduler calls */
#define	WDXTRACE	0x10000	/* trace wdsend attempts */
#define	WDMULTHDW	0x20000	/* trace multicast register filter bits */
#define	WDDDI		0x40000	/* trace ddi routines */
#define	WDENTRY		0x80000	/* trace each routine entry */
/* define llc class 1 and mac structures and macros */

struct llctype {
	unsigned short	llc_length;
	unsigned char	llc_dsap;
	unsigned char	llc_ssap;
	unsigned char	llc_control;
	unsigned char	llc_info[1];
};

struct ethertype {
	unsigned short ether_type;
	unsigned char ether_data[1];
};

struct wd_machdr {
	unsigned char mac_dst[6];
	unsigned char mac_src[6];
	union {
		struct ethertype ether;
		struct llctype llc;
	} mac_llc;
};

typedef struct wd_machdr machdr_t;

#define	LLC_SAP_LEN	1	/* length of sap only field */
#define	LLC_LSAP_LEN	2	/* length of sap/type field  */
#define	LLC_TYPE_LEN    2	/* ethernet type field length */
#define	LLC_ADDR_LEN	6	/* length of 802.3/ethernet address */
#define	LLC_LSAP_HDR_SIZE 3
#define	LLC_HDR_SIZE	(LLC_ADDR_LEN+LLC_ADDR_LEN+LLC_LSAP_HDR_SIZE+ \
				LLC_LSAP_LEN)
#define	LLC_EHDR_SIZE	(LLC_ADDR_LEN+LLC_ADDR_LEN+LLC_TYPE_LEN)

#define	LLC_LIADDR_LEN	(LLC_ADDR_LEN+LLC_SAP_LEN)
#define	LLC_ENADDR_LEN  (LLC_ADDR_LEN+LLC_TYPE_LEN)

union llc_bind_fmt {
	struct llca {
		unsigned char  lbf_addr[LLC_ADDR_LEN];
		unsigned short lbf_sap;
	} llca;
	struct llcb {
		unsigned char  lbf_addr[LLC_ADDR_LEN];
		unsigned short lbf_sap;
		unsigned long  lbf_xsap;
		unsigned long  lbf_type;
	} llcb;
	struct llcc {
		unsigned char lbf_addr[LLC_ADDR_LEN];
		unsigned char lbf_sap;
	} llcc;
};

/* dlpi address definitions */
#define	DLPI_SAP_LEN    2	/* two bytes SAP */
#define	DLPI_PHY_LEN	LLC_ADDR_LEN	/* six bytes physical address */
#define	DLPI_ADDR_LEN	(DLPI_SAP_LEN + DLPI_PHY_LEN)	/* full DLSAP address */

/* format of address used in dlpi, 6 bytes physical followed by 2 bytes SAP */
typedef struct {
	unsigned char dlpi_addr[DLPI_PHY_LEN];
	unsigned short dlpi_sap;
} dlpiaddr_t;

#define	LLC_LENGTH(m)	ntohs(((struct wd_machdr *)m)->mac_llc.llc.llc_length)
#define	LLC_DSAP(m)	(((struct wd_machdr *)m)->mac_llc.llc.llc_dsap)
#define	LLC_SSAP(m)	(((struct wd_machdr *)m)->mac_llc.llc.llc_ssap)
#define	LLC_CONTROL(m)	(((struct wd_machdr *)m)->mac_llc.llc.llc_control)

#define	ETHER_TYPE(m)	ntohs(((struct wd_machdr *)m)->mac_llc.ether.ether_type)

#define	WDMAXSAPVALUE	0xFF	/* largest LSAP value */

/* other useful macros */

#define	HIGH(x) ((x >> 8) & 0xFF)
#define	LOW(x)	(x & 0xFF)

/* recoverable error conditions */

#define	WDE_OK		0			/* normal condition */
#define	WDE_SYSERR	0x1000			/* or'd into an errno value */
#define	WDE_ERRMASK	0x0fff			/* mask to get errno value */
#define	WDE_NOBUFFER	(WDE_SYSERR|ENOSR)	/* couldn't allocb */
#define	WDE_INVALID	DL_OUTSTATE		/* operation isn't valid at */
						/* this time */
#define	WDE_BOUND	DL_BOUND		/* stream is already bound */
#define	WDE_BLOCKED	WDE_SYSERR		/* blocked at next queue */

/* LLC specific data - should be in separate header (later) */

#define	LLC_UI		0x03	/* unnumbered information field */
#define	LLC_XID		0xAF	/* XID with P == 0 */
#define	LLC_TEST	0xE3	/* TEST with P == 0 */

#define	LLC_P		0x10	/* P bit for use with XID/TEST */
#define	LLC_XID_FMTID	0x81	/* XID format identifier */
#define	LLC_SERVICES	0x01	/* Services supported */
#define	LLC_GLOBAL_SAP	0XFF	/* Global SAP address */
#define	LLC_GROUP_ADDR	0x01	/* indication in DSAP of a group address */
#define	LLC_RESPONSE	0x01	/* indication in SSAP of a response */

#define	LLC_XID_INFO_SIZE	3 /* length of the INFO field */

struct rcv_buf {
	unsigned char	status;
	unsigned char	nxtpg;
	short		datalen;
	machdr_t	pkthdr;
};

typedef struct rcv_buf rcv_buf_t;

/*
 * WD 800X event statistics
 */

#define	WDS_NSTATS	19

struct wdstat {
	ulong	wds_nstats;	/* number of stat fields */
	/* non-hardware */
	ulong	wds_nobuffer;	/* 0 */
	ulong	wds_blocked;	/* 1 */
	ulong	wds_blocked2;	/* 2 */
	ulong	wds_multicast;	/* 3 */

	/* transmit */
	ulong	wds_xpkts;	/* 4 */
	ulong	wds_xbytes;	/* 5 */
	ulong	wds_excoll;	/* 6 */
	ulong	wds_coll;	/* 7 */
	ulong	wds_fifounder;	/* 8 */
	ulong	wds_carrier;	/* 9 */

	/* receive */
	ulong	wds_rpkts;	/* 10 */
	ulong	wds_rbytes;	/* 11 */
	ulong	wds_crc;	/* 12 */
	ulong	wds_align;	/* 13 */
	ulong	wds_fifoover;	/* 14 */
	ulong	wds_lost;	/* 15 */

	/* added */
	ulong	wds_intrs;	/* 16 */
	ulong	wds_ovw;	/* 17 */
	ulong	wds_ierrs;	/* 18 */
	ulong	wds_dog;	/* 19 */
};

struct wdkstat {
	struct kstat_named wd_ipackets;
	struct kstat_named wd_ierrors;
	struct kstat_named wd_opackets;
	struct kstat_named wd_oerrors;
	struct kstat_named wd_collisions;
	struct kstat_named wd_nobuffer;
	struct kstat_named wd_blocked;
	struct kstat_named wd_blocked2;
	struct kstat_named wd_multicast;
	struct kstat_named wd_xbytes;
	struct kstat_named wd_fifounder;	/* currently not used */
	struct kstat_named wd_carrier;		/* currently not used */
	struct kstat_named wd_rbytes;
	struct kstat_named wd_crc;
	struct kstat_named wd_align;
	struct kstat_named wd_fifoover;
	struct kstat_named wd_lost;
	struct kstat_named wd_intrs;
	struct kstat_named wd_ovw;
	struct kstat_named wd_dog;
};

/* ioctl functions to board */
/* NET_ names are for WD/ViaNet compatibility */

#define	NET_INIT	(('D' << 8) | 1)
#define	NET_UNINIT	(('D' << 8) | 2)
#define	NET_GETBROAD	(('D' << 8) | 3)
#define	DLGBROAD	(('D' << 8) | 3)
#define	NET_GETSTATUS	(('D' << 8) | 4)
#define	DLGSTAT		(('D' << 8) | 4)
#define	NET_ADDR	(('D' << 8) | 5)
#define	DLGADDR		(('D' << 8) | 5)
#define	NET_SETPROM	(('D' << 8) | 6)
#define	DLPROM		(('D' << 8) | 6)
#define	DLSADDR		(('D' << 8) | 7)
#define	NET_WDBRDTYPE	(('D' << 8) | 8)
#define	NET_WDSTATUS	(('D' << 8) | 9)
#define	DLSMULT 	(('D' << 8) | 10)
#define	DLDMULT 	(('D' << 8) | 11)
#define	DLGMULT		(('D' << 8) | 12)

/* ---- GLOBAL PROTOTYPES ---- */
int		micro_channel(dev_info_t *);
unsigned	check_slot(int);
void		wdinit(dev_info_t *);
int		wdsetup(dev_info_t *);
int		check_addr_ROM(int);
void		wdinit_board(struct wddev *);
void		wduninit_board(struct wdparam *);
/* ---- END OF GLOBAL PROTOTYPES ---- */

#ifdef	__cplusplus
}
#endif

#endif	/* _SYS_SMC_H */
