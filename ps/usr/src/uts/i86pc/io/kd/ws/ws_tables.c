/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved					*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF		*/
/*	UNIX System Laboratories, Inc.				*/
/*	The copyright notice above does not evidence any	*/
/*	actual or intended publication of such source code.	*/

/*
 * Copyright (c) 1996 by Sun Microsystems, Inc.  All Rights Reserved.
 */

#pragma	ident "@(#)ws_tables.c	1.6	96/06/07 SMI"

#include "sys/param.h"
#include "sys/types.h"
#include "sys/sysmacros.h"
#include "sys/at_ansi.h"
#include "sys/ascii.h"
#include "sys/termios.h"
#include "sys/stream.h"
#include "sys/proc.h"
#include "sys/kd.h"
#include "sys/strtty.h"
#include "sys/stropts.h"
#include "sys/ws/ws.h"

/* prototypes for functions in this file */
int xlate_keymap(keymap_t *, keymap_t *, int);


/*
 * This table is used to translate keyboard scan codes to ASCII character
 * sequences for the AT386 keyboard/display driver.  It is the default table,
 * and may be changed with system calls.
 */

pfxstate_t kdpfxstr = {0};

keymap_t kdkeymap = { 0x80, {		/* Number of scan codes */
/*							   ALT    SPECIAL    */
/* SCAN			       CTRL	     ALT    ALT    CTRL   FUNC       */
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
/* 1c*/ {{A_CR,  A_CR,  KF+91, KF+91, K_ESN, K_ESN, KF+92, KF+92 },0x3f, L_O },
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
/* 2b*/ {{'\\',  '|',   A_FS,  '|',   KF+93, KF+93, K_NOP, K_NOP },0x0f, L_O },
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
/* 3b*/ {{KF+0,  KF+12, KF+24, KF+36, KF+61, KF+73, KF+24, KF+36 },0xff, L_O },
/* 3c*/ {{KF+1,  KF+13, KF+25, KF+37, KF+62, KF+74, KF+25, KF+37 },0xff, L_O },
/* 3d*/ {{KF+2,  KF+14, KF+26, KF+38, KF+63, KF+75, KF+26, KF+38 },0xff, L_O },
/* 3e*/ {{KF+3,  KF+15, KF+27, KF+39, KF+64, KF+76, KF+27, KF+39 },0xff, L_O },
/* 3f*/ {{KF+4,  KF+16, KF+28, KF+40, KF+65, KF+77, KF+28, KF+40 },0xff, L_O },
/* 40*/ {{KF+5,  KF+17, KF+29, KF+41, KF+66, KF+78, KF+29, KF+41 },0xff, L_O },
/* 41*/ {{KF+6,  KF+18, KF+30, KF+42, KF+67, KF+79, KF+30, KF+42 },0xff, L_O },
/* 42*/ {{KF+7,  KF+19, KF+31, KF+43, KF+68, KF+80, KF+31, KF+43 },0xff, L_O },
/* 43*/ {{KF+8,  KF+20, KF+32, KF+44, KF+69, KF+81, KF+32, KF+44 },0xff, L_O },
/* 44*/ {{KF+9,  KF+21, KF+33, KF+45, KF+70, KF+82, KF+33, KF+45 },0xff, L_O },
/* 45*/ {{K_NLK, K_NLK, K_NLK, K_NLK, K_NLK, K_NLK, K_NLK, K_NLK },0xff, L_O },
/* 46*/ {{K_SLK, K_SLK, K_SLK, K_SLK, K_SLK, K_SLK, K_SLK, K_SLK },0xff, L_O },
/* 47*/ {{KF+48, '7',   KF+85, '7'  , KF+48, K_ESN, K_NOP, K_NOP },0xaf, L_N },
/* 48*/ {{KF+49, '8',   KF+49, '8'  , KF+49, K_ESN, K_NOP, K_NOP },0xaf, L_N },
/* 49*/ {{KF+50, '9',   KF+86, '9'  , KF+50, K_ESN, K_NOP, K_NOP },0xaf, L_N },
/* 4a*/ {{KF+51, '-',   KF+51, '-'  , KF+51, K_ESN, K_NOP, K_NOP },0xaf, L_N },
/* 4b*/ {{KF+52, '4',   KF+87, '4'  , KF+52, K_ESN, K_NOP, K_NOP },0xaf, L_N },
/* 4c*/ {{KF+53, '5',   KF+53, '5'  , KF+53, K_ESN, K_NOP, K_NOP },0xaf, L_N },
/* 4d*/ {{KF+54, '6',   KF+88, '6'  , KF+54, K_ESN, K_NOP, K_NOP },0xaf, L_N },
/* 4e*/ {{KF+55, '+',   KF+55, '+'  , KF+55, K_ESN, K_NOP, K_NOP },0xaf, L_N },
/* 4f*/ {{KF+56, '1',   KF+89, '1'  , KF+56, K_ESN, K_NOP, K_NOP },0xaf, L_N },
/* 50*/ {{KF+57, '2',   KF+57, '2'  , KF+57, K_ESN, K_NOP, K_NOP },0xaf, L_N },
/* 51*/ {{KF+58, '3',   KF+90, '3'  , KF+58, K_ESN, K_NOP, K_NOP },0xaf, L_N },
/* 52*/ {{KF+59, '0',   KF+59, '0'  , KF+59, K_ESN, K_NOP, K_NOP },0xaf, L_N },
/* 53*/ {{A_DEL, '.',   A_DEL, '.'  , A_DEL, K_ESN, K_RBT, K_NOP },0x07, L_N },
/* 54*/ {{KF+59, KF+25, KF+59, K_NOP, K_SRQ, K_SRQ, K_SRQ, K_SRQ },0xff, L_O },
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
/* 67*/ {{K_RCT, K_RCT, K_RCT, K_RCT, K_RCT, K_RCT, K_RCT, K_RCT },0xff, L_O },
/* 68*/ {{A_DEL, A_DEL, A_DEL, A_DEL, A_DEL, A_DEL, A_DEL, A_DEL },0x0f, L_O },
/* 69*/ {{KF+59, KF+59, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP },0xff, L_O },
/* 6a*/ {{KF+54, KF+54, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP },0xff, L_O },
/* 6b*/ {{KF+52, KF+52, KF+52, KF+52, KF+52, KF+52, KF+52, KF+52 },0xff, L_O },
/* 6c*/ {{K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP },0xff, L_O },
/* 6d*/ {{K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP },0xff, L_O },
/* 6e*/ {{K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP },0xff, L_O },
/* 6f*/ {{KF+50, KF+50, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP },0xff, L_O },
/* 70*/ {{' ',   ' ',   A_NUL, A_NUL, K_ESN, K_ESN, K_NOP, K_NOP },0x0f, L_O },
/* 71*/ {{K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP },0xff, L_O },
/* 72*/ {{K_RAL, K_RAL, K_RAL, K_RAL, K_RAL, K_RAL, K_RAL, K_RAL },0xff, L_O },
/* 73*/ {{K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP },0xff, L_O },
/* 74*/ {{A_NL,  A_NL,  A_NL,  A_NL,  A_NL,  A_NL,  A_NL,  A_NL  },0x00, L_O },
/* 75*/ {{'/',   '/',   K_NOP, K_NOP, K_ESN, K_ESN, K_NOP, K_NOP },0x3f, L_O },
/* 76*/ {{K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP },0xff, L_O },
/* 77*/ {{K_SLK, K_SLK, K_BRK, K_BRK, K_SLK, K_SLK, K_NOP, K_NOP },0xff, L_O },
/* 78*/ {{KF+49, KF+49, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP },0xff, L_O },
/* 79*/ {{K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP },0xff, L_O },
/* 7a*/ {{KF+56, KF+56, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP },0xff, L_O },
/* 7b*/ {{K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP },0xff, L_O },
/* 7c*/ {{K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP },0xff, L_O },
/* 7d*/ {{K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP },0xff, L_O },
/* 7e*/ {{KF+58, KF+58, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP },0xff, L_O },
/* 7f*/ {{KF+48, KF+48, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP },0xff, L_O }
}};

/*
 * Extended code support
 */

extkeys_t ext_keys = {
/* SCAN						*/
/* CODE       BASE    SHIFT   CTRL    ALT	*/
/*   0 */  {  K_NOP,  K_NOP,  K_NOP,  K_NOP	},
/*   1 */  {  K_NOP,  K_NOP,  K_NOP,  K_NOP	},
/*   2 */  {  K_NOP,  K_NOP,  K_NOP,  0x78	},
/*   3 */  {  K_NOP,  K_NOP,  K_NOP,  0x79	},
/*   4 */  {  K_NOP,  K_NOP,  K_NOP,  0x7a	},
/*   5 */  {  K_NOP,  K_NOP,  K_NOP,  0x7b	},
/*   6 */  {  K_NOP,  K_NOP,  K_NOP,  0x7c	},
/*   7 */  {  K_NOP,  K_NOP,  K_NOP,  0x7d	},
/*   8 */  {  K_NOP,  K_NOP,  K_NOP,  0x7e	},
/*   9 */  {  K_NOP,  K_NOP,  K_NOP,  0x7f	},
/*  10 */  {  K_NOP,  K_NOP,  K_NOP,  0x80	},
/*  11 */  {  K_NOP,  K_NOP,  K_NOP,  0x81	},
/*  12 */  {  K_NOP,  K_NOP,  K_NOP,  0x82	},
/*  13 */  {  K_NOP,  K_NOP,  K_NOP,  0x83	},
/*  14 */  {  K_NOP,  K_NOP,  K_NOP,  K_NOP	},
/*  15 */  {  K_NOP,  0xf,    K_NOP,  K_NOP	},
/*  16 */  {  K_NOP,  K_NOP,  K_NOP,  0x10	},
/*  17 */  {  K_NOP,  K_NOP,  K_NOP,  0x11	},
/*  18 */  {  K_NOP,  K_NOP,  K_NOP,  0x12	},
/*  19 */  {  K_NOP,  K_NOP,  K_NOP,  0x13	},
/*  20 */  {  K_NOP,  K_NOP,  K_NOP,  0x14	},
/*  21 */  {  K_NOP,  K_NOP,  K_NOP,  0x15	},
/*  22 */  {  K_NOP,  K_NOP,  K_NOP,  0x16	},
/*  23 */  {  K_NOP,  K_NOP,  K_NOP,  0x17	},
/*  24 */  {  K_NOP,  K_NOP,  K_NOP,  0x18	},
/*  25 */  {  K_NOP,  K_NOP,  K_NOP,  0x19	},
/*  26 */  {  K_NOP,  K_NOP,  K_NOP,  K_NOP	},
/*  27 */  {  K_NOP,  K_NOP,  K_NOP,  K_NOP	},
/*  28 */  {  K_NOP,  K_NOP,  K_NOP,  K_NOP	},
/*  29 */  {  K_NOP,  K_NOP,  K_NOP,  K_NOP	},
/*  30 */  {  K_NOP,  K_NOP,  K_NOP,  0x1e	},
/*  31 */  {  K_NOP,  K_NOP,  K_NOP,  0x1f	},
/*  32 */  {  K_NOP,  K_NOP,  K_NOP,  0x20	},
/*  33 */  {  K_NOP,  K_NOP,  K_NOP,  0x21	},
/*  34 */  {  K_NOP,  K_NOP,  K_NOP,  0x22	},
/*  35 */  {  K_NOP,  K_NOP,  K_NOP,  0x23	},
/*  36 */  {  K_NOP,  K_NOP,  K_NOP,  0x24	},
/*  37 */  {  K_NOP,  K_NOP,  K_NOP,  0x25	},
/*  38 */  {  K_NOP,  K_NOP,  K_NOP,  0x26	},
/*  39 */  {  K_NOP,  K_NOP,  K_NOP,  K_NOP	},
/*  40 */  {  K_NOP,  K_NOP,  K_NOP,  K_NOP	},
/*  41 */  {  K_NOP,  K_NOP,  K_NOP,  K_NOP	},
/*  42 */  {  K_NOP,  K_NOP,  K_NOP,  K_NOP	},
/*  43 */  {  K_NOP,  K_NOP,  K_NOP,  K_NOP	},
/*  44 */  {  K_NOP,  K_NOP,  K_NOP,  0x2c	},
/*  45 */  {  K_NOP,  K_NOP,  K_NOP,  0x2d	},
/*  46 */  {  K_NOP,  K_NOP,  K_NOP,  0x2e	},
/*  47 */  {  K_NOP,  K_NOP,  K_NOP,  0x2f	},
/*  48 */  {  K_NOP,  K_NOP,  K_NOP,  0x30	},
/*  49 */  {  K_NOP,  K_NOP,  K_NOP,  0x31	},
/*  50 */  {  K_NOP,  K_NOP,  K_NOP,  0x32	},
/*  51 */  {  K_NOP,  K_NOP,  K_NOP,  K_NOP	},
/*  52 */  {  K_NOP,  K_NOP,  K_NOP,  K_NOP	},
/*  53 */  {  K_NOP,  K_NOP,  K_NOP,  K_NOP	},
/*  54 */  {  K_NOP,  K_NOP,  K_NOP,  K_NOP	},
/*  55 */  {  K_NOP,  K_NOP,  0x72,   K_NOP	},
/*  56 */  {  K_NOP,  K_NOP,  K_NOP,  K_NOP	},
/*  57 */  {  K_NOP,  K_NOP,  K_NOP,  K_NOP	},
/*  58 */  {  K_NOP,  K_NOP,  K_NOP,  K_NOP	},
/*  59 */  {  0x3b,   0x54,   0x5e,   0x68	}, /* f1, f13, f25, f37 */
/*  60 */  {  0x3c,   0x55,   0x5f,   0x69	}, /* f2, f14, f26, f38 */
/*  61 */  {  0x3d,   0x56,   0x60,   0x6a	}, /* f3, f15, f27, f39 */
/*  62 */  {  0x3e,   0x57,   0x61,   0x6b	}, /* f4, f16, f28, f40 */
/*  63 */  {  0x3f,   0x58,   0x62,   0x6c	}, /* f5, f17, f29, f41 */
/*  64 */  {  0x40,   0x59,   0x63,   0x6d	}, /* f6, f18, f30, f42 */
/*  65 */  {  0x41,   0x5a,   0x64,   0x6e	}, /* f7, f19, f31, f43 */
/*  66 */  {  0x42,   0x5b,   0x65,   0x6f	}, /* f8, f20, f32, f44 */
/*  67 */  {  0x43,   0x5c,   0x66,   0x70	}, /* f9, f21, f33, f45 */
/*  68 */  {  0x44,   0x5d,   0x67,   0x71	}, /* f10,f22, f34, f46 */
/*  69 */  {  K_NOP,  K_NOP,  K_NOP,  K_NOP	},
/*  70 */  {  K_NOP,  K_NOP,  0x00,   K_NOP	},
/*  71 */  {  0x47,   K_NOP,  0x77,   0x7	},
/*  72 */  {  0x48,   K_NOP,  K_NOP,  0x8	},
/*  73 */  {  0x49,   K_NOP,  0x84,   0x9	},
/*  74 */  {  K_NOP,  K_NOP,  K_NOP,  K_NOP	},
/*  75 */  {  0x4b,   K_NOP,  0x73,   0x4	},
/*  76 */  {  K_NOP,  K_NOP,  K_NOP,  0x5	},
/*  77 */  {  0x4d,   K_NOP,  0x74,   0x6	},
/*  78 */  {  K_NOP,  K_NOP,  K_NOP,  K_NOP	},
/*  79 */  {  0x4f,   K_NOP,  0x75,   0x1	},
/*  80 */  {  0x50,   K_NOP,  K_NOP,  0x2	},
/*  81 */  {  0x51,   K_NOP,  0x76,   0x3	},
/*  82 */  {  0x52,   K_NOP,  K_NOP,  0xb0	},
/*  83 */  {  0x53,   K_NOP,  K_NOP,  K_NOP	},
/*  84 */  {  K_NOP,  K_NOP,  K_NOP,  K_NOP	},
/*  85 */  {  0x85,   0x87,   0x89,   0x8b	}, /* f11, f23, f35, f47 */
/*  86 */  {  0x86,   0x88,   0x8a,   0x8c	}  /* f12, f24, f36, f48 */
};

/*
 * Table to translate 0xe0 prefixed scan codes to proper table indices
 */

esctbl_t kdesctbl = {
	0x1c, 0x74,	/* enter key */
	0x1d, 0x67,	/* right control key */
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
	0x4d, 0x6a,	/* right arrow key */
	0x4f, 0x7a,	/* end key */
	0x50, 0x55,	/* down arrow key */
	0x51, 0x7e,	/* page down key */
	0x52, 0x69,	/* insert key */
	0x53, 0x68,	/* delete key */
};

/*
 * Function key mapping table.  The function key values can be any length, as
 * long as the total length of all the strings is <= STRTABLN, including NULLs.
 */

strmap_t kdstrbuf = {
/* Base function keys 1-12 */
/*  0 */	'\033', 'O', 'P', '\0',
/*  1 */	'\033', 'O', 'Q', '\0',
/*  2 */	'\033', 'O', 'R', '\0',
/*  3 */	'\033', 'O', 'S', '\0',
/*  4 */	'\033', 'O', 'T', '\0',
/*  5 */	'\033', 'O', 'U', '\0',
/*  6 */	'\033', 'O', 'V', '\0',
/*  7 */	'\033', 'O', 'W', '\0',
/*  8 */	'\033', 'O', 'X', '\0',
/*  9 */	'\033', 'O', 'Y', '\0',
/* 10 */	'\033', 'O', 'Z', '\0',
/* 11 */	'\033', 'O', 'A', '\0',
/* Shift function keys 1-12 */
/* 12 */	'\033', 'O', 'p', '\0',
/* 13 */	'\033', 'O', 'q', '\0',
/* 14 */	'\033', 'O', 'r', '\0',
/* 15 */	'\033', 'O', 's', '\0',
/* 16 */	'\033', 'O', 't', '\0',
/* 17 */	'\033', 'O', 'u', '\0',
/* 18 */	'\033', 'O', 'v', '\0',
/* 19 */	'\033', 'O', 'w', '\0',
/* 20 */	'\033', 'O', 'x', '\0',
/* 21 */	'\033', 'O', 'y', '\0',
/* 22 */	'\033', 'O', 'z', '\0',
/* 23 */	'\033', 'O', 'a', '\0',
/* Ctrl function keys 1-12 */
/* 24 */	'\033', 'O', 'P', '\0',
/* 25 */	'\033', 'O', 'Q', '\0',
/* 26 */	'\033', 'O', 'R', '\0',
/* 27 */	'\033', 'O', 'S', '\0',
/* 28 */	'\033', 'O', 'T', '\0',
/* 29 */	'\033', 'O', 'U', '\0',
/* 30 */	'\033', 'O', 'V', '\0',
/* 31 */	'\033', 'O', 'W', '\0',
/* 32 */	'\033', 'O', 'X', '\0',
/* 33 */	'\033', 'O', 'Y', '\0',
/* 34 */	'\033', 'O', 'Z', '\0',
/* 35 */	'\033', 'O', 'A', '\0',
/* Ctrl-shift function keys 1-12 */
/* The above comment was "Alt function keys 1-12" in M0 */
/* 36 */	'\033', 'O', 'p', '\0',
/* 37 */	'\033', 'O', 'q', '\0',
/* 38 */	'\033', 'O', 'r', '\0',
/* 39 */	'\033', 'O', 's', '\0',
/* 40 */	'\033', 'O', 't', '\0',
/* 41 */	'\033', 'O', 'u', '\0',
/* 42 */	'\033', 'O', 'v', '\0',
/* 43 */	'\033', 'O', 'w', '\0',
/* 44 */	'\033', 'O', 'x', '\0',
/* 45 */	'\033', 'O', 'y', '\0',
/* 46 */	'\033', 'O', 'z', '\0',
/* 47 */	'\033', 'O', 'a', '\0',
/* Keypad */
/* 48 */	'\033', '[', 'H', '\0',	/* 7 : Home */
/* 49 */	'\033', '[', 'A', '\0',	/* 8 : Up Arrow */
/* 50 */	'\033', '[', 'V', '\0',	/* 9 : Page Up */
/* 51 */	'\033', '[', 'S', '\0',	/* - : */
/* 52 */	'\033', '[', 'D', '\0',	/* 4 : Left Arrow */
/* 53 */	'\033', '[', 'G', '\0',	/* 5 : */
/* 54 */	'\033', '[', 'C', '\0',	/* 6 : Right Arrow */
/* 55 */	'\033', '[', 'T', '\0',	/* + : */
/* 56 */	'\033', '[', 'Y', '\0',	/* 1 : End */
/* 57 */	'\033', '[', 'B', '\0',	/* 2 : Down Arrow */
/* 58 */	'\033', '[', 'U', '\0',	/* 3 : Page Down */
/* 59 */	'\033', '[', '@', '\0',	/* 0 : Insert */
/* Extra string */
/* 60 */	'\033', '[', '2', '\0',	/* Insert on 101/102 key keyboard */
/* Alt function keys 1-12 */
/* 61 */	'\033', 'a', '\033', 'O', 'P', '\033', 'A', '\0',
/* 62 */	'\033', 'a', '\033', 'O', 'Q', '\033', 'A', '\0',
/* 63 */	'\033', 'a', '\033', 'O', 'R', '\033', 'A', '\0',
/* 64 */	'\033', 'a', '\033', 'O', 'S', '\033', 'A', '\0',
/* 65 */	'\033', 'a', '\033', 'O', 'T', '\033', 'A', '\0',
/* 66 */	'\033', 'a', '\033', 'O', 'U', '\033', 'A', '\0',
/* 67 */	'\033', 'a', '\033', 'O', 'V', '\033', 'A', '\0',
/* 68 */	'\033', 'a', '\033', 'O', 'W', '\033', 'A', '\0',
/* 69 */	'\033', 'a', '\033', 'O', 'X', '\033', 'A', '\0',
/* 70 */	'\033', 'a', '\033', 'O', 'Y', '\033', 'A', '\0',
/* 71 */	'\033', 'a', '\033', 'O', 'Z', '\033', 'A', '\0',
/* 72 */	'\033', 'a', '\033', 'O', 'A', '\033', 'A', '\0',
/* Alt-shift function keys 1-12 */
/* 73 */	'\033', 'a', '\033', 'O', 'p', '\033', 'A', '\0',
/* 74 */	'\033', 'a', '\033', 'O', 'q', '\033', 'A', '\0',
/* 75 */	'\033', 'a', '\033', 'O', 'r', '\033', 'A', '\0',
/* 76 */	'\033', 'a', '\033', 'O', 's', '\033', 'A', '\0',
/* 77 */	'\033', 'a', '\033', 'O', 't', '\033', 'A', '\0',
/* 78 */	'\033', 'a', '\033', 'O', 'u', '\033', 'A', '\0',
/* 79 */	'\033', 'a', '\033', 'O', 'v', '\033', 'A', '\0',
/* 80 */	'\033', 'a', '\033', 'O', 'w', '\033', 'A', '\0',
/* 81 */	'\033', 'a', '\033', 'O', 'x', '\033', 'A', '\0',
/* 82 */	'\033', 'a', '\033', 'O', 'y', '\033', 'A', '\0',
/* 83 */	'\033', 'a', '\033', 'O', 'z', '\033', 'A', '\0',
/* 84 */	'\033', 'a', '\033', 'O', 'a', '\033', 'A', '\0',
/* Ctrl cursor keys */
/* 85 */	'\033', 'c', '\033', '[', 'H', '\033', 'C', '\0', /* Home */
/* 86 */	'\033', 'c', '\033', '[', 'V', '\033', 'C', '\0', /* Pg Up */
/* 87 */	'\033', 'c', '\033', '[', 'D', '\033', 'C', '\0', /* Left <- */
/* 88 */	'\033', 'c', '\033', '[', 'C', '\033', 'C', '\0', /* Right -> */
/* 89 */	'\033', 'c', '\033', '[', 'Y', '\033', 'C', '\0', /* End */
/* 90 */	'\033', 'c', '\033', '[', 'U', '\033', 'C', '\0', /* Pg Dn */
/* Ctrl Enter key */
/* 91 Ctrl  */	'\033', 'c', A_CR, '\033', 'C', '\0',
/* 92 CtrlAlt */ '\033', 'c', '\033', 'a', A_CR, '\033', 'A', '\033', 'C', '\0',
/* Alt \ key */
/* 93	    */	'\033', 'a', '\\', '\\', '\033', 'A', '\0',
/* Ctrl-Alt \ key */
#ifdef	DOESNT_FIT
/* 94	    */	'\033', 'c', '\033', 'a', '\\', '\\', '\033', 'A', '\033', 'C', '\0',
#endif
};

/*
 * System request mapping table
 */
srqtab_t srqtab = {
/*  0*/ K_NOP,
/*  1*/ K_NOP,
/*  2*/ K_NOP,
/*  3*/ K_NOP,
/*  4*/ K_NOP,
/*  5*/ K_NOP,
/*  6*/ K_NOP,
/*  7*/ K_NOP,
/*  8*/ K_NOP,
/*  9*/ K_NOP,
/*  a*/ K_NOP,
/*  b*/ K_NOP,
/*  c*/ K_NOP,
/*  d*/ K_NOP,
/*  e*/ K_NOP,
/*  f*/ K_NOP,
/* 10*/ K_NOP,
/* 11*/ K_NOP,
/* 12*/ K_NOP,
/* 13*/ K_NOP,
/* 14*/ K_NOP,
/* 15*/ K_NOP,
/* 16*/ K_NOP,
/* 17*/ K_NOP,
/* 18*/ K_NOP,
/* 19*/ K_PREV,		/* P */
/* 1a*/ K_NOP,
/* 1b*/ K_NOP,
/* 1c*/ K_NOP,
/* 1d*/ K_NOP,
/* 1e*/ K_NOP,
/* 1f*/ K_NOP,
/* 20*/ K_NOP,
/* 21*/ K_FRCNEXT,	/* F */
/* 22*/ K_NOP,
/* 23*/ K_VTF + 0,	/* H - go to vt00, aka "console" */
/* 24*/ K_NOP,
/* 25*/ K_NOP,
/* 26*/ K_NOP,
/* 27*/ K_NOP,
/* 28*/ K_NOP,
/* 29*/ K_NOP,
/* 2a*/ K_NOP,
/* 2b*/ K_NOP,
/* 2c*/ K_NOP,
/* 2d*/ K_NOP,
/* 2e*/ K_NOP,
/* 2f*/ K_NOP,
/* 30*/ K_NOP,
/* 31*/ K_NEXT,		/* N */
/* 32*/ K_NOP,
/* 33*/ K_NOP,
/* 34*/ K_NOP,
/* 35*/ K_NOP,
/* 36*/ K_NOP,
/* 37*/ K_NOP,
/* 38*/ K_NOP,
/* 39*/ K_NOP,
/* 3a*/ K_NOP,
/* 3b*/ K_VTF + 1,	/* F1 -> vt01 */
/* 3c*/ K_VTF + 2,	/* F2 -> vt02 */
/* 3d*/ K_VTF + 3,	/* F3 -> vt03 */
/* 3e*/ K_VTF + 4,	/* F4 -> vt04 */
/* 3f*/ K_VTF + 5,	/* F5 -> vt05 */
/* 40*/ K_VTF + 6,	/* F6 -> vt06 */
/* 41*/ K_VTF + 7,	/* F7 -> vt07 */
/* 42*/ K_VTF + 8,	/* F8 -> vt08 */
/* 43*/ K_VTF + 9,	/* F9 -> vt09 */
/* 44*/ K_VTF + 10,	/* F10 -> vt10 */
/* 45*/ K_NOP,
/* 46*/ K_NOP,
/* 47*/ K_NOP,
/* 48*/ K_NOP,
/* 49*/ K_NOP,
/* 4a*/ K_NOP,
/* 4b*/ K_NOP,
/* 4c*/ K_NOP,
/* 4d*/ K_NOP,
/* 4e*/ K_NOP,
/* 4f*/ K_NOP,
/* 50*/ K_NOP,
/* 51*/ K_NOP,
/* 52*/ K_NOP,
/* 53*/ K_NOP,
/* 54*/ K_NOP,
/* 55*/ K_NOP,
/* 56*/ K_NOP,
/* 57*/ K_VTF + 11,	/* F11 -> vt11 */
/* 58*/ K_VTF + 12,	/* F12 -> vt12 */
/* 59*/ K_NOP,
/* 5a*/ K_NOP,
/* 5b*/ K_NOP,
/* 5c*/ K_NOP,
/* 5d*/ K_NOP,
/* 5e*/ K_NOP,
/* 5f*/ K_NOP,
/* 60*/ K_NOP,
/* 61*/ K_NOP,
/* 62*/ K_NOP,
/* 63*/ K_NOP,
/* 64*/ K_NOP,
/* 65*/ K_NOP,
/* 66*/ K_NOP,
/* 67*/ K_NOP,
/* 68*/ K_NOP,
/* 69*/ K_NOP,
/* 6a*/ K_NOP,
/* 6b*/ K_NOP,
/* 6c*/ K_NOP,
/* 6d*/ K_NOP,
/* 6e*/ K_NOP,
/* 6f*/ K_NOP,
/* 70*/ K_NOP,
/* 71*/ K_NOP,
/* 72*/ K_NOP,
/* 73*/ K_NOP,
/* 74*/ K_NOP,
/* 75*/ K_NOP,
/* 76*/ K_NOP,
/* 77*/ K_NOP,
/* 78*/ K_NOP,
/* 79*/ K_NOP,
/* 7a*/ K_NOP,
/* 7b*/ K_NOP,
/* 7c*/ K_NOP,
/* 7d*/ K_NOP,
/* 7e*/ K_NOP,
/* 7f*/ K_NOP
};

ushort kb_shifttab[] = {	/* Translate shifts for kb_state */
	0, 0, LEFT_SHIFT, RIGHT_SHIFT, CAPS_LOCK, NUM_LOCK, SCROLL_LOCK,
	ALTSET, 0, CTRLSET, LEFT_ALT, RIGHT_ALT, LEFT_CTRL, RIGHT_CTRL
};

struct kb_shiftmkbrk kb_mkbrk[] = {
	{ LEFT_SHIFT,	0x2a,	0xaa,	0 },
	{ LEFT_ALT,	0x38,	0xb8,	0 },
	{ LEFT_CTRL,	0x1d,	0x9d,	0 },
	{ RIGHT_SHIFT,	0x36,	0xb6,	0 },
	{ RIGHT_ALT,	0x38,	0xb8,	1 },
	{ RIGHT_CTRL,	0x1d,	0x9d,	1 },
	{ CAPS_LOCK,	0x3a,	0xba,	0 },
	{ NUM_LOCK,	0x45,	0xc5,	0 },
	{ SCROLL_LOCK,	0x46,	0xc6,	0 },
} ;


#define NOP_KEY		90	/* 90 key is NOP for both SCO and USL */

short usl2sco_table[] = {
/* SCO 0 equal to USL */ 0,
/* SCO 1 equal to USL */ 1,
/* SCO 2 equal to USL */ 2,
/* SCO 3 equal to USL */ 3,
/* SCO 4 equal to USL */ 4,
/* SCO 5 equal to USL */ 5,
/* SCO 6 equal to USL */ 6,
/* SCO 7 equal to USL */ 7,
/* SCO 8 equal to USL */ 8,
/* SCO 9 equal to USL */ 9,
/* SCO 10 equal to USL */ 10,
/* SCO 11 equal to USL */ 11,
/* SCO 12 equal to USL */ 12,
/* SCO 13 equal to USL */ 13,
/* SCO 14 equal to USL */ 14,
/* SCO 15 equal to USL */ 15,
/* SCO 16 equal to USL */ 16,
/* SCO 17 equal to USL */ 17,
/* SCO 18 equal to USL */ 18,
/* SCO 19 equal to USL */ 19,
/* SCO 20 equal to USL */ 20,
/* SCO 21 equal to USL */ 21,
/* SCO 22 equal to USL */ 22,
/* SCO 23 equal to USL */ 23,
/* SCO 24 equal to USL */ 24,
/* SCO 25 equal to USL */ 25,
/* SCO 26 equal to USL */ 26,
/* SCO 27 equal to USL */ 27,
/* SCO 28 equal to USL */ 28,
/* SCO 29 equal to USL */ 29,
/* SCO 30 equal to USL */ 30,
/* SCO 31 equal to USL */ 31,
/* SCO 32 equal to USL */ 32,
/* SCO 33 equal to USL */ 33,
/* SCO 34 equal to USL */ 34,
/* SCO 35 equal to USL */ 35,
/* SCO 36 equal to USL */ 36,
/* SCO 37 equal to USL */ 37,
/* SCO 38 equal to USL */ 38,
/* SCO 39 equal to USL */ 39,
/* SCO 40 equal to USL */ 40,
/* SCO 41 equal to USL */ 41,
/* SCO 42 equal to USL */ 42,
/* SCO 43 equal to USL */ 43,
/* SCO 44 equal to USL */ 44,
/* SCO 45 equal to USL */ 45,
/* SCO 46 equal to USL */ 46,
/* SCO 47 equal to USL */ 47,
/* SCO 48 equal to USL */ 48,
/* SCO 49 equal to USL */ 49,
/* SCO 50 equal to USL */ 50,
/* SCO 51 equal to USL */ 51,
/* SCO 52 equal to USL */ 52,
/* SCO 53 equal to USL */ 53,
/* SCO 54 equal to USL */ 54,
/* SCO 55 equal to USL */ 55,
/* SCO 56 equal to USL */ 56,
/* SCO 57 equal to USL */ 57,
/* SCO 58 equal to USL */ 58,
/* SCO 59 equal to USL */ 59,
/* SCO 60 equal to USL */ 60,
/* SCO 61 equal to USL */ 61,
/* SCO 62 equal to USL */ 62,
/* SCO 63 equal to USL */ 63,
/* SCO 64 equal to USL */ 64,
/* SCO 65 equal to USL */ 65,
/* SCO 66 equal to USL */ 66,
/* SCO 67 equal to USL */ 67,
/* SCO 68 equal to USL */ 68,
/* SCO 69 equal to USL */ 69,
/* SCO 70 equal to USL */ 70,
/* SCO 71 equal to USL */ 71,
/* SCO 72 equal to USL */ 72,
/* SCO 73 equal to USL */ 73,
/* SCO 74 equal to USL */ 74,
/* SCO 75 equal to USL */ 75,
/* SCO 76 equal to USL */ 76,
/* SCO 77 equal to USL */ 77,
/* SCO 78 equal to USL */ 78,
/* SCO 79 equal to USL */ 79,
/* SCO 80 equal to USL */ 80,
/* SCO 81 equal to USL */ 81,
/* SCO 82 equal to USL */ 82,
/* SCO 83 equal to USL */ 83,
/* SCO 84 equal to USL */ 53,  /* Not Sure */
/* SCO 85 equal to USL */ NOP_KEY,
/* SCO 86 equal to USL */ NOP_KEY,
/* SCO 87 equal to USL */ 87,
/* SCO 88 equal to USL */ 88,
/* SCO 89 equal to USL */ 89,
/* SCO 90 equal to USL */ 90,
/* SCO 91 equal to USL */ 91,
/* SCO 92 equal to USL */ 92,
/* SCO 93 equal to USL */ 93,
/* SCO 94 equal to USL */ 94,
/* SCO 95 equal to USL */ 95,
/* SCO 96 equal to USL */ 120,
/* SCO 97 equal to USL */ 107,
/* SCO 98 equal to USL */ 85,
/* SCO 99 equal to USL */ 125,
/* SCO 100 equal to USL */ 127,
/* SCO 101 equal to USL */ 111,
/* SCO 102 equal to USL */ 122,
/* SCO 103 equal to USL */ 126,
/* SCO 104 equal to USL */ 123,
/* SCO 105 equal to USL */ 121,
/* SCO 106 equal to USL */ 76,
/* SCO 107 equal to USL */ NOP_KEY,
/* SCO 108 equal to USL */ 108,
/* SCO 109 equal to USL */ 109,
/* SCO 110 equal to USL */ 110,
/* SCO 111 equal to USL */ NOP_KEY,
/* SCO 112 equal to USL */ NOP_KEY,
/* SCO 113 equal to USL */ NOP_KEY,
/* SCO 114 equal to USL */ NOP_KEY,
/* SCO 115 equal to USL */ NOP_KEY,
/* SCO 116 equal to USL */ NOP_KEY,
/* SCO 117 equal to USL */ NOP_KEY,
/* SCO 118 equal to USL */ NOP_KEY,
/* SCO 119 equal to USL */ NOP_KEY,
/* SCO 120 equal to USL */ NOP_KEY,
/* SCO 121 equal to USL */ NOP_KEY,
/* SCO 122 equal to USL */ NOP_KEY,
/* SCO 123 equal to USL */ NOP_KEY,
/* SCO 124 equal to USL */ NOP_KEY,
/* SCO 125 equal to USL */ NOP_KEY,
/* SCO 126 equal to USL */ NOP_KEY,
/* SCO 127 equal to USL */ NOP_KEY,
/* SCO 128 equal to USL */ 115,
/* SCO 129 equal to USL */ 114,
/* SCO 130 equal to USL */ 84,
/* SCO 131 equal to USL */ 121,
/* SCO 132 equal to USL */ 127,
/* SCO 133 equal to USL */ 122,
/* SCO 134 equal to USL */ 111,
/* SCO 135 equal to USL */ 126,
/* SCO 136 equal to USL */ 125,
/* SCO 137 equal to USL */ 107,
/* SCO 138 equal to USL */ 120,
/* SCO 139 equal to USL */ 85,
/* SCO 140 equal to USL */ 117,
/* SCO 141 equal to USL */ 116
};


short sco2usl_table[] = {
/* USL 0 equal to SCO */ 0,
/* USL 1 equal to SCO */ 1,
/* USL 2 equal to SCO */ 2,
/* USL 3 equal to SCO */ 3,
/* USL 4 equal to SCO */ 4,
/* USL 5 equal to SCO */ 5,
/* USL 6 equal to SCO */ 6,
/* USL 7 equal to SCO */ 7,
/* USL 8 equal to SCO */ 8,
/* USL 9 equal to SCO */ 9,
/* USL 10 equal to SCO */ 10,
/* USL 11 equal to SCO */ 11,
/* USL 12 equal to SCO */ 12,
/* USL 13 equal to SCO */ 13,
/* USL 14 equal to SCO */ 14,
/* USL 15 equal to SCO */ 15,
/* USL 16 equal to SCO */ 16,
/* USL 17 equal to SCO */ 17,
/* USL 18 equal to SCO */ 18,
/* USL 19 equal to SCO */ 19,
/* USL 20 equal to SCO */ 20,
/* USL 21 equal to SCO */ 21,
/* USL 22 equal to SCO */ 22,
/* USL 23 equal to SCO */ 23,
/* USL 24 equal to SCO */ 24,
/* USL 25 equal to SCO */ 25,
/* USL 26 equal to SCO */ 26,
/* USL 27 equal to SCO */ 27,
/* USL 28 equal to SCO */ 28,
/* USL 29 equal to SCO */ 29,
/* USL 30 equal to SCO */ 30,
/* USL 31 equal to SCO */ 31,
/* USL 32 equal to SCO */ 32,
/* USL 33 equal to SCO */ 33,
/* USL 34 equal to SCO */ 34,
/* USL 35 equal to SCO */ 35,
/* USL 36 equal to SCO */ 36,
/* USL 37 equal to SCO */ 37,
/* USL 38 equal to SCO */ 38,
/* USL 39 equal to SCO */ 39,
/* USL 40 equal to SCO */ 40,
/* USL 41 equal to SCO */ 41,
/* USL 42 equal to SCO */ 42,
/* USL 43 equal to SCO */ 43,
/* USL 44 equal to SCO */ 44,
/* USL 45 equal to SCO */ 45,
/* USL 46 equal to SCO */ 46,
/* USL 47 equal to SCO */ 47,
/* USL 48 equal to SCO */ 48,
/* USL 49 equal to SCO */ 49,
/* USL 50 equal to SCO */ 50,
/* USL 51 equal to SCO */ 51,
/* USL 52 equal to SCO */ 52,
/* USL 53 equal to SCO */ 53,
/* USL 54 equal to SCO */ 54,
/* USL 55 equal to SCO */ 55,
/* USL 56 equal to SCO */ 56,
/* USL 57 equal to SCO */ 57,
/* USL 58 equal to SCO */ 58,
/* USL 59 equal to SCO */ 59,
/* USL 60 equal to SCO */ 60,
/* USL 61 equal to SCO */ 61,
/* USL 62 equal to SCO */ 62,
/* USL 63 equal to SCO */ 63,
/* USL 64 equal to SCO */ 64,
/* USL 65 equal to SCO */ 65,
/* USL 66 equal to SCO */ 66,
/* USL 67 equal to SCO */ 67,
/* USL 68 equal to SCO */ 68,
/* USL 69 equal to SCO */ 69,
/* USL 70 equal to SCO */ 70,
/* USL 71 equal to SCO */ 71,
/* USL 72 equal to SCO */ 72,
/* USL 73 equal to SCO */ 73,
/* USL 74 equal to SCO */ 74,
/* USL 75 equal to SCO */ 75,
/* USL 76 equal to SCO */ 76,
/* USL 77 equal to SCO */ 77,
/* USL 78 equal to SCO */ 78,
/* USL 79 equal to SCO */ 79,
/* USL 80 equal to SCO */ 80,
/* USL 81 equal to SCO */ 81,
/* USL 82 equal to SCO */ 82,
/* USL 83 equal to SCO */ 83,
/* USL 84 equal to SCO */ 104,
/* USL 85 equal to SCO */ 98,
/* USL 86 equal to SCO */ 97,
/* USL 87 equal to SCO */ 87,
/* USL 88 equal to SCO */ 88,
/* USL 89 equal to SCO */ 89,
/* USL 90 equal to SCO */ 90,
/* USL 91 equal to SCO */ 91,
/* USL 92 equal to SCO */ 92,
/* USL 93 equal to SCO */ 93,
/* USL 94 equal to SCO */ 94,
/* USL 95 equal to SCO */ 95,
/* USL 96 equal to SCO */ NOP_KEY,
/* USL 97 equal to SCO */ NOP_KEY,
/* USL 98 equal to SCO */ NOP_KEY,
/* USL 99 equal to SCO */ NOP_KEY,
/* USL 100 equal to SCO */ NOP_KEY,
/* USL 101 equal to SCO */ NOP_KEY,
/* USL 102 equal to SCO */ NOP_KEY,
/* USL 103 equal to SCO */ NOP_KEY,
/* USL 104 equal to SCO */ NOP_KEY,
/* USL 105 equal to SCO */ NOP_KEY,
/* USL 106 equal to SCO */ NOP_KEY,
/* USL 107 equal to SCO */ 97,
/* USL 108 equal to SCO */ 108,
/* USL 109 equal to SCO */ 109,
/* USL 110 equal to SCO */ 110,
/* USL 111 equal to SCO */ 101,
/* USL 112 equal to SCO */ 112,
/* USL 113 equal to SCO */ 113,
/* USL 114 equal to SCO */ 129,
/* USL 115 equal to SCO */ 128,
/* USL 116 equal to SCO */ 141,
/* USL 117 equal to SCO */ 117,
/* USL 118 equal to SCO */ 118,
/* USL 119 equal to SCO */ 70,
/* USL 120 equal to SCO */ 96,
/* USL 121 equal to SCO */ 131,
/* USL 122 equal to SCO */ 133,
/* USL 123 equal to SCO */ 130,
/* USL 124 equal to SCO */ 124,
/* USL 125 equal to SCO */ 136,
/* USL 126 equal to SCO */ 135,
/* USL 127 equal to SCO */ 132,
/* USL 128 equal to SCO */ NOP_KEY,
/* USL 129 equal to SCO */ NOP_KEY,
/* USL 130 equal to SCO */ NOP_KEY,
/* USL 131 equal to SCO */ NOP_KEY,
/* USL 132 equal to SCO */ NOP_KEY,
/* USL 133 equal to SCO */ NOP_KEY,
/* USL 134 equal to SCO */ NOP_KEY,
/* USL 135 equal to SCO */ NOP_KEY,
/* USL 136 equal to SCO */ NOP_KEY,
/* USL 137 equal to SCO */ NOP_KEY,
/* USL 138 equal to SCO */ NOP_KEY,
/* USL 139 equal to SCO */ NOP_KEY,
/* USL 140 equal to SCO */ NOP_KEY,
/* USL 141 equal to SCO */ NOP_KEY
};

int
xlate_keymap(keymap_t *src_kmap, keymap_t *dst_kmap, int format)
{

	int	nkeys;
	short  *xlation_table;
	register keyinfo_t *src_key, *dst_key;
	register i;

	switch(format) {
	case USL_FORMAT:
		nkeys = USL_TABLE_SIZE;
		xlation_table = sco2usl_table;
		break;
	case SCO_FORMAT:
		nkeys = SCO_TABLE_SIZE;
		xlation_table = usl2sco_table;
		break;
	default:
		return(0);
	}
	dst_kmap->n_keys = nkeys;
	src_key = src_kmap->key;
	dst_key = dst_kmap->key;
	for (i = 0; i < nkeys; i++)
		bcopy((caddr_t)&src_key[xlation_table[i]], (caddr_t)&dst_key[i],
		    sizeof(keyinfo_t));

	return(1);
}
