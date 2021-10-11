/*
 * Copyright (c) 1992 by Sun Microsystems, Inc.
 */

#ifndef _EL_H
#define	_EL_H

#pragma ident	"@(#)el.h	1.5	94/09/03 SMI"

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * Hardware specific driver declarations for a 3COM 3C503
 * Ethernet interface driver conforming to the Generic LAN Driver
 * model.
 */

/*
 * Ethernet addresses assigned to 3COM
 */
#define	EA3COM0	0x02		/* first byte */
#define	EA3COM1	0x60		/* second byte */
#define	EA3COM2	0x8c		/* third byte */

/*
 * IRQ values
 */

#define	ELMINIRQ 2		/* minimum IRQ */
#define	ELMAXIRQ 5		/* maximum IRQ */

/*
 * Receive buffer sizes recommended for 8K board.
 * These are page numbers (256 byte pages) with 0x20 added.
 */
#define	RCV_START8K  0x26
#define	RCV_START16K 0x06
#define	RCV_STOP 0x40

/*
 * Transmit buffer start pages
 */
#define	XMT_START8K	0x20
#define	XMT_START16K	0x00

/*
 * DP8390 NIC LAN controller register offsets - add base address (elbase)
 * to each offset
 */
/* page 0 - write registers */
#define	ELCR	0x00		/* Control register */
#define	ELPSTART 0x01		/* Page start register */
#define	ELPSTOP	0x02		/* Page stop register */
#define	ELBNDY	0x03		/* Boundary register */
#define	ELTPSR	0x04		/* Transmit page start address */
#define	ELTBCR0	0x05		/* Transmit byte count register 0 */
#define	ELTBCR1	0x06		/* Transmit byte count register 1 */
#define	ELISR	0x07		/* Interrupt status register */
#define	ELRSAR0	0x08		/* Remote start address register 0 */
#define	ELRSAR1	0x09		/* Remote start address register 1 */
#define	ELRBCR0	0x0a		/* Remote byte count register 0 */
#define	ELRBCR1	0x0b		/* Remote byte count register 1 */
#define	ELRCR	0x0c		/* Receive configuration register */
#define	ELTCR	0x0d		/* Transmit configuration register */
#define	ELDCR	0x0e		/* Data configuration register */
#define	ELIMR	0x0f		/* Interrupt mask register */
/* page 0 - read registers */
#define	ELTSR	0x04		/* Transmit status register */
#define	ELNCR	0x05		/* Number of collisions register */
#define	ELFIFO	0x06		/* FIFO register */
#define	ELRSR	0x0c		/* Receive status register */
#define	ELCNTR0	0x0d		/* Tally counter register 0 (Frame alignment) */
#define	ELCNTR1	0x0e		/* Tally counter register 1 (CRC) */
#define	ELCNTR2	0x0f		/* Tally counter register 2 (Missed packets) */
/* page 1 - write registers */
#define	ELPADR	0x01		/* Physical address register 0 */
#define	ELCURR	0x07		/* Current page register */
#define	ELMADR	0x08		/* Multicast address register 0 */

/*
 * Gate array register offsets - add base address (elbase) to each offset
 * gabase = elbase + 0x400
 */

#define	ELPSTR	0x400		/* Page start */
#define	ELPSPR	0x401		/* Page stop */
#define	ELDQTR	0x402		/* Drq timer */
#define	ELBCFR	0x403		/* Base configuration */
#define	ELPCFR	0x404		/* PROM configuration */
#define	ELGACFR	0x405		/* GA configuration */
#define	ELCTRL	0x406		/* Control register */
#define	ELSTREG	0x407		/* Status register */
#define	ELIDCFR	0x408		/* Int/DMA register */
#define	ELDAMSB	0x409		/* DMA address MSB */
#define	ELDALSB	0x40a		/* DMA address LSB */
#define	ELVPTR2	0x40b		/* Vector pointer 2 */
#define	ELVPTR1	0x40c		/* Vector pointer 1 */
#define	ELVPTR0	0x40d		/* Vector pointer 0 */
#define	ELRFMSB	0x40e		/* Register file LSB */
#define	ELRFLSB	0x40f		/* Register file MSB */

/*
 * bits in GA Control Register (ELCTRL)
 */

#define	ELARST	0x01		/* Software reset */
#define	ELAXCVR	0x02		/* Transceiver select: 1=BNC, 0=DIX */
#define	ELAEALO	0x04		/* Ethfernet address low */
#define	ELAEAHI	0x08		/* Ethernet address high */
#define	ELASHAR	0x10		/* Interrupt share */
#define	ELADBSL	0x20		/* Double buffer select */

/*
 * bits in GA Configuration Register (ELGACFR)
 */
#define	ELG0K	0x00		/* Memory bank select */
#define	ELG8K	0x01		/* Memory bank select */
#define	ELG16K	0x02		/* Memory bank select */
#define	ELG24K	0x03		/* Memory bank select */
#define	ELGRSEL	0x08		/* RAM select */

/*
 * bits in NIC Command Register (ELCR)
 */

#define	ELCSTP	0x01		/* Stop */
#define	ELCSTA	0x02		/* Start */
#define	ELCTXP	0x04		/* Transmit packet */
#define	ELCDMA	0x20		/* Complete remote DMA */
#define	ELCPG0	0x00		/* Page 0 select */
#define	ELCPG1	0x40		/* Page 1 select */
#define	ELCPG2	0x80		/* Page 2 select */
#define	ELPGMSK	0x3f		/* mask off page select */

/*
 * bits in NIC Receive Configuration Register (ELRCR)
 */

#define	ELRSEP	0x01		/* Save error packets */
#define	ELRAR	0x02		/* Accept runt packets */
#define	ELRAB	0x04		/* Accept broadcast packets */
#define	ELRAM	0x08		/* Accept multicast packets */
#define	ELRPRO	0x10		/* Promiscuous physical */
#define	ELRMON	0x20		/* Monitor mode */

/*
 * bits in NIC Receive Status Register (ELRSR)
 */

#define	ELRPRX	0x01		/* Packet received intact */
#define	ELRCRC	0x02		/* CRC error */
#define	ELRFAE	0x04		/* Frame alignment error */
#define	ELRFO	0x08		/* Fifo overrun */
#define	ELRMPA	0x10		/* Missed packet */
#define	ELRPHY	0x20		/* Physical/multicast address */

/*
 * bits in NIC Transmit Configuration Register (ELTCR)
 */

#define	ELTLOOP	 0x02		/* normal loopback mode */
#define	ELTLOOP3 0x6

/*
 * bits in NIC Transmit Status Register (ELTSR)
 */

#define	ELTPTX	0x01		/* Packet transmitted */
#define	ELTDFR	0x02		/* Non deferred transmission */
#define	ELTCOL	0x04		/* Transmit collided */
#define	ELTABT	0x08		/* Transmit aborted (excessive collisions) */
#define	ELTCRS	0x10		/* Carrier sense lost */
#define	ELTFU	0x20		/* Fifo underrun */

/*
 * bits in NIC Interrupt Mask Register (ELIMR) and Interrupt Status Register
 * (ISR)
 */

#define	ELPRXE	0x01		/* Packet received */
#define	ELPTXE	0x02		/* Packet transmitted */
#define	ELRXEE	0x04		/* Receive error */
#define	ELTXEE	0x08		/* Transmit error */
#define	ELOVWE	0x10		/* Overwrite warning */
#define	ELCNTE	0x20		/* Counter overflow */
#define	ELRDCE	0x40		/* DMA complete */
#define	ELRRST	0x80		/* Reset status (ISR only) */
#define	ISRMASK 0x7F		/* interrupts we expect */

/*
 * bits in the NIC DCR
 */
#define	WTS 0x01		/* bus size (Word Transfer Size) */

/*
 *	streams related definitions
 */

#define	ELVPKTSZ	(3*256)
#define	ELHIWAT		(16*ELVPKTSZ)
#define	ELLOWAT		(8*ELVPKTSZ)
#define	ELTXBUFLEN	1536		/* 6 * 256 */
#define	ELMAXPKT	1500
#define	ELMINSEND	60	/* 64 - 4 bytes CRC */

/*
 *	debug bits
 */
#define	ELTRACE		0x01
#define	ELERRS		0x02
#define	ELRECV		0x04
#define	ELDDI		0x08
#define	ELSEND		0x10
#define	ELINT		0x20

#ifdef DEBUG
#define	ELDEBUG
#endif

/*
 *	board state
 */
#define	ELB_IDLE	0
#define	ELB_WAITRCV	1
#define	ELB_XMTBUSY	2
#define	ELB_ERROR	3

/*
 *	structure for front of packet in board buffer
 */
struct rcv_buf {
	unsigned char	status;
	unsigned char	nxtpg;
	short		datalen;
	caddr_t		pkthdr;
};

typedef struct rcv_buf rcv_buf_t;

/*
 *	3COM 3C503 board dependent variables.
 *	In a structure to use multiple boards.
 */

struct elvar {
    int el_nxtpkt;		/* page number of next expected packet */
    int el_watch;		/* watchdog timeout */
    caddr_t el_base;		/* base memory address */
    int el_irq;			/* IRQ level to use */
    int el_mcastcnt[64];	/* to keep track of number of times */
				/* bit is set */
    unsigned char el_memsize;	/* number of 8K banks of memory */
    unsigned char el_ifopts;	/* mask to set interface options */
    unsigned char el_rcvstart;	/* first receive page */
    unsigned char el_curbank;
};

#define	el_inbank(page, curbank)	(((page >> 5) & 0x1F) == curbank)

#ifdef	__cplusplus
}
#endif

#endif	/* _EL_H */
