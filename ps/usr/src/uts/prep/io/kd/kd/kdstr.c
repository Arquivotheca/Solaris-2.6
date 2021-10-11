/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF		*/
/*	UNIX System Laboratories, Inc.				*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * Copyright (c) 1995, by Sun Microsystems, Inc.
 * All rights reserved.
 */

#pragma ident	"@(#)kdstr.c	1.83	96/06/02 SMI"

#include "sys/types.h"
#include "sys/errno.h"
#include "sys/inline.h"
#include "sys/termio.h"
#include "sys/stropts.h"
#include "sys/termios.h"
#include "sys/stream.h"
#include "sys/strtty.h"
#include "sys/kd.h"
#include "sys/kbd.h"
#include "sys/kbio.h"
#include "sys/consdev.h"
#include "sys/stat.h"
#include <sys/modctl.h>
#include <sys/debug.h>
#include <sys/promif.h>
#include "sys/reboot.h"
#include "sys/ddi.h"
#include "sys/sunddi.h"

/*
 * DEBUG (or KD_DEBUG for just this module) turns on a flag called
 * kd_enable_debug_hotkey.  If kd_enable_debug_hotkey is non-zero,
 * then the following hotkeys are enabled:
 *    F10 - toggle debugging "normal" translations
 *    F9  - toggle debugging "getchar" translations
 *    F8  - toggle debugging AT-to-XT translations
 * The default value for kd_enable_debug_hotkey is zero, disabling
 * these hotkeys.
 */

#ifdef	DEBUG
#define	KD_DEBUG
#endif

#ifdef	KD_DEBUG
int	kd_enable_debug_hotkey = 0;
int	kd_debug = 0;
int	kd_atxt_debug = 0;
int	kd_getchar_debug = 0;
int	kd_low_level_debug = 0;

unsigned short kd_trace_buf[100];
int kd_n_trace = sizeof (kd_trace_buf) / sizeof (kd_trace_buf[0]);
int kd_cur_trace = 0;
void kd_trace(int, int);
#define	KD_TRACE_SEND		0
#define	KD_TRACE_RECV_INT	1
#define	KD_TRACE_RECV_POLLED	2
#define	KD_TRACE_SPURIOUS	3
#endif

extern int atKeyboardConvertScan();

static int kdcksysrq(kbstate_t *kbp, ushort ch);
static void kdmiocdatamsg(queue_t *qp, mblk_t *mp);
static int kdopen(queue_t *qp, dev_t *devp, int flag, int sflag, cred_t *credp);
static int kdclose(queue_t *qp, int flag, cred_t *credp);
static int kdwsrv(queue_t *qp);
static u_int kdintr(caddr_t arg);
static void kdinit(dev_info_t *devi);
static void kdproto(queue_t *qp, mblk_t *mp);
static void kdmioctlmsg(queue_t *qp, mblk_t *mp);
static void do_kbd_cmd(queue_t *qp, mblk_t *mp);

struct module_info
	kds_info = { 42, "kd", 0, 32, 256, 128 };

static struct qinit
	kd_rinit = { NULL, NULL, kdopen, kdclose, NULL, &kds_info, NULL };

static struct qinit
	kd_winit = { putq, kdwsrv, kdopen, kdclose, NULL, &kds_info, NULL };

struct streamtab
	kd_str_info = { &kd_rinit, &kd_winit, NULL, NULL };

wstation_t	Kdws = {0};

kbstate_t	Kd0state = {0};

static int kd_info(dev_info_t *dip, ddi_info_cmd_t infocmd, void *arg,
		void **result);
static int kd_probe(dev_info_t *);
static int kd_attach(dev_info_t *, ddi_attach_cmd_t);
static dev_info_t *kd_dip;
extern int kb_raw_mode;
static unchar	lastchar;
static int	got_char = B_FALSE;
static int	set_led_state = 0;


static 	struct cb_ops cb_kd_ops = {
	nodev,			/* cb_open */
	nodev,			/* cb_close */
	nodev,			/* cb_strategy */
	nodev,			/* cb_print */
	nodev,			/* cb_dump */
	nodev,			/* cb_read */
	nodev,			/* cb_write */
	nodev,			/* cb_ioctl */
	nodev,			/* cb_devmap */
	nodev,			/* cb_mmap */
	nodev,			/* cb_segmap */
	nochpoll,		/* cb_chpoll */
	ddi_prop_op,		/* cb_prop_op */
	(&kd_str_info),		/* cb_stream */
	0			/* cb_flag */
};

struct dev_ops kd_ops = {
	DEVO_REV,		/* devo_rev */
	0,			/* devo_refcnt */
	(kd_info),		/* devo_getinfo */
	(nulldev),		/* devo_identify */
	kd_probe,		/* devo_probe */
	(kd_attach),		/* devo_attach */
	nodev,			/* devo_detach */
	nodev,			/* devo_reset */
	&(cb_kd_ops),		/* devo_cb_ops */
	(struct bus_ops *)NULL	/* devo_bus_ops */
};

/*
 * Module linkage information for the kernel.
 */

static struct modldrv modldrv = {
	&mod_driverops, /* Type of module.  This one is a pseudo driver */
	"KD driver",
	&kd_ops,	/* driver ops */
};

static struct modlinkage modlinkage = {
	MODREV_1,
	(void *)&modldrv,
	NULL
};

static int	kdgetchar();
static int	kdischar();

/* XXX KLUDGE do not unload when forceloaded from DU distribution */
int kd_forceload = 0;

int
_init()
{
	int	rv;

	mutex_init(&Kdws.w_hw_mutex, "KD hardware mutex",
		MUTEX_DRIVER, NULL);

	rv = mod_install(&modlinkage);

	if (rv != 0)
		mutex_destroy(&Kdws.w_hw_mutex);

	return (rv);
}


int
_fini()
{
	int rv;

	/* XXX KLUDGE do not unload when forceloaded from DU distribution */
	if (kd_forceload != 0)
		return (EBUSY);

	rv = mod_remove(&modlinkage);
	if (rv == 0)
		mutex_destroy(&Kdws.w_hw_mutex);

	return (rv);
}

int
_info(struct modinfo *modinfop)
{
	return (mod_info(&modlinkage, modinfop));
}

/*ARGSUSED*/
static int
kd_probe(dev_info_t *devi)
{
	if (i8042_main_is_present()) {
		return (DDI_PROBE_SUCCESS);
	} else {
		return (DDI_PROBE_FAILURE);
	}
}

/*ARGSUSED*/
static int
kd_attach(dev_info_t *devi, ddi_attach_cmd_t cmd)
{
	extern int	console;
	extern int	(*getcharptr)();
	extern int	(*ischarptr)();

	ddi_iblock_cookie_t 	tmp;

	if (ddi_create_minor_node(devi, "kd", S_IFCHR,
	    0, NULL, NULL) == DDI_FAILURE) {
		ddi_remove_minor_node(devi, NULL);
		return (-1);
	}
	kd_dip = devi;

	kdinit(devi);
	if (console == CONSOLE_IS_FB) {
		extern void kdkb_bell();

		getcharptr = kdgetchar;
		ischarptr = kdischar;
		/* I'm not happy about this. */
		ddi_set_console_bell(kdkb_bell);
	}

/*	Establish initial softc values 					*/
	if (ddi_add_intr(devi, (u_int) 0, &tmp,
		(ddi_idevice_cookie_t *)0, kdintr, (caddr_t)0)) {
		panic("kd_attach: cannot add intr");
		/* NOTREACHED */
	}
	return (DDI_SUCCESS);
}

/* ARGSUSED */
static int
kd_info(dev_info_t *dip, ddi_info_cmd_t infocmd, void *arg, void **result)
{
	register int error;

	switch (infocmd) {
	case DDI_INFO_DEVT2DEVINFO:
		if (kd_dip == NULL) {
			error = DDI_FAILURE;
		} else {
			*result = (void *) kd_dip;
			error = DDI_SUCCESS;
		}
		break;
	case DDI_INFO_DEVT2INSTANCE:
		*result = (void *)0;
		error = DDI_SUCCESS;
		break;
	default:
		error = DDI_FAILURE;
	}
	return (error);
}

static void
kdinit(dev_info_t *devi)
{
	int layout;
	extern int PReP_kb_layout;

	if (Kdws.w_init)
		return;

	/*
	 * "lang" is a keyboard layout number, rather than
	 * an ISO 8859 thing.
	 */
	if ((layout = ddi_getprop(DDI_DEV_T_NONE, devi,
	    DDI_PROP_DONTPASS, "lang", -1)) != -1)
		Kdws.w_kblayout = layout;
	else {
		/*
		 * NEEDSWORK:  really we should be doing a
		 *    getprop(...,"language",...)
		 * but ISA nodes are not currently allowed into the kernel
		 * from Open Firmware, so we're doing it in startup() where we
		 * can safely call OF.
		 */
		Kdws.w_kblayout = PReP_kb_layout;
	}

	Kdws.w_qp = (queue_t *)NULL;
	kdkb_init(&Kdws);

	/* read scan data - clear possible spurious data */
	mutex_enter(&Kdws.w_hw_mutex);
	/* Eat keyboard *and* mouse data */
	while (inb(KB_STAT) & KBC_STAT_OUTBF)
		(void) inb(KB_OUT);
	mutex_exit(&Kdws.w_hw_mutex);

	Kdws.w_init++;
	drv_setparm(SYSRINT, 1);	/* reset keyboard interrupts */
}

/*
 *
 */

/*ARGSUSED2*/
static int
kdopen(queue_t *qp, dev_t *devp, int flag, int sflag, cred_t *credp)
{
	kbstate_t	*kbp;

	Kdws.w_dev = *devp;

	kbp = &Kd0state;
	qp->q_ptr = (caddr_t)kbp;
	WR(qp)->q_ptr = qp->q_ptr;
	if (!Kdws.w_qp)
		Kdws.w_qp = qp;
	qprocson(qp);
	return (0);
}

/*
 * Close
 */

/*ARGSUSED1*/
static int
kdclose(queue_t *qp, int flag, cred_t *credp)
{
	Kdws.w_qp = (queue_t *)NULL;
	qprocsoff(qp);
	return (0);
}

/*
 *
 */

static int
kdwsrv(queue_t *qp)
{
	mblk_t	*mp;

	while ((mp = getq(qp))) {
		switch (mp->b_datap->db_type) {
		case M_PROTO:
		case M_PCPROTO:
			kdproto(qp, mp);
			continue;
		case M_DATA:
			do_kbd_cmd(qp, mp);
			break;
		case M_IOCTL:
			kdmioctlmsg(qp, mp);
			continue;
		case M_IOCDATA:
			kdmiocdatamsg(qp, mp);
			continue;
		case M_DELAY:
		case M_STARTI:
		case M_STOPI:
		case M_READ:	/* ignore, no buffered data */
			freemsg(mp);
			continue;
		case M_FLUSH:
			*mp->b_rptr &= ~FLUSHW;
			if (*mp->b_rptr & FLUSHR)
				qreply(qp, mp);
			else
				freemsg(mp);
			continue;
		default:
			cmn_err(CE_NOTE, "kdwsrv: bad msg %x",
				mp->b_datap->db_type);
			break;
		}
		qenable(qp);
		return (0);
	}
	return (0);
}

/*
 *
 */
static void
do_kbd_cmd(queue_t *qp, mblk_t *mp)
{
	unchar		cmd;
	mblk_t		*rmp;

/* XXXppc!  This processes only one character from each message! */
	cmd = *mp->b_rptr;

	if (set_led_state) {
		kbstate_t	*kbp;
		unchar	led_state = cmd;

		kbp = qp->q_ptr;
		kbp->kb_state &= ~(CAPS_LOCK | NUM_LOCK | SCROLL_LOCK);
		if (led_state & LED_CAPS_LOCK)
			kbp->kb_state |= CAPS_LOCK;
		if (led_state & LED_NUM_LOCK)
			kbp->kb_state |= NUM_LOCK;
		if (led_state & LED_SCROLL_LOCK)
			kbp->kb_state |= SCROLL_LOCK;
		kdkb_cmd(LED_WARN, FROM_DRIVER);
		set_led_state = 0;
	} else {
		switch (cmd) {
		case KBD_CMD_SETLED:
			set_led_state = 1;
			break;
		case KBD_CMD_RESET:
/* This will simply drop the request if there's no memory.  */
/* This is wrong. */
/* XXXppc need to upgrade to use qbufcall. */
			if ((rmp = allocb(2, BPRI_HI)) == NULL) break;
			/* queue RESETKEY, type code */
			*rmp->b_wptr++ = RESETKEY;
			*rmp->b_wptr++ = KB_PC;
			qreply(qp, rmp);
			break;
		case KBD_CMD_CLICK:
			Kdws.w_flags |= WS_KEYCLICK;
			break;
		case KBD_CMD_NOCLICK:
			Kdws.w_flags &= ~WS_KEYCLICK;
			break;
		case KBD_CMD_BELL:
			/*
			 *  There is no interface to set the freq.
			 *  This magic number is based on what the
			 *  x86 X server sends through KD_MKTONE.
			 *  1193180L + (bell_pitch >> 1) / bell_pitch
			 */
			kdkb_sound(0xba8);
			break;
		case KBD_CMD_NOBELL:
			kdkb_sound(0);
			break;
		case KBD_CMD_GETLAYOUT:
/* This will simply drop the request if there's no memory.  Ugly, */
/* but the code in kbd already has a timeout and can handle it. */
/* XXXppc should perhaps upgrade to use qbufcall. */
			if ((rmp = allocb(2, BPRI_HI)) == NULL) break;
			/* queue LAYOUTKEY, layout code */
			*rmp->b_wptr++ = LAYOUTKEY;
			*rmp->b_wptr++ = (char)Kdws.w_kblayout;
			qreply(qp, rmp);
			break;
		}
	}
	freemsg(mp);
}

static void
kdmioctlmsg(queue_t *qp, mblk_t *mp)
{
	caddr_t  data = NULL;
	struct iocblk	*iocp;
/*
 * XXX
	if (mp->b_wptr - mp->b_rptr != sizeof (struct iocblk)) {
		cmn_err(CE_NOTE, "!kdmioctlmsg: bad M_IOCTL msg");
		return;
	}
*/
	iocp = (struct iocblk *)mp->b_rptr;
	if (mp->b_cont != NULL)
		data = (caddr_t)mp->b_cont->b_rptr;

	switch (iocp->ioc_cmd) {

	case KIOCSLAYOUT:
		if (data != NULL) {
		    Kdws.w_kblayout = *(int *)data;
		    ws_iocack(qp, mp, iocp);
		} else
		    ws_iocnack(qp, mp, iocp, EFAULT);
		break;
	/*
	 * "kb" will helpfully try to initialize us.
	 * Quietly ignore it.
	 */
	case TCSETSW:
	case TCSETSF:
	case TCSETS:
	case TCSETAW:
	case TCSETAF:
	case TCSETA:
		ws_iocack(qp, mp, iocp);
		break;

	default:
#ifdef DEBUG1
		cmn_err(CE_NOTE, "!kdmioctlmsg %x", iocp->ioc_cmd);
#endif
		ws_iocnack(qp, mp, iocp, EINVAL);
	}
}

/*
 *
 */

/*ARGSUSED*/
static void
kdproto(queue_t *qp, mblk_t *mp)
{
	freemsg(mp);
}


/*
 * Process a byte received from the keyboard
 */
static void
kd_received_byte(
	int kbscan)	/* raw scan code */
{
	int	kdcksysrq(kbstate_t *, ushort);
	mblk_t	*mp;

	register unchar	rawscan,	/* XT raw keyboard scan code */
			scan;		/* "cooked" scan code */
	kbstate_t	*kbp;		/* pointer to keyboard state */
	unchar	kbrk,
		oldprev,
		legit;		/* is this a legit key pos'n? */
	ushort	ch;
	int	key_pos = -1, isup;

	if (kb_raw_mode == KBM_AT) {
		rawscan = kd_xlate_at2xt(kbscan);
#ifdef	KD_DEBUG
		if (kd_atxt_debug)
			prom_printf("kdintr:  AT 0x%x -> XT 0x%x\n",
			    kbscan, rawscan);
#endif
	} else
		rawscan = (unchar)kbscan;

#ifdef	KD_DEBUG
	if (kd_enable_debug_hotkey) {
		switch (rawscan) {
		case 0x44:	/* F10 */
			kd_debug = !kd_debug;
			break;
		case 0x43:	/* F9 */
			kd_getchar_debug = !kd_getchar_debug;
			break;
		case 0x42:	/* F8 */
			kd_atxt_debug = !kd_atxt_debug;
			break;
		case 0x41:	/* F7 */
			kd_low_level_debug = !kd_low_level_debug;
			break;
		}
	}
#endif

	switch (rawscan) {
	    case KB_ACK:	/* ack from keyboard? */
		/* Spurious ACK -- cmds to keyboard now polled */
		return;
	    case 0:
		/* Eaten by translation */
		return;
	}
	if (!Kdws.w_init)	/* can't do anything anyway */
		return;

	legit = atKeyboardConvertScan(rawscan, &key_pos, &isup);

#ifdef	KD_DEBUG
	if (kd_debug) {
		if (legit) {
			prom_printf("kdintr:  XT 0x%x -> %s key_pos %d\n",
			    rawscan, isup ? "released" : "pressed", key_pos);
		} else {
			prom_printf("kdintr:  XT 0x%x -> ignored\n", rawscan);
		}
	}
#endif

	kbrk = rawscan & KBD_BREAK;
	kbp = &Kd0state;
	oldprev = kbp->kb_prevscan;
	ch = ws_scanchar(kbp, rawscan);
	/* check for handling extended scan codes correctly */
	/* this is because ws_scanchar calls ws_procscan on its own */
	if (oldprev == 0xe0 || oldprev == 0xe1)
		kbp->kb_prevscan = oldprev;
	scan = ws_procscan(kbp, rawscan);
	if (legit == 0 || scan == 0)
		return;

	/*
	 * This is to filter out auto repeat since it can't be
	 * turned off at the hardware.
	 */
	if (kbrk) {
		if (kbp->kb_old_scan == scan)
			kbp->kb_old_scan = 0;
	} else {
		if (kbp->kb_old_scan == scan)
			return;
		kbp->kb_old_scan = scan;
	}

	/*
	 * Click and process debugger request
	 */
	if (!kbrk) {
		kdkb_keyclick();
		if (kdcksysrq(kbp, ch))
			return;
	}

	/*
	 * If there's no queue above us - as can happen if we've been
	 * attached but not opened - drop the keystroke.
	 * Note that we do this here instead of above so that
	 * Ctrl-Alt-D still works.
	 */
	if (Kdws.w_qp == NULL)
		return;

	mp = allocb(2, BPRI_MED);
	if (mp != NULL) {
		*mp->b_wptr++ = isup ? KBD_RELEASED_PREFIX : KBD_PRESSED_PREFIX;
		*mp->b_wptr++ = (char)key_pos;
		putnext(Kdws.w_qp, mp);
	}
}

/*
 * Called from interrupt handler when keyboard interrupt occurs.
 */

/*ARGSUSED*/
static u_int
kdintr(caddr_t arg)
{
	unchar kbscan;	/* raw scan code */

	/* don't care if drv_setparm succeeds */
	drv_setparm(SYSRINT, 1);

	mutex_enter(&Kdws.w_hw_mutex);
	if ((inb(KB_STAT) & (KBC_STAT_OUTBF | KBC_STAT_AUXBF)) !=
	    KBC_STAT_OUTBF) {
		/*
		 * No keyboard data
		 */
		/*
		 * I don't understand why this is necessary, but on some
		 * platforms (IBM 6050/6070) we will occasionally see a
		 * situation where we take an interrupt, the status register
		 * is 0x18 (normal idle), and where if we don't inb(KB_OUT)
		 * the keyboard goes away forever.  This is most often seen
		 * when you tap the Caps Lock key repeatedly, and probably
		 * has something to do with the fact that we poll for the
		 * acknowlegement on the LED control sequences.
		 */
		(void) inb(KB_OUT);
#if	defined(KD_DEBUG)
		if (kd_low_level_debug)
			prom_printf(" SPURIOUS ");
		kd_trace(KD_TRACE_SPURIOUS, 0);
#endif
		mutex_exit(&Kdws.w_hw_mutex);
		return (DDI_INTR_UNCLAIMED);
	}

	kbscan = inb(KB_OUT);
#if	defined(KD_DEBUG)
	if (kd_low_level_debug)
		prom_printf(" i<%x ", kbscan);
	kd_trace(KD_TRACE_RECV_INT, kbscan);
#endif
	mutex_exit(&Kdws.w_hw_mutex);
	kd_received_byte(kbscan);
	return (DDI_INTR_CLAIMED);
}

/*
 *
 */

static void
kdmiocdatamsg(queue_t *qp, mblk_t *mp)
{
	struct copyresp	*csp;

	csp = (struct copyresp *)mp->b_rptr;
	if (csp->cp_rval) {
		freemsg(mp);
		return;
	}

	switch (csp->cp_cmd) {

	default:
		ws_iocack(qp, mp, (struct iocblk *)mp->b_rptr);
		break;
	}
}

/*
 *
 */

#define	K_LEFT_ALT	60
#define	K_RIGHT_ALT	62
#define	K_LEFT_CTRL	58
#define	K_RIGHT_CTRL	64
#define	K_LEFT_SHIFT	44
#define	K_RIGHT_SHIFT	57

/*
 * On some systems (notably Motorola PowerPCs), we've seen a sequence
 * where, on coming out of kadb, a "release" comes in between a "set
 * LED" command and its ack.  This confuses the ack mechanism, and
 * leaves the keyboard controller wedged with an ack in its buffer.
 *
 * I don't know why it doesn't occur on more systems or in more
 * situations.
 *
 * I don't know what the right answer is.  My best guess is to make ack
 * processing be interrupt driven and run LED setting through a state machine
 * so that we can accept keystroke data while waiting for an ack.  This is
 * more reorg than I want to take on at the moment.
 *
 * For the moment, we'll just skip setting the LEDs as we enter and
 * leave kadb.  The LEDs may be wrong, but we shouldn't hang.  That's
 * probably a good tradeoff.  (To get the LEDs corrected, just tap any
 * lock key.)
 *
 * I've made this controllable via this variable so that people
 * wanting to experiment can do so easily.
 */

static int kd_kadb_set_leds = 0;

static int
kdcksysrq(kbstate_t *kbp, ushort ch)
{
	mblk_t	*mp;

	switch (ch) {
	case K_DBG:
		/*
		 * If no debugger return 0
		 * so the application sees the ctrl-alt-d
		 */
		if (!(boothowto & RB_DEBUG))
			return (0);

		/*
		 * If we're not the console, ignore it
		 */
		if (Kdws.w_dev != kbddev &&
		    Kdws.w_dev != stdindev &&
		    Kdws.w_dev != rconsdev)
			return (0);

		/* save the state on entry........... */
		kbp->kb_sstate = kbp->kb_state &
				(CAPS_LOCK | NUM_LOCK | SCROLL_LOCK);
		kbp->kb_state = kbp->kb_sa_state &
				(CAPS_LOCK | NUM_LOCK | SCROLL_LOCK);
		if (kd_kadb_set_leds)
		    kdkb_cmd(LED_WARN, FROM_DRIVER);

		kbp->kb_debugger_entered_through_kd = 1;
		debug_enter((char *)NULL);
		kbp->kb_debugger_entered_through_kd = 0;

		/* ......and restore it on exit */
		kbp->kb_sa_state = kbp->kb_state &
				(CAPS_LOCK | NUM_LOCK | SCROLL_LOCK);
		kbp->kb_state = kbp->kb_sstate;
		if (kd_kadb_set_leds)
		    kdkb_cmd(LED_WARN, FROM_DRIVER);

		/*
		 * It's (barely) possible that we could get an interrupt
		 * and a Ctrl-Alt-D request when we're attached but not
		 * open.  If we're not open, we don't have a stream.
		 */
		if (Kdws.w_qp != NULL) {
			/*
			 * send up sequences for both CTRL keys, both ALT keys
			 * and both SHIFT keys.
			 * This should reset the state of the kbd driver above
			 * us.
			 */
			mp = allocb(6*2, BPRI_MED);
			if (mp != NULL) {
				*mp->b_wptr++ = KBD_RELEASED_PREFIX;
				*mp->b_wptr++ = K_LEFT_ALT;
				*mp->b_wptr++ = KBD_RELEASED_PREFIX;
				*mp->b_wptr++ = K_RIGHT_ALT;
				*mp->b_wptr++ = KBD_RELEASED_PREFIX;
				*mp->b_wptr++ = K_LEFT_CTRL;
				*mp->b_wptr++ = KBD_RELEASED_PREFIX;
				*mp->b_wptr++ = K_RIGHT_CTRL;
				*mp->b_wptr++ = KBD_RELEASED_PREFIX;
				*mp->b_wptr++ = K_LEFT_SHIFT;
				*mp->b_wptr++ = KBD_RELEASED_PREFIX;
				*mp->b_wptr++ = K_RIGHT_SHIFT;
				putnext(Kdws.w_qp, mp);
			}
		}

		return (1);

	default:
		break;
	}
	return (0);
}

/*
 * Console I/O support:
 *
 * kdischar()	check for character pending
 */

static int
kdischar()
{
	register ushort ch;	/* processed scan code */
	register unchar rawscan;	/* raw keyboard scan code */
	unsigned char	kbscan;
	kbstate_t	*kbp;
	ushort		okbstate;
	extern int	ws_toglchange();

	/* If there's already a saved character return true */
	if (got_char)
		return (B_TRUE);

	for (;;) {	/* Discard any mouse data */
		/*
		 * CAUTION:  I don't think it will, but this might have
		 * problems on pre-PS/2 keyboard controllers that don't
		 * have mouse ports.  The KBC_STAT_AUXBF bit on old controllers
		 * was _not_ a reserved bit; it was used for reporting
		 * transmit timeout errors.  I think these include getting
		 * no response from the keyboard.  I don't believe this
		 * will cause us problems here, except that if (somehow)
		 * it's on, we'll discard data we perhaps shouldn't have.
		 */
		switch (inb(KB_STAT) & (KBC_STAT_OUTBF | KBC_STAT_AUXBF)) {
		case KBC_STAT_OUTBF:		/* Keyboard data */
			break;
		case KBC_STAT_OUTBF|KBC_STAT_AUXBF:	/* Mouse data */
			/* Discard it. */
			(void) inb(KB_OUT);
			continue;
		default:		/* No data */
			return (B_FALSE);
		}
		break;
	}

	/* get the scan code */
	kbscan = inb(KB_OUT);
#if	defined(KD_DEBUG)
	if (kd_low_level_debug)
		prom_printf(" g<%x ", kbscan);
	kd_trace(KD_TRACE_RECV_POLLED, kbscan);
#endif
	if (kb_raw_mode == KBM_AT) {
		rawscan = kd_xlate_at2xt(kbscan);
#ifdef	KD_DEBUG
		if (kd_atxt_debug)
			prom_printf("kdischar:  AT 0x%x -> XT 0x%x\n",
			    kbscan, rawscan);
#endif
	} else
		rawscan = kbscan;

	kdkb_force_enable();

#ifdef	KD_DEBUG
	if (kd_enable_debug_hotkey) {
		switch (rawscan) {
		case 0x44:	/* F10 */
			kd_debug = !kd_debug;
			break;
		case 0x43:	/* F9 */
			kd_getchar_debug = !kd_getchar_debug;
			break;
		case 0x42:	/* F8 */
			kd_atxt_debug = !kd_atxt_debug;
			break;
		case 0x41:	/* F7 */
			kd_low_level_debug = !kd_low_level_debug;
			break;
		}
	}
#endif

	if (rawscan == 0)
		return (B_FALSE);

	kbp = &Kd0state;
	/*
	 * Call ? to convert scan code to a character.
	 * ? returns a short, with flags in the top byte and the
	 * character in the low byte.
	 * A legal ascii character will have the top 9 bits off.
	 */
	okbstate = kbp->kb_state;
	ch = ws_scanchar(kbp, rawscan);
#ifdef	KD_DEBUG
	if (kd_getchar_debug)
		prom_printf("kdischar: XT %x -> ASCII %x\n", rawscan, ch);
#endif
	if (ws_toglchange(okbstate, kbp->kb_state))
		kdkb_cmd(LED_WARN, FROM_DEBUGGER);

	if (ch & 0xFF80)
		return (B_FALSE);
	else {
		lastchar = ch;
		got_char = B_TRUE;
		return (B_TRUE);
	}
}

/*
 * kdgetchar()	wait for character and return it
 */

static int
kdgetchar()
{
	kbstate_t	*kbp;

	kbp = &Kd0state;

	if (!kbp->kb_debugger_entered_through_kd) {
		/*
		 * Entered from a break point
		 * save the state on entry...........
		 */
		kbp->kb_sstate = kbp->kb_state;
		kbp->kb_state = kbp->kb_sa_state;
		if (kd_kadb_set_leds)
		    kdkb_cmd(LED_WARN, FROM_DEBUGGER);
	}

	while (!got_char)
		(void) kdischar();

	if (!kbp->kb_debugger_entered_through_kd) {
		/* ......and restore it on exit */
		kbp->kb_sa_state = kbp->kb_state;
		kbp->kb_state = kbp->kb_sstate;
		if (kd_kadb_set_leds)
		    kdkb_cmd(LED_WARN, FROM_DEBUGGER);
	}
	got_char = B_FALSE;
	return (lastchar);
}

#if	defined(KD_DEBUG)
void
kd_trace(int type, int code)
{
	int tmp;

	/*
	 * There's a small race here:  If this gets re-entered at just
	 * the wrong moment, a trace data item could get lost.
	 * Since this is only a debug mechanism, it doesn't seem to
	 * make sense to spend any real effort fixing this - just be
	 * aware of it when interpreting the debug results.
	 */
	kd_trace_buf[kd_cur_trace] = (type << 8) | code;
	/*
	 * Take care to avoid a more dangerous race:  never, ever,
	 * let kd_cur_trace point outside the buffer.  Not even for
	 * a moment.
	 */
	tmp = kd_cur_trace + 1;
	if (tmp >= kd_n_trace)
		kd_cur_trace = 0;
	else
		kd_cur_trace = tmp;
}
#endif
