/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF		*/
/*	UNIX System Laboratories, Inc.				*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * Copyright (c) 1992 by Sun Microsystems, Inc.  All Rights Reserved.
 */

#ifndef	_SYS_KD_H
#define	_SYS_KD_H

#pragma ident	"@(#)kd.h	1.25	96/06/02 SMI"

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * If the stripped kd used for common keyboard support was MT safe,
 * define this so that kdmouse knows.
 */
#undef	KD_IS_MT_SAFE

/*
 * Keyboard controller I/O port addresses
 */
#define	KB_OUT	0x60		/* output buffer R/O */
#define	KB_IDAT 0x60		/* input buffer data write W/O */
#define	KB_STAT 0x64		/* keyboard controller status R/O */
#define	KB_ICMD 0x64		/* input buffer command write W/O */

/*
 * Commands to keyboard controller
 */
#define	KBC_RCB		0x20	/* read command byte command */
#define	KBC_WCB		0x60	/* write command byte command */
#define	KBC_AUX_DISABLE	0xA7	/* Disable aux interface */
#define	KBC_AUX_ENABLE	0xA8	/* Enable aux interface */
#define	KBC_KB_ENABLE	0xAE	/* Enable keyboard interface */
#define	KBC_KB_DISABLE	0xAD	/* disable keyboard interface */
#define	KBC_ROP		0xD0	/* read output port command */
#define	KBC_WOP		0xD1	/* write output port command */
#define	KBC_WRT_KB_OB	0xD2	/* Write next byte to kb output buffer */
#define	KBC_WRT_AUX_OB	0xD3	/* Write next byte to aux output buffer */
#define	KBC_WRT_AUX	0xD4	/* Write next byte to aux interface */
#define	KBC_RESETCPU	0xFE	/* command to reset AT386 cpu */

/*
 * Keyboard controller command byte flags
 */
#define	KBC_CMD_EOBFI	0x01	/* enable interrupt on output buffer full */
#define	KBC_CMD_KB_DIS	0x10	/* disable keyboard */
#define	KBC_CMD_AUX_DIS	0x20	/* disable aux interface */
#define	KBC_CMD_XLATE	0x40	/* cmd bit for PC compatibility */

/*
 * Keyboard controller status flags
 */
#define	KB_OUTBF	0x01	/* output (to computer) buffer full flag */
#define	KBC_STAT_OUTBF	0x01	/* output (to computer) buffer full flag */
#define	KBC_STAT_INBF	0x02	/* input (from computer) buffer full flag */
#define	KBC_STAT_AUXBF	0x20	/* Data in output buffer is from aux port */

/*
 * Keyboard controller output port bits
 */
#define	KB_GATE20	0x02	/* set this bit to allow addresses > 1Mb */

/*
 * Commands to keyboard
 */
#define	LED_WARN	0xED	/* Tell kbd that following byte is led status */
#define	SCAN_WARN	0xF0	/* kbd command to set scan code set */
#define	KB_READID	0xF2	/* command to read keyboard id */
#define	TYPE_WARN	0xF3	/* command--next byte is typematic values */
#define	KB_ENABLE	0xF4	/* command to to enable keyboard */
				/* this is different from KB_ENAB above in */
				/* that KB_ENAB is a command to the 8042 to */
				/* enable the keyboard interface, not the */
				/* keyboard itself */
#define	KB_RESET	0xFF	/* command to reset keyboard */

/*
 * Codes from keyboard
 */
#define	KB_BAT_OK	0xAA	/* Keyboard says Basic Assurance Test OK */
#define	KAT_EXTEND	0xE0	/* first byte in two byte extended sequence */
#define	KAT_EXTEND2	0xE1	/* Used in "Pause" sequence */
#define	KAT_BREAK	0xF0	/* first byte in two byte break sequence */
#define	KB_ACK		0xFA	/* Acknowledgement byte from keyboard */
#define	KB_RESEND	0xFE	/* response from keyboard to resend data */
#define	KB_ERROR	0xFF	/* response from keyboard to resend data */

/* LED definitions, for LED_WARN command */
#define	LED_SCR		0x01	/* Flag bit for scroll lock */
#define	LED_NUM		0x02	/* Flag bit for num lock */
#define	LED_CAP		0x04	/* Flag bit for cap lock */

/* Typematic values, for TYPE_WARN command */
#define	TYPE_VALS	0x20	/* max speed (30/s) and 1/2 sec delay */
#define	O_TYPE_VALS	0x7F	/* min speed (2/s) and 1 sec delay */



/*
 * Structure of keyboard translation table
 */
#define	NUM_KEYS	256		/* Maximum number of keys */
#define	NUM_STATES	8		/* Number of key states */
typedef struct {
	unsigned char map[NUM_STATES];	/* Key code for each state */
	unsigned char spcl;	/* Bits marking states as special */
	unsigned char flgs;	/* Flags */
	} keyinfo_t;

typedef struct {
	short  n_keys;			/* Number of entries in table */
	keyinfo_t key[NUM_KEYS+1];		/* One entry for each key */
} keymap_t;

/*
 * Make/break distinctions
 */
#define	KBD_BREAK	0x80		/* Key make/break bit (break=1) */
/*
 * Flags for key state calculation.
 */
#define	SHIFTED	0x01			/* Keys are shifted */
#define	CTRLED	0x02			/* Keys are ctrl'd */
#define	ALTED 	0x04			/* Keys are alt'd */
/*
 * Key map table flags
 */
#define	KMF_CLOCK	0x01		/* Key affected by caps lock */
#define	KMF_NLOCK	0x02		/* Key affected by num lock */
/*
 * kb_state bit definitions
 */
#define	LEFT_SHIFT	0x0001	/* left shift key depressed */
#define	LEFT_ALT	0x0002	/* left alt key depressed */
#define	LEFT_CTRL	0x0004	/* left control key depressed */
#define	RIGHT_SHIFT	0x0008	/* right shift key depressed */
#define	RIGHT_ALT	0x0010	/* right alt key depressed */
#define	RIGHT_CTRL	0x0020	/* right control key depressed */
#define	CAPS_LOCK	0x0040	/* caps lock key down */
#define	NUM_LOCK	0x0080	/* num lock key down */
#define	SCROLL_LOCK	0x0100	/* scroll lock key down */
#define	ALTSET		(LEFT_ALT|RIGHT_ALT)
#define	SHIFTSET	(LEFT_SHIFT|RIGHT_SHIFT)
#define	CTRLSET		(LEFT_CTRL|RIGHT_CTRL)
#define	NONTOGGLES	(ALTSET|SHIFTSET|CTRLSET)
/*
 * Number of entries in 0xe0 prefix translation table
 */
#define	ESCTBLSIZ	18		/* Entries in 101/102 key table */
/*
 * Character flags.  Should not conflict with FRERROR and friends in tty.h
 */
#define	NO_CHAR		0x8000		/* Do not generate a char */
#define	GEN_ESCLSB	0x0800		/* Generate <ESC> [ prefix to char */
#define	GEN_ESCN	0x0400		/* Generate <ESC> N prefix to char */
#define	GEN_ZERO	0x0200		/* Generate 0 prefix to char */
#define	GEN_FUNC	0x0100		/* Generate function key */
#define	GEN_ESCO	0x1000		/* Generate <ESC> O prefix to char */
/*
 * Special key code definitions
 */
#define	K_NOP	0			/* Keys with no function */
#define	K_LSH	2			/* Left shift */
#define	K_RSH	3			/* Right shift */
#define	K_CLK	4			/* Caps lock */
#define	K_NLK	5			/* Num lock */
#define	K_SLK	6			/* Scroll lock */
#define	K_ALT	7			/* Alt */
#define	K_BTAB	8			/* Back tab */
#define	K_CTL	9			/* Control */
#define	K_LAL	10			/* Left alt */
#define	K_RAL	11			/* Right alt */
#define	K_LCT	12			/* Left control */
#define	K_RCT	13			/* Right control */
#define	K_AGR	14			/* ALT-GR key  -- 102 keyboard only */
#define	K_FUNF	27			/* First function key */
#define	K_FUNL	122			/* Last function key */
#define	K_SRQ	123			/* System request */
#define	K_BRK	124			/* Break */
#define	K_ESN	125			/* <ESC> N <unalt'd value> sequence */
#define	K_ESO	126
#define	K_ESL	127
#define	K_RBT	128			/* Reboot system */
#define	K_DBG	129			/* Invoke debugger */
#define	K_NEXT	130
#define	K_PREV	131
#define	K_PFXF	192
#define	K_PFXL	255



/*
 * Macro for recognizing scan codes for special keys
 */
#define	IS_SPECKEY(k, s, i) \
	((k)->key[(s)].spcl & (0x80 >> (i)))	/* Special? */
/*
 * Function key constants and macros
 */
#define	IS_FUNKEY(c) \
	(((int)(c) >= K_FUNF) && \
	((int)(c) <= K_FUNL) || ((int)(c) >= K_PFXF) && \
	((int)(c) <= K_PFXL))	/* Function? */


/* Shorthand for constants so columns line up neatly */
#define	KF	K_FUNF			/* First function key */
#define	L_O	0			/* Key not affected by locks */
#define	L_C	KMF_CLOCK		/* Key affected by caps lock */
#define	L_N	KMF_NLOCK		/* Key affected by num lock */
#define	L_B	(KMF_CLOCK|KMF_NLOCK)	/* Key affected by caps and num lock */

/* definitions for ringing the bell */
#define	NORMBELL	1331	/* initial value loaded into timer */
#define	TONE_ON		3	/* 8254 gate 2 and speaker and-gate enabled */
#define	TIMER		0x40	/* 8254.2 timer address */
#define	TIMERCR		TIMER+3	/* timer control register address */
#define	TIMER2		TIMER+2	/* timer tone generation port */
#define	T_CTLWORD	0xB6	/* value for timer control word */
#define	TONE_CTL	0x61	/* address for enabling timer port 2 */

/* keyboard mode -- set by KBIO_MODE */
#define	KBM_XT	0	/* XT keyboard mode */
#define	KBM_AT	1	/* AT keyboard mode */

/* Enhanced Application Compatibility Support */

/* VP/ix keyboard types */
#define	KB_84		1
#define	KB_101		2
#define	KB_OTHER	3

#ifdef _KERNEL

typedef struct {
	unchar	kb_prevscan,	/* previous scancode */
		kb_old_scan;	/* scancode for autorepeat filtering */
	ushort	kb_state,	/* keyboard shift/ctrl/alt state */
		kb_sstate,	/* saved keyboard shift/ctrl/alt state */
		kb_sa_state,	/* stand alone keyboard shift/ctrl/alt state */
		kb_togls;	/* caps/num/scroll lock toggles state */
	int	kb_debugger_entered_through_kd;
} kbstate_t;

typedef unchar	esctbl_t[ESCTBLSIZ][2];

typedef struct wstation {
	kmutex_t	w_hw_mutex;	/* hardware mutex */
	int	w_init,		/* workstation has been initialized */
		w_flags;
	unchar	w_kbtype;
	queue_t	*w_qp;		/* pointer to queue for this minor device */
	int	w_kblayout;	/* keyboard layout code */
	dev_t	w_dev;		/* major/minor for this device */
} wstation_t;

#define	WS_KEYCLICK	0x01	/* keyclick on or off */
#define	WS_TONE		0x02	/* tone generator in use */


/*
 * This little gem is needed because kd is unsafe but tries to do spls when
 * being called from the debugger.  These flags are passed to kdkb_cmd()
 * to tell lower levels whether or not it is safe to do spls.
 */
#define	FROM_DRIVER	0
#define	FROM_DEBUGGER	1

#define	SEND2KBD(port, byte) { \
	while (inb(KB_STAT) & KBC_STAT_INBF) \
		; \
	outb(port, byte); \
}

extern void	kdkb_init(wstation_t *);
extern void	kdkb_type(wstation_t *);
extern int	kdkb_resend(unchar, unchar, unchar);
extern void	kdkb_cmd(register unchar, register unchar);
extern void	kdkb_sound(int);
extern void	kdkb_toneoff(caddr_t);
extern int	kdkb_mktone(queue_t *, mblk_t *, struct iocblk *, int,
				caddr_t);
extern void	kdkb_setled(kbstate_t *, unchar);
extern void	kdkb_keyclick();
extern void	kdkb_force_enable();
extern void	ws_iocack(queue_t *qp, mblk_t *mp, struct iocblk *iocp);
extern void	ws_iocnack(queue_t *qp, mblk_t *mp, struct iocblk *iocp,
		    int error);
extern int	ws_procscan(kbstate_t *kbp, unchar scan);
extern ushort	ws_scanchar(kbstate_t *kbp, unsigned char rawscan);
extern int	kd_xlate_at2xt(int c);
extern int	i8042_main_is_present(void);
#endif

#ifdef	__cplusplus
}
#endif

#endif /* _SYS_KD_H */
