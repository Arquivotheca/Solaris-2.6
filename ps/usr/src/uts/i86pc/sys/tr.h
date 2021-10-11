/*
 *	tr - an LLC Class 1 driver for IBM 16/4 Token Ring Adapter
 *
 *	Copyright (c) 1993, 1996 Sun Microsystems, Inc.
 *	All Rights Reserved.
 *
 *	Copyrighted as an unpublished work.
 */

#ifndef	_TR_H
#define	_TR_H

#pragma ident	"@(#)tr.h	1.12	96/10/01 SMI"

#ifdef	__cplusplus
extern "C" {
#endif

#ifdef TR_DEBUG
int tr_debug;
#endif

/*
 * definitions for debug tracing
 */
#define	TRTRACE		0x0001	/* basic procedure level tracing */
#define	TRERRS		0x0002	/* trace errors */
#define	TRRECV		0x0004	/* trace receive path */
#define	TRSEND		0x0008	/* trace send path */
#define	TRPROT		0x0010	/* trace DLPI protocol */
#define	TRSRTE		0x0020  /* source routing debug info */
#define	TRFAST		0x0040  /* fast past specific debugging */

#define	WDOGTICKS	125
#define	TIMEOUT		250

/*
 * SAP info structure
 */
struct sap_info {
	ushort	sin_station_id;
	ushort	sin_refcnt;
	long	sin_status;
	long	sin_reason;  /* DLPI reason why a bind or unbind failed */
	long	sin_ureason; /* UNIX reason why a bind or unbind failed */
};
typedef struct sap_info sap_info_t;

/*
 * Types of packets we receive
 */
#define	TR_NORMPKT	0x1
#define	TR_BDCSTPKT	0x2
#define	TR_MULTIPKT	0x4

/*
 * MAC, LLC, and SNAP defines
 */
#define	MAC_ADDR_LEN		6	/* Length of 802(.3/.4/.5) address */
#define	ACFC_LEN		2	/* Length of AC + FC field */
#define	LLC_SAP_LEN		1	/* Length of sap only field */
#define	LLC_CNTRL_LEN		1	/* Length of control field */
#define	LLC_LSAP_LEN		2	/* Length of sap/type field  */
#define	LLC_SNAP_LEN		5	/* Length of LLC SNAP fields */
#define	LLC_8022_HDR_LEN	3	/* Full length of plain 802.2 header */
#define	LLC_SNAP_HDR_LEN	8	/* Full length of SNAP header */

/* Length of MAC address fields */
#define	MAC_HDR_SIZE	(ACFC_LEN+MAC_ADDR_LEN+MAC_ADDR_LEN)

/* Length of 802.2 LLC Header */
#define	LLC_HDR_SIZE	(MAC_HDR_SIZE+LLC_SAP_LEN+LLC_SAP_LEN+LLC_CNTRL_LEN)

/* Length of extended LLC header with SNAP fields */
#define	LLC_EHDR_SIZE	(LLC_HDR_SIZE + LLC_SNAP_LEN)

#define	MAX_ROUTE_FLD	30	/* Maximum of 30 bytes of routing info */
#define	MAX_RDFLDS	14	/* changed to 14 from 8 as per IEEE */

/* MAC layer stuff */

/*
 * Mask for checking if physical destination is a group address
 */
#define	TR_GR_ADDR	0x80

/*
 * Source Routing Route Information field.
 */
#define	TR_SR_ADDR	0x80		/* MAC utilizes source route */

/*
 * Important! Note the use of bit fields which make this definition
 * architecture specific. On a SPARC, the fields within bytes would have
 * to be declared in the opposite order to be correct.
 */
struct tr_ri {
	uchar_t	len:5;			/* length */
	uchar_t	rt:3;			/* routing type */
	uchar_t	res:4;			/* reserved */
	uchar_t	mtu:3;			/* largest frame */
	uchar_t	dir:1;			/* direction bit */
	ushort_t rd[MAX_RDFLDS];	/* route designators */
};

/* Routing types for rt field */
#define	RT_SRF		0x0		/* specifically routed frame */
#define	RT_APE		0x4		/* all paths explorer frame */
#define	RT_STE		0x6		/* spanning tree explorer frame */
#define	RT_STF		0x7		/* spanning tree routed frame */

/* Largest frame defines */
#define	MTU_516	  0x0	/* Up to 516 octects in the information field */
#define	MTU_1500  0x1	/* Up to 1500 octects in the information field */
#define	MTU_2052  0x2	/* Up to 2052 octects in the information field */
#define	MTU_4472  0x3	/* Up to 4472 octects in the information field */
#define	MTU_8191  0x4	/* Up to 8191 octects in the information field */
#define	MTU_ARB   0x7   /* Unknown number (all routes broadcast) */

/*
 * Source Route Cache table
 */
#define	SR_HASH_SIZE	256			/* Number of bins */
#define	SR_TIMEOUT	8			/* in mins */

/*
 * Source route table info
 */
struct srtab {
	struct srtab	*sr_next;		/* next in linked list */
	struct trd	*sr_tr;			/* associated tr structure */
	uchar_t		sr_mac[MAC_ADDR_LEN];	/* MAC address */
	struct tr_ri	sr_ri;			/* routing information */
	uint_t		sr_flags;		/* defined below */
	uint_t		sr_timer;
};

/*
 * Values for sr_flags;
 */
#define	SRF_PENDING	0x01		/* waiting for response */
#define	SRF_RESOLVED	0x02		/* all is well */
#define	SRF_LOCAL	0x04		/* local, don't use source route */
#define	SRF_DYING	0x08		/* waiting for delete */
#define	SRF_PERMANENT	0x10		/* permanent entry do not delete */

#define	ACFCDASA_LEN	14		/* length of AC|FC|DA|SA */

#define	TR_AC		0x10		/* Token Ring access control */
#define	TR_LLC_FC	0x40		/* Token Ring llc frame control */

/*
 * Structure of a Token Ring MAC frame including full routing info
 */
struct tr_mac_frm {
	uchar_t		ac;
	uchar_t		fc;
	uchar_t		dhost[MAC_ADDR_LEN];
	uchar_t		shost[MAC_ADDR_LEN];
	struct tr_ri	ri;
};

/*
 * Structure of a Token Ring MAC frame without routing info
 */
struct tr_nori_mac_frm {
	uchar_t		ac;
	uchar_t		fc;
	uchar_t		dhost[MAC_ADDR_LEN];
	uchar_t		shost[MAC_ADDR_LEN];
};

/*
 * 802.2 specific declarations
 */

struct trllchdr {
	unsigned char tr_dsap;
	unsigned char tr_ssap;
	unsigned char tr_ctl;
};
struct trhdr_xid {
	unsigned char llcx_format;
	unsigned char llcx_class;
	unsigned char llcx_window;
};

#define	TR_CSMACD_HDR_SIZE	(2*ETHERADDRL+2)

#define	ismulticast(cp)	(((*(uchar_t *)(cp)) == 0xC0) && \
				((*((uchar_t *)(cp)+1) == 0x0) && \
				(!(*((uchar_t *)(cp)+2) & 0x80))))

/*
 * special ioctl calls for SunSelect TR conformance
 */
#define	L_GETPPA	(('L'<<8)|1)
#define	L_SETPPA	(('L'<<8)|2)
#define	L_GETSTATS	(('L'<<8)|5)
#define	L_ZEROSTATS	(('L'<<8)|6)

#define	LI_SPPA		0x02		/* type of snioc structure */

struct ll_snioc {
	uchar_t	lli_type;
	uchar_t	lli_spare[3];
	uchar_t	lli_ppa;
	uchar_t	lli_index;
};

/* define llc class 1 and mac structures and macros */

union llc_header {
	struct llctype {
		uchar_t		llc_dsap;
		uchar_t		llc_ssap;
		uchar_t		llc_control;
		uchar_t		llc_info[3];
	}llc_sap;
	struct llc_snap {
		uchar_t		llc_dsap;
		uchar_t		llc_ssap;
		uchar_t		llc_control;
		uchar_t		org_code[3];
		ushort_t	ether_type;
		uchar_t		llc_info[3];
	}llc_snap;
	uchar_t		 ibm_route[18];
};

struct mac_llc_hdr {
	uchar_t			mac_dst[6];
	uchar_t			mac_src[6];
	union llc_header 	llc;
};

struct llc_info {
	ushort_t		ssap;		/* Source SAP */
	ushort_t		dsap;		/* Destination SAP */
	ushort_t		snap;		/* SNAP field */
	ushort_t		control;	/* LLC control Field */
	struct tr_mac_frm	*mac_ptr;	/* Pointer to the MAC header */
	struct tr_ri		*ri_ptr;	/* Pointer to route info */
	union llc_header 	*llc_ptr;	/* Pointer to the LLC header */
	ulong_t			data_offset;	/* Size of header (in bytes) */
	ushort_t		rsize;		/* Number of bytes of routing */
	ushort_t		direction;	/* Routing Direction bit */
};

struct llc_snap_hdr {
	uchar_t		d_lsap;		/* destination service access point */
	uchar_t		s_lsap;		/* source link service access point */
	uchar_t		control;	/* short control field */
	uchar_t		org[3];		/* Ethernet style organization field */
	ushort_t 	type;		/* Ethernet style type field */
};

#define	TR_MAX802SAP	0xFF	/* largest 802.2 LSAP value */
#define	TR_MINSNAPSAP	0x5DD	/* smallest SNAP LSAP value */
#define	TR_MAXSNAPSAP	0xFFFF	/* largest SNAP LSAP value */

/* DLPI specific stuff */

/*
 * DLPI request - address w/ attached sap format.
 */
struct trdlpiaddr {
	uchar_t dlpi_phys[MAC_ADDR_LEN];
	union saptypes {
		ushort_t lsap;
		uchar_t	 sap;
	} dlpi_sap;
};

#define	DL_802		0x01	/* for type field 802.2 type packets */
#define	DL_SNAP		0x02	/* for type field 802.2 SNAP type packets */
#define	DL_ROUTE	0x04	/* to indicate IBM routing field */
#define	DL_RESPONSE	0x08	/* to indicate a response type packet */
#define	DL_GROUP	0x10	/* destination is a group address */

/* recoverable error conditions */

#define	TRE_OK			0	/* normal condition */
#define	TRE_ERR			1	/* error condition */
#define	TRE_SLEEP		2	/* need to sleep */
#define	TRE_UDERR		3	/* error during unitdata request */

/* LLC specific data - should be in separate header (later) */

#define	LLC_UI		0x03	/* unnumbered information field */
#define	LLC_XID		0xAF	/* XID with P == 0 */
#define	LLC_TEST	0xE3	/* TEST with P == 0 */

#define	LLC_P		0x10	/* P bit for use with XID/TEST */
#define	LLC_XID_FMTID	0x81	/* XID format identifier */
#define	LLC_SERVICES	0x80	/* Services supported */
#define	LLC_GLOBAL_SAP	0XFF	/* Global SAP address */
#define	LLC_NULL_SAP	0X00	/* NULL SAP address */
#define	LLC_SNAP_SAP	0xAA	/* SNAP SAP */
#define	LLC_GROUP_ADDR	0x01	/* indication in DSAP of a group address */
#define	LLC_RESPONSE	0x01	/* indication in SSAP of a response */

#define	LLC_XID_INFO_SIZE	3 /* length of the INFO field */
#define	LLC_XID_CLASS_I		0x01 /* Class I */
#define	LLC_XID_CLASS_II	0x03 /* Class II */
#define	LLC_XID_CLASS_III	0x05 /* Class III */
#define	LLC_XID_CLASS_IV	0x07 /* Class IV */

/* Types can be or'd together */
#define	LLC_XID_TYPE_1		0x01 /* Type 1 */
#define	LLC_XID_TYPE_2		0x02 /* Type 2 */
#define	LLC_XID_TYPE_3		0x04 /* Type 3 */

/*
 * Adapter commands queue structure
 */
typedef struct cmdq {
	long	cmd;
	long	arg;	/* In case we don't have a stream pointer */
	struct trd	*trdp;
	struct trs	*trsp;
	mblk_t	*data;
	void	(*callback)();
	void	*callback_arg;
	struct cmdq	*next;
} cmdq_t;

/*  Stream info structure */
typedef struct trs {
	struct trs	*trs_nexts;    /* For keeping global list of streams */
	struct trs	*trs_nextwait; /* For keeping list of pending streams */
	struct trs	*trs_nexthold; /* For list of streams on hold to xmit */
	queue_t		*trs_rq;
	struct trd	*trs_dev;
	kmutex_t	trs_lock;
	long		trs_state;
	long		trs_flags;
	long		trs_minor;
	long		trs_type;
	long		trs_802sap;    /* sap stream bound to on the board */
	long		trs_usersap;   /* sap user believes stream bound to */

	/*
	 * We do a form of multicasting with token ring
	 * functional addresses.
	 */
	ulong_t		trs_multimask;
} trs_t;

/*
 * Device instance structure
 */
typedef struct trd {
	/* Pointers to onboard areas */
	struct arb  	*trd_arb;	/* Adapter Request Block */
	struct asb  	*trd_asb;	/* Adapter Status Block */
	struct srb  	*trd_srb;	/* System Request Block */
	struct ssb  	*trd_ssb;	/* System Status Block */
	struct trram	*trd_sramaddr;	/* the SRAM address */
	struct trmmio	*trd_mmio;    	/* MMIO on the board */
	uchar_t		*trd_pioaddr;	/* PIO address */
	uchar_t		*trd_wrbr;    	/* Write Region Base Register */
	paddr_t		trd_mmiophys;	/* physical address of the shared RAM */
	paddr_t		trd_sramphys; 	/* physical address of the shared RAM */
	long		trd_sramsize; 	/* SRAM size */

	/* General info about device state */
	dev_info_t	*trd_devnode;	/* Pointer into device info tree */
	long		trd_flags;	/* flags */
	long		trd_adprate;	/* Adapter data transfer rate */
	int		trd_ppanum;	/* PPA # for this board */
	unsigned long	trd_inittime; 	/* time when initialization started */
	ushort_t	trd_int;	/* interrupt vector number */
	long		trd_nproms;	/* number of promiscuous streams */
	long		trd_natts;	/* number of attached streams */

	uchar_t		trd_factaddr[MAC_ADDR_LEN]; /* factory burnt address */
	uchar_t		trd_macaddr[MAC_ADDR_LEN];  /* machine address */
	uchar_t		trd_groupaddr[MAC_ADDR_LEN]; /* group address */
	uchar_t		trd_multiaddr[MAC_ADDR_LEN]; /* multicast address */
	ulong_t		trd_multimask;	/* Current func addr mask */

	struct trd	*trd_nextd; /* Pointer to next PPA parameter struct */
	cmdq_t		*trd_cmdsq; /* queue of board commands to run */
	cmdq_t		*trd_cmdstail;	/* tail of said queue */
	mblk_t		*trd_pkt;	/* the packet ready to be sent */
	int		trd_xmitcnt;	/* number of xmit cmds queued */
	struct trs    	*trd_onhold;	/* list of streams waiting xmit */
	struct trs	*trd_onholdtail; /* tail of onhold list of streams */
	struct trs	*trd_pending;	/* list of streams waiting to wakeup */

	int    		trd_maxupkt;  /* Maximum size of headerless packet */
	long		trd_numdhb;   /* number of xmit bufs */
	long		trd_numrcvs;  /* min # of receive bufs */
	long		trd_dhbsize;  /* size of xmit bufs */
	long		trd_maxsaps;  /* max allowed SAPs */

	/*
	 * Value to stuff into largest frame bits' field
	 * when we are sending packets with source routing.
	 */
	uchar_t		trd_bridgemtu;
	short		trd_ring_num; /* Ring number of this board */

	kmutex_t	trd_intrlock;	/* protect intr-side fields */
	kcondvar_t	trd_initcv;	/* condition var for init wait */
	kstat_t		*trd_kstatp;	/* points to kernel stats struct */
	struct trdstat	*trd_statsd;	/* per device statistics */
	ddi_iblock_cookie_t trd_intr_cookie; /* for registering interrupt */
	sap_info_t	trd_saptab[TR_MAX802SAP+1];  /* device open saps */
	int		tr_open_retry;

	unsigned long	wdog_lbolt;
	int wdog_id;
	int detaching;
	int promiscuous;
	trs_t		*open_trsp;
	trs_t		*bind_trsp;
	int		not_first;
} trd_t;

/*
 * defines for trd_flags
 */
#define	TR_READY	0x0001  /* Interrupt handler and mutexes fully set up */
#define	TR_INIT		0x0002	/* device initialized */
#define	TR_OPEN		0x0004	/* device open */
#define	TR_WOPEN	0x0008	/* in process of opening */
#define	TR_XBUSY	0x0010	/* board is busy */
#define	TR_PROM		0x0020	/* promiscious mode */
#define	TR_OPENFAIL	0x0040	/* Adapter Open command failed */

/* trs_flags bits */
#define	TRS_FAST	0x0001  /* use "fast path" */
#define	TRS_RAW		0x0002	/* lower stream is in RAW mode */
#define	TRS_WBIND	0x0004  /* waiting for bind to finish */
#define	TRS_WTURN	0x0008	/* waiting for our turn to xmit */
#define	TRS_BOUND	0x0010	/* bind completed */
#define	TRS_BINDFAILED	0x0020	/* bind failed */
#define	TRS_WUBIND	0x0040  /* waiting for unbind to finish */
#define	TRS_UNBNDFAILED 0x0080  /* unbind failed */
#define	TRS_PROMSAP	0x0100  /* stream in promiscuous mode */
#define	TRS_PROMMULTI	0x0200  /* stream in promiscuous mode */
#define	TRS_AUTO_XID	0x0400	/* automatically respond to XID */
#define	TRS_AUTO_TEST	0x0800	/* automatically respond to TEST */
#define	TRS_SLEEPER	0x1000	/* waiting on board to set multimask */

#define	TR_ALL_MULTIS	0xffffffff  /* Mask to receive all multicasts */

struct trdstat {
	ulong_t trc_ipackets;
	ulong_t trc_ierrors;
	ulong_t trc_opackets;
	ulong_t trc_oerrors;
	ulong_t trc_notbufs;
	ulong_t trc_norbufs;
	ulong_t trc_nocanput;
	ulong_t trc_allocbfail;
	ulong_t trc_sralloc;
	ulong_t trc_srfree;
	ulong_t	trc_intrs;
	ulong_t	trc_rbytes;
	ulong_t	trc_tbytes;
	ulong_t	trc_brdcstrcv;
	ulong_t	trc_multircv;
};

/*
 * Export error counters via kstats mechanism.
 */
struct trkstat {
	struct kstat_named	trs_ipackets;
	struct kstat_named	trs_ierrors;
	struct kstat_named	trs_opackets;
	struct kstat_named	trs_oerrors;
	struct kstat_named	trs_notbufs;
	struct kstat_named	trs_norbufs;
	struct kstat_named	trs_nocanput;
	struct kstat_named	trs_allocbfail;
	struct kstat_named	trs_sralloc;
	struct kstat_named	trs_srfree;
	struct kstat_named	trs_intrs;
	struct kstat_named	trs_rbytes;
	struct kstat_named	trs_tbytes;
	struct kstat_named	trs_brdcstrcv;
	struct kstat_named	trs_multircv;
};

#ifdef	__cplusplus
}
#endif

#endif	/* _TR_H */
