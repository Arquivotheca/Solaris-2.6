/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF		*/
/*	UNIX System Laboratories, Inc.				*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * Copyright (c) 1992 by Sun Microsystems, Inc.  All Rights Reserved.
 */

#pragma ident	"@(#)kdkb.c	1.21	96/06/02 SMI"

#include <sys/types.h>
#include <sys/inline.h>
#include <sys/termios.h>
#include <sys/stream.h>
#include <sys/strtty.h>
#include <sys/kd.h>
#include <sys/ws/8042.h>
#include <sys/cmn_err.h>
#include <sys/kmem.h>
#include <sys/conf.h>
#include <sys/promif.h>
#include <sys/archsystm.h>
#include <sys/ddi.h>
#include <sys/sunddi.h>


struct kd_tone
{
	queue_t		*t_qp;
	mblk_t		*t_mp;
	struct iocblk	*t_iocp;
	int		t_arg;
	struct kd_tone	*t_next;
};


/*
 * this isn't right, but KD_DEBUG is used for something else in this
 * file.
 */
#if	defined(DEBUG)
extern int kd_low_level_debug;
extern void kd_trace(int, int);
#define	KD_TRACE_RESEND			0x10
#define	KD_TRACE_RESPONSE_UNKNOWN	0x11
#endif

extern wstation_t	Kdws;
extern kbstate_t	Kd0state;

struct kd_tone	*tone_queue = NULL;

/*
 * Initialize the keykoard..
 */

void
kdkb_init(wstation_t *wsp)
{
	kbstate_t	*kbp;
	unchar	cb;

	mutex_enter(&Kdws.w_hw_mutex);
	kbp = &Kd0state;
	if (inb(KB_STAT) & KBC_STAT_OUTBF) /* clear output buffer */
		(void) inb(KB_OUT);
	kdkb_type(wsp);		/* find out keyboard type */
	SEND2KBD(KB_ICMD, KBC_RCB);
	while (!(inb(KB_STAT) & KBC_STAT_OUTBF))
		;
	cb = inb(KB_OUT); /* get command byte */
	/* clear disable keyboard & translate flags */
	cb &= ~(KBC_CMD_KB_DIS|KBC_CMD_XLATE);
	cb |= KBC_CMD_EOBFI; /* set interrupt on output buffer full flag */
	SEND2KBD(KB_ICMD, KBC_WCB);
	SEND2KBD(KB_IDAT, cb);
	kbp->kb_old_scan = 0;
	kbp->kb_debugger_entered_through_kd = 0;
	kbp->kb_state = 0;
	kbp->kb_sa_state = kbp->kb_state;
	(void) i8042_aux_port(); /* ign ret val -- this is for init */
	mutex_exit(&Kdws.w_hw_mutex);
}

/*
 * Establish keyboard type.
 * Kdws.w_hw_mutex is already held.
 */

void
kdkb_type(wstation_t *wsp)
{
	int	cnt;
	unchar	byt;

	wsp->w_kbtype = KB_OTHER;
	SEND2KBD(KB_IDAT, KB_READID);
	while (!(inb(KB_STAT) & KBC_STAT_OUTBF))
		/* LOOP */;		/* wait for ACK byte */
	(void) inb(KB_OUT);		/* and discard it */
	/* wait for up to about a quarter-second for response */
	for (cnt = 0; cnt < 20000 && !(inb(KB_STAT) & KBC_STAT_OUTBF); cnt++)
		tenmicrosec();
	if (!(inb(KB_STAT) & KBC_STAT_OUTBF))
		wsp->w_kbtype = KB_84;	/* no response indicates 84-key */
	else if (inb(KB_OUT) == 0xAB) { /* first byte of 101-key response */
		/* wait for up to about a quarter-second for next byte */
		for (cnt = 0;
		    cnt < 20000 && !(inb(KB_STAT) & KBC_STAT_OUTBF);
		    cnt++)
			tenmicrosec();
		if ((byt = inb(KB_OUT)) == 0x41 || byt == 0x83 || byt == 0x85)
			/* these are apparently all valid 2nd bytes */
			wsp->w_kbtype = KB_101;
	}
	SEND2KBD(KB_ICMD, KBC_KB_ENABLE);
}

int
kdkb_resend(unchar cmd, unchar ack, unchar whence)
{
	int cnt = 10;

#ifdef KD_DEBUG
	cmn_err(CE_NOTE, "kdkb_resend: ack is 0x%x", ack);
#endif
	while (cnt --) {
		if (ack == KB_RESEND) {
#if	defined(DEBUG)
			if (kd_low_level_debug)
				prom_printf(" RESEND ");
			kd_trace(KD_TRACE_RESEND, 0);
#endif
			(void) i8042_send_cmd(cmd, P8042_TO_KBD, &ack,
			    1, whence);
			continue;
		} else if (ack == KB_ACK) {
			return (1);
		} else {
#if	defined(DEBUG)
			if (kd_low_level_debug)
			    prom_printf(" ??? ");
			kd_trace(KD_TRACE_RESPONSE_UNKNOWN, 0);
#endif
			break;
		}
	}
	cnt = 10;
	cmd = KB_ENABLE;
	while (cnt --) {
		(void) i8042_send_cmd(cmd, P8042_TO_KBD, &ack, 1, whence);
		if (ack == KB_ACK) {
			return (0);
		}
	}
#ifdef KD_DEBUG
	cmn_err(CE_NOTE, "kdkb_resend: did not enable keyboard");
#endif
	cmn_err(CE_WARN,
		"Integral console keyboard not found. If you are using");
	cmn_err(CE_CONT,
		"the integral console, check the keyboard connection.\n");
	return (0);
}

/*
 * Send command to keyboard.
 */

void
kdkb_cmd(register unchar cmd, register unchar whence)
{
	/*
	 * whence is:
	 * FROM_DRIVER or FROM_DEBUGGER
	 */
	register int	rv;
	register unsigned char	ledstat;
	unchar	ack;
	kbstate_t	*kbp;

	kbp = &Kd0state;
	i8042_acquire(whence);
	rv = i8042_send_cmd(cmd, P8042_TO_KBD, &ack, 1, whence);
#if defined(KD_DEBUG) || defined(lint)
	cmn_err(CE_NOTE, "!rv was %x", rv);
#endif
	if (ack != KB_ACK) {
#ifdef KD_DEBUG
		cmn_err(CE_WARN, "kdkb_cmd: unknown cmd %x ack %x", cmd, ack);
#endif
		if (!kdkb_resend(cmd, ack, whence)) {
			i8042_release(whence);
			return;
		}
	}

	switch (cmd) {
	case LED_WARN:	/* send led status next */
		ledstat = 0;
		if (kbp->kb_state & CAPS_LOCK)
			ledstat |= LED_CAP;
		if (kbp->kb_state & NUM_LOCK)
			ledstat |= LED_NUM;
		if (kbp->kb_state & SCROLL_LOCK)
			ledstat |= LED_SCR;
		i8042_send_cmd(ledstat, P8042_TO_KBD, &ack, 1, whence);
		if (ack != KB_ACK) {
#ifdef KD_DEBUG
			cmn_err(CE_WARN, "kdkb_cmd: LED_WARN, no ack from kbd");
#endif
			if (!kdkb_resend(ledstat, ack, whence)) {
				i8042_release(whence);
				return;
			}
		}
		break;
	case KB_ENABLE: /* waiting for keyboard ack */
		i8042_program(P8042_KBDENAB);
		break;
	}
	i8042_release(whence);
}

/*
 * For terminal emulator bell only
 */

void
kdkb_bell()
{
	kdkb_mktone((queue_t *)NULL, (mblk_t *)NULL, (struct iocblk *)NULL,
		(100<<16) + NORMBELL, (caddr_t)NULL);
}

/*
 *
 */

void
kdkb_sound(int freq)
{
	unchar	regval;

	if (freq) {	/* turn sound on? */
		outb(TIMERCR, T_CTLWORD);
		outb(TIMER2, (freq & 0xff));
		outb(TIMER2, ((freq >> 8) & 0xff));
		regval = (inb(TONE_CTL) | TONE_ON);
	} else
		regval = (inb(TONE_CTL) & ~TONE_ON);
	outb(TONE_CTL, regval);
}

/*
 *
 */

void
kdkb_toneoff(caddr_t addr)
{
	extern struct kd_tone	*tone_queue;

	kdkb_sound(0);

	Kdws.w_flags &= ~WS_TONE;
	if (addr) {
		struct kd_tone	*m;
		m = (struct kd_tone *)addr;
		/*
		 * this is a queued tone. remove it from the queue and ACK
		 * it.
		 */
		if (m->t_qp)
			ws_iocack(m->t_qp, m->t_mp, m->t_iocp);
		/* locate it in the queue and unlink it */
		tone_queue = m->t_next;
		/* de_allocate memory */
		kmem_free(m, sizeof (*m));
	}
	if (tone_queue != NULL) {
		/*
		 * There's more on the queue.  Get the next one and pass
		 * it to kdkb_mktone.
		 */
		kdkb_mktone(tone_queue->t_qp,
				tone_queue->t_mp,
				tone_queue->t_iocp,
				tone_queue->t_arg,
				(caddr_t)tone_queue);
	}
}

/*
 *
 */

int
kdkb_mktone(queue_t *qp, mblk_t *mp, struct iocblk *iocp,
		int arg, caddr_t addr)
{
	extern struct kd_tone	*tone_queue;
	ushort	freq, length;
	int	tval;

	if (Kdws.w_flags & WS_TONE) {
		struct kd_tone	*m;
		/*
		 * store away the tone at the end of the tone queue
		 */
		/* allocate memory */
		m = (struct kd_tone *)kmem_alloc(sizeof (*m), KM_NOSLEEP);
		if (m == (struct kd_tone *)NULL) {
			cmn_err(CE_WARN,
				"No memory for tone overflow - skipping");
			return (0);
		}
		m->t_qp = qp;
		m->t_mp = mp;
		m->t_iocp = iocp;
		m->t_arg = arg;
		m->t_next = NULL;
		/* put it at the end of the queue */
		if (tone_queue == NULL) {
			tone_queue = m;
		} else {
			struct kd_tone	*i;
			for (i = tone_queue; i->t_next != NULL; i = i->t_next)
				;
			i->t_next = m;
		}
		return (-1);
	}
	freq = (ushort)((long)arg & 0xffff);
	length = (ushort)(((long)arg >> 16) & 0xffff);
	if (!freq || !(tval = ((ulong)(length * HZ) / 1000L)))
		return (0);
	Kdws.w_flags |= WS_TONE;
	kdkb_sound(freq);
	timeout((void (*)())kdkb_toneoff, addr, tval);
	return (0);
}



/*
 *
 */

void
kdkb_setled(kbstate_t *kbp, unchar led)
{
	if (led & LED_CAP)
		kbp->kb_state |= CAPS_LOCK;
	else
		kbp->kb_state &= ~CAPS_LOCK;
	if (led & LED_NUM)
		kbp->kb_state |= NUM_LOCK;
	else
		kbp->kb_state &= ~NUM_LOCK;
	if (led & LED_SCR)
		kbp->kb_state |= SCROLL_LOCK;
	else
		kbp->kb_state &= ~SCROLL_LOCK;
	kdkb_cmd(LED_WARN, FROM_DRIVER);
}

/*
 *
 */

void
kdkb_keyclick()
{
	register int	cnt;

	if (Kdws.w_flags & WS_KEYCLICK) {
		kdkb_sound(NORMBELL);
		for (cnt = 0; cnt < 7; cnt++)
			tenmicrosec();
		kdkb_sound(0);
	}
}


/*
 * this routine is called by the kdputchar to force an enable of
 * the keyboard. We do this to avoid the i8042 interface.
 */

void
kdkb_force_enable()
{
	SEND2KBD(KB_ICMD, KBC_KB_ENABLE);
}
