/*
 * Copyright (c) 1993, by Sun Microsystems, Inc.
 */

#pragma ident	"@(#)nicdef.h	1.1	93/10/29 SMI"

/* NICDEF.EQU --- MB86960 definitions include file	(C) 1993 Xircom */

/*****************************************************************************************************************/
/* Revision Information												*/
/*														*/
/*	Author: 	Eric Henderson										*/
/*	Started:	12/8/92 9:32										*/
/*														*/
/*	Language:	MASM 5.1										*/
/*	Build:		none											*/
/*														*/
/*---------------------------------------------------------------------------------------------------------------*/
/* Release 	Date	Who		Why									*/
/* -------	----	---		--------------------------------------------------------		*/
/* 1.00		3/7/93	EKH		Begin baseline in PVCS (Software control)				*/
/*---------------------------------------------------------------------------------------------------------------*/
/* Notes:													*/
/*														*/
/*---------------------------------------------------------------------------------------------------------------*/
/* History:													*/
/*														*/
/* Date	 Who    What												*/
/* ----	 ---	------------------------------------------------------------------------------------------------*/
/* 														*/
/*---------------------------------------------------------------------------------------------------------------*/

/*****************************************************************************************************************/
/* Network Interface Controller (MB86960) Definitions								*/
/*---------------------------------------------------------------------------------------------------------------*/

/* Transmit Status Register (DLCR 0) */
#define TX_16_COL		0x2	/* collision 16 error */
#define TX_COL	   		0x4	/* collision error */
#define TX_CR_LOST		0x10	/* short packet error */
#define TX_RX	   		0x20	/* packet receive */
#define TX_NET_BSY		0x40	/* carrier sense */
#define TX_DONE	   		0x80	/* transmit complete */

#define TX_MASK			0x86

/* Receive Status Register (DLCR 1) */
#define RX_OVERFLO		0x1	/* overflow error */
#define RX_CRC_ERR		0x2	/* CRC error */
#define RX_ALIGN_ERR		0x4	/* alignment error */
#define RX_RX_SHORT_ERR		0x8	/* short packet error */
#define RX_RMT_900		0x10	/* remote reset */
#define RX_DMA_EOP		0x20	/* DMA End-of-Process */
#define RX_BUS_RD_ERR		0x40	/* bus read error */
#define RX_PKT			0x80	/* packet ready */

#define RX_MASK			0x8f

/* Transmit Interrupts Enable Register (DLCR 2) */
#define TX_INT_COL_16		0x20	/* collision 16 mask */
#define TX_INT_COL		0x40	/* collision mask */
#define TX_INT_RX		0x20	/* Self-reception */
#define TX_INT_DONE		0x80	/* transmit complete mask */

#define TX_INT_MASK		0x82

/* Receive Masks Register (DLCR 3) */
#define RX_INT_OVERFLO		0x1	/* overflow mask */
#define RX_INT_CRC_ERR		0x2	/* CRC mask */
#define RX_INT_ALIGN_ERR	0x4	/* alignment mask */
#define RX_INT_SHORT_ERR	0x8	/* short packet mask */
#define RX_INT_RMT_900		0x10	/* remote reset mask */
#define RX_INT_DMA_EOP		0x20	
#define RX_INT_BUS_RD_ERR	0x40	/* bus read error */
#define RX_INT_PKT		0x80	/* packet ready mask */

#define RX_INT_MASK		0x1f

/* Transmit Mode Register (DLCR 4) */
#define TX_MODE_DIS_TX_DEFER 	0x1	/* When hi, the transmitter will not defer to traffic on the network */
#define TX_MODE_NOT_LOOPBACK 	0x2	/* Disable loopback */
#define TX_MODE_CNTRL	 	0x4	/* The inverse of this bit is output for general use on pin 95 */
#define TX_MODE_COL_COUNT_MASK	0xf0	/* DLC4<7:4> plus 1 indicates the number of */
						/*  consecutive collisions encountered by the */
						/*  current transmit packet. (Read only) */

#define TX_MODE			0x2

/* Receive Mode Register (DLCR 5) */
#define RX_MODE_AF0	     	0x5	/* address match mode bit 0 */
#define RX_MODE_AF1	     	0x6	/* address match mode bit 1 */
#define RX_MODE_ACPT_SHORT_PKTS	0xc	/* enable short packet receive */
#define RX_MODE_40_BIT_ADDR	0x14	/* enable 5-byte address match */
#define RX_MODE_ACPT_BAD_PKTS	0x24	/* allows receiving of bad pkts */
#define RX_MODE_RX_BUF_EMPTY	0x40	/* buffer memory is empty */

#define RX_MODE			0x4

/* Configuration Register 0 (DLCR 6) */
#define CFG0_BS_8		0x40	/* Buffer Size = 8KBytes */
#define CFG0_BS_16		0x41	/* Buffer Size = 16KBytes */
#define CFG0_BS_32		0x42	/* Buffer Size = 32KBytes */
#define CFG0_BS_64		0x43	/* Buffer Size = 64KBytes */
#define CFG0_TBS_2		0x40	/* Tx Bufsize = 2KBytes */
#define CFG0_TBS_4		0x44	/* Tx Bufsize = 4KBytes */
#define CFG0_TBS_8		0x48	/* Tx Bufsize = 8KBytes */
#define CFG0_TBS_16		0x4c	/* Tx Bufsize = 16KBytes */
#define CFG0_BB			0x50	/* 8-bit Buffer */
#define CFG0_SB			0x60	/* 8-bit Bus */
#define CFG0_DIS_DLC		0xc0	/* disable NICE */

#define CFG0			(CFG0_BS_32 | CFG0_TBS_4 | CFG0_BB)

/* Configuration Register 1 (DLCR 7) */
#define CFG1_TENBT           	     0x80       /* 10Base-T */
#define CFG1_TENB2           	     0x00       /* 10Base-2 */

#define CFG1_ML			0x1	/* Enable byte swapping */
#define CFG1_EOP_POL		0x2	/* EOP active high */
#define CFG1_ID_BNK		0x0	/* Register Bank 0 */
#define CFG1_HT_BNK		0x4	/* Register Bank 1 */
#define CFG1_BM_BNK		0x8	/* Register Bank 2 */
#define CFG1_NOT_PWRDN		0x20	/* Not Standby Mode */
#define CFG1_ENDEC_MON		0x40	/* En/Dec + Monitor */
#define CFG1_MON		0x80	/* Monitor only */
#define CFG1_ENDEC_TEST		0xc0	/* Controller inactive */

#define CFG1			(CFG1_NOT_PWRDN | CFG1_TENBT)

/*Data Link register set */
#define DLCR0		0x0
#define TX_STATUS_REG	DLCR0
#define DLCR1		0x1
#define RX_STATUS_REG	DLCR1
#define DLCR2		0x2
#define TX_INT_ENB_REG	DLCR2
#define DLCR3		0x3
#define RX_INT_ENB_REG	DLCR3
#define DLCR4		0x4
#define TX_MODE_REG	DLCR4
#define DLCR5		0x5
#define RX_MODE_REG	DLCR5
#define DLCR6		0x6
#define CONFIG0_REG	DLCR6
#define DLCR7		0x7
#define CONFIG1_REG	DLCR7

/*Node ID register set */
#define IDR8		0x8
#define IDR9		0x9
#define IDR10		0xa
#define IDR11		0xb
#define IDR12		0xc
#define IDR13		0xd
#define TDR0		0xe
#define TDR1		0xf

/*Hash Table register set */
#define HTR8		0x8
#define HTR9		0x9
#define HTR10		0xa
#define HTR11		0xb
#define HTR12		0xc
#define HTR13		0xd
#define HTR14		0xe
#define HTR15		0xf

/*Buffer Management register set */
#define BMPR8		0x8	/*data register /low byte */
#define DATAPORT	BMPR8
#define BMPR9		0x9	/*data register /word */
#define TX_START_REG	0xa
#define BMPR10		0xa
#define COL_16_REG	0xb
#define BMPR11		0xb
#define DMA_ENB_REG	0xc
#define BMPR12		0xc
#define DMA_BURST_REG	0xd
#define BMPR13		0xd
#define SKIP_PKT_REG	0xe
#define BMPR14		0xe
#define BMPR15		0xf	/*reserved */

#define TX_START	0x80
#define SKIP_PKT	0x4
