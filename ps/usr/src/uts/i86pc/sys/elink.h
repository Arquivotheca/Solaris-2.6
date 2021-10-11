/*
 * Copyright (c) 1993, 1996 by Sun Microsystems, Inc.
 * All Rights Reserved.
 */

#ifndef	_SYS_ELINK_H
#define	_SYS_ELINK_H

#pragma ident	"@(#)elink.h	1.4	96/10/09 SMI"

/*
 * Hardware specific driver declarations for the Ether Link 16
 * driver conforming to the Generic LAN Driver model.
 */

#ifdef	__cplusplus
extern "C" {
#endif

#ifndef TRUE
#define	TRUE 			1
#define	FALSE 			0
#endif

#define	SUCCESS			0
#define	FAILURE			-1
#define	RETRY			1

/* debug flags */
#define	ELINKTRACE		0x01
#define	ELINKERRS		0x02
#define	ELINKRECV		0x04
#define	ELINKDDI		0x08
#define	ELINKSEND		0x10
#define	ELINKINT		0x20

#ifdef DEBUG
#define	ELINKDEBUG 		1
#endif

/* Misc */
#define	ELINKHIWAT		32768	/* driver flow control high water */
#define	ELINKLOWAT		4096	/* driver flow control low water */
#define	ELINKMAXPKT		1500	/* maximum media frame size */
#define	ELINKIDNUM		0	/* should be a unique id; zero works */

#define	MAX_ELINK_BOARDS 0x20
/* board state */
#define	ELINK_IDLE		0
#define	ELINK_WAITRCV		1
#define	ELINK_XMTBUSY		2
#define	ELINK_ERROR		3

#define	MAX_RXB_SIZE		1520

#define	TXB_SIZE 		1520
#define	RXB_SIZE 		512

#define	MIN_DATA_SIZE 		46	/* 64-(crc+preamble+ether_header) */

#define	ETHER_HDR_LEN sizeof (struct ether_header)  /* length of ether hdr */

#define	MAX_SCB_WAIT_TIME 	1000	/* Max waiting time for the SCB */
					/* command word to be cleared */

#define	PRO_ON  		1	/* Promiscuous mode */
#define	PRO_OFF 		0	/* Not promiscuous mode */
#define	LPBK_MASK 		0x8000	/* switch on loopback mode */

/*
 * Board specifications as given in EtherLink 16 tech. ref. manual
 */

#define	ELINK_ID_PORT		0x0100	/* Adapter's ID port location */

#define	IO_CTRL_REG		0x06	/* Offset of Control register */
#define	IO_INTCLR		0x0A	/* Offset of interrupt clear register */
#define	IO_CHAN_ATN		0x0B	/* Offset of Channel attention reg. */
#define	IO_ROM_CFG		0x0D	/* Offset of ROM config register */
#define	IO_RAM_CFG		0x0E	/* Offset of RAM config register */
#define	IO_INTR_CFG		0x0F	/* Offset of Intr config register */

#define	CFG_MSK			0x80	/* to get media type from ROM_CFG */
#define	RAM_CONF_MSK		0x3f	/* Used to get memory settings */
#define	IRQ_CONF_MSK		0x0f	/* Used to get IRQ level */

#define	BASE_ADDR_MSK		0x18000	/* Used to get RAM config value */

#define	TDR_TEST		0x01	/* Perform TDR test */

/*
 * ELINK Control Register bits
 */
#define	CR_VB0			0x00	/* 3COM signature = "*3COM*" */
#define	CR_VB1			0x01	/* Ethernet Address */
#define	CR_VB2			0x02	/* Adapter part # and rev level */

#define	CR_BS			0x03	/* 16K RAM bank select */
#define	CR_IEN			0x04	/* interrupt enable */
#define	CR_INT			0x08	/* interrupt latch active */
#define	CR_LAD			0x10	/* LA address decode */
#define	CR_LBK			0x20	/* loopback enable */
#define	CR_CA			0x40	/* channel attention */
#define	CR_RST			0x80	/* 82586 reset */

/*
 * SCB interrupt status  bits
 */
#define	SCB_INT_MSK		0xf000	/* SCB STAT bit mask */
#define	SCB_INT_CX		0x8000	/* CX bit, CU finished cmd w/ "I" set */
#define	SCB_INT_FR		0x4000	/* FR bit, RU finished receive */
#define	SCB_INT_CNA		0x2000	/* CNA bit, CU not active */
#define	SCB_INT_RNR		0x1000	/* RNR bit, RU not ready */

/*
 * SCB  command unit status bits
 */
#define	SCB_CUS_MSK		0x0700	/* SCB CUS bit mask */
#define	SCB_CUS_IDLE		0x0000	/* CU idle */
#define	SCB_CUS_SUSPND		0x0100	/* CU suspended */
#define	SCB_CUS_ACTV		0x0200	/* CU active */

/*
 * SCB Receive Unit status bits
 */
#define	SCB_RUS_MSK		0x0070	/* SCB RUS bit mask */
#define	SCB_RUS_IDLE		0x0000	/* RU idle */
#define	SCB_RUS_SUSPND		0x0010	/* RU suspended */
#define	SCB_RUS_NORESRC 	0x0020	/* RU no resource */
#define	SCB_RUS_READY		0x0040	/* RU ready */

/*
 * SCB ACK bits
 */
#define	SCB_ACK_MSK		0xf000	/* SCB ACK bit mask */
#define	SCB_ACK_CX		0x8000	/* ACK_CX, ack a completed cmd */
#define	SCB_ACK_FR		0x4000	/* ACK_FR, ack a frame reception */
#define	SCB_ACK_CNA		0x2000	/* ACK_CNA, acknowledge CU not active */
#define	SCB_ACK_RNR		0x1000	/* ACK_RNR, acknowledge RU not ready */

/*
 * SCB Command Unit commands
 */
#define	SCB_CUC_MSK		0x0700	/* SCB CUC bit mask */
#define	SCB_CUC_STRT		0x0100	/* start CU */
#define	SCB_CUC_RSUM		0x0200	/* resume CU */
#define	SCB_CUC_SUSPND		0x0300	/* suspend CU */
#define	SCB_CUC_ABRT		0x0400	/* abort CU */

/*
 * SCB Receive Unit commands
 */

#define	SCB_RUC_MSK		0x0070	/* SCB RUC bit mask */
#define	SCB_RUC_STRT		0x0010	/* start RU */
#define	SCB_RUC_RSUM		0x0020	/* resume RU */
#define	SCB_RUC_SUSPND		0x0030	/* suspend RU */
#define	SCB_RUC_ABRT		0x0040	/* abort RU */

/*
 * SCB software reset bit
 */

#define	SCB_RESET		0x0080	/* RESET, same as H/W reset */

/*
 * general defines for the command and descriptor blocks
 */

#define	CS_CMPLT		0x8000	/* C bit, completed */
#define	CS_BUSY			0x4000	/* B bit, Busy */
#define	CS_OK			0x2000	/* OK bit, error free */
#define	CS_ABORT		0x1000	/* A bit, abort */
#define	CS_EL			0x8000	/* EL bit, end of list */
#define	CS_SUSPND		0x4000	/* S bit, suspend */
#define	CS_INT			0x2000	/* I bit, interrupt */
#define	CS_STAT_MSK		0x3fff	/* Command status mask */
#define	CS_EOL			0xffff	/* set for fd_rbd_ofst on */
					/*   unattached FDs */
#define	CS_EOF			0x8000	/* EOF (End Of Frame) in */
					/*   the TBD and RBD */
#define	CS_RBD_CNT_MSK	0x3fff	/* actual count mask in RBD */

#define	CS_COLLISIONS	0x000f
#define	CS_CARRIER		0x0400
#define	CS_ERR_STAT		0x07e0

/*
 * 82586 commands
 */
#define	CS_CMD_MSK		0x07	/* command bits mask */
#define	CS_CMD_NOP		0x00	/* NOP */
#define	CS_CMD_IASET	0x01	/* Individual Address Set up */
#define	CS_CMD_CONF		0x02	/* Configure */
#define	CS_CMD_MCSET	0x03	/* Multi-Cast Setup */
#define	CS_CMD_XMIT		0x04	/* transmit */
#define	CS_CMD_TDR		0x05	/* Time Domain Reflectometer */
#define	CS_CMD_DUMP		0x06	/* dump */
#define	CS_CMD_DGNS		0x07	/* diagnose */



#define	elink_display_eaddr(ether_addr)\
{\
		int z; \
		for (z = 0; z < ETHERADDRL; z++)\
				cmn_err(CE_CONT, "%2x ", ether_addr[z]);\
}

typedef unsigned char	enet_addr_t[ETHERADDRL];

/*
 *	System Configuration Pointer (SCP)
 */
typedef struct
{
	volatile ushort scp_sysbus;		/* 82586 bus width, 0-16 bits */
	volatile ushort	scp_unused[2];	/* unused area */
	volatile ushort	scp_iscp;		/* iscp address */
	volatile ushort	scp_iscp_base;
} scp_t;

/*
 * Intermediate System Configuration Pointer (ISCP)
 */
typedef struct
{
	volatile ushort	iscp_busy;
				/* Set to 1 by cpu before its first CA */
				/* cleared by 82586 after reading  */
	volatile ushort	iscp_scb_ofst;
				/* offset of the scb in the shared memory */
	volatile paddr_t iscp_scb_base;	/* base of shared memory */
} iscp_t;

/*
 * System Control Block	(SCB)
 */
typedef struct
{
	volatile ushort	scb_status; /* Status word (STAT, CUS, RUS) */
	volatile ushort	scb_cmd;    /* Command (ACK, CUC, RUC) */
	volatile ushort	scb_cbl_ofst; /* Offset of first command block in CBL */
	volatile ushort	scb_rfa_ofst;
	/* Offset of first frame descriptor in RFA */
	volatile ushort	scb_crc_err;	/* CRC errors accumulated */
	volatile ushort	scb_aln_err;	/* Alignment errors */
	volatile ushort	scb_rsc_err;	/* Frame lost because of no resources */
	volatile ushort	scb_ovrn_err;	/* Overrun errors */
} scb_t;

/*
 * Configure command parameter structure
 * Following are the sub-fields of the structure
 */

typedef struct
{
	ushort byte_count : 4;
	ushort unused1 : 4;
	ushort fifo_lim : 4;
	ushort unused2 : 4;
} fifo_t;

typedef struct
{
	ushort unused : 6;
	ushort srdy : 1;
	ushort sav_bf : 1;
	ushort addr_len : 3;
	ushort al_loc : 1;
	ushort pream_len : 2;
	ushort int_lpbk : 1;
	ushort ext_lpbk : 1;
} addrlen_t;

typedef struct
{
	ushort lin_pri : 3;
	ushort unused : 1;
	ushort acr : 3;
	ushort bof_met : 1;
	ushort if_space : 8;
} ifspace_t;

typedef struct
{
	ushort slot_time: 11;
	ushort unused : 1;
	ushort retrylim : 4;
} slot_time_t;

typedef struct
{
	ushort prm : 1;
	ushort bcds : 1;
	ushort mancs : 1;
	ushort tncrs : 1;
	ushort ncrcins : 1;
	ushort crc16 : 1;
	ushort bstuff : 1;
	ushort pad : 1;
	ushort crsf : 3;
	ushort crssrc : 1;
	ushort ctdf : 3;
	ushort ctdsrc : 1;
} crsf_t;

typedef struct
{
	ushort min_frame_len : 8;
	ushort unused : 8;
} min_fr_len_t;

typedef struct
{
	fifo_t word1;
	addrlen_t word2;
	ifspace_t word3;
	slot_time_t word4;
	crsf_t word5;
	min_fr_len_t word6;
} conf_t;

/*
 * Transmit commad parameters structure
 */
typedef struct
{
	ushort		xmt_tbd_ofst;	/* Transmit Buffer Descriptor offset */
	enet_addr_t xmt_dest;		/* Destination Address */
	ushort		xmt_length;		/* length of the frame */
} xmit_t;

typedef struct
{
	unsigned char entry[ETHERADDRL]; /* Multicast addrs are 6 bytes */
} mcast_t;

typedef struct
{
    ushort	mcast_cnt;		/* Count of multicast addresses */
	char mcast_addr[GLD_MAX_MULTICAST * ETHERADDRL];
} mcast_addr_t;

typedef struct
{
	ushort tdr_status;
} tdr_t;

/*
 * General Command structure
 */
typedef struct
{
	ushort		cmd_status;	/* Status of the command */
	ushort		cmd_cmd;	/* Command */
	ushort		cmd_nxt_ofst;	/* pointer to the next command block */
	union
	{
		xmit_t	pr_xmit;		/* transmit */
		conf_t	pr_conf;		/* configure */
		enet_addr_t pr_ind_addr_set;	/* individual address setup */
		mcast_addr_t	pr_mcaddr;	/* multicast address setup */
		tdr_t	pr_tdr;
	} cmd_prm;
} gencmd_t;

/*
 * Tramsmit Buffer Descriptor (TBD)
 */
typedef struct {
	ushort		tbd_count;		/* Number of bytes in buffer */
	ushort		tbd_nxt_ofst;	/* offset of next TBD */
	ushort		tbd_buff;
	ushort		tbd_buff_base;
} tbd_t;

/*
 * Receive Buffer Descriptor
 */
typedef struct {
	ushort		rbd_status;	/* Indicates the bytes received */
					/* Also, EOF */
	ushort		rbd_nxt_ofst;	/* offset of next RBD */
	ushort	rbd_buff; 		/* Buffer address */
	ushort	rbd_buff_base;
	ushort	rbd_size;		/* Size of the buffer */
					/* Also, EL */
} rbd_t;

/*
 * Frame Descriptor (FD)
 */
typedef struct {
	ushort	fd_status;		/* C, B, OK, S6-S11 */
	ushort	fd_cmd;			/* End of List (EL), Suspend (S) */
	ushort	fd_nxt_ofst;	/* offset of next FD */
	ushort	fd_rbd_ofst;	/* offset of the RBD */
	enet_addr_t	fd_dest;		/* destination address */
	enet_addr_t	fd_src;			/* source address */
	ushort	fd_length;		/* length of the received frame */
} fd_t;

#define	MAX_RAM_SIZE 0x10000	/* 64K bytes of RAM */

/*
 * The following is the board dependent structure
 */

struct elinkinstance {
	caddr_t		ram_virt_addr;	/* Virtual addr of the RAM base */
	ulong		ram_size;	/* Size of the RAM */

	ushort		scb_ofst;	/* Offset of SCB structure */
	ushort		gencmd_ofst;	/* Offset of Command(gen) structure */
	ushort		cmd_ofst;	/* Offset of command(xmit) structure */

	/* These fields are used during transmission of a frame */
	ushort		tbd_ofst;	/* Offset of the TBD structure */
	ushort		txb_ofst;	/* Offset of the transmit buffer */

	/* These fields are used during frame reception */
	ushort		rxb_ofst;	/* Offset of Receive buffer's */
	ushort		rbd_ofst;	/* Offset of RBD's */
	ushort		fd_ofst;	/* Offset of the first FD */

	/* These fields are used during manipulation of linked lists */
	fd_t		*begin_fd;	/* The logically first fd in RFA */
	fd_t		*end_fd;	/* The logically last fd in RFA */
	rbd_t		*begin_rbd;	/* The logically first rbd in RFA */
	rbd_t		*end_rbd;	/* The logically last rbd in RFA */

	/* These are used to add/delete multicast address */
	mcast_t		elink_multiaddr[GLD_MAX_MULTICAST];
	int			mcast_count;

	/* This is to maintain statistics */
	int 		restart_count;		/* device specific data item */
};

#ifdef	__cplusplus
}
#endif

#endif	/* _SYS_ELINK_H */
