/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved					*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF		*/
/*	UNIX System Laboratories, Inc.				*/
/*	The copyright notice above does not evidence any	*/
/*	actual or intended publication of such source code.	*/

/*
 * Copyright (c) 1992-1996, by Sun Microsystems, Inc.  All Rights Reserved.
 */

#pragma ident	"@(#)ws_8042.c	1.13	96/06/07 SMI"

#include "sys/types.h"
#include "sys/stream.h"
#include "sys/kd.h"
#include "sys/kb.h"
#include "sys/ws/8042.h"
#include "sys/cmn_err.h"
#include "sys/inline.h"
#include "sys/ddi.h"
#include "sys/sunddi.h"

#define AUX_DISAB	0x20
#define	KBD_DISAB	0x10

struct i8042 {
	unchar	s_state,	/* state of 8042 */
		s_saved,	/* indicates data was saved */
		s_data,		/* saved data (scan code or 320 mouse input) */
		s_dev2;		/* device saved character is meant for */
} i8042_state = { AUX_DISAB, 0, 0, 0 };

int i8042_has_aux_port = -1;
int i8042_spin_time = 100; /* default to assuming flakey 8042 */


/* function prototypes */
int	i8042_aux_port(void);
void	i8042_program(int);
void	i8042_acquire(unchar);
void	i8042_release(unchar);
int	i8042_send_cmd(unchar, unchar, unchar *, unchar, unchar);


/*
 * Determine if machine has an auxiliary device port.
 * Return 1 if yes, 0 if no.
 *
 * The method used is to send the "enable auxillary interface" command,
 * and see if the controller "takes" the command, then send the command
 * to disable it, and verify that it takes that too.
 */
int
i8042_aux_port(void)
{
	int tmp1 = 0, tmp2 = 0;

	if (i8042_has_aux_port != -1)	/* only test once */
		return (i8042_has_aux_port);

	i8042_has_aux_port = 0;

	while ((inb(KB_STAT) & KB_OUTBF)) /* data in the output buffer */
		inb(KB_OUT);		/* flush it */

	SEND2KBD(KB_ICMD, 0xa8);	/* enable auxiliary interface */
	drv_usecwait(100);		/* give registers time to soak */

	SEND2KBD(KB_ICMD, 0x20);	/* read command byte */
	drv_usecwait(100);		/* give registers time to soak */
	inb(KB_OUT);

	SEND2KBD(KB_ICMD, 0x20);	/* read command byte */
	while (!(inb(KB_STAT) & KB_OUTBF))	/* wait for data to appear */
		;
	tmp1 = inb(KB_OUT);		/* get the command byte data */

	if (tmp1 & 0x20) {		/* enable did not take */
		goto next_test;		/* (bit 0x20 indicates *disabled*) */
	}

	SEND2KBD(KB_ICMD, 0xa7);	/* disable auxiliary interface */
	drv_usecwait(100);		/* give registers time to soak */

	SEND2KBD(KB_ICMD, 0x20);	/* read command byte */
	drv_usecwait(100);		/* give registers time to soak */
	inb(KB_OUT);

	SEND2KBD(KB_ICMD, 0x20);	/* read command byte */
	while (!(inb(KB_STAT) & KB_OUTBF))	/* wait for data to appear */
		;
	tmp2 = inb(KB_OUT);		/* get the command byte data */

	if (tmp2 & 0x20) {		/* disable successful */
		i8042_has_aux_port = 1;
	}

next_test:
	if (i8042_has_aux_port)
		/* we don't want to penalize aux device performance.
		 * If system supports aux 8042 device, robustness of
		 * 8042 should be good
		 */
		i8042_spin_time = 0;
	else
		i8042_spin_time = 100;	/* assume flakey 8042 -- groan */

	return (i8042_has_aux_port);
}

/*
 * Modify "state" of 8042 so that the next call to release_8042
 * changes the 8042's state appropriately.
 */

void
i8042_program(int cmd)
{
#ifdef KD_DEBUG
	cmn_err(CE_NOTE,"!i8042_program cmd %x",cmd);
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
 * Acquire the 8042 by changing to splstr, disabling the keyboard and auxiliary
 * devices (if any), and saving any data currently in the 8042 output port.
 */

/*ARGSUSED*/
void
i8042_acquire(unchar whence)
{
	SEND2KBD(KB_ICMD, 0xad);	/* disable keyboard interface */

	if (i8042_has_aux_port) {
		drv_usecwait(i8042_spin_time);	/* give registers time to soak */
		SEND2KBD(KB_ICMD, 0xa7);	/* disable aux interface */
	}

	if (inb(KB_STAT) & KB_OUTBF) {	/* data in the output buffer */
		i8042_state.s_saved = 1;
		i8042_state.s_dev2 = inb(KB_STAT) & 0x20;
		i8042_state.s_data = inb(KB_OUT);
	}
}

/*
 * Release the 8042.  If data was saved by the acquire, write back the
 * data to the appropriate port, enable the devices interfaces where
 * appropriate and restore the interrupt level.
 */

/*ARGSUSED*/
void
i8042_release(unchar whence)
{
	if (i8042_has_aux_port && i8042_state.s_saved) {
		if (i8042_state.s_dev2 & 0x20) {
			SEND2KBD(KB_ICMD, 0xd3);
		} else {
			SEND2KBD(KB_ICMD, 0xd2);
		}
		drv_usecwait(i8042_spin_time);	/* give registers time to soak */
		SEND2KBD(KB_IDAT, i8042_state.s_data);
		i8042_state.s_saved = 0;
	}

	if (!(i8042_state.s_state & KB_DISAB)) {	/* not disabled */
		drv_usecwait(i8042_spin_time);	/* give registers time to soak */
		SEND2KBD(KB_ICMD, 0xae);	/* enable kbd interface */
	}

	if (i8042_has_aux_port && !(i8042_state.s_state & AUX_DISAB)) {
		drv_usecwait(i8042_spin_time);	/* give registers time to soak */
		SEND2KBD(KB_ICMD, 0xa8);
	}
}

/*
 * Send a command to a device attached to the 8042.  The cmd argument is the
 * command to send.  To is the device to send it to.  Bufp is an array of
 * unchars into which any responses are placed, and cnt is the number of bytes
 * expected in the response.  whence is where the command came from - 
 *	FROM_DRIVER or FROM_DEBUGGER.
 * (Note:  This is for compatibility with the "new console" kd used on ppc.)
 * Return 1 for success, 0 for failure.
 */

/*ARGSUSED4*/
int
i8042_send_cmd (unchar cmd, unchar to, unchar *bufp, unchar cnt, unchar whence)
{
	register unchar	tcnt;
	int	rv = 1, lcnt;

	switch (to) {
	case P8042_TO_KBD:	/* keyboard */
		break;
	case P8042_TO_AUX:	/* auxiliary */
		drv_usecwait(i8042_spin_time);	/* give registers time to soak */
		SEND2KBD(KB_ICMD, 0xd4);
		break;
	default:
#ifdef KD_DEBUG
		cmn_err(CE_NOTE, "send_8042_dev: unknown device");
#endif
		return 0;
	}
	drv_usecwait(i8042_spin_time);	/* give registers time to soak */
	SEND2KBD(KB_IDAT, cmd);
	for (tcnt = 0; tcnt < cnt; tcnt++) {
		lcnt = 200000;
		while (!(inb(KB_STAT) & KB_OUTBF) && lcnt--)
			;
		if (lcnt > 0)
			bufp[tcnt] = inb(KB_OUT);
		else {
			rv = 0;
			break;
		}
	}
	return (rv);
}
