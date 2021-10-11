/*
 * Generic PC-Net/LANCE Solaris driver
 */

/*
 * Copyright (c) 1995, by Sun Microsystems, Inc.
 * All rights reserved.
 */

/*
 * Hardware specific driver declarations for the PC-Net Generic
 * driver conforming to the Generic LAN Driver model.
 */
#ifndef _PCN_H
#define	_PCN_H 1

#ident	"@(#)pcn.h 1.13	95/07/17 SMI"

/* debug flags */
#define	PCNTRACE	0x01
#define	PCNERRS		0x02
#define	PCNRECV		0x04
#define	PCNDDI		0x08
#define	PCNSEND		0x10
#define	PCNINT		0x20

#ifdef DEBUG
#define	PCNDEBUG 1
#endif

/* Misc */
#define	PCNHIWAT	32768		/* driver flow control high water */
#define	PCNLOWAT	4096		/* driver flow control low water */
#define	PCNMINPKT	64		/* minimum media frame size */
#define	PCNMAXPKT	1500		/* max media frame size (less LLC) */
#define	PCNIDNUM	0		/* should be a unique id; zero works */

/* board state */
#define	PCN_IDLE	0
#define	PCN_WAITRCV	1
#define	PCN_XMTBUSY	2
#define	PCN_ERROR	3


/*
 * Refer to AMD Data sheets for the Am7990, Am79C90, Am79C960, Am79C970
 * for details on the definitions used here.
 */

/*
 * IO port address offsets
 */
#if defined(prep)
#define _DWIO
#define SSIZE32
#elif defined(i386)
#define _WIO
#else
#error Need to select DWIO or WIO for this machine type
#endif

#if defined(_WIO)
#define	PCN_IO_ADDRESS	0x00	/* ether address PROM is here */
#define	PCN_IO_RDP	0x10	/* Register Data Port */
#define	PCN_IO_RAP	0x12	/* Register Address Port */
#define	PCN_IO_RESET	0x14	/* Reset */
#define	PCN_IO_IDP	0x16	/* ISA Bus Data Port */
#define PCN_IO_BDP	0x16	/* Bus Configuration Register Data Port */
#define	PCN_IO_VENDOR	0x18	/* Vendor specific word */
#elif defined(_DWIO)
#define	PCN_IO_ADDRESS	0x00	/* ether address PROM is here */
#define	PCN_IO_RDP	0x10	/* Register Data Port */
#define	PCN_IO_RAP	0x14	/* Register Address Port */
#define	PCN_IO_RESET	0x18	/* Reset */
#define	PCN_IO_BDP	0x1C	/* Bus Configuration Register Data Port */
#else
#error Which I/O mode is this machine?
#endif

/*
 * CSR indices
 */
#define	CSR0	0
#define	CSR1	1
#define	CSR2	2
#define	CSR3	3
#define CSR15	15
#define CSR58	58
#define	CSR88	88
#define	CSR89	89

/*
 * CSR0:
 */


#define	CSR0_INIT	(1<<0)
#define	CSR0_STRT	(1<<1)
#define	CSR0_STOP	(1<<2)
#define	CSR0_TDMD	(1<<3)
#define	CSR0_TXON	(1<<4)
#define	CSR0_RXON	(1<<5)
#define	CSR0_INEA	(1<<6)
#define	CSR0_INTR	(1<<7)
#define	CSR0_IDON	(1<<8)
#define	CSR0_TINT	(1<<9)
#define	CSR0_RINT	(1<<10)
#define	CSR0_MERR	(1<<11)
#define	CSR0_MISS	(1<<12)
#define	CSR0_CERR	(1<<13)
#define	CSR0_BABL	(1<<14)
#define	CSR0_ERR	(1<<15)

/*
 * CSR1: Initialization address.
 * CSR2: Initialization address.
 */

/*
 * CSR3:
 */

#define	CSR3_BCON    (1<<0)
#define	CSR3_ACON    (1<<1)
#define	CSR3_BSWP    (1<<2)

/*
 * CSR15:
 */
#define CSR15_DRX	(1 << 0)	/* Disable receive */
#define CSR15_DTX	(1 << 1)	/* Disable transmit */
#define CSR15_LOOP	(1 << 2)	/* Loopback enable */
#define CSR15_DXMTFCS	(1 << 3)	/* Disable transmit CRC */
#define CSR15_FCOLL	(1 << 4)	/* Force collision */
#define CSR15_DRTY	(1 << 5)	/* Disable retry */
#define CSR15_INTL	(1 << 6)	/* Internal loopback */
#define CSR15_PORTSEL	(1 << 7)	/* Port select (0 = AUI  1 = 10baseT) */
#define CSR15_LRT_TSEL	(1 << 9)	/* Low recv thrs or xmit mode select */
#define CSR15_MENDECL	(1 << 10)	/* MENDEC loopback mode */
#define CSR15_DAPC	(1 << 11)	/* Disable auto polarity correction */
#define CSR15_DLNKTST	(1 << 12)	/* Disable link status */
#define CSR15_DRCVPA	(1 << 13)	/* Disable phhysical address */
#define CSR15_DRCVBC	(1 << 14)	/* Disable broadcast */
#define CSR15_PROM	(1 << 15)	/* Promiscuous mode */

/*
 * BCR indices
 */
#define BCR_MSRDA	0
#define BCR_MSWRA	1
#define BCR_MC		2
#define BCR_LNKST	4
#define BCR_LED1	5
#define BCR_LED2	6
#define BCR_LED3	7
#define BCR_IOBASEL	16
#define BCR_IOBASEU	17
#define BCR_BSBC	18
#define BCR_EECAS	19
#define BCR_SWS		20
#define BCR_INTCON	21

/*
 * BCR_MC definitions
 */
#define BCR_MC_AUTO_SELECT	2

/*
 * BCR_SWS definitions
 */
#define BCR_SWS_PCNET_PCI	2

/*
 * Structure definitions for adapter access.
 * These structures assume no padding between members.
 */

/*
 * Initialization block
 */

#if defined(_BIT_FIELDS_LTOH)
#if defined(SSIZE32)
struct PCN_InitBlock {
	ushort	MODE;
	unchar	: 4;
	unchar 	RLEN : 4;
	unchar	: 4;
	unchar	TLEN : 4;
	ushort	PADR[3];
	ulong	LADRF[2];
	ulong	RDRA;
	ulong	TDRA;
};
#else
struct PCN_InitBlock {
	ushort	MODE;
	ushort	PADR[3];
	ushort	LADRF[4];
	ushort	RDRAL;
	ushort 	RDRAU : 8;
	ushort	RRES: 4;
	ushort	RZERO : 1;
	ushort	RLEN : 3;
	ushort	TDRAL;
	ushort 	TDRAU : 8;
	ushort	TRES : 4;
	ushort	TZERO : 1;
	ushort	TLEN : 3;
};
#endif
#else /* ! defined(_BIT_FIELDS_LTOH) */
#error Only low to high bit field description present
#endif /* ! defined(_BIT_FIELDS_LTOH) */

/*
 * MODE:
 */

#define	MODE_DRX	(1<<0)
#define	MODE_DTX	(1<<1)
#define	MODE_LOOP	(1<<2)
#define	MODE_DTCR	(1<<3)
#define	MODE_COLL	(1<<4)
#define	MODE_DRTY	(1<<5)
#define	MODE_INTL	(1<<6)
#define	MODE_EMBA	(1<<7)
#define	MODE_PROM	(1<<15)

/*
 * Message Descriptor
 *
 */

#if defined(_BIT_FIELDS_LTOH)
#if defined(SSIZE32)

union PCN_RecvMsgDesc {
	struct {
		ulong rbadr;			/* buffer address */
		volatile ulong bcnt : 12;	/* buffer byte count */
		volatile ulong ones : 4;	/* must be ones */
		volatile ulong : 8;
		volatile ulong enp : 1;		/* end of packet */
		volatile ulong stp : 1;		/* start of packet */
		volatile ulong buff : 1;	/* buffer error */
		volatile ulong crc : 1;		/* CRC error */
		volatile ulong oflo : 1;	/* overflow error */
		volatile ulong fram : 1;	/* framing error */
		volatile ulong err : 1;		/* or'ing of error bits */
		volatile ulong own : 1;		/* owner */
		volatile ulong mcnt : 12;	/* message byte count */
		volatile ulong zeros : 4;	/* will be zeros */
		volatile ulong rpc : 8;		/* runt packet count */
		volatile ulong rcc : 8;		/* receive collision count */
		ulong reserved;
	} rmd_bits;
	ulong rmd[4];
};

union PCN_XmitMsgDesc {
	struct {
		ulong tbadr;			/* buffer address */
		volatile ulong bcnt: 12;	/* buffer byte count */
		volatile ulong ones : 4;	/* must be ones */
		volatile ulong : 8;
		volatile ulong enp : 1;		/* end of packet */
		volatile ulong stp : 1;		/* start of packet */
		volatile ulong def : 1;		/* deferred */
		volatile ulong one : 1;		/* one retry required */
		volatile ulong more : 1;	/* more than 1 retry required */
		volatile ulong add_fcs : 1;	/* add FCS */
		volatile ulong err : 1;		/* or'ing of error bits */
		volatile ulong own : 1;		/* owner */
		volatile ulong trc : 4;		/* transmit retry count */
		volatile ulong : 12;
		volatile ulong tdr : 10;	/* time domain reflectometry */
		volatile ulong rtry : 1;	/* retry error */
		volatile ulong lcar : 1; 	/* loss of carrier error */
		volatile ulong lcol : 1;	/* late collision error */
		volatile ulong exdef : 1;	/* excessive deferral */
		volatile ulong uflo : 1;	/* underflow */
		volatile ulong buff : 1;	/* buffer error */
		ulong reserved;
	} tmd_bits;
	ulong tmd[4];
};

#else

union PCN_RecvMsgDesc {
	struct {
		ushort rbadrl;			/* lsbs of addr */
		volatile ushort rbadru : 8;	/* msbs of addr */
		volatile ushort enp : 1;	/* end of packet */
		volatile ushort stp : 1;	/* start of packet */
		volatile ushort buff : 1;	/* buffer error */
		volatile ushort crc : 1;	/* CRC error */
		volatile ushort oflo : 1;	/* overflow error */
		volatile ushort fram : 1;	/* framing error */
		volatile ushort err : 1;	/* frame error */
		volatile ushort own : 1;	/* owner (0 = host 1 = PCnet) */
		volatile ushort bcnt : 12;	/* buffer byte count */
		ushort ones : 4;		/* must be ones */
		volatile ushort mcnt : 12;	/* message byte count */
		ushort zeros : 4;		/* will be zeros */
	} rmd_bits;
	ushort rmd[4];
};

union PCN_XmitMsgDesc {
	struct {
		ushort tbadrl;			/* lsbs of addr */
		volatile ushort tbadru : 8;	/* msbs of addr */
		volatile ushort enp : 1;	/* end of packet */
		volatile ushort stp : 1;	/* start of packet */
		volatile ushort def : 1;	/* deferred */
		volatile ushort one : 1;	/* one retry required */
		volatile ushort more : 1;	/* more than 1 retry required */
		volatile ushort add_fcs : 1;	/* add FCS */
		volatile ushort err : 1;	/* frame error */
		volatile ushort own : 1;	/* owner (0 = host 1 = PCnet) */
		volatile ushort bcnt : 12;	/* buffer byte count */
		ushort ones : 4;		/* must be ones */
		volatile ushort tdr : 10;	/* time domain reflectometry */
		ushort rtry : 1;		/* retry error */
		ushort lcar : 1;		/* loss of carrier */
		ushort lcol : 1;		/* late collision */
		ushort exdef : 1;		/* excessive deferral */
		ushort uflo : 1;		/* underflow error */
		ushort buff : 1;		/* buffer error */
	} tmd_bits;
	ushort tmd[4];
};
#endif
#else /* ! defined(_BIT_FIELDS_LTOH) */
#error Only low to high bit field description present
#endif /* ! defined(_BIT_FIELDS_LTOH) */

/*
 * PCI Constants
 */
#define	PCI_AMD_VENDOR_ID	0x1022
#define	PCI_PCNET_ID		0x2000
#define	PCI_PCNET_BASE_ADDR	0x10
#define	PCI_PCNET_IRQ_ADDR	0x3C


/*
 * Bus scan array
 */

#define	PCN_IOBASE_ARRAY_SIZE	16

struct pcnIOBase {
	ushort	iobase;
	int	irq;
	int	dma;
	int	bustype;
	ulong	cookie;
};

#define	PCN_BUS_ISA	0
#define	PCN_BUS_EISA	1
#define	PCN_BUS_PCI	2
#define	PCN_BUS_MCA	3
#define	PCN_BUS_UNKNOWN -1


/*
 * LANCE/Ethernet constants
 */
#define	LANCE_FCS_SIZE	4
#define	LADRF_LEN	64

/*
 * Buffer ring definitions
 */

#define	PCN_RX_RING_VAL		7
#define	PCN_RX_RING_SIZE	(1<<PCN_RX_RING_VAL)
#define	PCN_RX_RING_MASK	(PCN_RX_RING_SIZE-1)
#define	PCN_RX_BUF_SIZE		128

#define	PCN_TX_RING_VAL		7
#define	PCN_TX_RING_SIZE	(1<<PCN_TX_RING_VAL)
#define	PCN_TX_RING_MASK	(PCN_TX_RING_SIZE-1)
#define	PCN_TX_BUF_SIZE		128

#define	NextRXIndex(index)	(((index)+1)&PCN_RX_RING_MASK)
#define	NextTXIndex(index)	(((index)+1)&PCN_TX_RING_MASK)
#define	PrevTXIndex(index)	(((index)-1)&PCN_TX_RING_MASK)

struct PCN_IOmem {
	struct PCN_IOmem    *next;

	void	*vbase;		/* virtual base address */
	void	*vptr;		/* virtual current pointer */
	ulong	pbase;		/* physical base address */
	ulong	pptr;		/* physical current pointer */
	int	avail;		/* number of bytes available */
};


/*
 * Each attached PCN is described by this structure
 */
struct pcninstance {
	dev_info_t	*devinfo;
	ddi_acc_handle_t	io_handle;
	int			io_reg;
	int	dma_attached;
	int	init_intr;
	int	irq_scan;
	int	dma_scan;

	/*
	 * Memory allocator management
	 */
	ddi_dma_lim_t	dmalim;
	ulong		page_size;
	struct PCN_IOmem *iomemp;

	/*
	 * Multi-cast list management
	 */
	int	mcref[LADRF_LEN];

	/*
	 * Init block management
	 */
	struct PCN_InitBlock *initp;
	ulong	phys_initp;

	/*
	 * Receive ring management
	 */
	void	*rx_ringp;
	int	rx_index;
	unchar	*rx_buf[PCN_RX_RING_SIZE];

	/*
	 * Transmit ring management
	 */
	void	*tx_ringp;
	int	tx_index;	/* next ring index to use */
	int	tx_index_save;	/* save of TX index for ISR/stats collection */
	int	tx_avail;	/* # of descriptors available */
	unchar	*tx_buf[PCN_TX_RING_SIZE];

};

#endif	/* _PCN_H */
