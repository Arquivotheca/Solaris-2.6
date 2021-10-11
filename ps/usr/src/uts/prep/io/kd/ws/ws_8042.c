/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF		*/
/*	UNIX System Laboratories, Inc.				*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * Copyright (c) 1992-4 by Sun Microsystems, Inc.  All Rights Reserved.
 */

#pragma ident	"@(#)ws_8042.c	1.18	96/06/02 SMI"

#include "sys/types.h"
#include "sys/stream.h"
#include "sys/termios.h"
#include "sys/strtty.h"
#include "sys/kd.h"
#include "sys/ws/8042.h"
#include "sys/cmn_err.h"
#include "sys/inline.h"
#include "sys/promif.h"
#include "sys/ddi.h"
#include "sys/sunddi.h"

#define	AUX_DISAB	0x20
#define	KBD_DISAB	0x10

#if	defined(DEBUG)
#define	KD_DEBUG
#endif

#if	defined(KD_DEBUG)
extern int kd_low_level_debug;
extern void kd_trace(int, int);
#define	KD_TRACE_ACQUIRE_MAIN	0x20
#define	KD_TRACE_ACQUIRE_AUX	0x21
#define	KD_TRACE_SEND		0x22
#define	KD_TRACE_RECV_RESP	0x23
#endif

/*
 * Caution:  turning this on causes kadb to crash, because cmn_err doesn't
 * work from inside kadb.
 */
#undef	DEBUG_8042

extern wstation_t	Kdws;

struct i8042 {
	unchar	s_state,	/* state of 8042 */
		s_saved,	/* indicates data was saved */
		s_data,		/* saved data (scan code or 320 mouse input) */
		s_dev2;		/* device saved character is meant for */
} i8042_state = { AUX_DISAB, 0, 0, 0 };

int i8042_has_aux_port = -1;
int i8042_spin_time = 100; /* default to assuming flakey 8042 */

/*
 * Determine whether an 8042 is present.
 *
 * Remember that this could be called from either "kd" or "kdmouse",
 * in either order.
 *
 * I haven't come up with any really good algorithm for this, so
 * here's the plan:
 *
 * - assume that the 8042 ports are either attached to an 8042 or are unused.
 * - probe only once, to avoid breaking the other driver.
 * - assume that the desired state is keyboard enabled, mouse disabled.
 * - if we can't send a command in a reasonable amount of time, lose.
 * - if we can't receive a response in a reasonable amount of time, lose.
 * - if the responses aren't right, lose.
 *
 * 1)  Disable the keyboard port.
 * 2)  Disable the aux port (if any).
 * 3)  Discard any data in the output buffer.
 * 4)  If the status indicates that the output buffer is still full, lose.
 * 5)  Read the command byte.
 * 6)  If the keyboard's enabled, lose.
 * 7)  Enable the keyboard.
 * 8)  Win.
 */

int i8042_probe_send_loop = 100;	/* About .1 seconds */
int i8042_probe_recv_loop = 100;	/* About .1 seconds */
int i8042_flush_loop = 100;

/*
 * Attempt to send a byte to the keyboard controller.
 * Fail if it takes more than a certain amount of time.
 * This routine is intended for use only while probing for the 8042 itself.
 */
static boolean_t
i8042_probe_send(int port, int byte)
{
	int i;

	for (i = 0; i < i8042_probe_send_loop; i++) {
		if (!(inb(KB_STAT) & KBC_STAT_INBF)) {
			outb(port, byte);
			return (B_TRUE);
		}
		drv_usecwait(1000);
	}
	return (B_FALSE);
}

/*
 * Attempt to receive a byte from the keyboard controller.
 * Fail if it takes more than a certain amount of time.
 * This routine is intended for use only while probing for the 8042 itself.
 */
static boolean_t
i8042_probe_recv(unsigned char *byte)
{
	int i;

	for (i = 0; i < i8042_probe_recv_loop; i++) {
		if (inb(KB_STAT) & KBC_STAT_OUTBF) {
			*byte = inb(KB_OUT);
			return (B_TRUE);
		}
		drv_usecwait(1000);
	}
	return (B_FALSE);
}

/*
 * Check for the presence of the 8042 itself.
 */
static boolean_t
i8042_is_present()
{
	static boolean_t i8042_probed = B_FALSE;
	static boolean_t i8042_present;	/* only if i8042_probed is TRUE */
	unsigned char cmd;
	int i;

	if (i8042_probed)
		return (i8042_present);

	/*
	 * Assume the worst for the moment, so that we can just "return"
	 * on failure.
	 */
	i8042_present = B_FALSE;

	/*
	 * Regardless, we will have probed for the device, and so can
	 * return the result without probing again.
	 */
	i8042_probed = B_TRUE;

	/*
	 * Disable keyboard and aux ports so we can safely read the
	 * command byte.
	 */
	if (!i8042_probe_send(KB_ICMD, KBC_KB_DISABLE))
		return (B_FALSE);

	if (!i8042_probe_send(KB_ICMD, KBC_AUX_DISABLE))
		return (B_FALSE);

	/*
	 * Flush the output buffer.  In case there's some kind of FIFO, be
	 * really enthusiastic about it.
	 */
	for (i = 0; i < i8042_flush_loop; i++) {
		(void) inb(KB_IDAT);
		drv_usecwait(10);
	}

	/*
	 * After all that, there's no excuse for the output buffer
	 * being still full.
	 */
	if (inb(KB_STAT) & KBC_STAT_OUTBF)
		return (B_FALSE);

	/*
	 * Ask the controller for the command byte.
	 */
	if (!i8042_probe_send(KB_ICMD, KBC_RCB))
		return (B_FALSE);

	/*
	 * Receive the command byte.  Arguably, this is *the* critical
	 * test, because a few steps ago we verified that the output
	 * buffer was empty, and to complete this receive it has to
	 * go "full".
	 */
	if (!i8042_probe_recv(&cmd))
		return (B_FALSE);

	/*
	 * As long as we have the command byte, verify that it says the
	 * keyboard's disabled like it should.
	 */
	if (!(cmd & KBC_CMD_KB_DIS))
		return (B_FALSE);

	/*
	 * Turn the keyboard back on since that's the way it's expected.
	 */
	if (!i8042_probe_send(KB_ICMD, KBC_KB_ENABLE))
		return (B_FALSE);

	/*
	 * Win!
	 */
	i8042_present = B_TRUE;

	return (i8042_present);
}

/*
 * Determine if machine has a main device port. Return 1 if yes,
 * 0 if no.
 * Since all 8042s have a main port, if the 8042 itself is present
 * then the main port is present.
 */
int
i8042_main_is_present(void)
{
	return (i8042_is_present() ? 1 : 0);
}

/*
 * Determine if machine has an auxiliary device port. Return 1 if yes,
 * 0 if no.
 *
 * I believe this routine may misbehave if keyboard input happens at
 * exactly the wrong moment.  That's not the problem du jour, so
 * I'm not going to try to fix it now.
 */
int
i8042_aux_port()
{
	int	tmp;

	/*
	 * Kdws.w_hw_mutex is already held.
	 * That's a lie.  kdmouse doesn't grab the mutex before calling
	 * this routine; only kdkb does.  We're safe because
	 * (1) Both kd and kdmouse are "unsafe" drivers.
	 * (2) This routine is called from both during their initialization.
	 * (3) This routine maintains a static flag so that the probe sequence
	 *     is executed only once.
	 * There's no race with interrupts because this routine is called
	 * before the interrupts are enabled.  There's no race between the
	 * two drivers because they're unsafe.
	 *
	 * This is ugly, but fixing it is more work than I want to take
	 * on now - the kd/kdmouse subsystem needs to be reorganized.
	 */

	if (i8042_has_aux_port != -1)
		return (i8042_has_aux_port);

	i8042_has_aux_port = 0;

	/*
	 * If there's no 8042 at all, then there's certainly no aux port.
	 */
	if (!i8042_is_present())
		return (i8042_has_aux_port);

	while ((inb(KB_STAT) & KBC_STAT_OUTBF)) /* data in the output buffer */
		(void) inb(KB_OUT);

	/* enable auxiliary interface */
	SEND2KBD(KB_ICMD, KBC_AUX_ENABLE);
	drv_usecwait(100); 	/* give registers time to soak */
	SEND2KBD(KB_ICMD, KBC_RCB);	/* read command byte */
	drv_usecwait(100); 	/* give registers time to soak */
	(void) inb(KB_OUT);
	SEND2KBD(KB_ICMD, KBC_RCB);	/* read command byte */
	/* wait until there is data in the output buffer */
	while (!(inb(KB_STAT) & KBC_STAT_OUTBF))
		/* LOOP */;
	tmp = inb(KB_OUT);
	if (tmp & KBC_CMD_AUX_DIS)	/* enable did not take */
		goto next_test;
	/* disable auxiliary interface */
	SEND2KBD(KB_ICMD, KBC_AUX_DISABLE);
	drv_usecwait(100); 	/* give registers time to soak */
	SEND2KBD(KB_ICMD, KBC_RCB);	/* read command byte */
	drv_usecwait(100); 	/* give registers time to soak */
	inb(KB_OUT);
	SEND2KBD(KB_ICMD, KBC_RCB);	/* read command byte */
	/* wait until there is data in the output buffer */
	while (!(inb(KB_STAT) & KBC_STAT_OUTBF))
		/* LOOP */;
	tmp = inb(KB_OUT);
	if (tmp & KBC_CMD_AUX_DIS) {	/* disable successful */
		i8042_has_aux_port = 1;
	}
next_test:
	if (i8042_has_aux_port) {
		/*
		 * we don't want to penalize aux device performance.
		 * If system is support aux 8042 device, robustness of
		 * 8042 should be good
		 */
		i8042_spin_time = 0;
	} else
		i8042_spin_time = 100; /* assume flakey 8042 -- groan */
	return (i8042_has_aux_port);
}

/*
 * Modify "state" of 8042 so that the next call to release_8042
 * changes the 8042's state appropriately.
 */

void
i8042_program(int cmd)
{
#ifdef DEBUG_8042
	cmn_err(CE_NOTE, "!i8042_program cmd %x", cmd);
#endif
	switch (cmd) {
	case P8042_KBDENAB:
		i8042_state.s_state &= ~KBD_DISAB;
		break;
	case P8042_KBDDISAB:
		i8042_state.s_state |= KBD_DISAB;
		break;
	case P8042_AUXENAB:
		i8042_state.s_state &= ~AUX_DISAB;
		break;
	case P8042_AUXDISAB:
		i8042_state.s_state |= AUX_DISAB;
		break;
	default:
		cmn_err(CE_PANIC, "program_8042: illegal command %x", cmd);
		break;
	}
}

/*
 * Acquire the 8042 by changing to disabling the keyboard and auxiliary
 * devices (if any), and saving any data currently in the 8042 output port.
 */

void
i8042_acquire(unchar whence)
{
#ifdef DEBUG_8042
	cmn_err(CE_NOTE, "!IN i8042_acquire");
#endif
	if (whence == FROM_DRIVER)
		mutex_enter(&Kdws.w_hw_mutex);

	SEND2KBD(KB_ICMD, KBC_KB_DISABLE);	/* disable keyboard interface */
	if (i8042_has_aux_port) {
		/* give registers time to soak */
		drv_usecwait(i8042_spin_time);
		/* disable auxiliary interface */
		SEND2KBD(KB_ICMD, KBC_AUX_DISABLE);
	}
	if (inb(KB_STAT) & KBC_STAT_OUTBF) {	/* data in the output buffer */
		i8042_state.s_saved = 1;
		i8042_state.s_dev2 = inb(KB_STAT) & KBC_STAT_AUXBF;
		i8042_state.s_data = inb(KB_OUT);
#if	defined(KD_DEBUG)
	    if (kd_low_level_debug) {
		    prom_printf("i8042_acquire:  saved 0x%x from %s\n",
				i8042_state.s_data,
				i8042_state.s_dev2 ? "aux" : "main");
	    }
	    kd_trace(i8042_state.s_dev2 ?
			KD_TRACE_ACQUIRE_AUX :
			KD_TRACE_ACQUIRE_MAIN,
		    i8042_state.s_data);
#endif
	}
	if (whence == FROM_DRIVER)
		mutex_exit(&Kdws.w_hw_mutex);
#ifdef DEBUG_8042
	cmn_err(CE_NOTE, "!out i8042_acquire");
#endif
}

/*
 * Release the 8042.  If data was saved by the acquire, write back the
 * data to the appropriate port, enable the devices interfaces where
 * appropriate and restore the interrupt level.
 */

void
i8042_release(unchar whence)
{
#ifdef DEBUG_8042
	cmn_err(CE_NOTE, "!IN i8042_release");
#endif
	if (whence == FROM_DRIVER)
		mutex_enter(&Kdws.w_hw_mutex);
	if (i8042_has_aux_port && i8042_state.s_saved) {
		if (i8042_state.s_dev2 & 0x20) {
			SEND2KBD(KB_ICMD, KBC_WRT_AUX_OB);
		} else {
			SEND2KBD(KB_ICMD, KBC_WRT_KB_OB);
		}
		/* give registers time to soak */
		drv_usecwait(i8042_spin_time);
		SEND2KBD(KB_IDAT, i8042_state.s_data);
		i8042_state.s_saved = 0;
	}
	if (!(i8042_state.s_state & KBD_DISAB)) {
#ifdef DEBUG_8042
		cmn_err(CE_NOTE, "!about to enable keyboard");
#endif
		/* give registers time to soak */
		drv_usecwait(i8042_spin_time);
		/* enable kbd interface */
		SEND2KBD(KB_ICMD, KBC_KB_ENABLE);
	}
#ifdef DEBUG_8042
	else
		cmn_err(CE_WARN, "!Keyboard is disabled");
#endif
	if (i8042_has_aux_port && !(i8042_state.s_state & AUX_DISAB)) {
		/* give registers time to soak */
		drv_usecwait(i8042_spin_time);
		SEND2KBD(KB_ICMD, KBC_AUX_ENABLE);
	}

	if (whence == FROM_DRIVER)
		mutex_exit(&Kdws.w_hw_mutex);

#ifdef DEBUG_8042
	cmn_err(CE_NOTE, "!out i8042_release");
#endif
}

/*
 * Send a command to a device attached to the 8042.  The cmd argument is the
 * command to send.  To is the device to send it to.  Bufp is an array of
 * unchars into which any responses are placed, and cnt is the number of bytes
 * expected in the response.  whence is where the command came from -
 *	FROM_DRIVER or FROM_DEBUGGER.
 * Return 1 for success, 0 for failure.
 */

int
i8042_send_cmd(unchar cmd, unchar to, unchar *bufp, unchar cnt, unchar whence)
{
	register unchar	tcnt;
	int	rv = 1;
	int	lcnt;

	if (whence == FROM_DRIVER)
		mutex_enter(&Kdws.w_hw_mutex);
	switch (to) {
	case P8042_TO_KBD:	/* keyboard */
		break;
	case P8042_TO_AUX:	/* auxiliary */
		/* give registers time to soak */
		drv_usecwait(i8042_spin_time);
		SEND2KBD(KB_ICMD, KBC_WRT_AUX);
		break;
	default:
#ifdef DEBUG_8042
		cmn_err(CE_NOTE, "send_8042_dev: unknown device");
#endif
		if (whence == FROM_DRIVER)
			mutex_exit(&Kdws.w_hw_mutex);
		return (0);
	}
	/* give registers time to soak */
	drv_usecwait(i8042_spin_time);
#if	defined(KD_DEBUG)
	if (kd_low_level_debug)
		prom_printf(">%x ", cmd);
	kd_trace(KD_TRACE_SEND, cmd);
#endif
	SEND2KBD(KB_IDAT, cmd);
	for (tcnt = 0; tcnt < cnt; tcnt++) {
		lcnt = 200000;
		while (!(inb(KB_STAT) & KBC_STAT_OUTBF) && lcnt--)
			;
		if (lcnt > 0) {
			bufp[tcnt] = inb(KB_OUT);
#if	defined(KD_DEBUG)
			if (kd_low_level_debug)
				prom_printf("<%x ", bufp[tcnt]);
			kd_trace(KD_TRACE_RECV_RESP, bufp[tcnt]);
#endif
		} else {
			rv = 0;
			break;
		}
	}
	if (whence == FROM_DRIVER)
		mutex_exit(&Kdws.w_hw_mutex);
	return (rv);
}
