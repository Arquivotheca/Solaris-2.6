/* Solaris notes:
 *
 * This file was provided by SMC and contains a Lower Mac network driver
 * for the WD/SMC 8003, 8013, 8216, and 8416 cards.
 *
 * In principle we do not make changes to this file, so that when SMC
 * provides us an update with bug fixes, performance enhancements, or
 * support for additional cards, we can simply plug in the new version
 * and run with it.
 *
 * Unfortunately this file started as a rather early version of their
 * lower mac driver, and has had to have some changes so that it would
 * work under Solaris.  Besides those necessary changes, we have made
 * changes required to make this file DDI compliant.  We have not and
 * should not fix lint warnings or make any kind of stylistic changes.
 * To make cstyle changes would have the effect of making this file
 * harder to maintain in the future, when we get an updated version,
 * which is contrary to the intended purpose of running cstyle.  So:
 *
 * DO NOT LINT OR CSTYLE THIS FILE.
 */

/*
 * Project: System V ViaNet Module: lm.c Copyright (c) 1992 Standard
 * MicroSystems Corporation All Rights Reserved Contains confidential
 * information and trade secrets proprietary to: STANDARD MICROSYSTEMS
 * CORPORATION 6 Hughes Irvine, CA 92718
 * 
 */

#ident "@(#)lm.c	1.4 - 92/06/29"
/*
 * Streams driver for SMC3032 Dual Port Ethernet controller Implements an
 * extended version of the AT&T Link Interface IEEE 802.2 Class 1 type 1
 * protocol is implemented and supports receipt and response to XID and TEST
 * but does not generate them otherwise. Ethernet encapsulation is also
 * supported by binding to a SAP greater than 0xFF.
 */

/* Copyright (c) 1995, Sun Microsystems, Inc. */
/* Extensive rewrite has been done in this source file */

#pragma ident	"@(#)smce_lm.c	1.13	95/03/22 SMI"

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/param.h>
#include <sys/stropts.h>
#include <sys/stream.h>
#include <sys/conf.h>
#include <sys/debug.h>

#include <sys/ddi.h>
#include <sys/sunddi.h>

#include "sys/smce_lm.h"

extern ulong_t  gldcrc32(uchar_t *);

#ifdef SMCEDEBUG
extern int      smcedebug;
#endif

#define nextsize(len) ((len+64)*2)


extern int      sm_boardcnt;	/* number of boards */
extern int      sm_multisize;	/* number of multicast addrs/board */
/* extern struct smdev smdevs[];	* queue specific parameters *AlanKa */
extern struct smparam smparams[];	/* board specific parameters */
/* extern struct smstat smstats[];	* board statistics *AlanKa */
extern struct smmaddr smmultiaddrs[];	/* storage for multicast addrs */
extern struct initparms sminitp[];	/* storage for initialization values */

extern unsigned char smhash();

void            clock_eeprom();


/*
 * initialize the necessary data structures for the driver to get called. the
 * hi/low water marks will have to be tuned
 */


/* streams dev switch table entries */

#ifdef M_XENIX
#include "sys/machdep.h"
#endif


#ifdef LAI_TCP
extern struct ifstats *ifstats;
extern struct ifstats sms_ifstats[];
extern int      smboardfirst[];
#endif


unsigned char   expected_id[4] = {ID_BYTE0, ID_BYTE1, ID_BYTE2, ID_BYTE3};
/* initialize the pad buffer here to save a little time */
unsigned short   pad_buf[(SMMINSEND+1)/2] = {0};

#ifdef NOT_SOLARIS
static int      new_board = 0;
static int      checked = 0;
#endif /* NOT_SOLARIS */

#define SRC_ALIGN	0
#define DEST_ALIGN	1


/*
 * print_msg, print_number, and print_addr are simple debug message routines.
 * ISC Unix doesn't handle printfs in the initialization code...
 */
void
print_msg(str)
	char           *str;
{
	cmn_err(CE_CONT, "%s", str);
}

void
print_number(num)
	unsigned char   num;
{
	cmn_err(CE_CONT, "0x%x", num);
}

void
print_addr(addr)
	unsigned long   addr;
{
	print_number((addr >> 24));
	print_number((addr >> 16) & 0x00ff);
	print_number((addr >> 8) & 0x0000ff);
	print_number((addr & 0x000000ff));
}

int
smce_lm_service_events(smp)
	struct smparam *smp;
{
	unsigned char reg, reg1;
	unsigned long   nice_offset = smp->io_base + smp->sm_nice_addr;
	int             current_buf;
	unsigned short	length;
	struct xmit_buffer_monitor *xmp;
	int		claimed = 0;

#ifdef SMCEDEBUG
	if (smcedebug) {
		int		x;

		print_addr(smp->io_base);
		print_msg(" ");
		for (x = 0; x < 8; x++) {
			print_number(inb(smp->io_base + smp->sm_nice_addr + x));
			print_msg(" ");
		}
	}
#endif

	/* mask out board interrupts */
	reg = inb(smp->io_base + MODE_REGISTER_OFFSET);
	reg |= 0x01;
	outb(smp->io_base + MODE_REGISTER_OFFSET, reg);

	/*
	 * check the alternate xmit buffer, since the current buffer is the
	 * one being filled
	 */
	reg = inb(nice_offset + XMIT_STATUS);
	if (reg & 0x86)	/* was 0x80 */
		claimed++;
	else
		goto check_recv;

	/* these conditions happen and need to be checked and reset */
	if (reg & 0x20) {
#ifdef SMCEDEBUG
		if (smcedebug)
			cmn_err(CE_CONT, "TX-RX ");
#endif
		outb(nice_offset + XMIT_STATUS, 0x20);
	}

	if (reg & 0x04) {
#ifdef SMCEDEBUG
		if (smcedebug)
			cmn_err(CE_CONT, "COL ");
#endif
		outb(nice_offset + XMIT_STATUS, 0x04);
	}
	/* end */
	
	if (reg & XMIT_16_COLLISIONS) {
#ifdef SMCEDEBUG
		if (smcedebug)
			cmn_err(CE_CONT, "16 coll::");
#endif
		outb(nice_offset + XMIT_STATUS, XMIT_16_COLLISIONS);
		outb(nice_offset + 11, 0x07);
	}

	if (reg & XMIT_BUS_ERROR) {
#ifdef SMCEDEBUG
		if (smcedebug)
			cmn_err(CE_CONT, "xmitbus error\n");
#endif
		outb(nice_offset + XMIT_STATUS, XMIT_BUS_ERROR);
	}

	if (reg & XMIT_OK) {
		outb(nice_offset + XMIT_STATUS, XMIT_OK);
	}

	/*
	 * Reset the status of the transmit buffer that was transmitting.
	 * Then check if the other transmit buffer is ready but not
	 * currently being loaded with a new packet.  If so, start
	 * transmission on that transmit buffer.  Otherwise, return now
	 * and lm_send() will then start the transmission when the current
	 * packet has been fully loaded to the transmit buffer.  This
	 * scheme can correctly handle the case when lm_send() is being
	 * interrupted by lm_service_events() in the middle of loading a
	 * new packet to the other transmit buffer.
	 *
	 * smp->nic.current_buf is the buffer being loaded with new packets
	 * so the other buffer is the one being transmitted.
	 */

	/* We only check for the following when a transmit-
	 * related interrupt was generated.  We don't want
	 * a receive interrupt to trigger a switching of the
	 * transmit buffer.
	 */
	if (reg & 0x80) {
		current_buf = smp->nic.current_buf;
		xmp = current_buf ?
			&(smp->nic.xmit_buf[0]) : &(smp->nic.xmit_buf[1]);
		if (xmp->xbm_status == XMIT_IN_PROGRESS) {
#ifdef SMCEDEBUG
			if (smcedebug) {
				if (current_buf == 1)
					cmn_err(CE_CONT,
					"xmit intr: setting xmit buf 0 to 0\n");
				else
					cmn_err(CE_CONT,
					"xmit intr: setting xmit buf 1 to 0\n");
			}
#endif
			xmp->bufr_bytes_avail = XMIT_BUF_SZ;
			xmp->ttl_packet_count = 0;
			xmp->xbm_status = 0;		/* reset the flag */

#ifdef NOT_SOLARIS
			/*** do not call to improve performance
			smce_um_send_complete(SUCCESS, smp);
			***/
#endif /* NOT_SOLARIS */
#ifdef SMCEDEBUG
			if (smcedebug & 0x01)
				cmn_err(CE_CONT, "0");
#endif
		}

		/*
		 * Check the other buffer for XMIT_READY.  If the buffer is
		 * being loaded, it will have its status == XMIT_LOADING so
		 * will not be affected.
		 */
		xmp = &(smp->nic.xmit_buf[current_buf]);
		if (xmp->xbm_status == XMIT_READY) {
#ifdef SMCEDEBUG
			if (smcedebug)
				cmn_err(CE_CONT,
					"start xmit on alternate buffer (%d)\n",
					current_buf);
#endif
			xmp->xbm_status = XMIT_IN_PROGRESS;
			smp->nic.current_buf = current_buf ? 0 : 1;
			outb(nice_offset + XMIT_PKT_CONTROL1,
				xmp->ttl_packet_count | 0x80);
#ifdef SMCEDEBUG
			if (smcedebug & 0x03)
				cmn_err(CE_CONT, "t%d", xmp->ttl_packet_count);
#endif
		}
	}

check_recv:;
	/*
	 * ****** check for packet received in nice ********
	 */
	reg = inb(nice_offset + RECV_STATUS);
	if (reg)
		claimed++;

	if (reg & RECV_OVERFLOW) {
#ifdef SMCEDEBUG
		if (smcedebug)
			cmn_err(CE_CONT, "receive overflow\n");
#endif
		outb(nice_offset + RECV_STATUS, RECV_OVERFLOW);
	}

	if (reg & RECV_SHORT_PACKET) {
#ifdef SMCEDEBUG
		if (smcedebug)
			cmn_err(CE_CONT, "short on %x...", smp->sm_nice_addr);
#endif
		outb(nice_offset + RECV_STATUS, RECV_SHORT_PACKET);
	}

	if (reg & RECV_BUS_ERROR) {
#ifdef SMCEDEBUG
		if (smcedebug)
			cmn_err(CE_CONT, "recv_bus error\n");
#endif
		outb(nice_offset + RECV_STATUS, RECV_BUS_ERROR);
	}

	/* these conditions should be check and cleared */
	if (reg & 0x10) {
#ifdef SMCEDEBUG
		if (smcedebug)
			cmn_err(CE_CONT, "rmt ");
#endif
		outb(nice_offset + RECV_STATUS, 0x10);
	}

	if (reg & 0x04) {
#ifdef SMCEDEBUG
		if (smcedebug)
			cmn_err(CE_CONT, "algn ");
#endif
		outb(nice_offset + RECV_STATUS, 0x04);
	}

	if (reg & 0x02) {
#ifdef SMCEDEBUG
		if (smcedebug)
			cmn_err(CE_CONT, "crc ");
#endif
		outb(nice_offset + RECV_STATUS, 0x02);
	}

	if (reg & 0x20) {
#ifdef SMCEDEBUG
		if (smcedebug)
			cmn_err(CE_CONT, "DMA ");
#endif
		outb(nice_offset + DMA_ENABLE, 0);
	}
	/* end */

	/* check for receive buffer empty bit */
	if (reg & 0x80 || ((inb(nice_offset + RECV_MODE) & 0x40) == 0)) {
		outb(nice_offset + RECV_STATUS, 0x80);
		/*
		 * setup mode register to indicate which nice and set
		 * direction bit to BOARD_TO_UNIX
		 */
		while ((inb(nice_offset + RECV_MODE) & 0x40) == 0) {
			/*
			 * Read it twice to get the length, because the
			 * first 2 bytes are the status of the packet and
			 * an unused byte.  Since we did not allow the
			 * reception of bad packets, the status byte
			 * can be ignored (will be good anyway).
			 */
			length = inw(nice_offset + BMPORT_LSB);
			length = inw(nice_offset + BMPORT_LSB);
			smce_um_receive_packet(length, smp);

		} /* end while */
	} /* end if received a frame */

	/* unmask board interrupts */
	reg = inb(smp->io_base + MODE_REGISTER_OFFSET);
	reg &= ~0x01;
	outb(smp->io_base + MODE_REGISTER_OFFSET, reg);

	return(claimed);
}

int
smce_lm_receive_copy(length, offset, dbuf, smp)
	int             length;
	long            offset;
	struct DataBuffStructure *dbuf;
	struct smparam *smp;
{
	int             carry_over = 0;
	unsigned short  carry_word;

	if (dbuf->fragment_list[0].fragment_length & 0x01) {
		carry_over = 1;
		dbuf->fragment_list[0].fragment_length--;
	} else {
		carry_over = 0;
	}
	xfer(smp, dbuf->fragment_list[0].fragment_ptr,
	     dbuf->fragment_list[0].fragment_length, BOARD_TO_UNIX);
	dbuf->fragment_list[0].fragment_ptr +=
				dbuf->fragment_list[0].fragment_length;
	/*
	 * In Solaris, the UMAC is always sending down 1 fragment so we are
	 * cutting corners here ...  It should really be a loop processing
	 * all the fragments and checking for carryover bytes from one
	 * fragment to another.
	 */
	if (carry_over) {
		carry_word = inw(smp->io_base + smp->sm_nice_addr + BMPORT_LSB);
		dbuf->fragment_list[0].fragment_ptr[0] =
				(unsigned char) (carry_word & 0x00ff);
	}
}

/* mflag was added for multicast addressing */
smce_lm_initialize_adapter(smp, mflag)
	struct smparam *smp;
	int             mflag;
{
	struct smparam *tsmp;
	unsigned char   elr;

	if (mflag == 0) {
		/* this board has not been initialized before */
		/*
		 * smp will always be the pointer to first of the
		 * adapter structure pair
		 */
		tsmp = smp;
		tsmp++;

		smp->sm_nice_addr = NICE_1_OFFSET;
		smp->bus_type = EISA_BUS_TYPE;
		smp->rom_size = 0x400;	/* this might need to be in config.h */
		smp->max_packet_size = SMMAXPKT;
		smp->num_of_tx_buffs = 2;
		smp->receive_mask = ACCEPT_BROADCAST;
		smp->adapter_status = INITIALIZED;
		smp->bic_type = BIC_NO_CHIP;
		smp->nic_type = 0;
		smp->adapter_type = BUS_EISA32M_TYPE;
		smp->sm_cp->sm_init = 0;/* board initialized */

		tsmp->sm_nice_addr = NICE_2_OFFSET;
		tsmp->rom_size = 0x400;	/* this might need to be in config.h */
		tsmp->max_packet_size = SMMAXPKT;
		tsmp->num_of_tx_buffs = 2;
		tsmp->receive_mask = ACCEPT_BROADCAST;
		tsmp->adapter_status = INITIALIZED;
		tsmp->bic_type = BIC_NO_CHIP;
		tsmp->nic_type = 0;
		tsmp->adapter_type = BUS_EISA32M_TYPE;
	}

	/* now setup the stuff on the board */
#ifdef SMCEDEBUG
	if (smcedebug)
		print_msg("before adap_reset\n");
#endif

	sm_adap_reset(smp, mflag);

#ifdef SMCEDEBUG
	if (smcedebug)
		print_msg("after adap_reset\n");
#endif
}

/*
 * lm_send is called when a packet is ready to be transmitted. A pointer to a
 * M_PROTO or M_PCPROTO message that contains the packet is passed to this
 * routine as a parameter. The complete LLC header is contained in the
 * message block's control information block, and the remainder of the packet
 * is contained within the M_DATA message blocks linked to the main message
 * block.
 */
int
smce_lm_send(mb, smp)
	struct DataBuffStructure *mb;	/* ptr to block containing data */
	struct smparam *smp;
{
	unsigned int    length;	/* total length of packet */
	unsigned char reg;
	unsigned long   nice_offset;
	unsigned char   carry_byte;
	int             carry_over;
	int             x;
	int             frag_count;
	int             pad = 0;
	int             pad_length;
	unsigned short  carry_word;
	int		current_buf;
	struct xmit_buffer_monitor *xmp, *xmp2;

	for (x = 0, length = 0; x < mb->fragment_count; x++)
		length += mb->fragment_list[x].fragment_length;

	current_buf = smp->nic.current_buf;
	xmp = &(smp->nic.xmit_buf[current_buf]);
	nice_offset = smp->io_base + smp->sm_nice_addr;

#ifdef SMCEDEBUG
	if (smcedebug)
		cmn_err(CE_CONT, "current buf %d, length=%d\n",
			current_buf, length);
#endif

	/* check length field for proper value; pad if needed */
	if (length < SMMINSEND) {
#ifdef SMCEDEBUG
		if (smcedebug > 30)
			cmn_err(CE_CONT, "padding::");
#endif
		pad = 1;
		pad_length = SMMINSEND - length;	/* smcedebug added */
		length = SMMINSEND;
	} else
		pad = 0;

	/*
	 * Check to see if the current buffer has enough room for this
	 * packet.  If so, queue it up.  If not, throw this packet away.
	 * We cannot just queue it up in the other buffer because this
	 * will keep the hardware and the variables out of sync without
	 * more variables to keep the various state information.
	 * Note that each packet must be preceded by 2 bytes of length
	 * so must include these 2 bytes when checking for available space.
	 * At the end of queuing, we check if the other transmit buffer
	 * has a xbm_status == 0.  If so, we can transmit.  If not, set
	 * xbm_status = XMIT_READY and wait for next turn.
	 */
	if ((length+2) > xmp->bufr_bytes_avail) {
#ifdef SMCEDEBUG
		if (smcedebug)
			cmn_err(CE_CONT, "not enough room in xmit buffer::");
#endif
		return (1);
	}

	if (xmp->ttl_packet_count >= 127) {
#ifdef SMCEDEBUG
		if (smcedebug)
			cmn_err(CE_CONT, "f");
#endif
		return (1);
	}

	/*
	 * Changing the status to XMIT_LOADING will prevent the
	 * switching of the bank by lm_service_events() while we
	 * are in the middle of loading this packet.  Remember we
	 * are running multi-threading ...
	 */
	reg = inb(smp->io_base + MODE_REGISTER_OFFSET);
	reg |= MR_MASK_IRQ;
	outb(smp->io_base + MODE_REGISTER_OFFSET, reg);

#ifdef SMCEDEBUG
	if (smcedebug)
		cmn_err(CE_CONT, "loading to buf %d\n", current_buf);
#endif

	/* keep track of the total number of packets in the xmit buffer */
	xmp->xbm_status = XMIT_LOADING;
	xmp->bufr_bytes_avail -= (length+2);
	xmp->ttl_packet_count++;

	/* write out the length to the nice */
#ifdef SMCEDEBUG
	if (smcedebug)
		cmn_err(CE_CONT, "smsend: pkt_length is %d::", length);
#endif
	outw(nice_offset + BMPORT_LSB, length);

	/* first, transfer the packet header to the board */
	length = mb->fragment_list[0].fragment_length;
#ifdef SMCEDEBUG
	if (smcedebug > 30)
		cmn_err(CE_CONT, "header: length is %d::::", length);
#endif

	if (length & 0x01) {
		carry_over = 1;
#ifdef SMCEDEBUG
		if (smcedebug > 10)
			cmn_err(CE_CONT, "header: carry over situation::\n");
#endif

		carry_byte = mb->fragment_list[0].fragment_ptr[length - 1];

#ifdef SMCEDEBUG
		if (smcedebug > 5)
			cmn_err(CE_CONT, "carry_byte is %x\n", carry_byte);
#endif

		length--;
	} else {

#ifdef SMCEDEBUG
		if (smcedebug > 10)
			cmn_err(CE_CONT, "header: no carryover\n");
#endif

		carry_over = 0;
	}

#ifdef SMCEDEBUG
	if (smcedebug > 10) {
		cmn_err(CE_CONT, "\nsmsend: the first frag:  ");
		for (x = 0; x < 14; x++)
			print_number(mb->fragment_list[0].fragment_ptr[x]);
		cmn_err(CE_CONT, "\n");
	}
#endif

	xfer(smp, mb->fragment_list[0].fragment_ptr, length, UNIX_TO_BOARD);

	/*
	 * load the rest of the packet onto the board by chaining through the
	 * M_DATA blocks attached to the M_PROTO header. The list of data
	 * messages ends when the pointer to the current message block is
	 * NULL
	 */
	for (frag_count = 1; frag_count < (mb->fragment_count); frag_count++) {
		if (carry_over) {

			carry_word = *(mb->fragment_list[frag_count].
								fragment_ptr);
			carry_word = carry_word << 8;
			carry_word |= carry_byte;
			mb->fragment_list[frag_count].fragment_ptr++;
			mb->fragment_list[frag_count].fragment_length--;

#ifdef SMCEDEBUG
			if (smcedebug > 10)
				cmn_err(CE_CONT,
					"smsend: carrying over %x", carry_word);
#endif
			outw(nice_offset + BMPORT_LSB, carry_word);
		}
		length = mb->fragment_list[frag_count].fragment_length;

		if (length & 0x01) {
			carry_over = 1;
			carry_byte = *(mb->fragment_list[frag_count].
							fragment_ptr+length-1);
			length--;
		} else {
			carry_over = 0;
		}

		xfer(smp, mb->fragment_list[frag_count].fragment_ptr,
		     length, UNIX_TO_BOARD);
	}

	/* if there is a solitary byte left over, write it out */
	if (carry_over) {
		carry_word = carry_byte;
		outw(nice_offset + BMPORT_LSB, carry_word);
		if (pad)
			pad_length--;
	}

	if (pad) {
		/*
		 * Since the pad_length will always be <= SMMINSEND <=
		 * SLAVE_CUTOFF, no point in calling xfer()
		 */
		if (pad_length & 0x01) {
			pad_length = pad_length >> 1;
			pad_length++;
		} else
			pad_length = pad_length >> 1;
		repoutsw(nice_offset + BMPORT_LSB, pad_buf, pad_length);
	}

	/*
	 * Packet loaded.  We can start transmitting this loaded packet if
	 * the other buffer's status is 0.  Otherwise, it must be
	 * XMIT_IN_PROGRESS, then we must wait.
	 */
	xmp2 = current_buf ? &(smp->nic.xmit_buf[0]) : &(smp->nic.xmit_buf[1]);
	if (xmp2->xbm_status == 0) {

#ifdef SMCEDEBUG
		if (smcedebug)
			cmn_err(CE_CONT,
				"xmt %d packets::", xmp->ttl_packet_count);
#endif
		smp->nic.current_buf = current_buf ? 0 : 1;
		xmp->xbm_status = XMIT_IN_PROGRESS;

		/* tell the hardware to start transmit */
		outb(nice_offset + XMIT_PKT_CONTROL1,
						xmp->ttl_packet_count|0x80);

#ifdef SMCEDEBUG
		if (smcedebug)
			cmn_err(CE_CONT, "start xmit on buf %d\n", current_buf);
		if (smcedebug & 0x01)
			cmn_err(CE_CONT, "T");
#endif

	} else {

#ifdef SMCEDEBUG
		if (smcedebug)
			cmn_err(CE_CONT,
			    "buf %d ready to xmit, status of other buf is %d\n",
			    current_buf, xmp2->xbm_status);
#endif

		xmp->xbm_status = XMIT_READY;

#ifdef SMCEDEBUG
		if (smcedebug & 0x01)
			cmn_err(CE_CONT, "R");
#endif
	}

	/* enable board interrupts */
	reg = inb(smp->io_base + MODE_REGISTER_OFFSET);
	reg &= ~MR_MASK_IRQ;
	outb(smp->io_base + MODE_REGISTER_OFFSET, reg);

#ifdef SMCEDEBUG
	if (smcedebug)
		cmn_err(CE_CONT, "smsent %x\n", smp->sm_nice_addr);
#endif

	return (0);
}

/* this function enables receive interrupts and board interrupts */
int
smce_lm_open_adapter(smp)
	struct smparam *smp;
{
	unsigned char   reg;

	smp->adapter_status |= OPEN;
	smp->sm_enabled = 1;

	/* unmask interrupts on NICE */
	outb(smp->io_base + smp->sm_nice_addr + XMIT_MASK, XMIT_MASK_VALUE);
	outb(smp->io_base + smp->sm_nice_addr + RECV_MASK, RECV_MASK_VALUE);

	return (SUCCESS);
}

int
smce_lm_close_adapter(smp)
	struct smparam *smp;
{
	unsigned char   reg;

	/* disable NICE interrupts */
	outb(smp->io_base + smp->sm_nice_addr + XMIT_MASK, 0);
	outb(smp->io_base + smp->sm_nice_addr + RECV_MASK, 0);

	smp->adapter_status = CLOSED;
	smp->sm_enabled = 0;
	return (SUCCESS);
}

int
smce_lm_disable_adapter(smp)
	struct smparam *smp;
{
	unsigned char   reg;

	/* disable board interrupts */
	reg = inb(smp->io_base + MODE_REGISTER_OFFSET);
	reg |= MR_MASK_IRQ;
	outb(smp->io_base + MODE_REGISTER_OFFSET, reg);

	return (SUCCESS);
}

int
smce_lm_enable_adapter(smp)
	struct smparam *smp;
{
	unsigned char   reg;

	/* enable board interrupts */
	reg = inb(smp->io_base + MODE_REGISTER_OFFSET);
	reg &= ~MR_MASK_IRQ;
	outb(smp->io_base + MODE_REGISTER_OFFSET, reg);

	return (SUCCESS);
}

int
smce_lm_set_receive_mask(smp)
	struct smparam *smp;
{
	unsigned char   reg;

	if (smp->receive_mask & PROMISCUOUS_MODE) {
		reg = inb(smp->io_base + smp->sm_nice_addr + RECV_MODE);
		reg |= 0x03;	/* set promiscuous address mode */
		outb(smp->io_base + smp->sm_nice_addr + RECV_MODE, reg);
		return;
	}
	if (smp->receive_mask & ACCEPT_MULTICAST) {
		reg = inb(smp->io_base + smp->sm_nice_addr + RECV_MODE);
		reg |= 0x02;
		outb(smp->io_base + smp->sm_nice_addr + RECV_MODE, reg);
		return;
	}
	if (smp->receive_mask & ACCEPT_BROADCAST) {
		reg = inb(smp->io_base + smp->sm_nice_addr + RECV_MODE);
		reg |= 0x01;
		outb(smp->io_base + smp->sm_nice_addr + RECV_MODE, reg);
		return;
	}
}

#define EVIL_MULTICAST
/*
 * The NICE chip interprets the multicast bits via an undocumented
 * strange ritual involving interpreting the CRC bits backwards.
 * We unlocked this secret language using "reverse" engineering.
 */
#ifdef EVIL_MULTICAST
#define CINATAS(x)\
    ((((int)(x) & 1) << 2) | ((int)(x) & 2) | (((int)(x) & 4) >> 2))
#endif

int
smce_lm_add_multi_address(smp)
	struct smparam *smp;
{
	unsigned char   filterbit, row, col;

	filterbit = gldcrc32(smp->multi_address) & 0x3f;
#ifdef EVIL_MULTICAST
	row = CINATAS(filterbit%8);
	col = CINATAS(filterbit/8);
#else
	row = filterbit / 8;
	col = filterbit % 8;
#endif
	
	/* check if the selected bit is already set */
	smp->totalmultaddr++;
	if (smp->multaddr[filterbit]++ > 1) {
		return;
	}
	init_nice(smp,ADD_MULTICAST);
	smce_lm_open_adapter(smp);
}



int
smce_lm_add2_multi_address(smp)
	struct smparam *smp;
{
	unsigned short  na;
	unsigned char   val, valsave, reg;
	unsigned char   filterbit, row, col;
	int y;

	filterbit = gldcrc32(smp->multi_address) & 0x3f;

#ifdef SMCEDEBUG
	if (smcedebug) {
		int             xx;

		cmn_err(CE_CONT, "add multicast");
		for (xx = 0; xx < 6; xx++)
			cmn_err(CE_CONT, ":%x", smp->multi_address[xx]);
		cmn_err(CE_CONT, "\n");
		cmn_err(CE_CONT, "filerbit = 0x%x\n", filterbit);
	}
#endif

#ifdef EVIL_MULTICAST
	row = CINATAS(filterbit%8);
	col = CINATAS(filterbit/8);
#else
	row = filterbit / 8;
	col = filterbit % 8;
#endif

	na = smp->io_base + smp->sm_nice_addr;

#ifdef SMCEDEBUG
	if (smcedebug)
		cmn_err(CE_CONT, "na = 0x%x\n", na);
#endif
	outb(na + CONTROL2, 0x64);

	/* we now have access to the multicast address registers */
#ifdef SMCEDEBUG
	if (smcedebug)
		cmn_err(CE_CONT, "row is %d and col is %d\n", row, col);
#endif

	val = inb(na + GROUPID0 + row);
	val |= (0x01 << col);
	outb(na + GROUPID0 + row , val); 
#ifdef SMCEDEBUG
	if (smcedebug)
		cmn_err(CE_CONT,
			"value read back in is %x\n", inb(na + GROUPID0 + row));
#endif

	/* put receive mode to multicast mode */
	outb(na + RECV_MODE, 0x02);
}

int
smce_lm_delete_multi_address(smp)
	struct smparam *smp;
{
	unsigned short  na;
	unsigned char   val, valsave, reg;
	unsigned char   filterbit, row, col;

	filterbit = gldcrc32(smp->multi_address) & 0x3f;

#ifdef SMCEDEBUG
	if (smcedebug) {
		int             xx;

		cmn_err(CE_CONT, "delete multicast");
		for (xx = 0; xx < 6; xx++)
			cmn_err(CE_CONT, ":%x", smp->multi_address[xx]);
		cmn_err(CE_CONT, "\n");
		cmn_err(CE_CONT, "filerbit = 0x%x\n", filterbit);
	}
#endif

#ifdef EVIL_MULTICAST
	row = CINATAS(filterbit%8);
	col = CINATAS(filterbit/8);
#else
	row = filterbit / 8;
	col = filterbit % 8;
#endif

	/* check if we have no more multicast address set */
	smp->totalmultaddr--;
	if (smp->multaddr[filterbit]-- > 0) {
		return;
	}

	na = smp->io_base + smp->sm_nice_addr;

#ifdef SMCEDEBUG
	if (smcedebug)
		cmn_err(CE_CONT, "na = 0x%x\n", na);
#endif

	/* disable this nice */
	reg = inb(na + CONTROL1);
	reg |= 0x80;		/* force bit 7 to be 1 */
	outb(na + CONTROL1, reg);

	/* select multicast address registers */
	valsave = inb(na + CONTROL2);
	outb(na + CONTROL2, 0x64);

	/* we now have access to the multicast address registers */
#ifdef SMCEDEBUG
	if (smcedebug)
		cmn_err(CE_CONT, "row is %d and col is %d\n", row, col);
#endif

	val = inb(na + GROUPID0 + row);
	val &= ~(0x01 << col);
	outb(na + GROUPID0 + row, val); /* write hashed addr to register */

#ifdef SMCEDEBUG
	if (smcedebug)
		cmn_err(CE_CONT,
			"value read back in is %x\n", inb(na + GROUPID0 + row));
#endif

	outb(na + CONTROL2, valsave);	/* restore control register */

	/* take receive mode out of multicast mode if needed*/
	if (smp->totalmultaddr)
		outb(na + RECV_MODE, 0x02);
	else
		outb(na + RECV_MODE, 0x01);

	/* re-enable nice */
	reg = inb(na + CONTROL1);
	reg &= 0x7f;		/* force bit 7 to be 0 */
	outb(na + CONTROL1, reg);
}

/*
 * sm_adap_reset this function assumes that an even-bound structure is
 * passed; i.e. we expect this pointer to be the first of the adapter_struc
 * pair... smparam[some even number]...
 */
int
sm_adap_reset(smp, mflag)
	struct smparam *smp;
	int             mflag;
{
	struct smparam *tsmp;
	unsigned char reg;
	int             y, x;

	if (mflag == 0) {
		/* initialize the direction bit in the mode register */
#ifdef SMCEDEBUG
		if (smcedebug) {
			print_msg("adaprst:io_base=");
			print_addr(smp->io_base);
		}
#endif
		/* we need to set the IRQ on the board */
		/* we need to set bits 1-3 in the MODE register */

		reg = inb(smp->io_base + MODE_REGISTER_OFFSET + 1);

		/* this is to make sure we start with a clean adapter */
		reg &= ~(MR_RESET1 | MR_RESET2 | MR_RESET3);

		/* DOUG --- not sure about this */
		reg &= 0xF1;		/* zero bits 1-3 */

#ifdef SMCEDEBUG
		if (smcedebug) {
			print_msg("intvec is ");
			print_number(smp->irq_value);
		}
#endif
		switch (smp->irq_value) {
		case 4:
			reg |= MR_IRQ_SEL1;
			break;
		case 5:
			reg |= MR_IRQ_SEL2;
			break;
		case 7:
			reg |= MR_IRQ_SEL1 | MR_IRQ_SEL2;
			break;
		case 9:
			reg |= MR_IRQ_SEL3;
			break;
		case 10:
			reg |= MR_IRQ_SEL3 | MR_IRQ_SEL1;
			break;
		case 11:
			reg |= MR_IRQ_SEL3 | MR_IRQ_SEL2;
			break;
		case 12:
			reg |= MR_IRQ_SEL3 | MR_IRQ_SEL2 | MR_IRQ_SEL1;
			break;
		}

		/* write out the irq value to the mode register */
		outb(smp->io_base + MODE_REGISTER_OFFSET + 1, reg);

		for (x = 0; x < 50000; x++);	/* timeout loop */
		outb(smp->io_base + MODE_REGISTER_OFFSET, MR_DIRECTION);

		/* take board out of reset */
		reg = inb(smp->io_base + MODE_REGISTER_OFFSET + 1);

#ifdef SMCEDEBUG
		if (smcedebug) {
			print_msg("reg is ");
			print_number(reg);
		}
#endif

		/* select media type bnc or aui */
		reg |= MR_RESET1 | MR_RESET2 | MR_RESET3 | MR_TP;
		if (smp->media_type != MEDIA_BNC)
			reg &= ~MR_TP;

		outb(smp->io_base + MODE_REGISTER_OFFSET + 1, reg);

		/* zero out the BMIC Counter */
		outw(smp->io_base + COUNTER_OFFSET, 0);

		/*
		 * if this is the first time for the board to be initialized,
		 * then read the eeprom address
		 */
		if (!smp->sm_cp->sm_init) {
			smp->sm_cp->sm_init++;

#ifdef SMCEDEBUG
			if (smcedebug)
				print_msg("initializaing card");
#endif

			/* save mode register values, first */
			reg = inb(smp->io_base + MODE_REGISTER_OFFSET);
			get_eeprom_address(smp);
			outb(smp->io_base + MODE_REGISTER_OFFSET, reg);
		}
	}

	/* initialize the xmit buffer for the two NICE controllers */
	if (mflag == 0 || mflag == 1) {
		for (y = 0; y < 2; y++) {
			smp->nic.xmit_buf[y].xbm_status = 0;
			smp->nic.xmit_buf[y].bufr_bytes_avail = XMIT_BUF_SZ;
			smp->nic.xmit_buf[y].ttl_packet_count = 0;
		}
		smp->nic.current_buf = 0;
		smp->nic.first_send = 1;
	}

	/* point to the other logical structure attached to this board */
	if (mflag == 0 || mflag == 2) {
		tsmp = smp;
		tsmp++;
		for (y = 0; y < 2; y++) {
			tsmp->nic.xmit_buf[y].xbm_status = 0;
			tsmp->nic.xmit_buf[y].bufr_bytes_avail = XMIT_BUF_SZ;
			tsmp->nic.xmit_buf[y].ttl_packet_count = 0;
		}
		tsmp->nic.current_buf = 0;
		tsmp->nic.first_send = 1;
	}

	/* the 2nd controller's address is 1 greater than the first's */
	if (mflag == 0) {
		bcopy((char *)smp->node_address, (char *)tsmp->node_address, 6);
		tsmp->node_address[5]++;
	}

	/* initialize the 1st nice controller */
	if (mflag == 0) {
		if (init_nice(smp, mflag)) {
			print_msg("couldn't initialize the nice port#1 \n");
			return (-1);
		}
	}

	/* initialize the 2nd nice controller */
	if (mflag == 0) {
		if (init_nice(tsmp, mflag)) {
			print_msg("couldn't initialize the nice port#2\n");
			return (-1);
		}
	}

	if (mflag == 0) {
		outb(smp->io_base + LOCAL_INDEX_REGISTER_OFFSET,
							GLOBAL_CONFIG);
		outb(smp->io_base + LOCAL_DATA_REGISTER_OFFSET,
							GLOBAL_CONFIG_VALUE);
		outb(smp->io_base + LOCAL_INDEX_REGISTER_OFFSET,
							C0_CONFIG);
		outb(smp->io_base + LOCAL_DATA_REGISTER_OFFSET,
							C0_CONFIG_VALUE);
		outb(smp->io_base + LOCAL_STATUS_REGISTER_OFFSET,
							STATUS_CONTROL_VALUE);
	}

#ifdef SMCEDEBUG
	if (smcedebug)
		print_msg("end of adap_reset\n");
#endif
}

/*
 * init_nice -- initialize a NICE controller
 */
static int
init_nice(smp, mflag)
	struct smparam *smp;
	int             mflag;
{
	unsigned char reg;
	unsigned long   nice_offset = smp->io_base + smp->sm_nice_addr;
	int             x;

#ifdef SMCEDEBUG
	if (smcedebug) {
		print_msg("board addr is ");
		print_addr(smp->io_base);
	}
#endif
		/* tell the mode register which nice we are using */
		reg = inb(smp->io_base + MODE_REGISTER_OFFSET);
		if (smp->sm_nice_addr == NICE_1_OFFSET) {

#ifdef SMCEDEBUG
			if (smcedebug)
				print_msg(" nice 1 selected  ");
#endif

			reg &= ~MR_PORT_SELECT;
		} else {

#ifdef SMCEDEBUG
			if (smcedebug)
				print_msg(" nice 2 selected  ");
#endif

			reg |= MR_PORT_SELECT;
		}
		outb(smp->io_base + MODE_REGISTER_OFFSET, reg);

		/* first set the nice to 8 bit mode */
		reg = inb(smp->io_base + MODE_REGISTER_OFFSET + 1);
		reg &= 0xbf;
		outb(smp->io_base + MODE_REGISTER_OFFSET + 1, reg);

		/* setup the nice controller: sw/bw, low rdy, ext endec */
		outb(nice_offset + CONTROL2, 0x60);

		/* keep out of reset and 16bit mode */
		reg = inb(smp->io_base + MODE_REGISTER_OFFSET + 1);
		reg |= 0xe1;
		outb(smp->io_base + MODE_REGISTER_OFFSET + 1, reg);

#ifdef SMCEDEBUG
		if (smcedebug) {
			print_msg("step3 MODEREG is ");
			print_number(reg);
		}
#endif
		/* set active hi ready */
		outb(nice_offset + CONTROL1, 0x8f);

/* do this only when init card */
	if (mflag == 0){
		outb(nice_offset + NODEID0, 0xa5);
		if (inb(nice_offset + NODEID0) != 0xa5) {
			print_msg("couldn't write to port NICE ");
			print_number(nice_offset);
			print_msg("!!\n");
			outb(nice_offset + CONTROL1, 0x0f);
			/* panic(""); */
			return (-1);
		}
	}

	/*
	 * NOTE:  we can't call delay() here either, since the delay function
	 * will crash the mahine, so just loop for a while
	 */
	for (x = 0; x < 1000000; x++);

	/* initialize the first six regiesters */
	outb(nice_offset + XMIT_STATUS, 0xff);
	outb(nice_offset + RECV_STATUS, 0xff);
	outb(nice_offset + XMIT_MASK, 0);
	outb(nice_offset + RECV_MASK, 0);
	outb(nice_offset + XMIT_MODE, 0x02);

	/* zero out all of the unwanted multicast addresses */
	/* do this if mflag=0 */
	if (mflag == 0) {
		outb(nice_offset + CONTROL2, 0x64);
		for (x = 0; x < 8; x++)
			outb(nice_offset + GROUPID0 + x, 0);
		/* do if mflag == 0 */
		outb(nice_offset + RECV_MODE, 0x01);
		/* set the node id that we read from eeprom */
#ifdef SMCEDEBUG
	if (smcedebug)
		print_msg("loading address from eeprom::");
#endif
		/* do if mflag == 0 */
		outb(nice_offset + CONTROL2, 0x60);
		for (x = 0; x < 6; x++)
			outb(nice_offset + NODEID0 + x, smp->node_address[x]);
		}
	else if (mflag == ADD_MULTICAST) {
	
		smce_lm_add2_multi_address(smp);
	}
	if ((mflag == ADD_MULTICAST)){
		int y;
		for (y = 0; y < 2; y++) {
		smp->nic.xmit_buf[y].xbm_status = 0;
		smp->nic.xmit_buf[y].bufr_bytes_avail = XMIT_BUF_SZ;
		smp->nic.xmit_buf[y].ttl_packet_count = 0;
		}
	smp->nic.current_buf = 0;
	}

	/* set back to active hi, ready */
	outb(nice_offset + CONTROL2, 0x68);	/* was 0x6c */
	outb(nice_offset + XMIT_PKT_CONTROL1, 0);

	/* 100 ns; take nice out of reset; sb/bw */
	outb(nice_offset + CONTROL1, 0x4f);

	/* clear receive status bits */
	reg = inb(nice_offset + RECV_STATUS);
	outb(nice_offset + RECV_STATUS, reg);

#ifdef SMCEDEBUG
	if (smcedebug) {
		print_number(inb(nice_offset + XMIT_STATUS));
		print_number(inb(nice_offset + RECV_STATUS));
		print_number(inb(nice_offset + XMIT_MASK));
		print_number(inb(nice_offset + RECV_MASK));
		print_number(inb(nice_offset + XMIT_MODE));
		print_number(inb(nice_offset + RECV_MODE));
	}
#endif

	smp->nic.first_send = 1;

	/* init bmr 11 */
	outb(nice_offset + XMIT_PKT_CONTROL2, XMIT_CONTROL2_VALUE);
	return (0);
}


/*
 * get_eeprom_address This function reads the burned in ethernet address from
 * the eeprom and saves it in our adapter structure
 */
static int
get_eeprom_address(smp)
	struct smparam *smp;
{
	unsigned char   eeprom_addr, tmp_addr;
	int             x, y;
	unsigned short  wreg;

	void            setup_cs();

	/*
	 * for now we will always read the first NICE controller eeprom, this
	 * will have to change when each NICE has its own unique address
	 */
	for (x = 0; x < 3; x++) {
		/* we'll read in two bytes at a time */
		setup_cs(smp);	/* send pattern to eeprom that clocks and
				 * bring CS high -- */
		write_ee_bit(smp, SER_DATA1);	/* bit 6 = 1..read opcode */
		write_ee_bit(smp, SER_DATA1);
		write_ee_bit(smp, 0);
		write_ee_bit(smp, 0);
		write_ee_bit(smp, 0);

		/*
		 * this algorithm will not work with values greater than
		 * three...I didn't feel like implementing the ROL
		 * instruction in C ; however, this function will never use
		 * values greater than 3, so it will never be a problem
		 */
		eeprom_addr = x << 3;
		for (y = 0; y < 4; y++) {
			tmp_addr = eeprom_addr & SER_DATA1;
			write_ee_bit(smp, tmp_addr);
			eeprom_addr = eeprom_addr << 1;
		}

		/* do a couple of dummy reads... */
		y = inb(smp->io_base + MODE_REGISTER_OFFSET);
		y = inb(smp->io_base + MODE_REGISTER_OFFSET);

		/* now actually read 16 bits of eeprom data */
		wreg = 0;
		for (y = 0; y < 16; y++) {
			wreg = wreg << 1;
			wreg |= read_ee_bit(smp);
		}

		/* now clock the eeprom */
		clock_eeprom(smp);

		/*
		 * now we have part of the address saved in wreg, so now we
		 * save it in our structure
		 */
		(smp->node_address[x * 2 + 1]) = wreg >> 8;
		(smp->node_address[x * 2]) = wreg & 0x00ff;
	}			/* end of for loop */
}

/*
 * smdelayd --- a delay function...normally we would use the kernel delay
 * fucntion, however, at initialization time, delay doesn't work since the
 * timer has not been started
 */
int
smdelayd()
{
	int             x;

	for (x = 0; x < 16; x++) {
		x = x + 5;
		x = x - 4;
		x--;
	}
}

/*
 * clock_eeprom -- clock the eeprom with chip select low
 */
static void
clock_eeprom(smp)
	struct smparam *smp;
{
	int             x;

	for (x = 0; x < 16; x++) {
		outb(smp->io_base + MODE_REGISTER_OFFSET, ROM_CSLO | ROM_CLKLO);
		smdelayd();
		outb(smp->io_base + MODE_REGISTER_OFFSET, ROM_CSLO | ROM_CLKHI);
		smdelayd();
	}
}

/*
 * read_ee_bit -- read a single bit from the eeprom
 */
static int
read_ee_bit(smp)
	struct smparam *smp;
{
	unsigned char   reg;

	outb(smp->io_base + MODE_REGISTER_OFFSET, ROM_CSHI | ROM_CLKLO);
	smdelayd();
	outb(smp->io_base + MODE_REGISTER_OFFSET, ROM_CSHI | ROM_CLKHI);
	smdelayd();

	/* now actually read the bit */
	reg = inb(smp->io_base + MODE_REGISTER_OFFSET);
	reg &= SER_DATA_OUT;

	/* the bit is in position 6, move it to 0 */
	reg = reg >> 6;
	smdelayd();
	return (reg);
}

/*
 * write_ee_bit -- write a single bit to the eeprom..ugh bit 6 of the byte is
 * the bit we're going to write to eeprom...how incredibly tedious
 */
static int
write_ee_bit(smp, data_byte)
	struct smparam *smp;
	unsigned char   data_byte;
{
	outb(smp->io_base + MODE_REGISTER_OFFSET, ROM_CSHI | ROM_CLKLO);
	smdelayd();
	outb(smp->io_base + MODE_REGISTER_OFFSET,
					data_byte | ROM_CSHI | ROM_CLKLO);
	smdelayd();
	outb(smp->io_base + MODE_REGISTER_OFFSET,
					data_byte | ROM_CSHI | ROM_CLKHI);
	smdelayd();
	outb(smp->io_base + MODE_REGISTER_OFFSET,
					data_byte | ROM_CSHI | ROM_CLKLO);
	smdelayd();
}

/*
 * cs_setup -- sets up the eeprom... send pattern to eeprom to bring CS
 * high....I have no idea what this actually does, I just copied it from the
 * DOS driver...
 */
static void
setup_cs(smp)
	struct smparam *smp;
{
	outb(smp->io_base + MODE_REGISTER_OFFSET, ROM_CSLO | ROM_CLKLO);
	smdelayd();
	outb(smp->io_base + MODE_REGISTER_OFFSET, ROM_CSLO | ROM_CLKHI);
	smdelayd();
	outb(smp->io_base + MODE_REGISTER_OFFSET, ROM_CSHI | ROM_CLKHI);
	smdelayd();
	outb(smp->io_base + MODE_REGISTER_OFFSET, ROM_CSHI | ROM_CLKHI);
	smdelayd();
	outb(smp->io_base + MODE_REGISTER_OFFSET, ROM_CSHI | ROM_CLKLO);
	smdelayd();
	outb(smp->io_base + MODE_REGISTER_OFFSET, ROM_CSHI | ROM_CLKHI);
	smdelayd();
	outb(smp->io_base + MODE_REGISTER_OFFSET, ROM_CSHI | ROM_CLKLO);
	smdelayd();
	outb(smp->io_base + MODE_REGISTER_OFFSET, ROM_CSHI | ROM_CLKHI);
	smdelayd();
}

/*
 * xfer copies data to/from the board using either outsw or the fabulous bmic
 */
int
xfer(smp, buf, length, direction)
	struct smparam *smp;
	caddr_t         buf;
	int             length, direction;
{
	unsigned char reg;
	char           *cptr_addr;
	int             x;
	unsigned char   reg1, reg2;
	ddi_dma_handle_t handlep;
	int             len;
#ifdef NOT_SOLARIS
	extern int      delaycount;
	extern int      microdata;
#endif
	paddr_t         phys_addr;

	reg = inb(smp->io_base + MODE_REGISTER_OFFSET);
	reg1 = reg;
	if (smp->sm_nice_addr == NICE_2_OFFSET)
		reg |= MR_PORT_SELECT;
	else
		reg &= ~MR_PORT_SELECT;

#ifdef SMCEDEBUG
	if (smcedebug > 10) {
		if (direction == UNIX_TO_BOARD)
			cmn_err(CE_CONT, "xfer to wire %d\n", length);
		else
			cmn_err(CE_CONT, "xfer from wire %d\n", length);
	}
	if (smcedebug > 10) {
		if (direction == UNIX_TO_BOARD) {
			cmn_err(CE_CONT, "xfer: length is %d::", length);
			for (x = 0; x < 10; x++) {
				print_number(buf[x]);
				print_msg(" ");
			}
			cmn_err(CE_CONT, "\n");
		}
	}
#endif

#ifdef NOT_SOLARIS
	if (1 || direction == UNIX_TO_BOARD || length < SLAVE_XFER_CUTOFF) {
#endif /* NOT_SOLARIS */
		/*
		 * Packet too small for BMIC xfer. Write pkt out one
		 * word at a time
		 */
		reg |= MR_DIRECTION;
		outb(smp->io_base + MODE_REGISTER_OFFSET, reg);

		/* length is always an even number */
		len = length >> 1;

#ifdef PPC_ALIGNMENT
		/*
		 * We have to do these writes as word I/O, because the card
		 * is programmed to be in word I/O mode.  On the x86 machine
		 * this does not matter, because word I/O does not require
		 * word alignment -- word I/O to odd addresses is acceptable.
		 *
		 * On an architecture that requires aligned I/O, the code
		 * will not work as currently written, if a buffer is passed
		 * in with an odd address, because the NICE is programmed in
		 * WORD I/O mode and cannot accept byte I/O instructions.
		 *
		 * If your architecture requires aligned I/O, then you will
		 * have to modify this driver in one of two ways:
		 *  1. Copy any odd-addressed buffers to an aligned address;
		 *  2. Put the NICE into byte I/O mode so that you can issue
		 *        byte I/O instructions.
		 */
		ASSERT(("PPC align needswork", !((long)buf&1)));
#endif

		if (direction == UNIX_TO_BOARD) {
			repoutsw(smp->io_base + smp->sm_nice_addr + BMPORT_LSB,
				 (unsigned short *)buf, len);
		} else {
			repinsw(smp->io_base + smp->sm_nice_addr + BMPORT_LSB,
				(unsigned short *)buf, len);
		}

#ifdef SMCEDEBUG
		if (smcedebug > 10) {
			if (direction == BOARD_TO_UNIX)
				cmn_err(CE_CONT, "xfer: length %d::", length);
		}
#endif
#ifdef NOT_SOLARIS
	/* This BMIC code does not work */
	} else {
		/* setup bmic xfer */
#ifdef SMCEDEBUG
		if (smcedebug > 20)
			cmn_err(CE_CONT, "xfer: bmic:::::");
		if (smcedebug)
			cmn_err(CE_CONT, "xfer size = %d, use BMIC\n", length);
#endif

		/* clear bmic channel */
		outb(smp->io_base + LOCAL_INDEX_REGISTER_OFFSET,
						C0_CONFIG); /* 0x48 */
		outb(smp->io_base + LOCAL_DATA_REGISTER_OFFSET, 0x3c);
		outb(smp->io_base + LOCAL_DATA_REGISTER_OFFSET,
						C0_CONFIG_VALUE); /* 0x38 */

		if (direction == UNIX_TO_BOARD)
			outw(smp->io_base + COUNTER_OFFSET, 0);
		else
			outw(smp->io_base + COUNTER_OFFSET, length >> 1);

		if (direction == BOARD_TO_UNIX)
			reg |= MR_DIRECTION;
		else
			reg &= ~MR_DIRECTION;

		/* clear BMIC Counter register */
		reg |= MR_START_BIT;
		outb(smp->io_base + MODE_REGISTER_OFFSET, reg);
		outb(smp->io_base + LOCAL_INDEX_REGISTER_OFFSET, C0_ADDRESS);

		/* get physical address of buffer */
		{
			dev_info_t     *devinfo;
			uint            flags;
			ddi_dma_win_t   dmawin;
			ddi_dma_seg_t   dmaseg;
			off_t           offset, len;
			ddi_dma_cookie_t dmacookie;
			int             rc;

			/* setup the DMA parameters */
			devinfo = smp->devinfo;
			flags = (direction == UNIX_TO_BOARD) ?
				DDI_DMA_WRITE : DDI_DMA_READ;
			if ((rc = ddi_dma_addr_setup(devinfo, (struct as *)NULL,
					buf, length, flags, DDI_DMA_SLEEP, NULL,
					(ddi_dma_lim_t *)NULL, &handlep))
					    != DDI_DMA_MAPPED) {
#ifdef SMCEDEBUG
				if (smcedebug)
					cmn_err(CE_CONT,
					"cannot map DMA resources, rc = %d\n",
					rc);
#endif
				return 0;
			}
			ddi_dma_nextwin(handlep, (ddi_dma_win_t) NULL, &dmawin);
			ddi_dma_nextseg(dmawin, (ddi_dma_seg_t) NULL, &dmaseg);
			if (ddi_dma_segtocookie(dmaseg, &offset, &len,
				&dmacookie) != DDI_SUCCESS) {
#ifdef SMCEDEBUG
				if (smcedebug)
					cmn_err(CE_CONT,
						"cannot get DMA cookie\n");
#endif
				ddi_dma_free(handlep);
				return 0;
			}
			/*
			 * Get the info in the cookie and program the BMIC.
			 * dmacookie.dmac_address is the 32 bit physical
			 * address dmacookie.dmac_size is the length
			 */
#ifdef SMCEDEBUG
			if (smcedebug) {
				cmn_err(CE_CONT,
					"dmac_address = 0x%x, dmac_size = %d\n",
				dmacookie.dmac_address, dmacookie.dmac_size);
				cmn_err(CE_CONT,
					"passed in DMA length = %d\n", length);
			}
#endif
			phys_addr = dmacookie.dmac_address;
		}

		/*
		 * write out physical address to bmic register one byte at a
		 * time
		 */
		cptr_addr = (char *) &phys_addr;

		for (x = 0; x < sizeof(phys_addr); x++)
			outb(smp->io_base + LOCAL_DATA_REGISTER_OFFSET,
							cptr_addr[x]);

		outb(smp->io_base + LOCAL_INDEX_REGISTER_OFFSET, C0_COUNT);
		outb(smp->io_base + LOCAL_DATA_REGISTER_OFFSET,
							length & 0x00ff);
		outb(smp->io_base + LOCAL_DATA_REGISTER_OFFSET, (length >> 8));

		/* set start bit and direction bit */
		reg = 0x80;
		if (direction == BOARD_TO_UNIX)
			reg |= 0x40;
		outb(smp->io_base + LOCAL_DATA_REGISTER_OFFSET, reg);
		outb(smp->io_base + LOCAL_INDEX_REGISTER_OFFSET, C0_STROBE);
		outb(smp->io_base + LOCAL_DATA_REGISTER_OFFSET, 1);

		outb(smp->io_base + LOCAL_INDEX_REGISTER_OFFSET,
						LOCAL_STATUS_REGISTER_OFFSET);
		while (1) {
			reg2 = inb(smp->io_base + LOCAL_STATUS_REGISTER_OFFSET);
			if (reg2 & 0x20)
				break;
		}

		/*
		 * fix for new board.  First we check to see if the busy bit
		 * is ever set
		 */
		if (!checked) {
			if ((inb(smp->io_base + MODE_REGISTER_OFFSET) & 0x80))
				new_board = 1;
			checked++;
		}

		/*
		 * xfer is finished, and we need to wait for the fifo to
		 * drain
		 */
		if (direction == UNIX_TO_BOARD) {
			if (!new_board) {
#ifdef SCO
				for (x = 0; x < 100 * (microdata); x++);
#else
				for (x = 0; x < 5 * (microdata); x++);
#endif
			} else {
				for (x = 0; x < 5 * (microdata); x++) {
					if (!(inb(smp->io_base +
					    MODE_REGISTER_OFFSET) & 0x80)) {
						break;
					}
				}
			}
		}
		outb(smp->io_base + LOCAL_INDEX_REGISTER_OFFSET, C0_STATUS);

		reg = inb(smp->io_base + MODE_REGISTER_OFFSET);
		reg |= MR_DIRECTION;
		reg &= ~(MR_START_BIT);	/* clear start bit */
		outb(smp->io_base + MODE_REGISTER_OFFSET, reg);
		outb(smp->io_base + LOCAL_INDEX_REGISTER_OFFSET, C0_STATUS);
		outb(smp->io_base + LOCAL_DATA_REGISTER_OFFSET,
							C0_STATUS_VALUE);

		/* clear bmic channel */
		outw(smp->io_base + COUNTER_OFFSET, 0);
		outb(smp->io_base + LOCAL_INDEX_REGISTER_OFFSET, C0_CONFIG);
		outb(smp->io_base + LOCAL_DATA_REGISTER_OFFSET,
							C0_CONFIG_VALUE);
		outb(smp->io_base + MODE_REGISTER_OFFSET, reg1);

#ifdef SMCEDEBUG
		if (smcedebug)
			cmn_err(CE_CONT, "BMIC xfer finished, returning\n");
#endif
		ddi_dma_free(handlep);
	}			/* else bmic xfer */
#endif /* NOT_SOLARIS */
	return 0;
}
