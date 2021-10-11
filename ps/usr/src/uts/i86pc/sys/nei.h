/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 */
#pragma ident "@(#)nei.h	1.5	95/01/31 SMI"

/*
 * nei.h
 * Hardware specific driver declarations for the EP2000Tplus AT/LANTIC
 * driver conforming to the Generic LAN Driver model.
 */

/*
 * Modification history
 *
 * ------------------------------------------------------------------------
 * Date		Author		Changes
 * ------------------------------------------------------------------------
 *				Ver  2.0
 */

#ifndef _NEI_H
#define _NEI_H 1

/* debug flags */
#define NEITRACE	0x01
#define NEIERRS		0x02
#define NEIRECV		0x04
#define NEIDDI		0x08
#define NEISEND		0x10
#define NEIINT		0x20

#ifdef DEBUG
#define NEIDEBUG 1
#endif

/* Misc */
#define NEIHIWAT	32768		/* driver flow control high water */
#define NEILOWAT	4096		/* driver flow control low water */
#define NEIMAXPKT	1500		/* maximum media frame size */
#define NEIIDNUM	0		/* should be a unique id; zero works */
#define NE2K_MAX_IOPORTS   8

#define NEITXBUFLEN	1536		/* 6 * 256 */
#define NEIMAXPKT	1500
#define NEIMINSEND	60		/* 64 - 4 bytes CRC */
#define NEICRCLENGTH	4		/* CRC on the ethernet frame */

/* board state */
#define NEI_IDLE	0
#define NEI_WAITRCV	1
#define NEI_XMTBUSY	2
#define NEI_ERROR	3

#define NEI_MEM8K	0	
#define NEI_MEM16K	1

/*
 * Receive buffer sizes recommended for 8/16K boards
 */
#define NEI_TXSTART8K 		(offset_8k + 0x20)
#define NEI_RCVSTART8K 		(offset_8k + 0x2C)
#define NEI_RCVSTOP8K  		(offset_8k + 0x40)

#define NEI_TXSTART16K		(offset_16k + 0x40)
#define NEI_RCVSTART16K 	(offset_16k + 0x4C)
#define NEI_RCVSTOP16K 		(offset_16k + 0x80)

#define NEI_TX_FREE		0		/* Ver 1.6 */
#define NEI_TX_BUSY		1

#define NEI_UNWIND_COUNT	8		/* Ver 1.7 */

/*
 * Transmit buffer for 8/16K boards
 */

/*
 * bits in SNIC Command Register (NEI_CR)
 */

#define NEI_CSTP	0x01		/* Stop */
#define NEI_CSTA	0x02		/* Start */
#define NEI_CTXP	0x04		/* Transmit packet */
#define NEI_CRREAD	0x08		/* remote read 	*/
#define NEI_CRWRIT	0x10		/* remote write */
#define NEI_CSNDPKT	0x18		/* Send packet	*/
#define NEI_CDMA	0x20		/* Complete remote DMA */
#define NEI_CPG0	0x00		/* Page 0 select */
#define NEI_CPG1	0x40		/* Page 1 select */
#define NEI_CPG2	0x80		/* Page 2 select */
#define NEI_PGMSK	0x3f		/* mask off page select */

/*
 * bits in SNIC Receive Configuration Register (NEI_RCR)
 */

#define NEI_RCSEP	0x01		/* Save error packets */
#define NEI_RCAR	0x02		/* Accept runt packets */
#define NEI_RCAB	0x04		/* Accept broadcast packets */
#define NEI_RCAM	0x08		/* Accept multicast packets */
#define NEI_RCPRO	0x10		/* Promiscuous physical */
#define NEI_RCMON	0x20		/* Monitor mode */

/*
 * bits in SNIC Receive Status Register (NEI_RSR)
 */

#define NEI_RSPRX	0x01		/* Packet received intact */
#define NEI_RSCRC	0x02		/* CRC error */
#define NEI_RSFAE	0x04		/* Frame alignment error */
#define NEI_RSFO	0x08		/* Fifo overrun */
#define NEI_RSMPA	0x10		/* Missed packet */
#define NEI_RSPHY	0x20		/* Physical/multicast address */

/*
 * bits in SNIC Transmit Configuration Register (NEI_TCR)
 */

#define NEI_TCLOOP	0x02		/* normal loopback mode */
#define NEI_TCLOOP2	0x04		/* internal ENDEC module loopback */
#define NEI_TCLOOP3 	0x06		/* external loopback */

/*
 * bits in SNIC Transmit Status Register (NEI_TSR)
 */

#define NEI_TSPTX	0x01	/* Packet transmitted */
#define NEI_TSDFR	0x02	/* Non deferred transmission */
#define NEI_TSCOL	0x04	/* Transmit collided */
#define NEI_TSABT	0x08	/* Transmit aborted (excessive collisions) */
#define NEI_TSCRS	0x10	/* Carrier sense lost */
#define NEI_TSFU	0x20	/* Fifo underrun */

/*
 * bits in SNIC Interrupt Mask Register (NEI_IMR) and Interrupt Status Register
 * (ISR)
 */

#define NEI_IPRXE	0x01		/* Packet received */
#define NEI_IPTXE	0x02		/* Packet transmitted */
#define NEI_IRXEE	0x04		/* Receive error */
#define NEI_ITXEE	0x08		/* Transmit error */
#define NEI_IOVWE	0x10		/* Overwrite warning */
#define NEI_ICNTE	0x20		/* Counter overflow */
#define NEI_IRDCE	0x40		/* DMA complete */
#define NEI_IRRST	0x80		/* Reset status (ISR only) */
#define NEI_ISRMASK 	0x7F		/* interrupts we expect */

/*
 * bits in the SNIC DCR
 */
#define NEI_DWTS 	0x01		/* bus size (Word Transfer Size) */
					/* 0 for Byte Transfer Size */
#define NEI_DARM	0x10		/* Execute send command */
#define NEI_DNORM 	0x08		/* Normal operation */
#define NEI_DFT0	0x20		/* FIFO Threshhold bits */
#define NEI_DFT1	0x40		

/*
 *	Physical and Muticast register offsets
 */

#define NEI_MULTOFF	0x08
#define NEI_PHYOFF	0x01


/*
 * Note: iobase is a local variable assigned with the I/O port base
 *	 address from the macinfo structure.
 */

#define NEI_COMMAND	(core_iobase)		/* Command Register	*/
#define NEI_PSTART	(core_iobase + 1)   	/* Page Start Register 	*/
#define NEI_PSTOP	(core_iobase + 2)  	/* Page Stop Register 	*/
#define NEI_BNDRY	(core_iobase + 3)  	/* Boundary Register 	*/
#define NEI_TXSTATUS	(core_iobase + 4)	/* Xnit status Register	*/
#define NEI_TXPAGE	(core_iobase + 4)	/* Xmit Page Register 	*/
#define NEI_TXBYTCNT0	(core_iobase + 5)	/* Xmit Byte count 0	*/
#define NEI_NCR		(core_iobase + 5)	/* # of collisions register*/
#define NEI_TXBYTCNT1	(core_iobase + 6)	/* Xmit Byte count 1	*/
#define NEI_INTRSTS	(core_iobase + 7)	/* Interrupt Status Register*/
#define NEI_CURRENT	(core_iobase + 7)	/* Current Page Register*/
#define NEI_RSTRTADR0	(core_iobase + 8)	/* Remote start Addr Register0*/
#define NEI_CRDMA0	(core_iobase + 8)	/* Current DMA  Register 0 */
#define NEI_RSTRTADR1	(core_iobase + 9)	/* Remote start Addr Register1*/
#define NEI_CRDMA1	(core_iobase + 9)	/* Current DMA  Register 1 */

#define NEI_RBYTCNT0	(core_iobase + 10) /* Remote Byte Cnt Register	*/
#define NEI_RBYTCNT1	(core_iobase + 11) /* Remote Byte Cnt Register	*/
#define NEI_RCVSTS	(core_iobase + 12) /* Receive Status		*/
#define NEI_RCR		(core_iobase + 12) /* Receive Configuration 	*/
#define NEI_TCR		(core_iobase + 13) /* Transmit Configuration 	*/
#define NEI_FAETALLY	(core_iobase + 13) /* */
#define NEI_DCR		(core_iobase + 14) /* Data Configuration Register */
#define NEI_CRCTALLY	(core_iobase + 14) /* */
#define NEI_INTRMASK	(core_iobase + 15) /* Interrupt Mask 	        */
#define NEI_MISPKTTLY	(core_iobase + 15) /* Miss Packet Tally	        */
#define NEI_IOPORT	(iobase + 16) /* I/O Port Register	        */
#define NEI_RESETPORT	(iobase + 0x1f) /* I/O reset Register	        */

# define 	NEI_16BIT_SLOT		'W'
# define 	NEI_8BIT_SLOT		'B'

/* Ver 1.6 defines and macros for Q implementation */
# define NEI_MAX_TX_BUF	2
# define NEI_Q_SIZE 	16	/* size should be 2 ^ n  */
# define NEI_Q_MASK 	15	/* should be Q_SIZE - 1 */

/* Add a mblk_t * to the Q */
# define	NEI_ADD_TO_Q(rValue)\
{\
	if(neip->count == NEI_Q_SIZE)\
		rValue = 0;\
	else\
	if(!(neip->send_q[neip->write_ptr & NEI_Q_MASK] = dupmsg(mp)))\
		rValue = 0;\
	else{\
	neip->write_ptr++;\
	neip->count++;\
	rValue = 1;\
	}\
}

/* Remove a mblk_t * from the Q */
# define	NEI_REMOVE_FROM_Q(rValue)\
{\
	if(!neip->count)\
		rValue = 0;\
	else{\
		neip->count--;\
		rValue = neip->send_q[neip->read_ptr++ & NEI_Q_MASK];\
	}\
}
	
/*
 *	structure for front of packet in board buffer
 */

# pragma pack(1)
struct recv_pkt_hdr {
	unsigned char	status;
	unsigned char	nxtpkt;
	short		pktlen;
	/* Frame data starts here */
	} ;
# pragma pack() 

/* driver specific declarations */
struct neiinstance {
	unchar	multcntrs[64] ;		/* Mutilcast counters */
	unchar	recvstart ;		/* Receive buffer start */
	unchar	recvstop ;		/* Receive buffer stop */
	unchar	nxtpkt ;		/* ptr to the nxt pkt in the buf ring*/
	unchar	memsize ;		/* 16 or 8 bit board */

	/* Ver 1.6 fields for double buffering */
	unchar	txstart[NEI_MAX_TX_BUF] ;	/* Transmit buffer start */
	int	tx_len[NEI_MAX_TX_BUF];
	unchar  tx_buf_flag[NEI_MAX_TX_BUF];
	unchar  tx_curr;		/* which tx buffer is in transmit */

	/* Ver 1.6 fields for Q implementation */
	mblk_t 	*send_q[NEI_Q_SIZE];
	unchar	read_ptr;
	unchar 	write_ptr;
	unchar 	count;

	unchar	tx_state ;	/* transmitter state */

	int	mode_sm;	/* indicates IO_MODE=0 or SM_MODE=1 */
	unchar	* shared_mem;	/* Shared Memory Virtual addr, from gldm_memp */
	int	ne2kplus;	/* if ne2k+ card detected, ne2kplus=1 */
	unchar	cntrl1;		/* in ne2kplus, control_Reg1 */
	unchar	cntrl2;		/* in ne2kplus, control_Reg2 */
};

/* Added for ne2kplus */
#define	NEI_ATDETREG	(iobase + 1)	/* SM ATDETECT -- 8/16 bit slot */
#define	NEI_CNTRREG1	(iobase)	/* SM Control Reg 1 */
#define	NEI_CNTRREG2	(iobase + 5)	/* SM Control Reg 2 */
#define	NEI_ETHRPROM	(iobase + 8)	/* SM  Ethernet Addr PROM */

/* bits in SM Mode, NEI_CNTRREG1 */
#define	NEI_REG1RST	0x80		/* RESET bit */
#define	NEI_REG1MEME	0x40		/* Memory_Enable bit */

/* bits in SM Mode, NEI_CNTRREG2 */
#define	NEI_REG28OR16	0x80		/* 8/16 bit access bit */
#define	NEI_REG2MEMW	0x40		/* 8/16 Kbyte memory  */

/* Registers A, B and C provided in AT/LANTIC, reserved in ne2k */
#define	NEI_CONFREGA	(core_iobase + 10)
#define	NEI_CONFREGB	(core_iobase + 11)
#define	NEI_CONFREGC	(core_iobase + 12)

/* bits in Config Reg A and B */
#define	MEM16K	0x01
#define	GDLINK	0x04
#define	EELOAD	0x80
#define	SMFLG	0x80
#define	SMIOMSK	0x7

#define	IO_MODE	0
#define	SM_MODE	1
#define	NO_MODE	-1

#define NEI_DEFAULT_NE2000P_MODE	SM_MODE 	/* CAN BE CHANGED TO IO_MODE */

/* defines for nei_other probe */
#define NEI_WD_PROM	8
#define NEI_WD_ROM_CHECKSUM_TOTAL	0xFF
#define NEI_WD_CARD_ID	NEI_WD_PROM+6
#define NEI_TYPE_WD8003S		0x02
#define NEI_TYPE_WD8003E		0x03
#define NEI_TYPE_WD8013EBT	0x05
#define NEI_TYPE_TOSHIBA1	0x11 /* named PCETA1 */
#define NEI_TYPE_TOSHIBA2	0x12 /* named PCETA2 */
#define NEI_TYPE_TOSHIBA3	0x13 /* named PCETB  */
#define NEI_TYPE_TOSHIBA4	0x14 /* named PCETC  */
#define NEI_TYPE_WD8003W		0x24
#define NEI_TYPE_WD8003EB	0x25
#define NEI_TYPE_WD8013W		0x26
#define NEI_TYPE_WD8013EP	0x27
#define NEI_TYPE_WD8013WC	0x28
#define NEI_TYPE_WD8013EPC	0x29
#define NEI_TYPE_SMC8216T	0x2a
#define NEI_TYPE_SMC8216C	0x2b
#define NEI_TYPE_WD8013EBP	0x2c
#define NEI_3COM_NIC_OFFSET	0
#define NEI_3COM_ASIC_OFFSET	0x400		/* offset to nic i/o regs */
#define NEI_3COM_BCFR		3
#define NEI_3COM_BCFR_2E0	0x01
#define NEI_3COM_BCFR_2A0	0x02
#define NEI_3COM_BCFR_280	0x04
#define NEI_3COM_BCFR_250	0x08
#define NEI_3COM_BCFR_350	0x10
#define NEI_3COM_BCFR_330	0x20
#define NEI_3COM_BCFR_310	0x40
#define NEI_3COM_BCFR_300	0x80
#define NEI_3COM_PCFR		4
#define NEI_3COM_PCFR_C8000	0x10
#define NEI_3COM_PCFR_CC000	0x20
#define NEI_3COM_PCFR_D8000	0x40
#define NEI_3COM_PCFR_DC000	0x80
#endif /* _NEI_H */
