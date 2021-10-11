/*
 * Copyright (c) 1992, 1993, by Sun Microsystems, Inc.  All Rights Reserved.
 */

/*
 * Module: WD8003
 * Project: System V ViaNet
 *
 *		Copyright (c) 1987, 1989 by Western Digital Corporation.
 *		All rights reserved.  Contains confidential information and
 *		trade secrets proprietary to
 *			Western Digital Corporation
 *			2445 McCabe Way
 *			Irvine, California  92714
 */

#ifndef	_SYS_SMCHDW_H
#define	_SYS_SMCHDW_H

#pragma ident	"@(#)smchdw.h	1.5	94/09/03 SMI"

#ifdef	__cplusplus
extern "C" {
#endif

/* Hardware definitions for the WD8003 and 8390 LAN controller		*/

/* WD8003 Definitions and Statistics					*/

#define	TX_BUF_LEN	(6*256)
#define	SEG_LEN		(1024 * 8)
#define	EDEVSIZ		0x20

/* WD8003 Commands							*/

#define	SFTRST	0x80			/* software reset command	*/
#define	MEMENA	0x40			/* memory enable switch		*/

/* WD8003 register locations						*/

#define	ICR		1	/* Interrupt Control Register (on	*/
				/* boards with interface chip)		*/
#define	ICR_585		6	/* moved to offset 6 on 585 chip	*/
#define	IRR		4	/* Interrupt Request Register (on	*/
				/* boards with WD83C583 chip)		*/
#define	CCR		5	/* Configuration Control Register	*/
				/* (Micro Channel boards with 		*/
				/* WD83C593 chip)			*/
#define	LAAR		5	/* LA Address Register (16 bit AT bus	*/
				/* boards)				*/
#define	UBA_STA		8
#define	WD_BYTE		0xE
#define	LAAR_MASK	0x1f

/* ICR register definitions */
#define	ICR_IR2		0x04

/* ICR_585 register definitions */
#define	ICR_EIL		0x01
#define	ICR_MASK2	0x04

/* IRR register definitions */
#define	IRR_IEN		0x80	/* Interrupt Enable */
#define	IRR_IRQMASK	0x60	/* mask for IRQ bits */
#define	IRR_IRQ2	0x00	/* IRQ2 selected */
#define	IRR_IRQ3	0x20	/* IRQ3 selected */
#define	IRR_IRQ4	0x40	/* IRQ4 selected */
#define	IRR_IRQ7	0x60	/* IRQ7 selected */
#define	IRR_IR0		0x20	/* from eazysetup listing. Unknown meaning */
#define	IRR_IR1		0x40	/* from eazysetup listing. Unknown meaning */

/* CCR register definitions */
#define	EIL		0x4		/* enable interrupts		*/

/* LAAR register definitions */
#define	MEM16ENB	0x80
#define	LAN16ENB	0x40

/* 8390 Registers: Page 0						*/
/* NOTE: All addresses are offsets from the command register (cmd_reg)	*/

#define	PSTART	0x1
#define	PSTOP	0x2
#define	BNRY	0x3
#define	TPSR	0x4
#define	TBCR0	0x5
#define	TBCR1	0x6
#define	ISR	0x7
#define	RBCR0	0xA
#define	RBCR1	0xB
#define	RCR	0xC
#define	TCR	0xD
#define	DCR	0xE
#define	IMR	0xF
#define	RSR	0xC
#define	CNTR0	0xD
#define	CNTR1	0xE
#define	CNTR2	0xF

/* 8390 Registers: Page 1						*/
/* NOTE: All addresses are offsets from the command register (cmd_reg)	*/

#define	PAR0	0x1
#define	CURR	0x7
#define	MAR0	0x8

/* 8390 Commands							*/

#define	PAGE_0	0x00
#define	PAGE_1	0x40
#define	PAGE_2	0x80
#define	PAGE_3	0xC0

#define	PG_MSK	0x3F			/* Used to zero the page select */
					/* bits in the command register */

#define	STA	0x2			/* Start 8390			*/
#define	STP	0x1			/* Stop 8390			*/
#define	TXP	0x4			/* Transmit Packet		*/
#define	ABR	0x20			/* Value for Remote DMA CMD	*/

/* 8390 ISR conditions							*/

#define	PRX	0x1
#define	PTX	0x2
#define	RXE	0x4
#define	TXE	0x8
#define	OVW	0x10
#define	CNT	0x20
#define	RST	0x80

/* 8390 IMR bit definitions						*/

#define	PRXE	0x1
#define	PTXE	0x2
#define	RXEE	0x4
#define	TXEE	0x8
#define	OVWE	0x10
#define	CNTE	0x20
#define	RDCE	0x40

/* 8390 DCR bit definitions						*/

#define	WTS	0x1
#define	BOS	0x2
#define	LAS	0x4
#define	BMS	0x8
#define	FT0	0x20
#define	FT1	0x40


/* 8390 TCR bit definitions						*/

#define	CRC	0x1
#define	LB0_1	0x2
#define	ATD	0x8
#define	OFST	0x10


/* RCR bit definitions							*/

#define	SEP	0x1
#define	AR	0x2
#define	AB	0x4
#define	AM	0x8
#define	PRO	0x10
#define	MON	0x20

/* TSR bit definitions							*/
#define	TSR_COL 0x4
#define	TSR_ABT 0x8

/* 8390 Register initialization values					*/

#define	INIT_IMR	(PRXE + PTXE + RXEE + TXEE + OVWE + CNTE)
#define	INIT_DCR	BMS + FT1
#define	INIT_TCR	0
#define	INIT_RCR	AB + AM
#define	RCRMON		MON

/* Misc. Commands & Values						*/

#define	CLR_INT		0xFF		/* Used to clear the ISR */
#define	NO_INT		0		/* no interrupts conditions */
#define	ADDR_LEN	6
#define	NETPRI		PZERO+3

/* PS2 specific defines */
/* Defines for PS/2 Micro Channel POS ports */

#define	SYS_ENAB	0x94		/* System board enable / setup */
#define	ADAP_ENAB	0x96		/* Adaptor board enable / setup */
#define	POS_0		0x100		/* POS reg 0 - adaptor ID lsb */
#define	POS_1		0x101		/* POS reg 1 - adaptor ID msb */
#define	POS_2		0x102		/* Option Select Data byte 1 */
#define	POS_3		0x103		/* Option Select Data byte 2 */
#define	POS_4		0x104		/* Option Select Data byte 3 */
#define	POS_5		0x105		/* Option Select Data byte 4 */
#define	POS_6		0x106		/* Subaddress extension lsb */
#define	POS_7		0x107		/* Subaddress extension msb */

#define	SETUP		0x08		/* put into setup mode */
#define	DISSETUP	0x00		/* disable setup, back to normal mode */

#define	MC_SLOTS	8		/* max number of Micro Channel slots */

/* Defines for Adapter Board ID's for Micro Channel */

#define	IDMSB	0x6F			/* adapter id MSB */
#define	IDLSBE	0xC0			/* adapter id LSB for 8003E[T]/A */
#define	IDLSBS	0xC1			/* adapter id LSB for 8003S[TH]/A ID */
#define	IDLSBW	0xC2			/* adapter id LSB for 8003W/A ID */
#define	WD_ETA_ID ((IDMSB << 8) | IDLSBE)
#define	WD_STA_ID ((IDMSB << 8) | IDLSBS)
#define	WD_WA_ID  ((IDMSB << 8) | IDLSBW)
#define	WD_EA1_ID 0x61c8		/* adapter id for 8013e/a */
#define	WD_WA1_ID 0x61c9		/* adapter id for 8013w/a */
#define	IBM_13E_ID 0xefd5		/* adapter id for IBM ENA 13E */
#define	IBM_13W_ID 0xefd4		/* adapter id for IBM ENA 13W */
#define	IOBASE_594 0x800		/* I/O base address if 594 chip */
#define	PS2_RAMSZ8 8192

#define	NUM_ID	4

#define	PS2_RAMSZ	16384

/*
 * Register offset definitions...the names for this registers are consistent
 * across all boards, so specific names will be assigned
 */
#define	LAN_ADDR_0	0x08		/* these 6 registers hold the */
#define	LAN_ADDR_1	0x09		/* LAN address for this node */
#define	LAN_ADDR_2	0x0A
#define	LAN_ADDR_3	0x0B
#define	LAN_ADDR_4	0x0C
#define	LAN_ADDR_5	0x0D
#define	LAN_TYPE_BYTE	0x0E		/* 02 if starlan, 03 if ethernet */
#define	CHCKSM_BYTE	0x0F		/* the address ROM checksum byte */

/*
 * Western Digital ID byte values
 */
#define	WD_ID_BYTE0	0x00
#define	WD_ID_BYTE1	0x00
#define	WD_ID_BYTE2	0xC0

/*
 * Offset 04: HWR - Hardware Support Register (585/790)
 */
#define	REG_HWR		0x004	/* Register Offset */
#define	HWR_SWH		0x080	/* Switch Register Set */
#define	HWR_LPRM	0x040	/* LAN Address ROM Select */
#define	HWR_ETHER	0x020	/* NIC Type. 1=83C690, 0=83C825. (Read) */
#define	HWR_HOST16	0x010	/* Set When Host has 16 bit bus. (Read) */
#define	HWR_STAT2	0x008	/* Interrupt Status (Read) */
#define	HWR_STAT1	0x004	/* Interrupt Status (Read) */
#define	HWR_GIN2	0x002	/* General Purpose Input 2 (Read) */
#define	HWR_GIN1	0x001	/* General Purpose Input 1 (Read) */

#define	HWR_MASK	0x020	/* Interrupt Mask Bit (Write) */
#define	HWR_NUKE	0x008	/* Hardware Reset (Write) */
#define	HWR_CLR1	0x004	/* Clear Interrupt (Write, 585 only) */
#define	HWR_HWCS	0x002	/* WCS Control (Write, 585 only) */
#define	HWR_CA		0x001	/* Control Attention (Write, 585 only) */

#ifdef	__cplusplus
}
#endif

#endif	/* _SYS_SMCHDW_H */
