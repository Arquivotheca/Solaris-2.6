/*
 * Copyright (c) 1993, by Sun Microsystems, Inc.
 */

#pragma indent	"@(#)iee.h	1.3	94/10/26 SMI"

/*
 * NAME
 *		iee.h ver. 2.2
 *
 *
 * SYNOPSIS
 *		Header file for the source code of the "iee" driver for the Intel
 * EtherExpress-16 Ethernet LAN adapter board on Solaris 2.1 (x86)
 *      Contains hardware specific driver declarations for the "iee"
 * EtherExpress-16 driver conforming to the Generic LAN Driver model.
 *
 *
 * SEE ALSO
 *      iee.c - source code file of the "iee" driver
 *
 *
 * MODIFICATION HISTORY
 * Interim version ver. 1.4 07/28/93.
 *  * Prototype version, fully functional.
 *
 * Version 2.0 08/25/93 released on 3 Sep '93.
 *	* Field "irq_level" introduced in ieeinstance to store irq level read
 *    from EEPROM.
 *	* Bitmap structures used in iee_config().
 *	* "mcast_address_t" structure introduced in ieeinstance to support
 *    multicast addresses.
 *  * Fields iee_multiaddr[] and multicast_count introduced in ieeinstance to
 *    facilitate programming of multicast addresses
 *	* A new field restart_count has been introduced in ieeinstance to keep
 *    track of the number of times RU has been restarted
 *	* Unused fields n_fd, n_rbd, ofst_rxb, ofst_rbd and ofst_fd 
 *	  have been removed from ieeinstance. 
 *      
 * Version 2.1 09/16/93 released on 17 Sep '93
 *  * No changes were needed for the header file corresponding to the update
 *    release version 2.1
 *
 * Version 2.2 10/28/93 released on 28 Oct '93
 *  * #defines introduced for AUI, BNC and TPE that denote the connector types
 *  * #defines introduced for host shadow registers SHADOW_AUTO_ID and ECR1
 *  * Changes made to WRITE_TO_SRAM :: repoutsw() used instead of repoutsb()
 *
 *
 * MISCELLANEOUS
 * 		vi options for viewing this file::
 *				set ts=4 sw=4 ai wm=4
 *
 */
#ident "@(#)iee.h	2.2  28 Oct 1993"    /* SCCS identification string */ 

#ifndef _IEE_H
#define _IEE_H 1

/*
 * STREAMS specific :: needed by gld
 */

#define IEEHIWAT	32768		/* driver flow control high water */
#define IEELOWAT	4096		/* driver flow control low water */
#define IEEMAXPKT	1500		/* maximum media frame size */
#define IEEIDNUM	0			/* should be a unique id; zero works */

/*
 * board state :: used by gld
 */

#define IEE_IDLE	0
#define IEE_WAITRCV	1
#define IEE_XMTBUSY	2
#define IEE_ERROR	3

/*
 * For debugging:: debug flags
 */

#define IEETRACE	0x01
#define IEEERRS		0x02
#define IEERECV		0x04
#define IEEDDI		0x08
#define IEESEND		0x10
#define IEEINT		0x20
#define IEEIOCTL	0x40

#ifdef DEBUG
#define IEEDEBUG 1
#endif

/*
 * For those functions returning 1 for success and 0 for failure
 */

#ifndef TRUE
#define TRUE		1
#define FALSE		0
#endif

/*
 * For those functions returning 0 for success  and -1 for failure
 */

#define SUCCESS		0
#define FAIL		-1
#define RETRY		1		/* used by iee_send() only */

#define GLD_MAX_MULTICAST	16	/* max no. of multicast addresses */

/* 
 * ioctls currently supported
 * and masks for interpreting result of TDR test
 */

#define	TDR_TEST		0x1		/* perform TDR test */
#define LNK_OK			0x8000	/* link ok */
#define XVR_PRB			0x4000	/* tranceiver/cable problem */
#define ET_OPN			0x2000	/* open on cable */
#define ET_SRT			0x1000	/* short on cable */

/*
 * minimum length of frame that can be transmitted on Ethernet sans headers
 */

#define MIN_FRAME_LEN 	46

#define XMIT_BUFFER_SIZE	1520	/* size of transmit buffer */
#define MAX_BUFFER_SIZE		1520	/* maximum size of receive buffer */
#define RECV_BUFFER_SIZE	512		/* actual size of receive buffer */

#define MAX_IEE_BOARDS	0x20		/* Max. no. of IEE boards supported */
#define MAX_SRAM_SIZE	0x10000		/* Max. amount of SRAM IEE can have */

/*
 * Masks for configuration
 */

#define PRM_MASK 		1			/* mask for promiscuous mode */
#define PRO_ON  		PRM_MASK	/* switch on promiscuous mode */
#define PRO_OFF 		0			/* switch off promiscuous mode */
#define LPBK_MASK 		0x8000		/* switch on loopback mode */
#define LPBK_OFF		0			/* switch off loopback mode */


/*
 *                     Board specific defines
 */

/* 
 * board registers - starting from base i/o address
 * see EtherExpress EPS, pg. viii
 */

#define DX_REG			0x00		/* Data Transfer Register */
#define WR_PTR			0x02		/* Write Address Pointer */
#define RD_PTR			0x04		/* Read Address Pointer */
#define CA_CTRL			0x06		/* Channel Attention */
#define SEL_IRQ			0x07		/* IRQ select */
#define SMB_PTR			0x08		/* Shadow Memory bank Pointer */
#define CONFIG			0x0D		/* Configuration */
#define RESET			0x0E		/* Reset */
#define AUTO_ID			0x0F		/* AUTO ID register */

/*
 * host shadow registers (to do a direct inw from them)
 */

#define SCB_STATREG		0xc008		/* status word of 82586 SCB */
#define SCB_CMDREG		0xc00a		/* command word of 82586 SCB */
#define SCB_CBLREG		0xc00c		/* offset of CBL in SCB */
#define SCB_RFAREG		0xc00e		/* offset of RFA in SCB */

/*
 * extended host shadow registers
 */

#define ECR1            0x300e      /* extended control register */
#define SHADOW_AUTO_ID  0x300f      /* shadow auto-id register */

/*
 * registers of EEPROM
 */

#define BASE_IO_REG            0	/* where IRQ value is stored */
#define SEL_CONNECTOR_REG      5	/* BNC/TPE selection register */
#define INTEL_EADDR_L          0x3c	/* where Ethernet address is stored */	
#define EEPROM_READ_OPCODE_REG 06	/* opcode for reading from EEPROM */

#define EEPROM_READ_OPCODE	06		
#define EEPROM_WRITE_OPCODE 05
#define EEPROM_ERASE_OPCODE 07
#define EEPROM_EWEN_OPCODE  19    /* Erase/write enable */
#define EEPROM_EWDS_OPCODE  16    /* Erase/write disable */


/*
 * Masks for EEPROM control
 */

#define RESET_586		0x80		/* reset bit for 586 */
#define GA_RESET		0x40		/* ASIC Reset */
#define EEDO			0x08		/* Data Out */
#define EEDI			0x04		/* Data In */
#define EECS			0x02		/* Chip Select */
#define EESK			0x01		/* For raising and lowering clock */


/*
 * Connector types
 */

#define AUI				0x00        /* connector type :: AUI */
#define BNC             0x01        /* connector type :: BNC */
#define RJ45            0x02        /* connector type :: RJ45 */


/*
 *                      82586 specific defines
 */

/*
 * SCB status bits-- give the cause of an interrupt
 */

#define	SCB_INT_MASK	0xf000	/* SCB status bit mask */
#define	SCB_INT_CX		0x8000	/* Cmd unit finished a command */
#define	SCB_INT_FR		0x4000	/* RU received a frame */
#define	SCB_INT_CNA		0x2000	/* Cmd unit not active */
#define	SCB_INT_RNR		0x1000	/* Recv unit not ready */

/* 
 * SCB Command Unit status bits
 */

#define	SCB_CUS_MASK	0x0700	/* SCB CUS bit mask */
#define	SCB_CUS_IDLE	0x0000	/* CU idle */
#define	SCB_CUS_SUSPEND	0x0100	/* CU suspended */
#define	SCB_CUS_ACTIVE	0x0200	/* CU active */

/* 
 * SCB Receive Unit status bits
 */

#define	SCB_RUS_MASK	0x0070	/* SCB RUS bit mask */
#define	SCB_RUS_IDLE	0x0000	/* RU idle */
#define SCB_RUS_SUSPEND	0x0010	/* RU suspended */
#define SCB_RUS_NORESRC 0x0020	/* RU out of resources */
#define	SCB_RUS_READY	0x0040	/* RU ready */

/*
 * SCB ACK bits
 */

#define SCB_ACK_MASK	0xf000	/* SCB ACK bit mask */
#define SCB_ACK_CX		0x8000	/* ack. a completed cmd */
#define SCB_ACK_FR		0x4000	/* ack. a frame reception */
#define	SCB_ACK_CNA		0x2000	/* ack. CU not active */
#define SCB_ACK_RNR		0x1000	/* ack. RU not ready */

/* 
 * SCB Command Unit commands
 */

#define	SCB_CUC_MASK	0x0700	/* SCB CUC bit mask */
#define	SCB_CUC_START	0x0100	/* start CU */
#define	SCB_CUC_RESUME	0x0200	/* resume CU */
#define	SCB_CUC_SUSPEND	0x0300	/* suspend CU */
#define	SCB_CUC_ABORT	0x0400	/* abort CU */

/* 
 * SCB Receive Unit commands 
 */

#define SCB_RUC_MASK	0x0070	/* SCB RUC bit mask */
#define	SCB_RUC_START	0x0010	/* start RU */
#define	SCB_RUC_RESUME	0x0020	/* resume RU */
#define	SCB_RUC_SUSPEND	0x0030	/* suspend RU */
#define	SCB_RUC_ABORT	0x0040	/* abort RU */

/*
 * some defines for the command and receive buffer descriptor blocks
 * and their status bits 
 * see 82586 manual for more details
 */

#define CS_CMPLT		0x8000	/* Completion of frame reception */
#define CS_BUSY			0x4000	/* 586 currently rcving a frame */
#define CS_OK			0x2000	/* Frame received successfully */
#define CS_ABORT		0x1000	/* Abort */
#define CS_EL			0x8000	/* End of list */
#define CS_SUSPEND		0x4000	/* S bit, suspend */
#define CS_INT			0x2000	/* I bit, interrupt */
#define	CS_STAT_MASK	0x3fff	/* Command status mask */
#define CS_EOL			0xffff	/* set for fd/rbd offsets */
#define CS_EOF			0x8000	/* End Of Frame in the TBD and RBD */
#define	CS_RBD_CNT_MASK	0x3fff	/* actual count mask in RBD */

#define	CS_FAIL			0x0800	/* F bit; in diagnose command only */
#define	CS_COLLISIONS	0x000f
#define	CS_CARRIER		0x0400
#define	CS_ERR_STAT		0x07e0

/*
 * 82586 commands
 */

#define CS_CMD_MASK		0x07	/* command bits mask */
#define	CS_CMD_NOP		0x00	/* NOP */
#define	CS_CMD_IASET	0x01	/* Individual Address Setup */
#define	CS_CMD_CONF		0x02	/* Configure */
#define	CS_CMD_MCSET	0x03	/* Multi-Cast Setup */
#define	CS_CMD_XMIT		0x04	/* Xmit */
#define	CS_CMD_TDR		0x05	/* Time Domain Reflectometry */
#define CS_CMD_DUMP		0x06	/* Dump */
#define	CS_CMD_DGNS		0x07	/* Diagnostics */


#define SIGLEN  4
#define	BYTE	0
#define WORD	1


/*
 * 82586 data structures definition
 *
 * NOTE: Only the first 16 bits of the physical addresses are set. These
 *       are the offset of the structure from the base of the shared 
 *		 memory segment. (0 - 0xffff)
 */

/*
 * Ethernet address type
 */

typedef unsigned char enet_address_t[ETHERADDRL];

/*
 *	System Configuration Pointer (SCP)
 */

typedef struct
{
	ushort 	scp_sysbus;		/* system bus width */
	ushort	scp_unused[2];	/* unused area */
	ushort	scp_iscp;
	ushort	scp_iscp_base;
} scp_t;

/*
 * Intermediate System Configuration Pointer (ISCP)
 */

typedef struct
{
	ushort	iscp_busy;		/* 1 means 82586 is initializing */
	ushort	iscp_scb_offset;	/* offset of the scb in the shared memory */
	paddr_t		iscp_scb_base;	/* base of shared memory */
} iscp_t;

/*
 * System Control Block	(SCB)
 */

typedef struct
{
	ushort	scb_status;		/* STAT, CUS, RUS */
	ushort	scb_cmd;		/* ACK, CUC, RUC */
	ushort	scb_cbl_offset;	/* Command Block List offset */
	ushort	scb_rfa_offset;	/* Receive Frame Area offset */
	ushort	scb_crc_err;	/* # of CRC errors */
	ushort	scb_aln_err;	/* # of alignment errors */
	ushort	scb_rsc_err;	/* # of no resource errors */
	ushort	scb_ovrn_err;	/* # of overrun errors */
} scb_t;

#define	SCB_CRC_ERR	8

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
 * Transmit command structure
 */

typedef struct
{
	ushort	xmit_tbd_offset;	/* Transmit Buffer Descriptor offset */
	enet_address_t xmit_dest;	/* Destination Address */
	ushort	xmit_length;		/* length of the frame */
} xmit_t;

/*
 * Format of each entry in the multicast addresses list
 */

typedef struct 
{
	enet_address_t entry;  /* Multicast addrs are 6 bytes */
} multicast_t;

/*
 * For multicast address setup
 */

typedef struct
{
	ushort mcast_count; /* count of the number of multicast addresses */
	enet_address_t mcast_addr[GLD_MAX_MULTICAST][ETHERADDRL];
} mcast_address_t;

/*
 * General (non-transmit) Command structure
 */

typedef struct
{
	ushort	cmd_status;		/* C, B, command specific status */
	ushort	cmd_cmd;		/* EL, S, I, opcode */
	ushort cmd_nxt_offset;	/* pointer to the next command block */
	union 
	{
		xmit_t	prm_xmit;			/* transmit */
		conf_t	prm_conf;			/* configure */
		enet_address_t prm_ia_set;	/* individual address setup */
		mcast_address_t prm_mc_set; /* multicst address setup */
	} parameter;
} gen_cmd_t;

#define	CMD_STATUS		0
#define	CMD_PARAMETER	6

/*
 * Tramsmit Buffer Descriptor (TBD)
 */

typedef struct
{
	ushort tbd_count;		/* End Of Frame(EOF), Actual count(ACT_COUNT) */
	ushort tbd_nxt_offset;	/* offset of next TBD */
	ushort tbd_buff;		/* offset (wrt base_address) of transmit buffer */
	ushort tbd_buff_base;	/* base address of transmit buffer */
} tbd_t;

/*
 * Receive Buffer Descriptor
 */

typedef struct
{
	ushort rbd_status;		/* EOF, ACT_COUNT */
	ushort rbd_nxt_offset;	/* offset of next RBD */
	ushort rbd_buff;		/* offset (wrt base address) of receive buffer */
	ushort rbd_buff_base;	/* base address of receive buffer */
	ushort rbd_size;		/* EL, size of the buffer */
} rbd_t;

#define	RBD_STATUS		0
#define	RBD_NXT_OFFSET	2
#define	RBD_BUFF		4
#define	RBD_SIZE		8

/*
 * Frame Descriptor (FD)
 */
typedef struct
{
	ushort fd_status;		/* C, B, OK, S6-S11 */
	ushort fd_cmd;			/* End of List (EL), Suspend (S) */
	ushort fd_nxt_offset;	/* offset of next FD */
	ushort fd_rbd_offset;	/* offset of the RBD */
	enet_address_t fd_dest;	/* destination address */
	enet_address_t fd_src;	/* source address */
	ushort	fd_length;		/* length of the received frame */
} fd_t;

#define	FD_STATUS		0
#define	FD_CMD			2
#define	FD_NXT_OFFSET	4
#define	FD_RBD_OFFSET	6

/*
 * Drivate private data structure :: ieeinstance structure
 * - allocated at attach time
 * - one for every board found
 * - a pointer to this will be stored in macinfo
 */

struct ieeinstance 
{
	/*
	 * we store offsets of SCB and other structures (in the SRAM) here 
	 * so that we can retrieve them faster
	 */

	unchar  irq_level;		/* IRQ level read from EEPROM */
	ushort	offset_scb;		/* Offset of SCB */
	ushort	offset_gencmd;	/* Offset of general cmd struct */
	ushort	offset_cmd;		/* Offset of xmit cmd struct */
	ushort	offset_tbd;		/* Offset of TBD struct */
	ushort	offset_tbuf;	/* Offset of xmit buffer */

	/*
	 * The following are set once and never changed, they're
	 * used to reset the RFA if we get a No Resources interrupt.
	 */
	ushort	offset_fd;		/* Offset of receive frame area */
	ushort	num_fds;		/* number of receive frames */
	ushort	offset_rbd;		/* Offset of receive buffer descriptors */
	ushort	num_rbds;		/* number of receive buffer descriptors */

	/* 
	 * The following are useful for requeuing of fds and rbds
	 */

	ushort	begin_fd;		/* first FD in FDL */
	ushort	end_fd;			/* last FD in FDL */
	ushort	begin_rbd;		/* first RBD in RBL */
	ushort	end_rbd;		/* lsat RBD in RBL */

	multicast_t  iee_multiaddr[GLD_MAX_MULTICAST];
	int multicast_count;	/* number of multicast addresses */
	int      restart_count; /* number of times RU was restarted */
};


/* 
 *                         General Macros used
 */

/*
 * IEEPRINT_EADDR prints the Ethernet address
 * The 6 bytes of the Ethernet address are pointed at by ether_addr
 */

#define IEEPRINT_EADDR(ether_addr)  \
{\
	int byte;\
	for(byte = 0; byte < ETHERADDRL; byte++)\
		cmn_err(CE_CONT, "%2x ", ether_addr[byte]);\
}

#define IEEPRINT_CONNECT_TYPE(connect_type) \
{\
	switch (connect_type) \
	{\
		case AUI :\
			cmn_err(CE_CONT, "(AUI)\n");\
			break;\
		case BNC :\
			cmn_err(CE_CONT, "(BNC)\n");\
			break;\
		case RJ45 :\
			cmn_err(CE_CONT, "(RJ45)\n");\
			break;\
	}\
}

/* 
 * READ_BYTE macro
 * init RD_PTR to location
 * read from the location to the destination thro' inb
 * note: DX_REG will be autoincremented by 1 after this macro
 * base_io_address is the I/O base address
 */

#define READ_BYTE(location, base_io_address, dest) \
{\
	outw(base_io_address + RD_PTR, (ushort)(location));	   \
	dest = inb(base_io_address + DX_REG);\
}

/* 
 * READ_WORD macro
 * init RD_PTR to location
 * read from the location to the destination thro' inw
 * base_io_address is the I/O base address
 * note: DX_REG will be autoincremented by 2 after this macro
 */

#define READ_WORD(location, base_io_address, dest) \
{\
	outw(base_io_address + RD_PTR, (ushort)(location));	   \
	dest = inw(base_io_address + DX_REG);\
}

/* 
 * WRITE_BYTE macro
 * init WR_PTR to location
 * output the value to location thro' outb
 * note: DX_REG will be autoincremented by 1 after this macro
 */

#define WRITE_BYTE(location, value, base_io_address) \
{\
	outw(base_io_address + WR_PTR, (ushort)(location)); 	    \
	outb(base_io_address + DX_REG, value);		\
}

/* 
 * WRITE_WORD macro
 * init WR_PTR to location
 * output the value to location thro' outw
 * note: DX_REG will be autoincremented by 2 after this macro
 */

#define WRITE_WORD(location, value, base_io_address) \
{\
	outw(base_io_address + WR_PTR, (ushort)(location)); 	    \
	outw(base_io_address + DX_REG, value);		\
}

/*
 * WRITE_TO_SRAM writes len bytes starting from src to the location at
 * offset dest in SRAM
 */

#define WRITE_TO_SRAM(src, dest, len, base_io_addr)\
{\
    ushort *source = (ushort *) (src);\
    unchar *src1   = (unchar *) (src);\
\
	/* init WR_PTR */\
	outw(base_io_addr + WR_PTR, (ushort)(dest));\
\
  	repoutsw(base_io_addr + DX_REG, source, (len) / 2); \
	if ((len) % 2)\
		outb(base_io_addr + DX_REG, src1[(len) - 1]);\
}

/*
 * READ_FROM_SRAM reads len bytes starting from src to the location
 * pointed at by src
 */

#define READ_FROM_SRAM(dest, src, len, base_io_addr)\
{\
\
	/* init RD_PTR */\
	outw(base_io_addr + RD_PTR, (ushort)(src));\
\
	(void)repinsw(base_io_addr + DX_REG, (ushort *)dest, len/2);\
	if (len % 2)\
	{\
		/* odd count - input one byte ... */\
		*((unchar *)dest + len - 1) = inb(base_io_addr + DX_REG);\
	}\
}

#endif
