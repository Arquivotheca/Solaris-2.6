/*
 * Copyright (c) 1993, by Sun Microsystems, Inc.
 */

#pragma ident	"@(#)8390nicdef.h	1.1	93/10/29 SMI"

/* nicdef.inc --- 8390 definitions include file		(c) 1989 Xircom */

/****************************************************************************************************************; */
/* Revision Information												; */
/*														; */
/*	Author: 	Dirk Gates										; */
/*	Started:	June 14, 1989										; */
/*	Code From:	Pocket Ethernet Novell Shell Driver (Dirk Gates, 6/01/89)				; */
/*														; */
/*	Language:	MASM 5.1										; */
/*	Build:		none											; */
/*														; */
/*---------------------------------------------------------------------------------------------------------------; */
/* Release 	Date	Who		Why									; */
/* -------	----	---		--------------------------------------------------------		; */
/* 1.02		101089	DIG		release for driver development kit update				; */
/* 1.00		062389	DIG		beta release								; */
/*														; */
/*---------------------------------------------------------------------------------------------------------------; */
/* Notes:													; */
/*														; */
/*---------------------------------------------------------------------------------------------------------------; */
/* History:													; */
/*														; */
/* Date	 Who    What												; */
/* ----	 ---	-------------------------------------------------------------------------------------------	; */
/* 062389 DIG	beta release											; */
/* 061789 DIG	first assembly											; */
/* 061489 DIG	began coding											; */
/*														; */
/*---------------------------------------------------------------------------------------------------------------; */

/****************************************************************************************************************; */
/* Network Interface Controller (8390) Definitions								; */
/*---------------------------------------------------------------------------------------------------------------; */

/*;;; Command Register Definitions */
/* */
#define CMD				0x00
#define CMD_MASK			0xfd
#define CMD_RD_PG			0
#define CMD_WR_PG			0
						/* ---------- bit definitions ---------- */
#define STP				0x01	/* stop bit */
#define STA				0x02	/* start bit */
#define TXP				0x04	/* transmit packet */
#define RDC_MASK			0x38	/* remote dma command mask */
#define RDC_RD				0x08	/* remote dma read command */
#define RDC_WR				0x10	/* remote dma write command */
#define RDC_SP				0x18	/* remote dma send packet command */
#define RDC_ABT				0x20	/* remote dma abort command */
#define PAGE_MASK			0xc0	/* page select mask */
#define PAGE_0				0x00	/* page 0 select command */
#define PAGE_1				0x40	/* page 1 select command */
#define PAGE_2				0x80	/* page 2 select command */


/*////// Current Local DMA Address Register Definitions */
/* */
#define CLDA0				0x01
#define CLAD0_RD_PG			PAGE_0
#define CLAD0_WR_PG			PAGE_2
#define CLDA0_MASK			0xff
/* */
#define CLDA1				0x02
#define CLAD1_MASK			0xff
#define CLAD1_RD_PG			PAGE_0
#define CLAD1_WR_PG			PAGE_2


/*////// Boundary Pointer Register Definitions */
#define BNDRY				0x03
#define BNDRY_MASK			0xff
#define BNDRY_RD_PG			PAGE_0
#define BNDRY_WR_PG			PAGE_0


/*////// Transmit Status Register Definitions */
/* */
#define TSR				0x04
#define TSR_MASK			0xfd
#define TSR_RD_PG			PAGE_0
/*							// ---------- bit definitions ---------- */
#define PTXOK				0x01		/* packet transmitted ok */
#define COL				0x04		/* transmit collided */
#define ABT				0x08		/* transmit aborted */
#define CRS				0x10		/* carrier sense lost */
#define FFU				0x20		/* fifo underrun */
#define CDH				0x40		/* collision detect heartbeat */
#define OWC				0x80		/* out of window collision */


/*////// Number of Collisions Register Definitions */
/* */
#define NCR				0x05
#define NCR_MASK			0x0f
#define NCR_RD_PG			PAGE_0


/*////// FIFO Access Register Definitions */
/* */
#define FIFO				0x06
#define FIFO_MASK			0xff
#define FIFO_RD_PG			PAGE_0


/*////// Interrupt Status Register Definitions */
/* */
#define ISR				0x07
#define ISR_MASK			0xff
#define ISR_RD_PG			PAGE_0
#define ISR_WR_PG			PAGE_0
							/* ---------- bit definitions ---------- */
#define PRX				0x01		/* packet received interrupt/enable */
#define PTX				0x02		/* packet transmitted interrupt/enable */
#define RXE				0x04		/* receive error interrupt/enable */
#define TXE				0x08		/* transmit error interrupt/enable */
#define OVW				0x10		/* overwrite warning interrupt/enable */
#define CNT				0x20		/* counter overflow interrupt/enable */
#define RDC				0x40		/* remote dma complete interrupt/enable */
#define RST				0x80		/* reset status */


/*////// Current Remote DMA Address Register Definitions */
/* */
#define CRDA0				0x08
#define CRAD0_MASK			0xff
#define CRAD0_RD_PG			PAGE_0
/* */
#define CRDA1				0x09
#define CRAD1_MASK			0xff
#define CRAD1_RD_PG			PAGE_0


/*////// Receive Status Register Definitions */
/* */
#define RSR				0x0c
#define RSR_MASK			0xff
#define RSR_RD_PG			PAGE_0
							/* ---------- bit definitions ---------- */
#define PRXOK				0x01		/* packet received ok */
#define CRC				0x02		/* crc error */
#define FAE				0x04		/* frame alignment error */
#define FFO				0x08		/* fifo overrun */
#define MPA				0x10		/* missed packet */
#define MBAM				0x20		/* multicast/broadcast address match */
#define DIS				0x40		/* receiver disabled */
#define DFR				0x80		/* deferring */


/*////// Tally Counter 0 (Frame Errors) Register Definitions */

#define CNTR0				0x0d
#define CNTR0_MASK			0xff
#define CNTR0_RD_PG			PAGE_0


/*////// Tally Counter 1 (CRC Errors) Register Definitions */
/* */
#define CNTR1				0x0e
#define CNTR1_MASK			0xff
#define CNTR1_RD_PG			PAGE_0


/*////// Tally Counter 2 (Missed Packets) Register Definitions */
/* */
#define CNTR2				0x0f
#define CNTR2_MASK			0xff
#define CNTR2_RD_PG			PAGE_0


/*////// Start Page Register Definitions */
/* */
#define PSTART				0x01
#define PSTART_MASK			0xff
#define PSTART_RD_PG			PAGE_2
#define PSTART_WR_PG			PAGE_0


/*////// Stop Page Register Definitions */
/* */
#define PSTOP				0x02
#define PSTOP_MASK			0xff
#define PSTOP_RD_PG			PAGE_2
#define PSTOP_WR_PG			PAGE_0


/*////// Transmit Page Start Address Register Definitions */
/* */
#define TPSR				0x04
#define TPSR_MASK			0xff
#define TPSR_RD_PG			PAGE_2
#define TPSR_WR_PG			PAGE_0


/*////// Transmit Byte Count Register Definitions */
/* */
#define TBCR0				0x05
#define TBCR0_MASK			0xff
#define TBCR0_WR_PG			PAGE_0
/* */
#define TBCR1				0x06
#define TBCR1_MASK			0xff
#define TBCR1_WR_PG			PAGE_0


/*////// Remote Start Address Register Definitions */
/* */
#define RSAR0				0x08
#define RSAR0_MASK			0xff
#define RSAR0_WR_PG			PAGE_0
/* */
#define RSAR1				0x09
#define RSAR1_MASK			0xff
#define RSAR1_WR_PG			PAGE_0


/*////// Remote Byte Count Register Definitions */
/* */
#define RBCR0				0x0a
#define RBCR0_MASK			0xff
#define RBCR0_WR_PG			PAGE_0
/* */
#define RBCR1				0x0b
#define RBCR1_MASK			0xff
#define RBCR1_WR_PG			PAGE_0


/*////// Receive Configuration Register Definitions */
/* */
#define RXCR				0x0c
#define RXCR_MASK			0x3f
#define RXCR_RD_PG			PAGE_2
#define RXCR_WR_PG			PAGE_0
							/* ---------- bit definitions ---------- */
#define SEP				0x01		/* save errored packets */
#define ARP				0x02		/* accept runt packets */
#define ABP				0x04		/* accept broadcast packets */
#define AMP				0x08		/* accept multicast packets */
#define PRO				0x10		/* promiscuous mode */
#define MON				0x20		/* monitor mode */


/*////// Transmit Configuration Register Definitions */

#define TXCR				0x0d
#define TXCR_MASK			0x1f
#define TXCR_RD_PG			PAGE_2
#define TXCR_WR_PG			PAGE_0
							/* ---------- bit definitions ---------- */
#define CHK_CRC				0x01		/* inhibit crc */
#define LB_MASK				0x06		/* loopback mode mask */
#define LB_INT				0x02		/* internal loopback mode */
#define LB_SNI				0x04		/* external loopback at sni */
#define LB_EXT				0x06		/* external loopback at coax */
#define ATD				0x08		/* auto transmit disable */
#define OFST				0x10		/* collision offset enable */


/*////// Data Configuration Register Definitions */

#define DCR				0x0e
#define DCR_MASK			0x7f
#define DCR_RD_PG			PAGE_2
#define DCR_WR_PG			PAGE_0
							/* ---------- bit definitions ---------- */
#define WTS				0x01		/* word transfer select */
#define BOS				0x02		/* byte order select (0=8086, 1=68000) */
#define LAS				0x04		/* long address select */
#define NO_LB				0x08		/* non-loopback select */
#define AIR				0x10		/* auto-initialize remote */
#define FT_MASK				0x60		/* fifo threshold select */
#define FT_2				0x00		/* fifo threshold =  2 bytes */
#define FT_4				0x20		/* fifo threshold =  4 bytes */
#define FT_8				0x40		/* fifo threshold =  8 bytes */
#define FT_12				0x60		/* fifo threshold = 12 bytes */


/*////// Interrupt Mask Register Definitions (see ISR bit definitions) */

#define IMR				0x0f
#define IMR_MASK			0x7f
#define IMR_RD_PG			PAGE_2
#define IMR_WR_PG			PAGE_0


/*////// Physical Address Register Definitions */
/* */
#define PAR0				0x01
#define PAR0_MASK			0xff
#define PAR0_RD_PG			PAGE_1
#define PAR0_WR_PG			PAGE_1
/* */
#define PAR1				0x02
#define PAR1_MASK			0xff
#define PAR1_RD_PG			PAGE_1
#define PAR1_WR_PG			PAGE_1
/* */
#define PAR2				0x03
#define PAR2_MASK			0xff
#define PAR2_RD_PG			PAGE_1
#define PAR2_WR_PG			PAGE_1
/* */
#define PAR3				0x04
#define PAR3_MASK			0xff
#define PAR3_RD_PG			PAGE_1
#define PAR3_WR_PG			PAGE_1
/* */
#define PAR4				0x05
#define PAR4_MASK			0xff
#define PAR4_RD_PG			PAGE_1
#define PAR4_WR_PG			PAGE_1
/* */
#define PAR5				0x06
#define PAR5_MASK			0xff
#define PAR5_RD_PG			PAGE_1
#define PAR5_WR_PG			PAGE_1


/*////// Current Page Register Definitions */
/* */
#define CURR				0x07
#define CURR_MASK			0xff
#define CURR_RD_PG			PAGE_1
#define CURR_WR_PG			PAGE_1


/*////// Multicast Address Register Definitions */
/* */
#define MAR0				0x08
#define MAR0_MASK			0xff
#define MAR0_RD_PG			PAGE_1
#define MAR0_WR_PG			PAGE_1
/* */
#define MAR1				0x09
#define MAR1_MASK			0xff
#define MAR1_RD_PG			PAGE_1
#define MAR1_WR_PG			PAGE_1
/* */
#define MAR2				0x0a
#define MAR2_MASK			0xff
#define MAR2_RD_PG			PAGE_1
#define MAR2_WR_PG			PAGE_1
/* */
#define MAR3				0x0b
#define MAR3_MASK			0xff
#define MAR3_RD_PG			PAGE_1
#define MAR3_WR_PG			PAGE_1
/* */
#define MAR4				0x0c
#define MAR4_MASK			0xff
#define MAR4_RD_PG			PAGE_1
#define MAR4_WR_PG			PAGE_1
/* */
#define MAR5				0x0d
#define MAR5_MASK			0xff
#define MAR5_RD_PG			PAGE_1
#define MAR5_WR_PG			PAGE_1
/* */
#define MAR6				0x0e
#define MAR6_MASK			0xff
#define MAR6_RD_PG			PAGE_1
#define MAR6_WR_PG			PAGE_1
/* */
#define MAR7				0x0f
#define MAR7_MASK			0xff
#define MAR7_RD_PG			PAGE_1
#define MAR7_WR_PG			PAGE_1


/*////// Remote Next Packet Pointer Register Definitions */
/* */
#define RNPP				0x03
#define RNPP_MASK			0xff
#define RNPP_RD_PG			PAGE_2
#define RNPP_WR_PG			PAGE_2


/*////// Local Next Packet Pointer Register Definitions */
/* */
#define LNPP				0x05
#define LNPP_MASK			0xff
#define LNPP_RD_PG			PAGE_2
#define LNPP_WR_PG			PAGE_2


/*////// Address Counter Register Definitions */
/* */
#define ACR1				0x06
#define ACR1_MASK			0xff
#define ACR1_RD_PG			PAGE_2
#define ACR1_WR_PG			PAGE_2
/* */
#define ACR0				0x07
#define ACR0_MASK			0xff
#define ACR0_RD_PG			PAGE_2
#define ACR0_WR_PG			PAGE_2
