/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF		*/
/*	UNIX System Laboratories, Inc.				*/
/*	The copyright notice above does not evidence any	*/
/*	actual or intended publication of such source code.	*/

/*
 * Copyright (c) 1992-1996, by Sun Microsystems, Inc.  All Rights Reserved.
 */

#pragma ident	"@(#)kdkb.c	1.16	96/07/30 SMI"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/sysmacros.h>
#include <sys/inline.h>
#include <sys/termios.h>
#include <sys/stream.h>
#include <sys/strtty.h>
#include <sys/stropts.h>
#include <sys/proc.h>
#include <sys/kd.h>
#include <sys/ws/ws.h>
#include <sys/ws/8042.h>
#include <sys/kb.h>
#include <sys/cmn_err.h>
#include <sys/kmem.h>
#include <sys/ws/chan.h>
#include <sys/ws/8042.h>
#include <sys/ws/ws.h>
#include <sys/kb.h>
#include <sys/archsystm.h>
#include <sys/ddi.h>
#include <sys/sunddi.h>

struct kd_tone {
	queue_t		*t_qp;
	mblk_t		*t_mp;
	struct iocblk	*t_iocp;
	int		t_arg;
	struct kd_tone	*t_next;
};


extern channel_t	*ws_activechan();
extern wstation_t	Kdws;
extern channel_t	Kd0chan;

struct kd_tone	*tone_queue = NULL;

/* Keyboard Key-Repeat Rate/Delay Time */
unchar	kdkb_krr_dt = TYPE_VALS;

/* function prototypes */
void	kdkb_init(wstation_t *);
void	kdkb_type(wstation_t *);
int	kdkb_resend(unchar, unchar, unchar);
void	kdkb_cmd(unchar, unchar);
static void	kdkb_cktone(caddr_t);
void	kdkb_tone(void);
void	kdkb_sound(int);
void	kdkb_toneoff(caddr_t);
int	kdkb_mktone(queue_t *, mblk_t *, struct iocblk *, int, caddr_t, int *);
void	kdkb_setled(channel_t *, kbstate_t *, unchar);
int	kdkb_scrl_lock(channel_t *);
int	kdkb_locked(ushort, unchar);
void	kdkb_keyclick(ushort);
void	kdkb_force_enable(void);

/*
 *
 */
void
kdkb_init(wstation_t *wsp)
{
	kbstate_t	*kbp;
	channel_t	*chp;
	unchar	cb;

	chp = ws_activechan(wsp);
	if (chp == (channel_t *)NULL)
		chp = &Kd0chan;

	kbp = &chp->ch_kbstate;
	if (inb(KB_STAT) & KB_OUTBF)	/* clear output buffer */
		(void) inb(KB_OUT);
	kdkb_type(wsp);			/* find out keyboard type */
	SEND2KBD(KB_ICMD, KB_RCB);
	while (!(inb(KB_STAT) & KB_OUTBF))
		;
	cb = inb(KB_OUT);	/* get command byte */
	cb &= ~KB_DISAB;	/* clear disable keyboard flag */
	cb |= KB_EOBFI;		/* set interrupt on output buffer full flag */
	SEND2KBD(KB_ICMD, KB_WCB);
	while (inb(KB_STAT) & KB_INBF)	/* wait for input buffer to clear */
		;
	outb(KB_IDAT, cb);
	kbp->kb_state = 0;
	kbp->kb_rstate = kbp->kb_state;
	kbp->kb_proc_state = B_TRUE;
	(void) i8042_aux_port();	/* ign ret val -- this is for init */
	kdkb_cmd(LED_WARN, FROM_DRIVER);
	kdkb_cmd(TYPE_WARN, FROM_DRIVER);
}

/*
 * call with interrupts off only!! (Since this is called at init time,
 * we cannot use i8042* interface since it spl's.
 */
void
kdkb_type(wstation_t *wsp)
{
	int	cnt;
	unchar	byt;

	wsp->w_kbtype = KB_OTHER;
	SEND2KBD(KB_IDAT, KB_READID);
	while (!(inb(KB_STAT) & KB_OUTBF))
		;		/* wait for ACK byte */
	inb(KB_OUT);		/* and discard it */
	/* wait for up to about a quarter-second for response */
	for (cnt = 0; cnt < 20000 && !(inb(KB_STAT) & KB_OUTBF); cnt++)
		drv_usecwait(10);
	if (!(inb(KB_STAT) & KB_OUTBF))
		wsp->w_kbtype = KB_84;	/* no response indicates 84-key */
	else if (inb(KB_OUT) == 0xAB) { /* first byte of 101-key response */
		/* wait for up to about a quarter-second for next byte */
		for (cnt = 0; cnt < 20000 && !(inb(KB_STAT) & KB_OUTBF); cnt++)
			drv_usecwait(10);
		if ((byt = inb(KB_OUT)) == 0x41 || byt == 0x83 || byt == 0x85)
			/* these are apparently all valid 2nd bytes */
			wsp->w_kbtype = KB_101;
	}
	SEND2KBD(KB_ICMD, KB_ENAB);
}

int
kdkb_resend(unchar cmd, unchar ack, unchar whence)
{
	int	cnt = 10;

#ifdef KD_DEBUG
	cmn_err(CE_NOTE, "kdkb_resend: ack is 0x%x", ack);
#endif
	while (cnt --) {
		if (ack == 0xfe) {
			(void) i8042_send_cmd(cmd, P8042_TO_KBD, &ack, 1, whence);
			continue;
		} else if (ack == KB_ACK) {
			return (1);
		} else
			break;
	}
	cnt = 10;
	cmd = 0xf4;
	while (cnt --) {
		(void) i8042_send_cmd(cmd, P8042_TO_KBD, &ack, 1, whence);
		if (ack == KB_ACK)
			return (0);
	}
#ifdef KD_DEBUG
	cmn_err(CE_NOTE, "kdkb_resend: did not enable keyboard");
#endif
	return (0);
}

/*
 * Send command to keyboard. Assumed to only be called when spl's are valid
 * 'whence' should be either FROM_DRIVER or FROM_DEBUGGER.
 */
void
kdkb_cmd(register unchar cmd, register unchar whence)
{
	register int	rv;
	register unsigned char	ledstat;
	unchar	ack;
	channel_t	*chp;
	kbstate_t	*kbp;

	chp = ws_activechan(&Kdws);
	if (chp == (channel_t *) NULL)
		chp = &Kd0chan;
	kbp = &chp->ch_kbstate;
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
			if (!kdkb_resend(cmd, ack, whence)) {
				i8042_release(whence);
				return;
			}
		}
		break;
	case TYPE_WARN:	/* send typematic */
		i8042_send_cmd(kdkb_krr_dt, P8042_TO_KBD, &ack, 1, whence);
		if (ack != KB_ACK) {
#ifdef KD_DEBUG
			cmn_err(CE_WARN,
			    "kdkb_cmd: TYPE_WARN, no ack from kbd");
#endif
			if (!kdkb_resend(cmd, ack, whence)) {
				i8042_release(whence);
				return;
			}
		}
		break;
	case KB_ENAB: /* waiting for keyboard ack */
		i8042_program(P8042_KBDENAB);
		break;
	default:
		cmn_err(CE_WARN, "kdkb_cmd: illegal kbd command %x", cmd);
		break;
	}
	i8042_release(whence);
}

/*
 *
 */

/*ARGSUSED*/
static void
kdkb_cktone(caddr_t arg)
{
	unchar regval;

	if (Kdws.w_ticks-- > 1)
		timeout(kdkb_cktone, (caddr_t)0, BELLLEN);
	else {	/* turn tone off */
		regval = (inb(TONE_CTL) & ~TONE_ON);
		outb(TONE_CTL, regval);
	}
}

/*
 *
 */
void
kdkb_tone(void)
{
	unchar regval;
	unsigned int 	bfreq;
	int 		btime;
	termstate_t	*tsp;
	channel_t	*chp = ws_activechan(&Kdws);

	if (chp == NULL)
		return;

	tsp = &chp->ch_tstate;
	btime = tsp->t_bell_time;
	bfreq = tsp->t_bell_freq;

	if (!Kdws.w_ticks) {
		Kdws.w_ticks = btime;
		outb(TIMERCR, T_CTLWORD);
		outb(TIMER2, bfreq & 0xFF);
		outb(TIMER2, bfreq >> 8);
		/* turn tone generation on */
		regval = (inb(TONE_CTL) | TONE_ON);
		outb (TONE_CTL, regval);
		/* go away and let tone ring a while */
		timeout (kdkb_cktone, (caddr_t)0, BELLLEN);
	} else {
		Kdws.w_ticks = btime; /* make it ring btime longer */
	}
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
	unchar	regval;

	regval = (inb(TONE_CTL) & ~TONE_ON);	/* turn off the tone */
	outb(TONE_CTL, regval);

	Kdws.w_tone = 0;

	if (addr) {
		struct kd_tone	*m;
		m = (struct kd_tone *)addr;
		/*
		 * This is a queued tone which we have finished playing.
		 * Remove it from the queue.
		 */
		/* locate it in the queue and unlink it */
		tone_queue = m->t_next;

		/* deallocate memory */
		kmem_free(m, sizeof (*m));
	}

	if (tone_queue != NULL) {
		/*
		 * There's more on the queue.  Get the next one and pass
		 * it to kdkb_mktone.
		 */

		/*
		 * Ack it *before* starting it.  This is consistent with what
		 * happens if the speaker isn't busy when the request is first
		 * made, which is that the ioctl returns immediately, while
		 * the speaker plays onward.
		 */
		ws_iocack (tone_queue->t_qp, tone_queue->t_mp,
		    tone_queue->t_iocp);

		/*
		 * Now start playing the tone.
		 */
		(void) kdkb_mktone(tone_queue->t_qp,
				tone_queue->t_mp,
				tone_queue->t_iocp,
				tone_queue->t_arg,
				(caddr_t)tone_queue,
				(int *)0);
	}
}

/*
 *
 */
int
kdkb_mktone(queue_t *qp, mblk_t *mp, struct iocblk *iocp, int arg, caddr_t addr, int *reply)
{
	extern struct kd_tone	*tone_queue;
	ushort	freq, length;
	int	tval;
	unchar	regval;

	if (reply)
		*reply = 1;	/* will be changed to 0 if tone is queued */

	if (Kdws.w_tone) {
		struct kd_tone	*m;

		/*
		 * store away the tone at the end of the tone queue
		 */
		/* allocate memory */
		m = (struct kd_tone *) kmem_alloc(sizeof (*m), KM_NOSLEEP);
		if (m == (struct kd_tone *) NULL) {
			cmn_err (CE_WARN,
			    "No memory for tone overflow - skipping");
			return (EAGAIN);	/* try again later */
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
		/*
		 * The tone is now queued to be played when those previous
		 * to it have completed.
		 */
		if (reply)
			*reply = 0;		/* we will reply later */
		return (0);
	}
	freq = (ushort)((long)arg & 0xffff);
	length = (ushort)(((long)arg >> 16) & 0xffff);
	if (!freq || !(tval = ((ulong)(length * HZ) / 1000L)))
		return (0);	/* zero frequency or length - nothing to do */
	Kdws.w_tone = 1;

	/* set up timer mode and load initial value */
	outb(TIMERCR, T_CTLWORD);
	outb(TIMER2, freq & 0xff);
	outb(TIMER2, (freq >> 8) & 0xff);

	/* turn tone generator on */
	regval = (inb(TONE_CTL) | TONE_ON);
	outb(TONE_CTL, regval);

	/* arrange to turn the tone off later */
	timeout(kdkb_toneoff, addr, tval);
	return (0);
}



/*
 *
 */
void
kdkb_setled(channel_t *chp, kbstate_t *kbp, unchar led)
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
	if (chp == ws_activechan(&Kdws))
		kdkb_cmd(LED_WARN, FROM_DRIVER);
}

/*ARGSUSED*/
int
kdkb_scrl_lock(channel_t *chp)
{
	kdkb_cmd(LED_WARN, FROM_DRIVER);
	return (0);
}

/*
 *
 */
int
kdkb_locked(ushort ch, unchar kbrk)
{
	int	locked = (Kdws.w_flags & WS_LOCKED) ? 1 : 0;

	if (kbrk)
		return (locked);
	if (Kdws.w_flags & WS_LOCKED) {	/* we are locked, do we unlock? */
		switch (Kdws.w_lkstate) {
		case 0:	/* look for ESC */
			if (ch == '\033')
				Kdws.w_lkstate++;
			else
				Kdws.w_lkstate = 0;
			break;
		case 1:	/* look for '[' */
			if (ch == '[')
				Kdws.w_lkstate++;
			else
				Kdws.w_lkstate = 0;
			break;
		case 2:	/* look for '2' */
			if (ch == '2')
				Kdws.w_lkstate++;
			else
				Kdws.w_lkstate = 0;
			break;
		case 3:	/* look for 'l' */
			if (ch == 'l')
				Kdws.w_flags &= ~WS_LOCKED;
			Kdws.w_lkstate = 0;
			break;
		}
	} else {	/* we are unlocked, do we lock? */
		switch (Kdws.w_lkstate) {
		case 0:	/* look for ESC */
			if (ch == '\033')
				Kdws.w_lkstate++;
			else
				Kdws.w_lkstate = 0;
			break;
		case 1:	/* look for '[' */
			if (ch == '[')
				Kdws.w_lkstate++;
			else
				Kdws.w_lkstate = 0;
			break;
		case 2:	/* look for '2' */
			if (ch == '2')
				Kdws.w_lkstate++;
			else
				Kdws.w_lkstate = 0;
			break;
		case 3:	/* look for 'h' */
			if (ch == 'h')
				Kdws.w_flags |= WS_LOCKED;
			Kdws.w_lkstate = 0;
			break;
		}
	}
	return (locked);
}

/*
 *
 */
/*ARGSUSED*/
void
kdkb_keyclick(register ushort ch)
{
	register unchar	tmp;
	register int	cnt;

	if (Kdws.w_flags & WS_KEYCLICK) {
		tmp = (inb(TONE_CTL) | TONE_ON);	/* start click */
		outb(TONE_CTL, tmp);
		for (cnt = 0; cnt < 0xff; cnt++)
			;
		tmp = (inb(TONE_CTL) & ~TONE_ON);	/* end click */
		outb(TONE_CTL, tmp);
	}
}


/* this routine is called by the kdputchar to force an enable of
 * the keyboard. We do this to avoid the spls and flags of the
 * i8042 interface
 */
void
kdkb_force_enable(void)
{
	SEND2KBD(KB_ICMD, KB_ENAB);
}
