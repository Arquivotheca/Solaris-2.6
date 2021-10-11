#pragma ident "@(#)nfe.h	1.10	96/02/29 SMI"

/*
 * Hardware specific driver declarations for the Netflex 2 Ethernet
 * driver conforming to the Generic LAN Driver model.
 */
#ifndef	_NFE_H
#define	_NFE_H	1

/* debug flags */
#define	NFETRACE	0x01
#define	NFEERRS		0x02
#define	NFERECV		0x04
#define	NFEDDI		0x08
#define	NFESEND		0x10
#define	NFEINT		0x20

#define	NFE_HW	0x110e
#define	NFE_TYPE_DUAL	0x62
#define	NFE_TYPE_ENTR	0x61

/* Eisa Configuration */

#define	NFE_ID0		0xc80
#define	NFE_ID1		0xc81
#define	NFE_ID2		0xc82
#define	NFE_ID3		0xc83
#define	NFE_IRQ		0xc85
#define	PROT_CFG1	0xc84	/* control register */

#define CFG_PORT1_IF	0x08
#define CFG_PORT2_IF	0x04

/* Eisa ID bytes */

#define	DUALPORT_ID0	0x0e
#define	DUALPORT_ID1	0x11
#define	DUALPORT_ID2	0x62
#define	DUALPORT_ID3	0x00
#define DUALPORT_REV	0x00

#define	ENETTR_ID0	0x0e
#define	ENETTR_ID1	0x11
#define	ENETTR_ID2	0x61
#define	ENETTR_ID3	0x00
#define ENETTR_REV	0x01

/* attach flags */

#define NFE_ALLOC_BUFFERS	0x0001
#define NFE_GLD_REGISTER	0x0002
#define NFE_START_BOARD		0x0004

/* Register offsets */

#define	SIFDAT 0
#define	SIFDAI 2
#define	SIFCMD 6
#define	SIFACL 8
#define	SIFADR 0xa
#define	SIFADX 0xc
#define	DMALEN 0xe
#define	DUAL_OFFSET 0x20

#ifdef DEBUG
#define	NFEDEBUG 1
#endif

/* Misc */
#define	NFEHIWAT	32768		/* driver flow control high water */
#define	NFELOWAT	4096		/* driver flow control low water */
#define	NFEMAXPKT	1500		/* maximum media frame size */
#define	NFEMAXFRAME	1518		/* Maximum ethernet frame size */
#define	NFE_XMIT_BUFSIZE	0x800
#define	NFE_RX_BUFSIZE		0x800
#define	NFE_NFRAMES	100		/* Maximum allocated frames */
#define NFE_SAPLEN	-2

#define	NFEIDNUM	0		/* should be a unique id; zero works */

/* board state */
#define	NFE_IDLE	0
#define	NFE_WAITRCV	1
#define	NFE_XMTBUSY	2
#define	NFE_ERROR	3

#pragma pack(1)
struct scb
{
	volatile ushort cmd;		/* Stores the DMA'd command */
	volatile ushort parm0;		/* Command- */
	volatile ushort parm1;		/* Parameters */
};

struct ssb
{
	volatile ushort cmd;		/* Result from which command */
	volatile ushort parm0;		/* Result-codes... */
	volatile ushort parm1;
	volatile ushort parm2;
};

/* A transmit buffer descriptor */

struct tlist
{
	paddr_t next;	/* Forward pointer */
	ushort cstat;	/* Used for options, and completion status */
	ushort frsize;	/* Size of whole frame */
	ushort count;	/* Size of this buffer (same as whole frame) */
	paddr_t address; /* Physical address of buffer */
};

/* A receive buffer descriptor */

struct rlist
{
	paddr_t next;	/* Forward link */
	ushort cstat;	/* Used for options, and completion status */
	ushort frame_size;	/* Size of the whole frame (R/W) */
	ushort count;	/* Room available in this buffer (R/W) */
	paddr_t address;	/* Physical address of buffer */
};


/* The INIT command */

struct nfe_initcmd
{
	short options;		/* Options */
	unsigned char cvec;	/* Command interrupt vector */
	unsigned char tvec;	/* Transmit interrupt vector */
	unsigned char rvec;	/* Receive interrupt vector */
	unsigned char reserved;
	unsigned char svec;	/* SCB_CLEAR interrupt vector */
	unsigned char avec;	/* Fatal adapter check interrupt vector */
	short rbsize;		/* Receive buffer DMA Burst size */
	short tbsize;		/* Xmit buffer DMA Burst size */
	short dma_threshold;	/* # times adapter attempts dma before abort */
	paddr_t scb;		/* Address of SCB */
	paddr_t ssb;		/* Address of SSB */
};

/* OPEN command */

struct nfe_opencmd
{
	short options;		/* Options */
	char node[6];		/* Hardware ethernet address */
	long reserved1, reserved2;
	short rlist_size;	/* Receive list internal size */
	short tlist_size;	/* Xmit list internal size */
	short buffer_size;	/* Internal buffer area size */
	ushort ram_start;	/* Unknown, not documented */
	ushort ram_end;		/* Unknown, not documented */
	char trans_max;		/* Max number internal xmit bytes */
	char trans_min;		/* Min number internal xmit bytes */
	char prod[8];		/* Not used */
};

/* Set/reset multicast command	*/
struct nfe_mccmd
{
	short options;			/* Add, delete, delete-all */
	unsigned char address[6];	/* Multicast address */
};

/* Read Statistics Log command */

struct nfe_stats
{
	ulong rx_ok;
	ulong reserved;
	ulong fcs_err;
	ulong align_err;
	ulong deferred_tx;
	ulong xs_coll;
	ulong late_coll;
	ulong carr_sense_err;
	ulong tx_ok;
	ulong coll_1;
	ulong coll_2;
	ulong coll_3;
	ulong coll_4;
	ulong coll_5;
	ulong coll_6;
	ulong coll_7;
	ulong coll_8;
	ulong coll_9;
	ulong coll_10;
	ulong coll_11;
	ulong coll_12;
	ulong coll_13;
	ulong coll_14;
	ulong coll_15;
	ulong fcs_err_last;	/* XXX - From here on not in TMS380 docs */
	ulong align_err_last;
	ulong defer_tx_last;
	ulong xs_coll_last;
	ulong late_coll_last;
	ulong carr_sense_last;
	ulong coll_1_last;
	ulong coll_2_last;
	ulong coll_3_last;
	ulong coll_4_last;
	ulong coll_5_last;
	ulong coll_6_last;
	ulong coll_7_last;
	ulong coll_8_last;
	ulong coll_9_last;
	ulong coll_10_last;
	ulong coll_11_last;
	ulong coll_12_last;
	ulong coll_13_last;
	ulong coll_14_last;
	ulong coll_15_last;
	ulong coll_last;
};

#pragma pack()

/*
 * This structure is allocated contiguous, and represents all of the
 * shared memory controll structures
 */

struct nfe_shared_mem
{
	struct scb scb;
	struct ssb ssb;
	struct tlist tlist[NFE_NFRAMES];
	struct rlist rlist[NFE_NFRAMES];
	union
	{
		struct nfe_initcmd init;
		struct nfe_opencmd open;
		struct nfe_mccmd mc;
		struct nfe_stats stats;
		char etheraddr[6];
	} cparm;
	char prod[100];
};

#pragma pack(1)

/* driver specific declarations */
struct nfeinstance {
	dev_info_t *dip;
	unsigned short nfe_type;	/* Dual ethernet or ethernet/token */
	int nfe_framesize;		/* Size of receive frames */
	int nfe_nframes;		/* # of receive frames */
	int nfe_xbufsize;		/* Size of xmit buffer */
	int nfe_xmits;			/* # of xmit buffers */
	int base_io_address;		/* Base I/O Address */

	/* DIO Addresses */

	int sifdat;
	int sifdai;
	int sifadr;
	int sifcmd;
	int sifacl;
	int sifadx;
	int dmalen;

	ushort bia;		/* internal addr of Ethernet address */
	struct scb *scb;	/* Address of SCB */
	struct ssb *ssb;	/* Address of SSB */
	struct tlist *tlist;	/* Address of xmit descriptor list */
	struct rlist *rlist;	/* Address of receive descriptor list */
	caddr_t cparm;		/* Multi-use parameter list */
	caddr_t prod;
	struct nfe_shared_mem *params_v; /* Virtual address of shared mem */
	struct nfe_shared_mem *params_p; /* Physical address of shared mem */
	struct nfe_opencmd open_parms;	/* Open parameters */
	int rtotal;		/* Total mem allocated for receive */
	caddr_t nfe_rbufs;	/* Address of receive allocated mem */
	caddr_t rbufsv[NFE_NFRAMES];	/* List of virtual addrs of rcv bufs */
	paddr_t rbufsp[NFE_NFRAMES];	/* List of physical addrs of rcv bufs */
	int ttotal;		/* Total mem allocated for xmit */
	caddr_t tbufs;		/* Address of xmit allocated mem */
	caddr_t tbufsv[NFE_NFRAMES];	/* List of virtual addrs of xmit bufs */
	paddr_t tbufsp[NFE_NFRAMES];	/* List of phys addrs of xmit bufs */
	int xmit_current;	/* Next xmit descriptor to be used */
	int xmit_last;		/* Last xmit descriptor not reaped */
	int xmit_first;		/* First xmit descriptor not reaped */
	int receive_current;	/* Next receive descriptor to be filled */
	int receive_end;	/* Last receive descriptor not filled */
	int nmcast;		/* Number of multicast addresses */
	unsigned char enable_receive;	/* nfe_start_board() called? */
	unsigned char media;	/* 10BT or DB15? */
	int flags;		/* attach flags */
	char controller;	/* first or second controller */
};

typedef struct cet_obc	/* Compaq Ethernet/Token Ring on-board code. */
{
	char name[32];	/* Name of the on-board code file. */
	caddr_t obc;	/* Pointer to the "on-board" code file in memory. */
	ushort obcs;	/* Size of the "on-board" code file in memory. */
} CET_OBC;

typedef struct obc_chap	/* TMS380 on-board code chapter header. */
{
	ushort chapter;	/* TMS380 chapter address. */
	ushort address;	/* TMS380 start address within chapter. */
	ushort bytes;	/* TMS380 bytes of code from start address. */
} OBC_CHAP;

typedef ushort TMS380_WORD;

typedef struct obc_hdr	/* TMS380 on-board code header. */
{
	ushort length;	/* Total length of on-board code header. */
	/*
	 * First chapter header. There may be more. This is just
	 * a template for the beginning of the header.
	 */
	OBC_CHAP chap_hdr;
} OBC_HDR;

#define	OBC_HDR_END	0x7ffe	/* On-board code header end marker. */


/*	Proteon Commands	*/

#define	RESET_ASSERT		0	/* Assert RESET line. */
#define	RESET_DEASSERT		1	/* Deassert RESET line. */

/*	Adapter control register bits (Eagle only) */

#define	ACTL_SWHLDA	0x0800	/* Software hold acknowledge */
#define	ACTL_SWDDIR	0x0400	/* Current SDDIR signal value */
#define	ACTL_SWHRQ	0x0200	/* Current SHRQ signal value */
#define	ACTL_PSDMAEN	0x0100	/* Pseudo DMA enable */
#define	ACTL_ARESET	0x0080	/* Adapter reset */
#define	ACTL_CPHALT	0x0040	/* Communication processor halt */
#define	ACTL_BOOT	0x0020	/* Bootstrapped CP code */
#define	ACTL_FPA	0x0010	/* Enable packet blaster */
#define	ACTL_SINTEN	0x0008	/* System interrupt enable */
#define	ACTL_PEN	0x0004	/* Adapter parity enable */
#define ACTL_MASK	0x0064
#define	NFE_RJ45 	0x0002
#define	NFE_DB15 	0x0001

/* Initialization Parameter Block */

#define	INIT_OPTS		0x9f00	/* Default INIT_OPTIONS */
#define	INIT_DMAT		0x0505	/* DMA abort threshold	 */
#define INIT_ADDR_HI		0x0001  /* Internal adapter addr 5 MSBs */
#define	INIT_ADDR_LO		0x0A00	/* Internal adapter addr 16 LSBs */
#define	HARD_ADDR		0x0
#define	SOFT_ADDR		0x4
#define	SPEED			0x0c

/* Open Parameter Block */

#define	OPEN_OPTS		0x4000	/* OPEN options */
#define	COPY_ALL_FRAMES		0x0202
#define	RAM_START		0x0640	/* Starting RAM addr (Swapped) */
#define	RAM_END			0xFE7F	/* Ending RAM addr (Swapped) */
#define	DFL_BUF			2048	/* Default buffer size		 */
#define XMIT_BUF_MIN		10
#define XMIT_BUF_MAX		10

/* Transmit CSTAT Bits (swapped) */

#define	XMT_SOF			0x0020	/* Start of frame transmit list */
#define	XMT_EOF			0x0010	/* End of frame transmit list */
#define	XMT_VALID		0x0080	/* Transmit list is valid */
#define	XMT_CPLT		0x0040	/* Transmit list complete */
#define	XMT_FRM_INT		0x0008	/* Interrupt when frame xmit complete */

/* Receive CSTAT Bits (swapped) */

#define	RCV_VALID		0x0080	/* Receive list valid */
#define	RCV_CPLT		0x0040
#define	RCV_FRM_INT		0x0008	/* Single frame interrupt */
#define	RCV_FRMWT		0x0004	/* Interframe wait */
#define	RCV_CRC			0x0002	/* Pass CRC */

/* Receive CSTAT complete bits (Swapped bytes) */

#define	RCV_SOF			0x0020	/* Start of frame transmit list */
#define	RCV_EOF			0x0010	/* End of frame transmit list */
#define	RCV_CPLT		0x0040	/* Receive list complete */

#define	CMD_SET_MCA	0x1200	/* Set Multicast Address (Super Eagle) */
#define	CET_DEL_MCA	0x0000	/* Delete a specific multicast address. */
#define	CET_ADD_MCA	0x0100	/* Add a specific multicast address. */
#define	CET_DEL_ALL_MCA	0x0200	/* Clear all multicast addresses. */
#define	CET_SET_ALL_MCA	0x0300	/* Set all multicast addresses. */

/*	SIF interrupt register commands */

#define	SIF_INT_ENABLE		0	/* Re-enable interrupts from adapter. */
#define	SIF_RESET	0xFF00	/* Reset the adapter */
#define	SIF_SSBCLR	0xA080	/* System status block clear */
#define	SIF_CMDINT	0x9080	/* Interrupt and execute scb command */
#define	SIF_SCBREQ	0x0880	/* Request interrupt when scb is clear */
#define	SIF_RCVCONT	0x0480	/* Purge active rcv frame from adapter */
#define	SIF_RCVVAL	0x8280	/* Receive list is valid */
#define	SIF_XMTVAL	0x8180	/* Transmit list is valid */

/*	SIF interrupt register status definitions */

#define	SIF_DIAGOK	0x0040	/* Bring up diags OK */
#define	SIF_INITOK	0x0070	/* Initialize OK if && 0 */
#define	SIF_SYSINT	0x0080	/* Interrupt system valid */

/*	SSB command completion codes (Swapped bytes) */

#define	SSB_DIROK	0x0080	/* Direct command success */
#define	COMMAND_REJECT	0x0200	/* Command code in SSB for rejected command. */

/*	TMS380 Command codes (swapped bytes) */

#define	CMD_OPEN	0x0300	/* Open adapter */
#define	CMD_XMT 	0x0400	/* Transmit frame */
#define	CMD_XMT_HLT	0x0500	/* Transmit halt */
#define	CMD_RCV		0x0600	/* Receive */
#define	CMD_CLOSE	0x0700	/* Close adapter */
#define	CMD_SET_GROUP	0x0800	/* Set group address */
#define	CMD_SET_FUNC	0x0900	/* Set functional address */
#define	CMD_READ_LOG	0x0A00	/* Read statistics/error log command */
#define	CMD_READ_ADAP	0x0B00	/* Read adapter */
#define	CMD_MOD_OPEN	0x0D00	/* Modify open parameters */
#define	CMD_RES_OPEN	0x0E00	/* Restore open parameters */
#define	CMD_SET_GRP16	0x0F00	/* Set 16 bits grp addr (Eagle) */
#define	CMD_SET_BRIDGE	0x1000	/* Set bridge params (Eagle) */
#define	CMD_CFG_BRIDGE	0x1100	/* Config bridge parms (Eagle) */


/*	Adapter -> Host interrupt types. */

#define	INTERRUPT_TYPE		0x000f	/* Defined by SIF Status bits 12-15. */
#define	ADAPTER_CHECK		0x0000	/* Adapter Check interrupt. */
#define	RING_STATUS		0x0004	/* Ring Status interrupt. */
#define	SCB_CLEAR		0x0006	/* SCB Cleared interrupt. */
#define	COMMAND_STATUS		0x0008	/* Command Status interrupt. */
#define	RECEIVE_STATUS		0x000a	/* Receive Status interrupt. */
#define	TRANSMIT_STATUS		0x000c	/* Transmit Status interrupt. */

#define ADAPTER_CHECK_HI	0x0001
#define ADAPTER_CHECK_LO	0x05e0

#define	REVERSE_4_BYTES(v, r)	((u_char *)(r))[0] = ((u_char *)(v))[3];\
				((u_char *)(r))[1] = ((u_char *)(v))[2];\
				((u_char *)(r))[2] = ((u_char *)(v))[1];\
				((u_char *)(r))[3] = ((u_char *)(v))[0]
#define	REVERSE_2_WORDS(v, r) 	((ushort *)(r))[0] = ((ushort *)(v))[1];\
				((ushort *)(r))[1] = ((ushort *)(v))[0]
#define	REVERSE_2_BYTES(v, r) 	((u_char *)(r))[0] = ((u_char *)(v))[1];\
				((u_char *)(r))[1] = ((u_char *)(v))[0]



#define	NFE_KVTOP(addr) (((long)hat_getkpfnum((caddr_t)(addr)) * \
				(long)pgsize) + ((long)(addr) & pgoffset))

/* Align an address to mask + 1 */
#define	NFE_ALIGN(addr, mask) (((long)(addr) + (mask)) & ~(mask))

#define	NFE_PGX(addr) \
	((((long)(addr) & pgmask) != (((long)((addr) + 1) - 1) & pgmask)))

#define	NFE_SAMEPAGE(a1, a2) (((long)(a1) & pgmask) == \
					((long)(a2) & pgmask))

/* flags */

#define NFE_INTR_ENABLED	0x1

#endif
