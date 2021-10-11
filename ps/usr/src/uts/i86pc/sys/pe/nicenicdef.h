/*
 * Copyright (c) 1993, by Sun Microsystems, Inc.
 */

#pragma ident	"@(#)nicenicdef.h	1.1	93/10/29 SMI"

/* nicdef.inc --- MB86960 definitions include file	(c) 1992 Xircom */

/***************************************************************************************************************; */
/* Revision Information												; */
/*														; */
/*	Author: 	Matt Knudstrup										; */
/*	Started:	April 2, 1992										; */
/*														; */
/*	Language:	MASM 5.1										; */
/*	Build:		none											; */
/*														; */
/*--------------------------------------------------------------------------------------------------------------; */
/* Release 	Date	Who		Why									; */
/* -------	----	---		--------------------------------------------------------		; */
/*														; */
/*--------------------------------------------------------------------------------------------------------------; */
/* Notes:													; */
/*														; */
/*--------------------------------------------------------------------------------------------------------------; */
/* History:													; */
/*														; */
/* Date	 Who    What												; */
/* ----	 ---	------------------------------------------------------------------------------------------------; */
/* 4/93  EKH	Ported to C											; */
/*--------------------------------------------------------------------------------------------------------------; */

/****************************************************************************************************************; */
/* Network Interface Controller (MB86960) Definitions								; */
/*---------------------------------------------------------------------------------------------------------------; */

/* Transmit Status Register (DLCR 0) */

#define SQE		0x01	/* SQE Error */
#define COL_16	   	0x02	/* collision 16 error */
#define COL	   	0x04	/* collision error */
#define JAB		0x08	/* Jabber */
#define TX_SRT_PKT	0x10	/* short packet error */
#define TX_DEF	   	0x20	/* packet receive */
#define NET_BSY	   	0x40	/* carrier sense */
#define TX_OK	   	0x80	/* transmit complete */

/* Receive Status Register (DLCR 1) */

#define OVR_FLO	  	0x01	/* overflow error */
#define CRC_ERR	  	0x02	/* CRC error */
#define ALG_ERR	  	0x04	/* alignment error */
#define RX_SH_PKT  	0x08	/* short packet error */
#define RST_PKT	  	0x10	/* remote reset */
#define BUS_RD_ERR	0x40	/* bus read error */
#define PKT_RDY	  	0x80	/* packet ready */

/* Transmit Masks Register (DLCR 2) */

#define MSK_SQE		0x01	/* signal quality error */
#define MSK_COL_16  	0x02	/* collision 16 mask */
#define MSK_COL	    	0x04	/* collision mask */
#define MSK_JAB		0x08	/* jabber mask */
#define MSK_DEF		0x20	/* deferred tx mask */
#define MSK_TX_OK  	0x80	/* transmit complete mask */

#define TX_MASK		0xab

/* Receive Masks Register (DLCR 3) */

#define MSK_OVR_FLO	0x01	/* overflow mask */
#define MSK_CRC_ERR	0x02	/* CRC mask */
#define MSK_ALG_ERR	0x04	/* alignment mask */
#define MSK_SRT_PKT	0x08	/* short packet mask */
#define MSK_RST_PKT	0x10	/* remote reset mask */
#define MSK_BR_ERR	0x40	/* bus read error */
#define MSK_PKT_RDY	0x80	/* packet ready mask */

#define RX_MASK		0x87

/* Transmit Mode Register (DLCR 4) */

#define DIS_CD	 	0x01	/* disable carrier detect */
#define NOT_LPK	 	0x02	/* disable loopback */
#define NOT_TM	 	0x04	/* disable TM pin */
#define TEST_RX_RNG	0x08
#define TX_MODE		0x06
#define LOOPBACK	0x04

/* Receive Mode Register (DLCR 5) */

#define AM0	     	0x01	/* address match mode bit 0 */
#define AM1	     	0x02	/* address match mode bit 1 */
/*reserved-1  		0x04	// enable remote reset */
#define EN_SH_PKT  	0x08	/* enable short packet receive */
#define FIVE_BYTE_ADDR	0x10	/* enable 5-byte address match */
#define EN_BAD_PKT     	0x20	/* allows receiving of bad pkts */
#define RX_BUF_EMPTY	0x40	/* buffer memory is empty */

#define RX_MODE		0x01

/* Control Register 1 (DLCR 6) */

#define BUF_SZ_8	0x00	/* Buffer Size = 8KBytes */
#define BUF_SZ_16	0x01	/* Buffer Size = 16KBytes */
#define BUF_SZ_32	0x02	/* Buffer Size = 32KBytes */
#define BUF_SZ_64	0x03	/* Buffer Size = 64KBytes */
#define TX_BUF_2	0x00	/* Tx Bufsize = 2KBytes */
#define TX_BUF_4	0x04	/* Tx Bufsize = 4KBytes */
#define TX_BUF_8	0x08	/* Tx Bufsize = 8KBytes */
#define TX_BUF_16	0x0c	/* Tx Bufsize = 16KBytes */
#define BYTE_BUF	0x10	/* 8-bit Buffer */
#define BYTE_BUS 	0x20	/* 8-bit Bus */
#define SRAM_100	0x40	/* 100 ns cycle for SRAM */
#define SRAM_150	0x00	/* 150 ns cycle for SRAM */
#define DIS_NICE	0x80	/* disable NICE */

#define CTRL_REG1	BUF_SZ_32+TX_BUF_4+BYTE_BUF+BYTE_BUS+SRAM_100

/* Control Register 2 (DLCR 7) */
    
#define BYTE_SWAP	0x01	/* Enable byte swapping */
#define EOP_POS		0x02	/* EOP active high */
#define NOT_STBY	0x20	/* Not Standby Mode */
#define ENDEC_CFG	0xc0

#define ID_BNK		0x20	/* Register Bank 0 */
#define HT_BNK		0x24	/* Register Bank 1 */
#define BM_BNK		0x28	/* Register Bank 2 */
#define BNK_MASK	0x0c	/* Register Bank Mask */

/*Data Link register set */

#define DLCR0		0x00
#define TX_STATUS	DLCR0
#define DLCR1		0x01
#define RX_STATUS	DLCR1
#define DLCR2		0x02
#define TX_INT_MASK	DLCR2
#define DLCR3		0x03
#define RX_INT_MASK	DLCR3
#define DLCR4		0x04
#define TX_MODE_REG	DLCR4
#define DLCR5		0x05
#define RX_MODE_REG	DLCR5
#define DLCR6		0x06
#define CONTROL_REG1	DLCR6
#define DLCR7		0x07
#define CONTROL_REG2	DLCR7

#define D0_MSK		0x00
#define D1_MSK		0x00
#define D2_MSK		0x87
#define D3_MSK		0xff
#define D4_MSK		0x07
#define D5_MSK		0x3f
#define D6_MSK		0xff
#define D7_MSK		0xef

/*Node ID register set */

#define IDR8		0x08
#define IDR9		0x09
#define IDR10		0x0a
#define IDR11		0x0b
#define IDR12		0x0c
#define IDR13		0x0d
#define TDR_LOW		0x0e
#define TDR_HIGH	0x0f

#define ID_MSK		0xff

/*Hash Table register set */

#define HTR8		0x08
#define HTR9		0x09
#define HTR10		0x0a
#define HTR11		0x0b
#define HTR12		0x0c
#define HTR13		0x0d
#define HTR14		0x0e
#define HTR15		0x0f

#define HT_MSK		0xff

/*Buffer Management register set */

#define BMPR8		0x08			/*data register /low byte */
#define DATAPORT	BMPR8
#define BMPR9		0x09			/*data register /word */
#define TX_CTRL		0x0a
#define TX_START	0x80

#define BMPR13		0x0d
#define LINK_DISABLE	0x20
#define PS_AUI		0x18
#define PS_TP		0x08

#define COL_CTRL	0x0b
#define COL_CTRL_VAL	0x07			/* Don't stop, and throw packet away */

#define DMA_EN		0x0c
#define DMA_BURST	0x0d
#define RX_CTRL		0x0e
#define SKIP_PKT	0x04
#define BMPR15		0x0f			/*reserved */
