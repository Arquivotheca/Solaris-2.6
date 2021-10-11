/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * Copyright (c) 1992 by Sun Microsystems, Inc.  All Rights Reserved.
 */

#pragma ident	"@(#)ws_tables.c	1.5	94/06/09 SMI"

#include "sys/types.h"
#include "sys/ascii.h"
#include "sys/termios.h"
#include "sys/stream.h"
#include "sys/strtty.h"
#include "sys/kd.h"

/*
 * This table is used to translate keyboard scan codes to ASCII character
 * sequences for the AT386 keyboard/display driver.  It is the default table,
 * and may be changed with system calls.
 */

keymap_t kdkeymap = { 0x80, {		/* Number of scan codes */
/*                                                         ALT    SPECIAL    */
/* SCAN                        CTRL          ALT    ALT    CTRL   FUNC       */
/* CODE   BASE   SHIFT  CTRL   SHIFT  ALT    SHIFT  CTRL   SHIFT  FLAGS LOCK */
/*  0*/ {{K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP },0xff, L_O },
/*  1*/ {{A_ESC, A_ESC, A_ESC, A_ESC, A_ESC, A_ESC, A_ESC, A_ESC },0x00, L_O },
/*  2*/ {{'1',   '!',   '1',   '1',   K_ESN, K_ESN, K_NOP, K_NOP },0x0f, L_O },
/*  3*/ {{'2',   '@',   '2',   A_NUL, K_ESN, K_ESN, K_NOP, K_NOP },0x0f, L_O },
/*  4*/ {{'3',   '#',   '3',   '3',   K_ESN, K_ESN, K_NOP, K_NOP },0x0f, L_O },
/*  5*/ {{'4',   '$',   '4',   '4',   K_ESN, K_ESN, K_NOP, K_NOP },0x0f, L_O },
/*  6*/ {{'5',   '%',   '5',   '5',   K_ESN, K_ESN, K_NOP, K_NOP },0x0f, L_O },
/*  7*/ {{'6',   '^',   '6',   A_RS,  K_ESN, K_ESN, K_NOP, K_NOP },0x0f, L_O },
/*  8*/ {{'7',   '&',   '7',   '7',   K_ESN, K_ESN, K_NOP, K_NOP },0x0f, L_O },
/*  9*/ {{'8',   '*',   '8',   '8',   K_ESN, K_ESN, K_NOP, K_NOP },0x0f, L_O },
/*  a*/ {{'9',   '(',   '9',   '9',   K_ESN, K_ESN, K_NOP, K_NOP },0x0f, L_O },
/*  b*/ {{'0',   ')',   '0',   '0',   K_ESN, K_ESN, K_NOP, K_NOP },0x0f, L_O },
/*  c*/ {{'-',   '_',   '-',   A_US,  K_ESN, K_ESN, K_NOP, K_NOP },0x0f, L_O },
/*  d*/ {{'=',   '+',   '=',   '=',   K_ESN, K_ESN, K_NOP, K_NOP },0x0f, L_O },
/*  e*/ {{A_BS,  A_BS,  A_BS,  A_BS,  A_BS,  A_BS,  A_BS,  A_BS  },0x00, L_O },
/*  f*/ {{A_HT,  A_GS,  A_HT,  A_GS,  A_HT,  A_GS,  A_HT,  A_GS  },0x00, L_O },
/* 10*/ {{'q',   'Q',   A_DC1, A_DC1, K_ESN, K_ESN, K_NOP, K_NOP },0x0f, L_C },
/* 11*/ {{'w',   'W',   A_ETB, A_ETB, K_ESN, K_ESN, K_NOP, K_NOP },0x0f, L_C },
/* 12*/ {{'e',   'E',   A_ENQ, A_ENQ, K_ESN, K_ESN, K_NOP, K_NOP },0x0f, L_C },
/* 13*/ {{'r',   'R',   A_DC2, A_DC2, K_ESN, K_ESN, K_NOP, K_NOP },0x0f, L_C },
/* 14*/ {{'t',   'T',   A_DC4, A_DC4, K_ESN, K_ESN, K_NOP, K_NOP },0x0f, L_C },
/* 15*/ {{'y',   'Y',   A_EM,  A_EM,  K_ESN, K_ESN, K_NOP, K_NOP },0x0f, L_C },
/* 16*/ {{'u',   'U',   A_NAK, A_NAK, K_ESN, K_ESN, K_NOP, K_NOP },0x0f, L_C },
/* 17*/ {{'i',   'I',   A_HT,  A_HT,  K_ESN, K_ESN, K_NOP, K_NOP },0x0f, L_C },
/* 18*/ {{'o',   'O',   A_SI,  A_SI,  K_ESN, K_ESN, K_NOP, K_NOP },0x0f, L_C },
/* 19*/ {{'p',   'P',   A_DLE, A_DLE, K_ESN, K_ESN, K_NOP, K_NOP },0x0f, L_C },
/* 1a*/ {{'[',   '{',   A_ESC, K_NOP, K_ESN, K_ESN, K_NOP, K_NOP },0x1f, L_O },
/* 1b*/ {{']',   '}',   A_GS,  K_NOP, K_ESN, K_ESN, K_NOP, K_NOP },0x1f, L_O },
/* 1c*/ {{A_CR,  A_CR,  A_CR,  A_CR,  A_CR,  A_CR,  A_CR,  A_CR  },0x00, L_O },
/* 1d*/ {{K_LCT, K_LCT, K_LCT, K_LCT, K_LCT, K_LCT, K_LCT, K_LCT },0xff, L_O },
/* 1e*/ {{'a',   'A',   A_SOH, A_SOH, K_ESN, K_ESN, K_NOP, K_NOP },0x0f, L_C },
/* 1f*/ {{'s',   'S',   A_DC3, A_DC3, K_ESN, K_ESN, K_NOP, K_NOP },0x0f, L_C },
/* 20*/ {{'d',   'D',   A_EOT, A_EOT, K_ESN, K_ESN, K_DBG, K_NOP },0x0f, L_C },
/* 21*/ {{'f',   'F',   A_ACK, A_ACK, K_ESN, K_ESN, K_NOP, K_NOP },0x0f, L_C },
/* 22*/ {{'g',   'G',   A_BEL, A_BEL, K_ESN, K_ESN, K_NOP, K_NOP },0x0f, L_C },
/* 23*/ {{'h',   'H',   A_BS,  A_BS,  K_ESN, K_ESN, K_NOP, K_NOP },0x0f, L_C },
/* 24*/ {{'j',   'J',   A_LF,  A_LF,  K_ESN, K_ESN, K_NOP, K_NOP },0x0f, L_C },
/* 25*/ {{'k',   'K',   A_VT,  A_VT,  K_ESN, K_ESN, K_NOP, K_NOP },0x0f, L_C },
/* 26*/ {{'l',   'L',   A_FF,  A_FF,  K_ESN, K_ESN, K_NOP, K_NOP },0x0f, L_C },
/* 27*/ {{';',   ':',   ';',   ':',   K_ESN, K_ESN, K_NOP, K_NOP },0x0f, L_O },
/* 28*/ {{'\'',  '"',   '\'',  '"',   K_ESN, K_ESN, K_NOP, K_NOP },0x0f, L_O },
/* 29*/ {{'`',   '~',   '`',   '~',   K_ESN, K_ESN, K_NOP, K_NOP },0x0f, L_O },
/* 2a*/ {{K_LSH, K_LSH, K_LSH, K_LSH, K_LSH, K_LSH, K_LSH, K_LSH },0xff, L_O },
/* 2b*/ {{'\\',  '|',   A_FS,  '|',   K_ESN, K_ESN, K_NOP, K_NOP },0x0f, L_O },
/* 2c*/ {{'z',   'Z',   A_SUB, A_SUB, K_ESN, K_ESN, K_NOP, K_NOP },0x0f, L_C },
/* 2d*/ {{'x',   'X',   A_CAN, A_CAN, K_ESN, K_ESN, K_NOP, K_NOP },0x0f, L_C },
/* 2e*/ {{'c',   'C',   A_ETX, A_ETX, K_ESN, K_ESN, K_NOP, K_NOP },0x0f, L_C },
/* 2f*/ {{'v',   'V',   A_SYN, A_SYN, K_ESN, K_ESN, K_NOP, K_NOP },0x0f, L_C },
/* 30*/ {{'b',   'B',   A_STX, A_STX, K_ESN, K_ESN, K_NOP, K_NOP },0x0f, L_C },
/* 31*/ {{'n',   'N',   A_SO,  A_SO,  K_ESN, K_ESN, K_NOP, K_NOP },0x0f, L_C },
/* 32*/ {{'m',   'M',   A_CR,  A_CR,  K_ESN, K_ESN, K_NOP, K_NOP },0x0f, L_C },
/* 33*/ {{',',   '<',   ',',   '<',   K_ESN, K_ESN, K_NOP, K_NOP },0x0f, L_O },
/* 34*/ {{'.',   '>',   '.',   '>',   K_ESN, K_ESN, K_NOP, K_NOP },0x0f, L_O },
/* 35*/ {{'/',   '?',   '/',   A_US,  K_ESN, K_ESN, K_NOP, K_NOP },0x0f, L_O },
/* 36*/ {{K_RSH, K_RSH, K_RSH, K_RSH, K_RSH, K_RSH, K_RSH, K_RSH },0xff, L_O },
/* 37*/ {{'*',   '*',   '*',   '*'  , K_ESN, K_ESN, K_NOP, K_NOP },0x0f, L_O },
/* 38*/ {{K_LAL, K_LAL, K_LAL, K_LAL, K_LAL, K_LAL, K_LAL, K_LAL },0xff, L_O },
/* 39*/ {{' ',   ' ',   A_NUL, A_NUL, K_ESN, K_ESN, K_NOP, K_NOP },0x0f, L_O },
/* 3a*/ {{K_CLK, K_CLK, K_CLK, K_CLK, K_CLK, K_CLK, K_CLK, K_CLK },0xff, L_O },
/* 3b*/ {{KF+0,  KF+12, KF+24, KF+36, KF+0, KF+12, KF+24, KF+36 },0xff, L_O },
/* 3c*/ {{KF+1,  KF+13, KF+25, KF+37, KF+1, KF+13, KF+25, KF+37 },0xff, L_O },
/* 3d*/ {{KF+2,  KF+14, KF+26, KF+38, KF+2, KF+14, KF+26, KF+38 },0xff, L_O },
/* 3e*/ {{KF+3,  KF+15, KF+27, KF+39, KF+3, KF+15, KF+27, KF+39 },0xff, L_O },
/* 3f*/ {{KF+4,  KF+16, KF+28, KF+40, KF+4, KF+16, KF+28, KF+40 },0xff, L_O },
/* 40*/ {{KF+5,  KF+17, KF+29, KF+41, KF+5, KF+17, KF+29, KF+41 },0xff, L_O },
/* 41*/ {{KF+6,  KF+18, KF+30, KF+42, KF+6, KF+18, KF+30, KF+42 },0xff, L_O },
/* 42*/ {{KF+7,  KF+19, KF+31, KF+43, KF+7, KF+19, KF+31, KF+43 },0xff, L_O },
/* 43*/ {{KF+8,  KF+20, KF+32, KF+44, KF+8, KF+20, KF+32, KF+44 },0xff, L_O },
/* 44*/ {{KF+9,  KF+21, KF+33, KF+45, KF+9, KF+21, KF+33, KF+45 },0xff, L_O },
/* 45*/ {{K_NLK, K_NLK, K_NLK, K_NLK, K_NLK, K_NLK, K_NLK, K_NLK },0xff, L_O },
/* 46*/ {{K_SLK, K_SLK, K_SLK, K_SLK, K_SLK, K_SLK, K_SLK, K_SLK },0xff, L_O },
/* 47*/ {{KF+48, '7',   KF+48, '7'  , KF+48, K_ESN, K_NOP, K_NOP },0xaf, L_N },
/* 48*/ {{KF+49, '8',   KF+49, '8'  , KF+49, K_ESN, K_NOP, K_NOP },0xaf, L_N },
/* 49*/ {{KF+50, '9',   KF+50, '9'  , KF+50, K_ESN, K_NOP, K_NOP },0xaf, L_N },
/* 4a*/ {{KF+51, '-',   KF+51, '-'  , KF+51, K_ESN, K_NOP, K_NOP },0xaf, L_N },
/* 4b*/ {{KF+52, '4',   KF+52, '4'  , KF+52, K_ESN, K_NOP, K_NOP },0xaf, L_N },
/* 4c*/ {{KF+53, '5',   KF+53, '5'  , KF+53, K_ESN, K_NOP, K_NOP },0xaf, L_N },
/* 4d*/ {{KF+54, '6',   KF+54, '6'  , KF+54, K_ESN, K_NOP, K_NOP },0xaf, L_N },
/* 4e*/ {{KF+55, '+',   KF+55, '+'  , KF+55, K_ESN, K_NOP, K_NOP },0xaf, L_N },
/* 4f*/ {{KF+56, '1',   KF+56, '1'  , KF+56, K_ESN, K_NOP, K_NOP },0xaf, L_N },
/* 50*/ {{KF+57, '2',   KF+57, '2'  , KF+57, K_ESN, K_NOP, K_NOP },0xaf, L_N },
/* 51*/ {{KF+58, '3',   KF+58, '3'  , KF+58, K_ESN, K_NOP, K_NOP },0xaf, L_N },
/* 52*/ {{KF+59, '0',   KF+59, '0'  , KF+59, K_ESN, K_NOP, K_NOP },0xaf, L_N },
/* 53*/ {{A_DEL, '.',   A_DEL, '.'  , A_DEL, K_ESN, K_RBT, K_NOP },0x07, L_N },
/* 54*/ {{KF+59, KF+25, KF+59, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP },0xff, L_O },
/* 55*/ {{KF+57, KF+57, KF+57, KF+57, KF+57, KF+57, KF+57, KF+57 },0xff, L_O },
/* 56*/ {{KF+52, KF+52, KF+52, KF+52, KF+52, KF+52, KF+52, KF+52 },0xff, L_O },
/* 57*/ {{KF+10, KF+22, KF+34, KF+46, KF+10, KF+22, KF+34, KF+46 },0xff, L_O },
/* 58*/ {{KF+11, KF+23, KF+35, KF+47, KF+11, KF+23, KF+35, KF+47 },0xff, L_O },
/* 59*/ {{K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP },0xff, L_O },
/* 5a*/ {{K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP },0xff, L_O },
/* 5b*/ {{K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP },0xff, L_O },
/* 5c*/ {{K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP },0xff, L_O },
/* 5d*/ {{K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP },0xff, L_O },
/* 5e*/ {{K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP },0xff, L_O },
/* 5f*/ {{K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP },0xff, L_O },
/* 60*/ {{K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP },0xff, L_O },
/* 61*/ {{K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP },0xff, L_O },
/* 62*/ {{K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP },0xff, L_O },
/* 63*/ {{K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP },0xff, L_O },
/* 64*/ {{K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP },0xff, L_O },
/* 65*/ {{K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP },0xff, L_O },
/* 66*/ {{K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP },0xff, L_O },
/* 67*/ {{K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP },0xff, L_O },
/* 68*/ {{K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP },0xff, L_O },
/* 69*/ {{K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP },0xff, L_O },
/* 6a*/ {{K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP },0xff, L_O },
/* 6b*/ {{KF+52, KF+52, KF+52, KF+52, KF+52, KF+52, KF+52, KF+52 },0xff, L_O },
/* 6c*/ {{K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP },0xff, L_O },
/* 6d*/ {{K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP },0xff, L_O },
/* 6e*/ {{K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP },0xff, L_O },
/* 6f*/ {{KF+50, KF+50, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP },0xff, L_O },
/* 70*/ {{' ',   ' ',   A_NUL, A_NUL, K_ESN, K_ESN, K_NOP, K_NOP },0x0f, L_O },
/* 71*/ {{K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP },0xff, L_O },
/* 72*/ {{K_RAL, K_RAL, K_RAL, K_RAL, K_RAL, K_RAL, K_RAL, K_RAL },0xff, L_O },
/* 73*/ {{K_RCT, K_RCT, K_RCT, K_RCT, K_RCT, K_RCT, K_RCT, K_RCT },0xff, L_O },
/* 74*/ {{A_NL,  A_NL,  A_NL,  A_NL,  A_NL,  A_NL,  A_NL,  A_NL  },0x00, L_O },
/* 75*/ {{'/',   '/',   K_NOP, K_NOP, K_ESN, K_ESN, K_NOP, K_NOP },0x3f, L_O },
/* 76*/ {{K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP },0xff, L_O },
/* 77*/ {{K_SLK, K_SLK, K_BRK, K_BRK, K_SLK, K_SLK, K_NOP, K_NOP },0xff, L_O },
/* 78*/ {{KF+49, KF+49, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP },0xff, L_O },
/* 79*/ {{A_DEL, A_DEL, A_DEL, A_DEL, A_DEL, A_DEL, A_DEL, A_DEL },0x0f, L_O },
/* 7a*/ {{KF+56, KF+56, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP },0xff, L_O },
/* 7b*/ {{' ', ' ', K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP },0x0f, L_O },
/* 7c*/ {{K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP },0xff, L_O },
/* 7d*/ {{'\\', '|', K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP },0x0f, L_O },
/* 7e*/ {{KF+58, KF+58, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP },0xff, L_O },
/* 7f*/ {{KF+48, KF+48, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP },0xff, L_O }
}};

/*
 * Table to translate 0xe0 prefixed scan codes to proper table indices
 */

esctbl_t kdesctbl = {
	0x1c, 0x74,	/* enter key */
	0x1d, 0x73,	/* right control key */
	0x2a, 0x00,	/* map to no key stroke */
	0x35, 0x75,	/* keypad '/' key */
	0x36, 0x00,	/* map to no key stroke */
	0x37, 0x54,	/* print screen key */
	0x38, 0x72,	/* right alt key */
	0x46, 0x77,	/* pause/break key */
	0x47, 0x7f,	/* home key */
	0x48, 0x78,	/* up arrow key */
	0x49, 0x6f,	/* page up key */
	0x4b, 0x6b,	/* left arrow key */
	0x4d, 0x7d,	/* right arrow key */
	0x4f, 0x7a,	/* end key */
	0x50, 0x55,	/* down arrow key */
	0x51, 0x7e,	/* page down key */
	0x52, 0x7b,	/* insert key */
	0x53, 0x79,	/* delete key */
};

ushort kb_shifttab[] = {	/* Translate shifts for kb_state */
	0, 0, LEFT_SHIFT, RIGHT_SHIFT, CAPS_LOCK, NUM_LOCK, SCROLL_LOCK,
	ALTSET, 0, CTRLSET, LEFT_ALT, RIGHT_ALT, LEFT_CTRL, RIGHT_CTRL
};
