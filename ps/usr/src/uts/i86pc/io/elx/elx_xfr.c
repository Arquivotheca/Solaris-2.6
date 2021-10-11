/*
 * Copyright (c) 1996 by Sun Microsystems, Inc.
 * All Rights Reserved.
 */
#ident "@(#)elx_xfr.c	1.10	96/05/21 SMI"

#include <sys/types.h>
#include <sys/stream.h>
#include <sys/kmem.h>
#include "sys/dlpi.h"
#include "sys/ethernet.h"
#include <sys/kstat.h>

#include <sys/ddi.h>
#include <sys/devops.h>
#include <sys/pci.h>
#if	defined PCI_DDI_EMULATION || COMMON_IO_EMULATION
#include <sys/xpci/sunddi_2.5.h>
#else	/* PCI_DDI_EMULATION */
#include <sys/sunddi.h>
#endif	/* PCI_DDI_EMULATION */

#include <sys/gld.h>
#include <sys/elx.h>
#include <sys/debug.h>

extern	void	elx_discard(gld_mac_info_t *, int);
extern	int	elx_restart(gld_mac_info_t *);
extern	void	elx_poll_cip(elx_t *, int, int, int);

int	elx_xmt_dma_thresh = ELX_DMA_THRESH;
int	elx_rcv_dma_thresh = ELX_DMA_THRESH;

#ifdef ELXDEBUG
extern	int	elxdebug;
#endif

void
elx_pio_send(ddi_acc_handle_t handle, int port, unchar *buf, int len)
{
	register int i;
	ushort window;

	SWTCH_WINDOW(port, 1, window);

	if (len >= sizeof (long)) {
		/* First, send bytes until the buffer's aligned */
		i = (4- ((ulong)buf & 3)) & 3;
		if (i > 0) {
			DDI_REPOUTSB(port + ELX_TX_PIO, buf, i);
			buf += i;
			len -= i;
		}

		/* Next, send as many 4-byte units as possible */
		i = len & ~3;
		if (i > 0) {
			DDI_REPOUTSD(port + ELX_TX_PIO, (ulong *)buf, i>>2);
			buf += i;
			len -= i;
		}
	}

	/* Finally, send any trailing bytes */
	/* Deal with bufs of len < 4 */
	if (len > 0)
		    DDI_REPOUTSB(port + ELX_TX_PIO, buf, len);

	RESTORE_WINDOW(port, window, 1);
}

void
elx_pio_recv(ddi_acc_handle_t handle, int port, unchar *buf, int len)
{
	register int i;
	ushort window;

	SWTCH_WINDOW(port, 1, window);

	if (len >= sizeof (long)) {
		/* First, get bytes until the buffer's aligned */
		i = (4- ((ulong)buf & 3)) & 3;
		if (i > 0) {
			if (i == 2)
				*(ushort *)buf = DDI_INW(port + ELX_RX_PIO);
			else
				DDI_REPINSB(port + ELX_RX_PIO, buf, i);
			buf += i;
			len -= i;
		}

		/* Next, get as many 4-byte units as possible */
		i = len & ~3;
		if (i > 0) {
			DDI_REPINSD(port + ELX_RX_PIO, (ulong *)buf, i>>2);
			buf += i;
			len -= i;
		}
	}

	/* Finally, get any trailing bytes */
	if (len > 0)
		DDI_REPINSB(port + ELX_RX_PIO, buf, len);

	RESTORE_WINDOW(port, window, 1);
}

int
elx_dma_send(gld_mac_info_t *macinfo, int port, mblk_t *mp, int len, int pad)
{
	elx_t *elxp = (elx_t *)macinfo->gldm_private;
	ddi_acc_handle_t handle = elxp->io_handle;
	ushort window;

	ELX_LOG(mp, ELXLOG_BCOPY_FROM);
	bcopy((char *)mp->b_rptr, elxp->elx_dma_xbuf, len);

	/*
	 * Send the message header.
	 */
	SWTCH_WINDOW(port, 1, window);
	DDI_OUTL(port + ELX_TX_PIO, ELTX_REQINTR | len);

	SET_WINDOW(port, 7);
	if (elxp->elx_bus == ELX_EISA) {
		DDI_OUTW(port + ELX_COMMAND, COMMAND(ELC_GLOBAL_RESET, 0xbf));
		elx_poll_cip(elxp, port, ELX_CIP_RETRIES, 1);
	}
	DDI_OUTL(port + ELX_MASTER_ADDR, (ulong)elxp->elx_phy_xbuf);
	DDI_OUTW(port + ELX_MASTER_LEN, (ushort)(len + pad));
	DDI_OUTW(port + ELX_COMMAND, COMMAND(ELC_START_DMA, ELX_DMA_WRITE));
	RESTORE_WINDOW(port, window, 7);
	return (0);
}

int
elx_dma_recv(elx_t *elxp, int port, mblk_t *mp, int len)
{
	ddi_acc_handle_t handle = elxp->io_handle;
	ushort dmalen;

	dmalen = (len + 3) & 0xfffc;
	elxp->elx_rcvbuf = mp;
	elxp->elx_flags |= ELF_DMA_RCVBUF;

	elxp->elx_rcvlen = dmalen;
	if (elxp->elx_bus == ELX_EISA) {
		DDI_OUTW(port + ELX_COMMAND, COMMAND(ELC_GLOBAL_RESET, 0xbf));
		elx_poll_cip(elxp, port, ELX_CIP_RETRIES, 1);
	}
	DDI_OUTL(port + ELX_MASTER_ADDR, (ulong)elxp->elx_phy_rbuf);
	DDI_OUTW(port + ELX_MASTER_LEN, (ushort)dmalen);
	DDI_OUTW(port + ELX_COMMAND, COMMAND(ELC_START_DMA, ELX_DMA_READ));
	return (0);
}

/*
 * Process a DMA interrupt.
 */
void
elx_dma_intr(gld_mac_info_t *macinfo, elx_t *elxp, int port)
{
	ddi_acc_handle_t handle = elxp->io_handle;
	int err = 0;
	ushort window;
	ushort dma_status;
	ushort len;
	mblk_t *mp;

	SWTCH_WINDOW(port, 7, window);
	dma_status = DDI_INW(port + ELX_MASTER_STAT);

	if (dma_status & ELMS_TARG_DISC) {
#if defined(ELXDEBUG)
		if (elxdebug & ELXINT) {
			ushort status = DDI_INW(port + ELX_STATUS);
			ushort netdiag, fifodiag, rxsts;
			unsigned char txsts;
			SET_WINDOW(port, 4);
			netdiag = DDI_INW(port + ELX_NET_DIAGNOSTIC);
			fifodiag = DDI_INW(port + ELX_FIFO_DIAGNOSTIC);
			SET_WINDOW(port, 7);
			txsts = DDI_INB(port + ELX_TX_STATUS);
			rxsts = DDI_INW(port + ELX_RX_STATUS);
			cmn_err(CE_CONT, "elxdmaintr: target disconnect:"
				" int:%x dma:%x nd:%x fd:%x ts:%x rs:%x\n",
				status, dma_status, netdiag, fifodiag,
				txsts, rxsts);
		}
#endif
		DDI_OUTW(port + ELX_MASTER_STAT, ELMS_TARG_DISC);
	}
	if (dma_status & ELMS_TARG_RETRY) {
#ifdef ELXDEBUG
		if (elxdebug & ELXINT)
			cmn_err(CE_WARN, "elx_dma_intr: target retry");
#endif
		DDI_OUTW(port + ELX_MASTER_STAT, ELMS_TARG_RETRY);
	}
	if (dma_status & ELMS_TARG_ABORT) {
		cmn_err(CE_WARN, "elx_dma_intr: target abort");
		elx_discard(macinfo, port);
		(void) elx_restart(macinfo);
		err = 1;
	}
	if (dma_status & ELMS_MAST_ABORT) {
		cmn_err(CE_WARN, "elx_dma_intr: master abort: %x",
			dma_status);
		DDI_OUTW(port + ELX_MASTER_STAT, ELMS_MAST_ABORT);
		elx_discard(macinfo, port);
		(void) elx_restart(macinfo);
		err = 1;
	}
	if (err) {
		RESTORE_WINDOW(port, window, 7);
		return;
	}
	if (dma_status & ELMS_SEND) {
		/* transmit dma completed */
		if (elxp->elx_flags & ELF_DMA_SEND)
			/* mark dma available */
			elxp->elx_flags &= ~ELF_DMA_SEND;
		else
			cmn_err(CE_WARN, "elx_dma_intr:"
					"send dma not in progress");
		DDI_OUTW(port + ELX_MASTER_STAT, (dma_status | ELMS_SEND));
		if (elxp->elx_xmtbuf)
			freemsg(elxp->elx_xmtbuf);

	} else if (dma_status & ELMS_RECV) {
		/* receive dma completed */
		if ((elxp->elx_flags & ELF_DMA_RECV) == 0) {
			cmn_err(CE_WARN, "elx_dma_intr:"
					"receive dma not in progress");
		}
		/* mark dma available */
		elxp->elx_flags &= ~ELF_DMA_RECV;
		mp = elxp->elx_rcvbuf;
		if (elxp->elx_flags & ELF_DMA_RCVBUF) {
			ELX_LOG(mp, ELXLOG_DMA_IN_DONE);
			elxp->elx_flags &= ~ELF_DMA_RCVBUF;
			len = elxp->elx_rcvlen - DDI_INW(port + ELX_MASTER_LEN);
			if ((len != 0) &&
			    ((mp->b_wptr + len) < mp->b_datap->db_lim)) {
				bcopy((char *)elxp->elx_dma_rbuf,
					(char *)mp->b_wptr, len);
				elxp->elx_rcvbuf = NULL;
				mp->b_wptr += len;
				ELX_LOG(mp, ELXLOG_SEND_UP);
				gld_recv(macinfo, mp);
				elxp->elx_rcvlen = 0;
			}
		}
		elx_discard(macinfo, port);
		DDI_OUTW(port + ELX_MASTER_STAT, (dma_status | ELMS_RECV));
	}
	RESTORE_WINDOW(port, window, 7);
}


/*
 *  Load the packet onto the board by chaining through the M_DATA
 *  blocks attached to the M_PROTO header.  The list of data messages
 *  ends when the pointer to the current message block is NULL.
 *
 *  Note that if the mblock is going to have to stay around, it
 *  must be dupmsg() since the caller is going to freemsg() the
 *  message.
 */
int
elx_send_msg(gld_mac_info_t *macinfo, int port, mblk_t *mp)
{
	uint len;
	ushort window;
	unchar pad;
	mblk_t *m;
	static unchar padding[4];
	elx_t *elxp = (elx_t *)macinfo->gldm_private;
	ddi_acc_handle_t handle = elxp->io_handle;
	int can_dma = ELX_CAN_DMA(elxp);

	len = 0;
	for (m = mp; m != NULL; m = m->b_cont) {
		uint l;

		l = m->b_wptr - m->b_rptr;
		len += l;
	}

	pad = (4 - (len & 0x3)) & 3;

	if (mp->b_cont)
		goto pio_send;

	/* dma decision threshold should be optimized */
	if (can_dma && len > elx_xmt_dma_thresh) {
		ushort dma_status;

		ELX_LOG(mp, ELXLOG_DMA_SEND);
		/* prevent a simultaneous dma receive attempt */

		SWTCH_WINDOW(port, 7, window);
		dma_status = DDI_INW(port + ELX_MASTER_STAT);

		/* check that dma isn't already in progress */
		if ((dma_status & ELMS_MIP) == 0) {
			if (elxp->elx_flags & ELF_DMA_XFR) {
#if defined(ELXDEBUG)
				ushort istatus = DDI_INW(port + ELX_STATUS);
				ushort mlen = DDI_INW(port + ELX_MASTER_LEN);

				cmn_err(CE_WARN, "elx_send_msg: "
					"dma in progress: "
					"flg=%x dma=%x int=%x len=%x",
					elxp->elx_flags, dma_status,
					istatus, mlen);
#endif
				RESTORE_WINDOW(port, window, 7);
				macinfo->gldm_stats.glds_defer++;
				return (1);
			}
			elxp->elx_flags |= ELF_DMA_SEND;
			(void) elx_dma_send(macinfo, port, mp, len, pad);
			RESTORE_WINDOW(port, window, 7);
			return (0);

		} else if (elxp->elx_flags & ELF_DMA_SEND) {
			/*
			 * if send dma is already in progress can't do
			 * pio writes into the Tx FIFO at the same time
			 * so try again later.
			 */
			RESTORE_WINDOW(port, window, 7);
			macinfo->gldm_stats.glds_defer++;
			return (1);
		}
		/* ok to do pio, fall through to code below */
	}

pio_send:

	SWTCH_WINDOW(port, 1, window);

	if (DDI_INW(port + ELX_FREE_TX_BYTES) < (len + pad + 4)) {
		RESTORE_WINDOW(port, window, 1);
		macinfo->gldm_stats.glds_defer++;
		return (1);
	}

	/*
	 * Send the message header.
	 */
	DDI_OUTW(port + ELX_TX_PIO, ELTX_REQINTR | len);
	DDI_OUTW(port + ELX_TX_PIO, 0);

	for (; mp != NULL; mp = mp->b_cont) {
		caddr_t addr;

		addr = (caddr_t)mp->b_rptr;
		len = mp->b_wptr - mp->b_rptr;

		elx_pio_send(handle, port, (unchar *)addr, len);
	}

	/*
	 * The last mblk has been sent; see if we need to pad
	 * the data in order to complete the send.
	 */
	if (pad)
		DDI_REPOUTSB(port + ELX_TX_PIO, padding, pad);

	RESTORE_WINDOW(port, window, 1);

	return (0);
}


int
elx_recv_msg(elx_t *elxp, int port, mblk_t *mp, int len)
{
	ddi_acc_handle_t handle = elxp->io_handle;
	ushort window;

	/* dma decision threshold should be optimized */
	if (ELX_CAN_DMA(elxp) && len > elx_rcv_dma_thresh) {
		ushort dma_status;

		/* prevent a simultaneous dma transmit attempt */

		SWTCH_WINDOW(port, 7, window);
		dma_status = DDI_INW(port + ELX_MASTER_STAT);

		/* can't do dma if it's already in progress */
		if ((dma_status & ELMS_MIP) == 0) {
			ELX_LOG(mp, ELXLOG_DMA_IN);
			if (elxp->elx_flags & ELF_DMA_XFR) {
#if defined(ELXDEBUG)
				ushort istatus = DDI_INW(port + ELX_STATUS);
				ushort mlen = DDI_INW(port + ELX_MASTER_LEN);
				cmn_err(CE_WARN, "elx_recv_msg: "
					"dma in progress: "
					"flg=%x dma=%x int=%x len=%x",
					elxp->elx_flags, dma_status,
					istatus, mlen);
#endif
				RESTORE_WINDOW(port, window, 7);
				return (1);
			}
			elxp->elx_flags |= ELF_DMA_RECV;
			(void) elx_dma_recv(elxp, port, mp, len);
			RESTORE_WINDOW(port, window, 7);
			return (0);

		}
		/* have to do pio, fall through to code below */
	}

	ELX_LOG(mp, ELXLOG_PIO_IN);
	elx_pio_recv(handle, port, mp->b_wptr, len);
	mp->b_wptr += len;

	return (0);
}
