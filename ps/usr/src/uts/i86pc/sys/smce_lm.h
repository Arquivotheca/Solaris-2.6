/*
 * Copyright (c) 1993, 1995 by Sun Microsystems, Inc.
 * All Rights Reserved.
 */

#ifndef	_SYS_SMCELM_H
#define	_SYS_SMCELM_H

#pragma ident	"@(#)smce_lm.h	1.2	95/03/22 SMI"

/*
 * Solaris notes:
 *
 * This file was provided by SMC and contains a Lower Mac network driver
 * for the WD/SMC 8003, 8013, 8216, and 8416 cards.
 *
 * In principle we do not make changes to this file, so that when SMC
 * provides us an update with bug fixes, performance enhancements, or
 * support for additional cards, we can simply plug in the new version
 * and run with it.
 *
 * Unfortunately this file started as a rather early version of their
 * lower mac driver, and has had to have some changes so that it would
 * work under Solaris.  Besides those necessary changes, we have made
 * changes required to make this file DDI compliant.  We have not and
 * should not fix lint warnings or make any kind of stylistic changes.
 * To make cstyle changes would have the effect of making this file
 * harder to maintain in the future, when we get an updated version,
 * which is contrary to the intended purpose of running cstyle.  So:
 *
 * DO NOT LINT OR CSTYLE THIS FILE.
 *
 * ident "@(#)sm.h	1.7 - 92/06/25"
 */

#ifdef	__cplusplus
extern "C" {
#endif

/* EISA defines */
#define EISA_ID_OFFSET		0xc80
#define ID_BYTE0		0x4d
#define ID_BYTE1		0xa3
#define ID_BYTE2		0x01
#define ID_BYTE3		0x10
#define SM_SLOTS		16	/* max number of EISA slots */

#define LOCAL_DATA_REGISTER_OFFSET	0x800
#define LOCAL_INDEX_REGISTER_OFFSET	0x801
#define LOCAL_STATUS_REGISTER_OFFSET	0x802
#define MODE_REGISTER_OFFSET		0x830
#define COUNTER_OFFSET			0x840
#define NICE_1_OFFSET			0x810
#define NICE_2_OFFSET			0x820

/* NICE register offsets */
#define XMIT_STATUS			0
#define RECV_STATUS			1
#define XMIT_MASK			2
#define RECV_MASK			3
#define XMIT_MODE			4
#define RECV_MODE			5
#define CONTROL1			6
#define CONTROL2			7
#define NODEID0				8
#define BMPORT_LSB			8
#define GROUPID0			8
#define XMIT_PKT_CONTROL1		10
#define XMIT_PKT_CONTROL2		11
#define DMA_ENABLE			12

#define XMIT_CONTROL2_VALUE	0x01

#define CLR_BUS_RD_ERR			0


#define C0_ADDRESS			0xc3		/* reg=43 + hi bit set */
#define C0_COUNT			0xc0		/* reg=40 + hi bit se */
#define C0_CONFIG			0x48
#define C0_CONFIG_VALUE			0x38		/* default value */
#define C0_STATUS			0x4a
#define C0_STATUS_VALUE			0x03		/* reset LINT */
#define C0_STROBE			0x49
#define GLOBAL_CONFIG			0x08
/* #define GLOBAL_CONFIG_VALUE		0x02		* default value */
#define GLOBAL_CONFIG_VALUE		0x0A		/* default value */
#define STATUS_CONTROL_VALUE		0x10		/* 10 enable LINT */
/*****************************************************************************
 			    MODE REGISTER EQUATES
 *****************************************************************************/

/*  HIGH byte defines */ 

#define MR_RESET1			0x80		/* 0:reset 1:Active */
#define MR_M8				0x40		/* 0:M8 1:M16 */
#define MR_RESET2			0x20		/* 0:Reset 1:Active */
#define MR_TP				0x10		/* 0:AUI 1:TP */
#define MR_IRQ_SEL3			0x08		/* high bit */
#define MR_IRQ_SEL2			0x04		/* middle bit */
#define MR_IRQ_SEL1			0x02		/* low bit */
#define MR_RESET3			0x01		/* 0:Reset 1:Active */


/* LOW byte equates */

#define MR_DATA_IN			0x40		/* 0:Not accessing EEPROM */
#define MR_CLOCK			0x20		/* 0:not accessing EEPROM */
#define MR_CHIP_SELECT			0x10		/* 0:not accessing EEPROM */
#define MR_DIRECTION			0x08		/* 0:bus->network 1:network->bus */
#define MR_PORT_SELECT			0x04		/* 0:NICE#1 1:NICE#2 */
#define MR_START_BIT			0x02
#define MR_MASK_IRQ			0x01		/* 0:INT's 1:no INT's */

/*****************************************************************************
 			   MISCELLANEOUS DEFINES
*****************************************************************************/

/* #define SLAVE_XFER_CUTOFF		1600		* 1600 if bypass DMA*/	
#define SLAVE_XFER_CUTOFF		100		/* 1600 if bypass DMA*/	
#define SLAVE_XFER_CUTOFF_WD		1500
#define BOARD_TO_UNIX			1
#define UNIX_TO_BOARD			2

#define	MAX_RETRIES			100
#define PIC_CONTROL_PORT_1		0x20		/* interrupt controller 1 */
#define PIC_CONTROL_PORT_2		0xa0		/* interrupt controller 2 */
#define EOI				20h		/* eoi command for PIC */

#define LOOK_SZ				40		/* look ahead size */

#define XMIT_IN_PROGRESS	0x01
#define XMIT_READY		0x02
#define XMIT_LOADING	0x10

/*****************************************************************************
 			        NICE DEFINES
*****************************************************************************/

#define XMIT_OK		      	0x80
#define XMIT_16_COLLISIONS	0x02
#define XMIT_BUS_ERROR		0x01
#define XMIT_MASK_VALUE		XMIT_OK + XMIT_16_COLLISIONS + XMIT_BUS_ERROR

#define RECV_PACKET_READY	0x80
#define RECV_BUS_ERROR		0x40
#define RECV_SHORT_PACKET	0x08
#define RECV_ALIGNMENT_ERROR	0x04
#define RECV_CRC_ERROR		0x02
#define RECV_OVERFLOW		0x01

#define RECV_OK_MASK		RECV_OVERFLOW+RECV_CRC_ERROR+RECV_ALIGNMENT_ERROR+RECV_SHORT_PACKET+RECV_BUS_ERROR
/* #define RECV_MASK_VALUE		0xc7		* enable hw int's on all recv bits */
/* #define RECV_MASK_VALUE		0xcf		* enable hw int's on all recv bits */
#define RECV_MASK_VALUE		0xc1		/* enable hw int's on all recv bits */

#define RECV_BUFFER_EMPTY	0x40		/* in receive mode */

struct rh_struc {
/*	unsigned char status;
	unsigned char reserved;
	unsigned short bytecount; */
	unsigned char dest_addr[6];
	unsigned char src_addr[6];
	unsigned char pkt_length[2];
};

/*****************************************************************************
 			  TRANSMIT BUFFER MONITOR
*****************************************************************************/

#define MAX_PACKETS 24		/* max # of packets allowed in xmit buffer */
#define XMIT_BUF_SZ  8100

struct packet_info {
	unsigned short packet_sz;
	caddr_t packet_ecb;
	unsigned short xmit_time;
};

struct xmit_buffer_monitor {
	struct xmit_buf_monitor *xmit_link;
	unsigned char	xbm_status;
	unsigned short	bufr_bytes_avail;
	unsigned char	ttl_packet_count;
	unsigned char	number_retransmits;
	unsigned long	xmit_start_time;
	struct packet_info pkt_info[ MAX_PACKETS ] ;	
};

/*****************************************************************************
 				NIC STRUCTURE
*****************************************************************************/

struct NIC {
	unsigned char xmit_status;		/* dlcr0;  bank=0,1,2,3 */
	unsigned char recv_status;		/* dlcr1;  bank=0,1,2,3 */
	unsigned char xmit_mask;		/* dlcr2;  bank=0,1,2,3 */
	unsigned char recv_mask;		/* dlcr3;  bank=0,1,2,3 */
	unsigned char xmit_mode;		/* dlcr4;  bank=0,1,2,3 */
	unsigned char recv_mode;		/* dlcr5;  bank=0,1,2,3 */
	unsigned char control1;			/* dlcr6;  bank=0,1,2,3 */
	unsigned char control2;			/* dlcr7;  bank=0,1,2,3 */
	unsigned char node_id [ 6 ] ;		/* dlcr 8 - 13 */
	unsigned short tdr;			/* dlcr14 & 15; bank=0,1,2,3 */
	unsigned char group_id [ 8 ] ;		/* htr8 - 15, bank 1 */
	unsigned short bmport;			/* bmr 8 & 9; bank = 2,3 */
	unsigned char tx_pkt_control1;		/* bmr10; bank = 2,3 */
	unsigned char tx_pkt_control2;		/* bmr10; bank = 2,3 */
	unsigned char dma_enable;		/* bmr12; bank = 2,3 */
	unsigned char dma_dst_cnt;		/* bmr13; bank = 2,3 */
	unsigned char rcv_buf_ptr;		/* bmr13; bank = 2,3 */
	struct xmit_buffer_monitor xmit_buf[2];	/* monitor for the xmit buffer */
	unsigned char current_buf;		/* index for current xmit buffer */
	unsigned char first_send;		/* flag for first send */
};

#define tx_pkt_control2_value	0x01		/* halt after 16col; retransmit */

/*****************************************************************************
 			CARD CONFIGURATION INFORMATION
*****************************************************************************/

struct config_info	{
int x;
	/* TBD */
};

/* receive header. */

struct rxheaderstructure {
	unsigned char NICrxstatus;
	unsigned char nextpacket;
	unsigned short rxcount;
};

unsigned short NICE_1;
unsigned short NICE_2;
/* NICE_1_Node_ID		db	6	dup(0)	;00h, 80h, 0fh, 26h, 26h, 00h
NICE_2_Node_ID		db	6	dup(0)	;00h, 80h, 0fh, 28h, 28h, 00h
Status_Flag	    	dw	07fh		;bits 2, 3, and 6 always set
						;bits 0, 1, 4 and 5 init to 1
*/

/* defines for reading the eeprom */
#define ROM_CSHI	0x10
#define ROM_CSLO	0
#define ROM_CLKLO	0
#define ROM_CLKHI	0x20
#define SER_DATA_OUT	0x40
#define SER_DATA1	0x40
#define DELAY_CNT	10


/*
 * Configuration options for CM 8003
 */

#define SMVPKTSZ	(3*256)
/* #define SMHIWAT		(32*SMVPKTSZ) */
#define SMHIWAT		(32*1024)
/* #define SMLOWAT		(8*SMVPKTSZ) */
#define SMLOWAT		(16*1024)
#define SMMAXPKT	1500
#define SMMAXPKTLLC	1497	/* max packet when using LLC1 */
#define SMMINSEND	60	/* 64 - 4 bytes CRC */

/*
 *  NOTE: Structure should be ordered so that they will pack on 2 byte
 *  boundaries.
 */

struct smdev {
   unsigned short  sm_flags;	/* flags to indicate various status' */
   unsigned short  sm_type;	/* LLC/Ether */
   queue_t	  *sm_qptr;	/* points queue associated with open device */
   unsigned short  sm_mask;	/* mask for ether type or LLC SAPs */
   unsigned short  sm_state;	/* state variable for DL_INFO */
   long		   sm_sap;	/* sap or ethertype depending on sm_type */
   long		   sm_oldsap;
   struct smparam *sm_macpar;	/* board specific parameters */
   struct smstat  *sm_stats;	/* driver and board statistics */
   unsigned short  sm_no;	/* index number from front of array */
   unsigned short  sm_rws;	/* receive window size - for LLC2 */
   unsigned short  sm_sws;	/* send window size - for LLC2 */
   unsigned short  sm_rseq;	/* receive sequence number - for LLC2 */
   unsigned short  sm_sseq;	/* send sequence number - for LLC2 */
   unsigned short  sm_snap[3];	/* for SNAP and other extentions */
};

/* status bits */
#define SMS_OPEN	0x01	/* minor device is opened */
#define SMS_PROM	0x04	/* promiscuous mode enabled */
#define SMS_XWAIT	0x02	/* waiting to be rescheduled */
#define SMS_ISO		0x08	/* ISO address has been bound */
#define SMS_SU		0x80	/* opened by privileged user */
#define SMS_RWAIT	0x100	/* waiting for read reschedule */

/* link protocol type */
/* defined in lihdr.h */

struct smmaddr {
   unsigned char filterbit;	/* the hashed value of entry */
   unsigned char entry[6];	/* multicast addresses are 6 bytes */
   unsigned char dummy;	        /* For alignment problems with SCO */
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

struct FragmentStructure {
	caddr_t fragment_ptr;
	ushort	fragment_length;
};

#define MAX_FRAGS 100

struct DataBuffStructure {
	ushort	fragment_count;
	struct FragmentStructure fragment_list[MAX_FRAGS];
};

struct sm_common {
  int	    sm_init;		/* board has been initialized */
  /* struct BMIC bmic;	DOUG -- not sure we need this */
  unsigned char xfer_in_progress;
  unsigned short xfer_wait;	/* used by procs that are sleeping while */
				/* waiting for the BMIC to finish */
  long	    sm_devmode;		/* device mode (e.g. PROM) */
  long	    sm_proms;		/* number of promiscuous streams */

};

/* this structure is a kludge to get UNIX to accept the LM 
   adapter structure guidelines */
struct initparms {
	ushort adapter_num;
	ushort irq_value;
	ushort io_base;
	ushort ram_base;
	ulong ram_access;
	ushort sm_minors;
	struct sm_common *sm_cp;
	ushort media_type;
};

#define EISA_BUS_TYPE 2
#define ACCEPT_MULTICAST 0x01
#define ACCEPT_BROADCAST 0x02
#define PROMISCUOUS_MODE 0x04
#define ACCEPT_ERR_PACKETS 0x10
#define OPEN		0x01
#define INITIALIZED	0x02
#define CLOSED		0x03
#define FAILED		0x05
#define	MEDIA_BNC	0x02
#define AUI_UTP		0x01
#define BUS_EISA32M_TYPE	0x04
#define BIC_NO_CHIP	0x00
#define INTERRUPT_REQ	1
#define SUCCESS		0
#define MAX_COLLISIONS	0x0b
#define FIFO_UNDERRUN	0x10
#define ADAPTER_NOT_FOUND	0xffff
#define ADAPTER_NO_CONFIG	2
#define ADAPTER_AND_CONFIG	1
#define OUT_OF_RESOURCES	6

#define ADD_MULTICAST		1
#define DEL_MULTICAST		2

/* this represents a "logical board"...since unix thinks we have two
   boards when we actually only have one, we have to use two board
   structures for one board.  There are a set of variables that will
   have to be shared between the two board structs, we will reference
   these by a pointer to a common structure */

/* this structure differs from the LM_adap_struc...UNIX requires
   that the first seven elements be here...it's so UNIX can set
   up the values at boot time */
struct smparam {
   ushort 	    adapter_num;	/* board index */
   unsigned char    bus_type;
   unsigned char    mc_slot_num; 
   unsigned char    adapter_flags;
   ushort   	    pos_id;
   ushort   	    io_base;
   caddr_t	    adapter_text_ptr;
   ushort    	    irq_value;		/* interrupt vector number */
   ushort    	    previous_int_vector;
   short	    rom_size;		/* memory size */
   long	    	    rom_base;		/* memory address */
   long		    rom_access;
   short	    ram_size;
   long		    ram_base;
   long		    ram_access;
   short	    ram_usable;
   unsigned short   io_base_new;
   unchar   node_address[6];  	/* machine address */
   unchar   multi_address[6];  	/* multicast address */
   ushort	    max_packet_size;
   ushort	    num_of_tx_buffs;
   ushort	    receive_mask;
   ushort	    adapter_status;
   ushort	    media_type; 
   ushort	    bic_type;
   ushort	    nic_type;
   ushort	    adapter_type;

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
   short    	    sm_minors;		/* number of minor devices allowed */
   struct sm_common *sm_cp;	/* pointer to common variables */
   short    	    sm_bflags;	/* flags used for various config options */
   short    sm_major;		/* major device number */
   int	    sm_noboard;		/* board present flag */
   int	    sm_str_open;	/* number of streams open */
   long	    sm_nextq;		/* next queue to be scheduled */
   long	    sm_ncount;		/* count of bufcalls */
   short    sm_firstd;		/* first minor device for this major */
   int	    sm_enabled;		/* board has been enabled. can now receive data */
   unsigned long sm_nice_addr; /* phys addr of nice controller */
   struct NIC nic;		/* info about nice controller */
   int	    sm_multicnt;	/* number of defined multicast addresses */
   struct   smmaddr *sm_multip; /* last referenced multicast address */
   struct   smmaddr *sm_multiaddrs;	/* array of multicast addresses */
   /* Following added in the Solaris scheme, Alanka */
   caddr_t  private;		/* pointer to private data structure */
   dev_info_t *devinfo;
   unsigned short multaddr[64];
   unsigned int   totalmultaddr;
};

struct smstat {
   ulong	    no_entries;		/* number of entries */
   ulong	    rx_crc_errors;
   ulong	    rx_too_big;
   ulong	    rx_lost_pkts;
   ulong	    rx_align_errors;
   ulong	    rx_overruns;
   ulong	    tx_deferred;
   ulong	    tx_max_collisions;
   ulong	    tx_one_collisions;
   ulong	    tx_mult_collisions;
   ulong	    tx_ow_collisions;
   ulong	    tx_CD_heartbeat;
   ulong	    tx_carrier_lost;
   ulong	    tx_underruns;
};

/* board config (bflags) */
#define SMF_MEM16OK   0x01	/* safe to leave board in MEM16 mode and not */
				/* disable interrupts on the smbcopy */
#define SMF_NODISABLE 0x02	/* is it safe to not disable interrupts during */
				/* MEM16 operation */

/* Debug Flags and other info */

#define SMSYSCALL	0x01	/* trace syscall functions */
#define SMPUTPROC	0x02	/* trace put procedures */
#define SMSRVPROC	0x04	/* trace service procedures */
#define SMRECV		0x08	/* trace receive processing */
#define SMRCVERR	0x10	/* trace receive errors */
#define SMDLPRIM	0x20	/* trace DL primitives */
#define SMINFODMP	0x40	/* dump info req data */
#define SMDLPRIMERR	0x80	/* mostly llccmds errors */
#define SMDLSTATE      0x100	/* print state chages */
#define SMTRACE	       0x200	/* trace loops */
#define SMINTR	       0x400	/* trace interrupt processing */
#define SMBOARD	       0x800	/* trace access to the board */
#define SMLLC1	      0x1000	/* trace llc1 processing */
#define SMSEND	      0x2000	/* trace sending */
#define SMBUFFER      0x4000	/* trace buffer/canput fails */
#define SMSCHED       0x8000	/* trace scheduler calls */
#define SMXTRACE      0x10000	/* trace smsend attempts */
#define SMMULTHDW     0x20000	/* trace multicast register filter bits */

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

struct sm_machdr {
   unsigned char mac_dst[6];
   unsigned char mac_src[6];
   union {
      struct ethertype ether;
      struct llctype llc;
   } mac_llc;
};

typedef struct sm_machdr machdr_t;

#define ETHERADDRL		(6)
#define LLC_SAP_LEN	1	/* length of sap only field */
#define LLC_LSAP_LEN	2	/* length of sap/type field  */
#define LLC_TYPE_LEN    2	/* ethernet type field length */
#define LLC_ADDR_LEN	ETHERADDRL	/* length of 802.3/ethernet address */
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

#define LLC_LENGTH(m)	ntohs(((struct sm_machdr *)m)->mac_llc.llc.llc_length)
#define LLC_DSAP(m)	(((struct sm_machdr *)m)->mac_llc.llc.llc_dsap)
#define LLC_SSAP(m)	(((struct sm_machdr *)m)->mac_llc.llc.llc_ssap)
#define LLC_CONTROL(m)	(((struct sm_machdr *)m)->mac_llc.llc.llc_control)
#define LLC_SNAP(m)	(((struct sm_machdr *)m)->mac_llc.llc.llc_info)

#define ETHER_TYPE(m)	ntohs(((struct sm_machdr *)m)->mac_llc.ether.ether_type)

#define SMMAXSAPVALUE	0xFF	/* largest LSAP value */

/* other useful macros */

#define HIGH(x) (((x)>>8)&0xFF)
#define LOW(x)	((x)&0xFF)

/* recoverable error conditions */

#define SME_OK		0	/* normal condition */
#define SME_NOBUFFER	1	/* couldn't allocb */
#define SME_INVALID	2	/* operation isn't valid at this time */
#define SME_BOUND	3	/* stream is already bound */
#define SME_BLOCKED	4	/* blocked at next queue */
#define SME_FULL	5

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
 * SM 8003 event statistics
 */

#define SMS_NSTATS	18

#ifdef M_XENIX
typedef unsigned long	ulong;
#endif

/* datalink layer i/o controls */
#define DLIOC		('D' << 8)
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
#define MACIOC(x)	(('M' << 8) | (x))
#define MACIOC_DIAG	MACIOC(1)	/* set diagnostic mode */
#define MACIOC_UNIT	MACIOC(2)	/* unit select */
#define MACIOC_SETMCA	MACIOC(3)	/* multicast setup */
#define MACIOC_DELMCA	MACIOC(4)	/* multicast delete */
#define MACIOC_DELAMCA	MACIOC(5)	/* flush multicast table */
#define MACIOC_GETMCA	MACIOC(6)	/* get multicast table */
#define MACIOC_GETSTAT	MACIOC(7)	/* dump statistics */
#define MACIOC_GETADDR	MACIOC(8)	/* get mac address */
#define MACIOC_SETADDR	MACIOC(9)	/* set mac address */

#define ELCR1	0x4d0		/* Edge/Level Control register for intr 0-7 */
#define ELCR2	0x4d1		/* Edge/Level Control register for intr 8-16 */

#ifdef	__cplusplus
}
#endif

#endif	/* _SYS_SMCELM_H */
