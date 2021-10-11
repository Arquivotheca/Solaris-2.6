/*
 * This is an SMC LMAC driver.  It will periodically be updated with
 * new versions from SMC.  It is important that minimal changes be
 * made to this file, to facilitate maintenance and running diffs with
 * new versions as they arrive.  DO NOT cstyle or lint this file, or
 * make any other unnecessary changes.
 *
 * This file should be identical in the PS and Realmode source trees.
 */

#ident	"@(#)sebm.h 1.2	95/07/18 SMI"

/*
 * Module: Header File for SMC Ethernet LAN Adapters
 * Project: UMAC/LMAC for UNIX SVR3.2
 *
 *
 *		Copyright (c) 1993 by Standard MicroSystems Corporation
 *		All rights reserved.  Contains confidential information and
 *		trade secrets proprietary to
 *			Standard MicroSystems Corporation
 *			6 Hughes
 *			Irvine, California  92718
 *
 */


#ident "@(#)sebm.h	1.01 - 94/01/08"

/*
 *
 * Revision History:
 *
 * 1.01  T_NGO 01/08/94 Define ntohs locally
 * 1.0   T_NGO 06/10/93 Created
 *
 *
 */


#ifndef	FALSE
#define	FALSE	0
#endif

#ifndef	TRUE
#define	TRUE 1
#endif

#ifndef STATIC
#define STATIC static
#endif

#ifndef LM_STREAMS
typedef unsigned long	DWORD;
#endif

typedef	int LM_STATUS;
typedef	unsigned int UINT;
typedef	unsigned long ULONG;
typedef	unsigned long *PULONG;
typedef	unsigned short USHORT;
typedef	unsigned short *PUSHORT;
typedef	unsigned char UCHAR;
typedef	unsigned char *PUCHAR;
typedef	char	BOOLEAN;
typedef	void VOID;
#ifndef MC_SLOTS
#define MC_SLOTS 8
#endif

#ifndef EISA_SLOTS
#define EISA_SLOTS 8
#endif
#define SRC_ALIGN	0
#define DEST_ALIGN	1

#define nextsize(len) ((len+64)*2)

/*
 * Configuration options for WD 8003
 */

#define SEBMVPKTSZ	(3*256)
#define SEBMHIWAT		(32*SEBMVPKTSZ)
#define SEBMLOWAT		(8*SEBMVPKTSZ)
#define SEBMMAXPKT	1500
#define SEBMMAXPKTLLC	1497	/* max packet when using LLC1 */
#define SEBMMINSEND	60	/* 64 - 4 bytes CRC */
#define SEBMMINRECV	60	/* 64 - 4 bytes CRC */
#define NUM_TX_BUFFERS  2 
#define	MULTI_TX_BUFS	1	/* enable the code that makes this work right */


/* status bits */
#define SEBMS_OPEN	0x01	/* minor device is opened */
#define SEBMS_PROM	0x02	/* promiscuous mode enabled */
#define SEBMS_XWAIT	0x04	/* waiting to be rescheduled */
#define SEBMS_ISO		0x08	/* ISO address has been bound */
#define SEBMS_SU		0x80	/* opened by privileged user */
#define SEBMS_RWAIT	0x100	/* waiting for read reschedule */

/* link protocol type */
/* defined in lihdr.h */

#ifdef DEADCODE		/* Causes macro redef in UMAC */
#define		ismulticast(mp)	(*mp->b_rptr&1)
#endif


struct sebmmaddr {
   unsigned char filterbit;	/* the hashed value of entry */
   unsigned char entry[6];		/* multicast addresses are 6 bytes */
   unsigned char dummy;	      /* For alignment problems with SCO */
};

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

/* this structure is a kludge to get UNIX to accept the LM 
   adapter structure guidelines */

struct initparms {
	ushort adapter_num;
	ushort irq_value;
	ushort io_base;
	ulong ram_base;
	ulong ram_size;
	ushort sebm_minors;
	int	padlen;
/*
	ushort media_type;
	/*caddr_t adapter_text_ptr;*/
};

#ifndef LM_STREAMS
#define MAX_FRAGS 20

typedef	struct {
	DWORD		fragment_count;
	struct {
		DWORD	fragment_ptr;
		DWORD	fragment_length;
	} fragment_list[MAX_FRAGS];
} Data_Buff_Structure;

#endif /* -LM_STREAMS */

typedef struct _ADAPTER_STRUC{
	unsigned char		adapter_num;
	unsigned char		pc_bus;
	unsigned short		io_base;
  	unsigned char		adapter_name[12];
  	unsigned short		irq_value;   
  	unsigned short		rom_size;
  	unsigned long		rom_base;
	unsigned long		rom_access;	/* OS dependent */
  	unsigned short		ram_size;
  	unsigned long		ram_base;
  	unsigned long		ram_access;	/* OS dependent */
  	unsigned short		ram_usable;
  	unsigned short		io_base_new;	/* for PutConfig */
  	unsigned char		node_address[6];
  	unsigned short		max_packet_size;
  	unsigned short		num_of_tx_buffs;
  	unsigned short		receive_mask;
  	unsigned short		adapter_status;
  	unsigned short		media_type;
	unsigned short		adapter_bus;
	unsigned short		pos_id;	
	unsigned short		adapter_flags;
  	unsigned char		slot_num;
	unsigned char		rx_lookahead_size;	/* Size of UMAC's max lookahead size*/
							/* in 16-byte chunks.*/
	unsigned short		media_set;			/* Media type(s) being used */

/* Local vars for each adapter 
*/

	unsigned short		bic_type;
	unsigned short		nic_type;
	unsigned long		board_id;
/*
	unsigned short		board_id;
	unsigned short		extra_info;
*/
	unsigned short		mode_bits;
	unsigned short		status_bits;
	unsigned short		xmit_buf_size;
	unsigned short		xmit_flag_offset;
	unsigned short		config_mode;      /* 1 to save to EEROM, 0 for local save */
	unsigned short		page_offset_mask;
	unsigned long		*easp;
	unsigned char	LaarEnter;
	unsigned char	LaarExit;
	unsigned char	InterruptMask;
/*
 * Early Stuff
 */
	unsigned char	XmitRetry;
	unsigned int	XmitThreshold;
	unsigned short Early_Rx_Slope;
#ifdef BUG
	unsigned char	*rcv_start;
	unsigned char	*rcv_stop;
#else
	caddr_t		rcv_start;
	caddr_t		rcv_stop;
#endif
	unsigned char	StartBuffer;
	unsigned char	LastBuffer;
	caddr_t	Rcv_Buffer_Pending;
        unsigned int	Rcv_DMA_Pending;
        unsigned int	Xmit_DMA_Pending;
#ifndef LM_STREAMS
	caddr_t		CurRcvbuf;
	unsigned int	Page_Offset;
#endif


   caddr_t	    ptr_rx_crc_errors;
   caddr_t	    ptr_rx_too_big;
   caddr_t	    ptr_rx_lost_pkts;
   caddr_t	    ptr_rx_align_errors;
   caddr_t	    ptr_rx_overruns;
   caddr_t	    ptr_tx_deferred;
   caddr_t	    ptr_tx_max_collisions;
   caddr_t	    ptr_tx_one_collision;
   caddr_t	    ptr_tx_mult_collisions;
   caddr_t	    ptr_tx_ow_collisions;
   caddr_t	    ptr_tx_CD_heartbeat;
   caddr_t	    ptr_tx_carrier_lost;
   caddr_t	    ptr_tx_underruns;

/* now we have our smc-unix unique stuff */

   int		padlen;				/* padding length allowed */
   short    sebm_minors;		/* number of minor devices allowed */
   short    sebm_bflags;		/* flags used for various config options */
   short    sebm_major;		/* major device number */
   short    sebm_firstd;		/* first minor device for this major */
   int	   sebm_noboard;	/* board present flag */
   int	   sebm_str_open;	/* number of streams open */
   long	   sebm_nextq;		/* next queue to be scheduled */
   int	   sebm_enabled;	/* board has been enabled. can now receive data */
   int	   sebm_init;		/* board has been initialized */
   int	   sebm_txbuf_busy;
#ifndef BUG
   unsigned char multi_address[6];
#endif
   unsigned char perm_node_address[6]; /* uniq node addr for this lan */
   unsigned char   BaseRamWindow;
   unsigned char   curtxbuf; 

   unsigned char   sebm_txbufstate[NUM_TX_BUFFERS];/* flag for transmission buffer */
#ifdef BUG
   unsigned char   sebm_txbufaddr[NUM_TX_BUFFERS];/* flag for transmission buffer */
#else
   unsigned short   sebm_txbufaddr[NUM_TX_BUFFERS];/* flag for transmission buffer */
#endif
   unsigned short   sebm_txbuflen[NUM_TX_BUFFERS];/* flag for transmission buffer */
#ifdef MULTI_TX_BUFS
   unsigned char sebm_txbuffree;         /* next possible free packet */
   char sebm_txbufload;                  /* next possible loaded packet */
#endif

   unsigned char   sebm_nxtpkt;		/* page # of next packet to remove */
   unsigned char   Page_Num;
   long	    sebm_ncount;		/* count of bufcalls */
   long	    sebm_proms;		/* number of promiscuous streams */
   long	    sebm_devmode;		/* device mode (e.g. PROM) */
   int	    sebm_multicnt;	/* number of defined multicast addresses */
   struct   sebmmaddr *sebm_multip; /* last referenced multicast address */
   struct   sebmmaddr *sebm_multiaddrs;	/* array of multicast addresses */
#if ((UNIX & 0xff00) == 0x400)	/* Solaris 2.x */
   void    *sm_private;		/* for UMAC private use */
#endif
}Adapter_Struc, *Ptr_Adapter_Struc;

#define TX_FREE		0
#define TX_LOADED	1	
#define TX_MTING	2
#define TX_PENDING	3


/*
 *  NOTE: Structure should be ordered so that they will pack on 2 byte
 *  boundaries.
 */

#ifdef LM_STREAMS
struct sebmdev {
   unsigned short  sebm_flags;	/* flags to indicate various status' */
   unsigned short  sebm_type;	/* LLC/Ether */
   queue_t	  *sebm_qptr;	/* points queue associated with open device */
   unsigned short  sebm_mask;	/* mask for ether type or LLC SAPs */
   unsigned short  sebm_state;	/* state variable for DL_INFO */
   long		   sebm_sap;	/* sap or ethertype depending on sebm_type */
   long		   sebm_oldsap;
   Ptr_Adapter_Struc sebm_macpar;	/* board specific parameters */
   struct sebmstat  *sebm_stats;	/* driver and board statistics */
   struct mac_stats *pmac_stats;	/* DLPI - driver statistics */
   unsigned short  sebm_no;	/* index number from front of array */
   unsigned short  sebm_rws;	/* receive window size - for LLC2 */
   unsigned short  sebm_sws;	/* send window size - for LLC2 */
   unsigned short  sebm_rseq;	/* receive sequence number - for LLC2 */
   unsigned short  sebm_sseq;	/* send sequence number - for LLC2 */
   unsigned short  sebm_snap[3];	/* for SNAP and other extentions */
};
#endif /* LM_STREAMS */

/* Debug Flags and other info */

#define SEBMSYSCALL	0x01	/* trace syscall functions */
#define SEBMPUTPROC	0x02	/* trace put procedures */
#define SEBMSRVPROC	0x04	/* trace service procedures */
#define SEBMRECV		0x08	/* trace receive processing */
#define SEBMRCVERR	0x10	/* trace receive errors */
#define SEBMDLPRIM	0x20	/* trace DL primitives */
#define SEBMINFODMP	0x40	/* dump info req data */
#define SEBMDLPRIMERR	0x80	/* mostly llccmds errors */
#define SEBMDLSTATE      0x100	/* print state chages */
#define SEBMTRACE	       0x200	/* trace loops */
#define SEBMINTR	       0x400	/* trace interrupt processing */
#define SEBMBOARD	       0x800	/* trace access to the board */
#define SEBMLLC1	      0x1000	/* trace llc1 processing */
#define SEBMSEND	      0x2000	/* trace sending */
#define SEBMBUFFER      0x4000	/* trace buffer/canput fails */
#define SEBMSCHED       0x8000	/* trace scheduler calls */
#define SEBMXTRACE      0x10000	/* trace wdsend attempts */
#define SEBMMULTHDW     0x20000	/* trace multicast register filter bits */
#define SEBMOVERFLOW     0x40000	/* trace OVW */
#define SEBMIOCTL     0x80000	/* trace IOCTL */

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

struct sebm_machdr {
   unsigned char mac_dst[6];
   unsigned char mac_src[6];
   union {
      struct ethertype ether;
      struct llctype llc;
   } mac_llc;
};

typedef struct sebm_machdr machdr_t;

#define LLC_SAP_LEN	1	/* length of sap only field */
#define LLC_LSAP_LEN	2	/* length of sap/type field  */
#define LLC_TYPE_LEN    2	/* ethernet type field length */
#ifndef LLC_ADDR_LEN
#define LLC_ADDR_LEN	6	/* length of 802.3/ethernet address */
#endif
#define LLC_LSAP_HDR_SIZE 3
#define LLC_SNAP_SIZE   5	/* SNAP header size */
#define LLC_HDR_SIZE	(LLC_ADDR_LEN+LLC_ADDR_LEN+LLC_LSAP_HDR_SIZE+LLC_LSAP_LEN)
#define LLC_EHDR_SIZE	(LLC_ADDR_LEN+LLC_ADDR_LEN+LLC_TYPE_LEN)

#define LLC_LIADDR_LEN	(LLC_ADDR_LEN+LLC_SAP_LEN)
#define LLC_ENADDR_LEN  (LLC_ADDR_LEN+LLC_TYPE_LEN)

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
      unsigned char lbf_snap[5];
   } llcc;
};

#define LLC_LENGTH(m)	ntohs(((struct sebm_machdr *)m)->mac_llc.llc.llc_length)
#define LLC_DSAP(m)	(((struct sebm_machdr *)m)->mac_llc.llc.llc_dsap)
#define LLC_SSAP(m)	(((struct sebm_machdr *)m)->mac_llc.llc.llc_ssap)
#define LLC_CONTROL(m)	(((struct sebm_machdr *)m)->mac_llc.llc.llc_control)
#define LLC_SNAP(m)	(((struct sebm_machdr *)m)->mac_llc.llc.llc_info)

#define ETHER_TYPE(m)	ntohs(((struct sebm_machdr *)m)->mac_llc.ether.ether_type)

#define SEBMMAXSAPVALUE	0xFF	/* largest LSAP value */

/* other useful macros */

#ifdef BUG
#define HIGH(x) ((x>>8)&0xFF)
#define LOW(x)	(x&0xFF)
#else
#define HIGH(x) (((x)>>8)&0xFF)
#define LOW(x)	((x)&0xFF)
#endif

/* recoverable error conditions */

#define SEBME_OK		0	/* normal condition */
#define SEBME_NOBUFFER	1	/* couldn't allocb */
#define SEBME_INVALID	2	/* operation isn't valid at this time */
#define SEBME_BOUND	3	/* stream is already bound */
#define SEBME_BLOCKED	4	/* blocked at next queue */

/* LLC specific data - should be in separate header (later) */

#define LLC_UI		0x03	/* unnumbered information field */
#define LLC_XID		0xAF	/* XID with P == 0 */
#define LLC_TEST	0xE3	/* TEST with P == 0 */

#define LLC_P		0x10	/* P bit for use with XID/TEST */
#define LLC_XID_FMTID	0x81	/* XID format identifier */
#define LLC_SERVICES	0x01	/* Services supported */
#define LLC_GLOBAL_SAP	0XFF	/* Global SAP address */
#define LLC_GROUP_ADDR	0x01	/* indication in DSAP of a group address */
#define LLC_RESPONSE	0x01	/* indication in SSAP of a response */

#define	RETIX_ISO	(-2)
#define NOVELL_SAP	(-3)	/* Novell 802.3 mode */
#define	DEFAULT_SAP	((ulong) 0xFFFFFFFE)	/* LLI or RETIX_ISO */
#define	NETWARE_SAP	((ulong) 0x000000FF)	/* LLI */
#define MIN_ETYPE	0x5DD
#define LLC_SNAP_SAP	0xAA	/* SNAP sap from 802.1 */

#define LLC_XID_INFO_SIZE	3 /* length of the INFO field */

struct rcv_buf {
	unsigned char	status;
	unsigned char	nxtpg;
	short		datalen;
	machdr_t	pkthdr;
};

typedef struct rcv_buf rcv_buf_t;

/*
 * WD 8003 event statistics
 */

#define SEBMS_NSTATS	21	

#ifdef M_XENIX
typedef unsigned long	ulong;
#endif

struct sebmstat {
    ulong	sebms_nstats;	/* number of stat fields */
    /* non-hardware */
    ulong	sebms_nobuffer;	/* 0 */
    ulong	sebms_blocked;	/* 1 */
    ulong	sebms_blocked2;	/* 2 */
    ulong	sebms_multicast;	/* 3 */

   /* transmit */
   ulong	sebms_xpkts;	/* 4 */
   ulong	sebms_xbytes;	/* 5 */
   ulong	sebms_excoll;	/* 6 */
   ulong	sebms_one_coll;	/* 7 */
   ulong	sebms_coll;	/* 8 */
   ulong	sebms_ow_coll;	/* 9 */
   ulong	sebms_fifounder;	/* 10 */
   ulong	sebms_carrier;	/* 11 */

   /* receive */
   ulong	sebms_rpkts;	/* 12 */
   ulong	sebms_rbytes;	/* 13 */
   ulong	sebms_crc;	/* 14 */
   ulong	sebms_align;	/* 15 */
   ulong	sebms_fifoover;	/* 16 */
   ulong	sebms_lost;	/* 17 */

   /* added */
   ulong	sebms_intrs;	/* 18 */
   ulong	sebms_ovw;	/* 19 */
   ulong	sebms_rx_too_big;	/* 20 */
   ulong	sebms_lbolt;
	
};

/* datalink layer i/o controls */
#ifdef DEADCODE		/* Causes macro redef in UMAC */
#define DLIOC	('D'<<8)
#endif
#define DLGBROAD	(('D' << 8) | 3)  /* get broadcast address entry */
#define DLGSTAT		(('D' << 8) | 4)  /* get statistics values */
#define DLGADDR		(('D' << 8) | 5)  /* get physical addr of interface */
#define DLPROM		(('D' << 8) | 6)  /* toggle promiscuous mode */
#define DLSADDR		(('D' << 8) | 7)  /* set physical addr of interface */
#define DLGCLRSTAT	(('D' << 8) | 8)  /* get statistics and zero entries */
#define DLSMULT		(('D' << 8) | 9)  /* set multicast address entry */
#define DLRESET		(('D' << 8) | 10) /* reset to power up condition */
#define DLGSAP		(('D' << 8) | 11) /* get driver sap value */
#define DLGMULT		(('D' << 8) | 12) /* get multicast address entry */
#define DLGBRDTYPE	(('D' << 8) | 13) /* get board type */
#define DLDMULT 	(('D' << 8) | 14) /* delete multicast address entry */

/* mac layer i/o controls */
#ifndef	DLPI
#define MACIOC(x)	(('M' << 8) | (x))
#endif
#define MACIOC_DIAG	MACIOC(1)	/* set diagnostic mode */
#define MACIOC_UNIT	MACIOC(2)	/* unit select */
#define MACIOC_SETMCA	MACIOC(3)	/* multicast setup */
#define MACIOC_DELMCA	MACIOC(4)	/* multicast delete */
#define MACIOC_DELAMCA	MACIOC(5)	/* flush multicast table */
#define MACIOC_GETMCA	MACIOC(6)	/* get multicast table */
#ifndef	DLPI
/*#define MACIOC_GETSTAT	MACIOC(7)	/* dump statistics */
#endif
#define MACIOC_GETADDR	MACIOC(8)	/* get mac address */
#define MACIOC_SETADDR	MACIOC(9)	/* set mac address */
#define htons ntohs
#ifdef BADLINT
#define ntohs(s) ( (((s) & 0x00ff) << 8) | (((s) & 0xff00) >> 8) )
#else
#define ntohs(s) ( (((s) & 0x00ff) << 8) | (((s) & (uint)0xff00) >> 8) )
#endif
