/* 
 * Copyright 1995 Sun Microsystems, Inc.
 * All rights reserved.
 */
#pragma ident "@(#)riles.h	1.2	95/01/12 SMI"

/*
 * NAME
 *		riles.h 
 *
 *
 * SYNOPIS
 *      Hardware specific driver declarations for the "Racal Interlan ES3210"
 * driver conforming to the Generic LAN Driver model on Solaris 2.1 (x86).
 *		Depends on the gld module of Solaris 2.1 (Generic LAN Driver)
 *
 * 
 * DESCRIPTION
 *		The riles Ethernet driver is a multi-threaded, dynamically loadable,
 * gld-compliant, clonable STREAMS hardware driver that supports the
 * connectionless service mode of the Data Link Provider Interface,
 * dlpi (7) over an Racal Interlan ES-3210 controller. The driver
 * can support multiple ES-3210 controllers on the same system. It provides
 * basic support for the controller such as chip initialization,
 * frame transmission and reception, multicasting and promiscuous mode support,
 * and maintenance of error statistic counters.
 *      For more details refer riles (7).
 *
 *
 * SEE ALSO
 *  riles.c, the corresponding source file
 *	/kernel/misc/gld
 *  riles (7)
 *  dlpi (7)
 *	"Skeleton Network Device Drivers",
 *		Solaris 2.1 Device Driver Writer's Guide-- February 1993
 *
 *
 * MODIFICATION HISTORY
 * Version 1.1 10 Jun 1994
 *
 * MISCELLANEOUS
 * 		vi options for viewing this file::
 *				set ts=4 sw=4 ai wm=4
 */

#ifndef _RILES_H
#define _RILES_H 1

/*
 * debug flags
 */

#define DEBUG_ERRS      0x001   /* enable err messages during receipt/send */
#define DEBUG_RECV      0x002   /* to trace receive routine */
#define DEBUG_DDI       0x004   /* to trace all DDI entry points */
#define DEBUG_SEND      0x008   /* to trace send routine */
#define DEBUG_INT       0x010   /* to trace interrupt service routine */
#define DEBUG_DMA       0x020   /* to trace DMA setup routine */
#define DEBUG_BOARD     0x040   /* to trace board programming functions */
#define DEBUG_NVM       0x080   /* to trace functions that access nvm */
#define DEBUG_MCAST     0x100   /* to debug mcast addr addition & deletion */
#define DEBUG_WDOG      0x200   /* to debug calls to riles_watchdog() */
#define DEBUG_TRACE     0xFFF   /* to trace everything (enable all flags) */

#ifdef DEBUG
#define RILESDEBUG 1
#endif

#ifndef TRUE
#define TRUE	1
#define FALSE   0
#endif

#define SUCCESS       0
#define FAILURE       -1
#define RETRY         1         /* used only by riles_send() */

#ifdef _KERNEL
#define MAX_RILES_BOARDS 0x10         /* no. of EISA slots to be probed */
#define EISA_ELCR0       0x4d0        /* EISA edge level control reg 0 */
#define EISA_ELCR1       0x4d1        /* EISA edge level control reg 1 */
#define RILES_WDOG_TICKS 100 * 3      /* every 3 seconds */
#endif

/*
 * flags used by rilesp->riles_watch for the watchdog routine
 */

#define RILES_ACTIVE     0x01   /* board has been active */
#define RILES_NOXVR      0x02   /* no transceiver (possibly) */

/*
 * Used during DMA setup
 */

#define RILESSEND        0x01   /* xfers from host to board */
#define RILESRECV        0x02   /* xfers from board to host */

/*
 * Miscellaneous flags, used in STREAMS setup
 */

#define RILESHIWAT	32768		/* driver flow control high water */
#define RILESLOWAT	4096		/* driver flow control low water */
#define RILESMAXPKT	1500		/* maximum media frame size */
#define RILESIDNUM	0           /* should be a unique id; zero works */

/*
 * board state
 */

#define RILES_IDLE      0x00
#define RILES_RUNNING   0x01
#define RILES_XMTBUSY   0x02
#define RILES_DMA       0x04

/* 
 * MAC DEFINES 
 */

#define OUREISA_ID        0x2949    /* EISA ID of the board */
#define XMIT_BUF_SIZE     1600      /* Size of the temp xmit buffer */
#define NUM_XMIT_BUFS     0x06      /* Number of transmit buffers */
#define RILES_NUM_PAGES   0x40      /* Number of 256k pages in DPRAM */
#define RILES_MAXMC_ADDR  10        /* Max multicast adrs allowed */
#define	SHARED_MEM_ENAB   0x80      /* Enable shared memory accesses */
#define SHARED_MEM_SIZE   0x4000    /* size of shared memory: 16K (fixed) */
#define MAX_XMIT_RETRIES  5         /* max attempts to retry transmit */
#define MIN_FRAME_LEN	  60        /* min length of frame - CRC header */

#define	DMASZ             1024      /* minimum DMA size */

#define	DMAIN	4	/* In (brd to host) direction for DMA controller mode */
#define	DMAOUT	8	/* Out (host to brd) direction for EISA DMA ctrlr */


/*
 * useful macros
 */

/*
 * RILES_PRINT_EADDR prints the Ethernet address
 * The 6 bytes of the Ethernet address are pointed at by ether_addr
 */
		  
#define RILES_PRINT_EADDR(ether_addr)  \
{\
    int byte;\
    for (byte = 0; byte < ETHERADDRL; byte++)\
        cmn_err(CE_CONT, "%2x ", ether_addr[byte]);\
}

/*
 * Board registers of the ES-3210
 */

#define  PRODUCT_ID1(bio)	(bio + 0xC80)   /* R Product Id 1st byte */
#define  PRODUCT_ID2(bio)	(bio + 0xC81)   /* R Product Id 2nd byte */
#define  PRODUCT_ID3(bio)	(bio + 0xC82)   /* R Product Id 3rd byte */
#define  PRODUCT_ID4(bio)	(bio + 0xC83)   /* R Product Id 4th byte */
#define  EXBOCTL(bio)       (bio + 0xC84)   /* W Expansion Board control */
#define  BOARD_ENABLE       (0x01)          /* board enabled */
#define  IOCHKRST           (0x02)          /* reset ES-3210 */

/*
 * Start of I/O location containing the Ethernet address in the board's PROM
 */

#define  PROM_PAR0(bio)	(bio + 0xC90)
#define  START_DMA		PROM_PAR0	        /* start dma request */

/*
 * Offsets 0xCA0 - 0xCBF in the I/O address space are used by the
 * NIC internal registers
 */

#define  NIC_CR(bio)	(bio + 0xCA0)    /* R/W command register */
#define  CR_STP         (0x01)			 /* stop */
#define  CR_STA         (0x02)           /* start */
#define  CR_TPX         (0x04)           /* transmit packet */
#define  CR_RD0         (0x08)           /* remote dma ctl */
#define  CR_RD1         (0x10)           /* remote dma ctl */
#define  CR_RD2         (0x20)           /* remote dma ctl */
#define  CR_RD_READ     (0x08)           /* remote dma ctl */
#define  CR_RD_WRIT     (0x10)           /* remote dma ctl */
#define  CR_RD_SEND     (0x18)           /* remote dma ctl */
#define  CR_RD_ABOR     (0x20)           /* remote dma ctl */
#define  CR_PS0         (0x40)           /* page select bit */
#define  CR_PS1         (0x80)           /* page select bit */
#define  CR_PS01        (0xC0)           /* page select bit */

/*
 * page select values (refer pg. 1-17 of 8390 NIC specs)
 */

#define  SELECT_PAGE0      (0x00)              /* Select Page 0 */
#define  SELECT_PAGE1      (0x40)              /* Select Page 1 */
#define  SELECT_PAGE2      (0x80)              /* Select Page 2 */

/* 
 * Page zero register values of the NIC
 */

#define  NIC_PSTART(bio) (bio + 0xCA1)  /* page start register */
#define  NIC_PSTOP(bio) (bio + 0xCA2)   /* page stop register */
#define  NIC_BNDRY(bio) (bio + 0xCA3)   /* NIC boundary pointer */
#define  NIC_TSR(bio)   (bio + 0xCA4)   /* transmit status register */
#define  NIC_TPSR(bio)	(bio + 0xCA4)   /* transmit page start register */
#define  NIC_NCR(bio)	(bio + 0xCA5)   /* number of collisions register */
#define  NIC_TBCR0(bio)	(bio + 0xCA5)   /* transmit byte count 0 register */
#define  NIC_TBCR1(bio)	(bio + 0xCA6)   /* transmit byte count 1 register */
#define  NIC_FIFO(bio)	(bio + 0xCA6)   /* FIFO */
#define  NIC_ISR(bio)	(bio + 0xCA7)   /* interrupt status register */
#define  NIC_RSAR0(bio) (bio + 0xCA8)   /* remote start address reg 0 (W) */
#define  NIC_CRDA0(bio) (bio + 0xCA8)   /* current remote DMA reg 0 (R) */
#define  NIC_RSAR1(bio) (bio + 0xCA9)   /* remote start address reg 1 (W) */
#define  NIC_CRDA1(bio) (bio + 0xCA9)   /* current remote DMA reg 1 (R) */

/*
 * defines to interpret the interrupt mask register
 * refer pg. 1-19 of NIC 8390 specs
 */

#define  IMR_PRXE       (0x01)              /* packet received enable */
#define  IMR_PTXE       (0x02)              /* packet transmitted enable */
#define  IMR_RXEE       (0x04)              /* receive error enable */
#define  IMR_TXEE       (0x08)              /* transmit error enable */
#define  IMR_OVWE       (0x10)              /* overwrite warning enable */
#define  IMR_CNTE       (0x20)              /* counter overflow enable */
#define  IMR_RDCE       (0x40)              /* remote dma complete enable */

/*
 * defines to interpret the data configuration register
 * refer pg. 1-20 of NIC 8390 specs
 */

#define  DCR_WTS        (0x01)              /* word-wide dma transfers */
#define  DCR_LAS        (0x04)              /* long address select */
#define  DCR_LS         (0x08)              /* loopback disabled */
#define  DCR_FT0        (0x20)              /* 8 bytes FIFO threshold */

/*
 * defines to interpret the transmit status register
 * refer pg. 1-22 of NIC 8390 specs
 */

#define  TSR_PTX        (0x01)              /* packet trasmitted */
#define  TSR_COL        (0x04)              /* transmit collided */
#define  TSR_ABT        (0x08)              /* transmit aborted */
#define  TSR_CRS        (0x10)              /* carrier sense lost */
#define  TSR_FU         (0x20)              /* FIFO underrun */
#define  TSR_CDH        (0x40)              /* CD heartbeat */

/*
 * defines to interpret the interrupt status register
 * refer pg. 1-22 of NIC 8390 specs
 */

#define  ISR_PRX        (0x01)              /* packet received */
#define  ISR_PTX        (0x02)              /* packet transmitted */
#define  ISR_RXE        (0x04)              /* receive error */
#define  ISR_TXE        (0x08)              /* transmit error  */
#define  ISR_OVW        (0x10)              /* overwrite warning */
#define  ISR_CNT        (0x20)              /* counter overflow */
#define  ISR_RDC        (0x40)              /* remote dma complete */
#define  ISR_RST        (0x80)              /* reset status */

/*
 * defines to program the receiver configuration register
 * refer pg. 1-23 of NIC 8390 specs
 */

#define  RCR_BCAST      (0x04)              /* accept broadcast addresses */
#define  RCR_MCAST      (0x08)              /* accept multicast addresses */
#define  RCR_PROM       (0x10)              /* (physical) promiscuous mode */

/*
 * defines to interpret the receiver status register
 * refer pg. 1-24 of NIC 8390 specs
 */

#define  RSR_PRX        (0x01)              /* packet received intact */
#define  RSR_CRC        (0x02)              /* CRC error */
#define  RSR_FAE        (0x04)              /* frame alignment error */
#define  RSR_FO         (0x08)              /* FIFO overrun */
#define  RSR_MPA        (0x10)              /* missed packet */
#define  RSR_DIS        (0x40)              /* receiver disabled */

#define  NIC_RSAR0(bio)	(bio + 0xCA8)       /* remote start addr 0 */
#define  NIC_RSAR1(bio)	(bio + 0xCA9)       /* remote start addr 1 */

#define  NIC_RBCR0(bio)	(bio + 0xCAA)       /* remote byte count reg 0 */
#define  NIC_RBCR1(bio)	(bio + 0xCAB)       /* remote byte count reg 1 */

#define  NIC_RSR(bio)	(bio + 0xCAC)       /* receive status register */
#define  NIC_RCR(bio)	(bio + 0xCAC)       /* receiver configuration reg */

#define  NIC_CNTR0(bio)	(bio + 0xCAD)       /* alignment error count */
#define  NIC_TCR(bio)	(bio + 0xCAD)       /* transmit configuration reg */

#define  NIC_CNTR1(bio)	(bio + 0xCAE)       /* CRC errors count */
#define  NIC_DCR(bio)	(bio + 0xCAE)       /* data configuration register */

#define  NIC_CNTR2(bio)	(bio + 0xCAF)       /* R Missed Packet count */
#define  NIC_IMR(bio)	(bio + 0xCAF)       /* W Int. Mask Reg */

/*
 * Page one register values
 */

/*
 * Start i/o address of programmable ethernet address byte registers
 */
#define  NIC_PAR0(bio)	(bio + 0xCA1)

#define  NIC_CURR(bio)	(bio + 0xCA7) /* current page register */

/*
 * Start i/o address of programmable multicast address registers
 */
#define  NIC_MAR0(bio)	(bio + 0xCA8)

/*
 * configuration registers of the ES-3210
 */

#define  CONF_REG1(bio)	(bio + 0xCC0)
#define  CONF_REG2(bio)	(bio + 0xCC1)
#define  CONF_REG3(bio)	(bio + 0xCC2)
#define  CONF_REG4(bio)	(bio + 0xCC3)
#define  CONF_REG5(bio)	(bio + 0xCC4)   /* used for DMA (R.F.U.?) */
#define  THINNET    (0x00)              /* BNC connector */
#define  THICKNET   (0x01)	            /* AUI connector */
#define  RAWAR(bio)	(bio + 0xCC5)       /* receive address wrap register */
#define  RARC(bio)	(bio + 0xCC6)       /* receive address reg counter */
#define  TARC(bio)	(bio + 0xCC7)       /* transmit address reg counter*/
#define  DATAPORT(bio)	(bio + 0xCE0)   /* data port for I/O mapped I/O */

/*
 * Ethernet address type
 */

typedef unsigned char enet_address_t[ETHERADDRL];

typedef  union
{   long    lw;
	ushort  word[2];
    unchar  byte[4];
} longword_union;

typedef  struct eisaslot 
{
	short        num;
	NVM_SLOTINFO info;
} eisaslot;

typedef struct 
{
	enet_address_t byte;  /* Multicast addrs are 6 bytes */
} multicast_t;

/*
 * driver specific declarations
 */

#ifdef _KERNEL
struct rilesinstance
{
	caddr_t  ram_virt_addr_start;   /* virtual address (start) of DPRAM */
	caddr_t  ram_virt_addr_end;     /* virtual address (end) of DPRAM */
	ushort   irq_level;             /* IRQ value  */
	short    dma_channel;           /* DMA channel */
	ulong    ram_phys_addr;         /* physical address of DPRAM */
	unchar   nic_bndry;             /* NIC boundary pointer */
	unchar   nic_rcr;               /* NIC reciever configuration register */
	unchar   riles_flags;           /* current state of NIC :: xmit/idle */

	multicast_t mcast_addr[GLD_MAX_MULTICAST];
	int      mcast_count;           /* number of multicast addresses */

	caddr_t  xmit_bufp;             /* temporary transmit buffer */
	ushort   pkt_size;              /* size of packet in xmit buffer */
	ushort   retry_count;           /* maximum no. of transmission retries */

	int      riles_watch;           /* to keep track of functional boards */
	int      timeout_id;            /* used to cancel a pending timeout */
};
#endif _KERNEL
#endif
