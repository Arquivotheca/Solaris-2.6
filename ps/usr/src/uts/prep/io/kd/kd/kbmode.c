/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * Copyright (c) 1992-1993, by Sun Microsystems, Inc.  All Rights Reserved.
 */

#pragma ident	"@(#)kbmode.c	1.11	96/06/02 SMI"

#include "sys/param.h"
#include "sys/types.h"
#include "sys/sysmacros.h"
#include "sys/ascii.h"
#include "sys/termios.h"
#include "sys/stream.h"
#include "sys/strtty.h"
#include "sys/stropts.h"
#include "sys/proc.h"
#ifdef DONT_INCLUDE
#include "sys/xque.h"
#endif
#include "sys/errno.h"
#include "sys/cmn_err.h"
#include "sys/kd.h"
#include "sys/promif.h"

/* Table indexed by AT scan codes yielding XT scan codes */
unsigned char at2xt[] = {
	/*AT	XT	Key*/
	/*00*/	0,	
	/*01*/	67,	/* F9 */
	/*02*/	65,	/* F7 */
	/*03*/	63,	/* F5 */
	/*04*/	61,	/* F3 */
	/*05*/	59,	/* F1 */
	/*06*/	60,	/* F2 */
	/*07*/	88,	/* F12 */
	/*08*/	0,
	/*09*/	68,	/* F10 */
	/*10*/	66,	/* F8 */
	/*11*/	64,	/* F6 */
	/*12*/	62,	/* F4 */
	/*13*/	15,	/* TAB */
	/*14*/	41,	/* ~ ` */
	/*15*/	0,
	/*16*/	0,
	/*17*/	56,	/* Left Alt, (ext) Right Alt */
	/*18*/	42,	/* Left Shift */
	/*19*/	112,
	/*20*/	29,	/* Left Control, (ext) Right Control */
	/*21*/	16,	/* Q */
	/*22*/	2,	/* 1 */
	/*23*/	0,
	/*24*/	0,
	/*25*/	0,
	/*26*/	44,	/* Z */
	/*27*/	31,	/* S */
	/*28*/	30,	/* A */
	/*29*/	17,	/* W */
	/*30*/	3,	/* 2 */
	/*31*/	0,	
	/*32*/	0,	
	/*33*/	46,	/* C */
	/*34*/	45,	/* X */
	/*35*/	32,	/* D */
	/*36*/	18,	/* E */
	/*37*/	5,	/* 4 */
	/*38*/	4,	/* 3 */
	/*39*/	0,	
	/*40*/	0,	
	/*41*/	57,	/* Space */
	/*42*/	47,	/* V */
	/*43*/	33,	/* F */
	/*44*/	20,	/* T */
	/*45*/	19,	/* R */
	/*46*/	6,	/* 5 */
	/*47*/	0,	
	/*48*/	0,	
	/*49*/	49,	/* N */
	/*50*/	48,	/* B */
	/*51*/	35,	/* H */
	/*52*/	34,	/* G */
	/*53*/	21,	/* Y */
	/*54*/	7,	/* 6 */
	/*55*/	0,	
	/*56*/	0,
	/*57*/	0,
	/*58*/	50,	/* M */
	/*59*/	36,	/* J */
	/*60*/	22,	/* U */
	/*61*/	8,	/* 7 */
	/*62*/	9,	/* 8 */
	/*63*/	0,	
	/*64*/	0,
	/*65*/	51,	/* , < */
	/*66*/	37,	/* K */
	/*67*/	23,	/* I */
	/*68*/	24,	/* O */
	/*69*/	11,	/* 0 */
	/*70*/	10,	/* 9 */
	/*71*/	0,	
	/*72*/	0,
	/*73*/	52,	/* . > */
	/*74*/	53,	/* / ?, (ext) num pad / */
	/*75*/	38,	/* L */
	/*76*/	39,	/* ; : */
	/*77*/	25,	/* P */
	/*78*/	12,	/* - _ */
	/*79*/	0,	
	/*80*/	0,
	/*81*/	115,
	/*82*/	40,	/* ' " */
	/*83*/	0,	
	/*84*/	26,	/* [ { */
	/*85*/	13,	/* = + */
	/*86*/	0,	
	/*87*/	0,	
	/*88*/	58,	/* Caps Lock */
	/*89*/	54,	/* Right Shift */
	/*90*/	28,	/* Enter, (ext) num pad Enter */
	/*91*/	27,	/* ] } */
	/*92*/	0,	
	/*93*/	43,	/* \ | */
	/*94*/	0,	
	/*95*/	0,	
	/*96*/	0,	
	/*97*/	86,	/* S005 102 foreign keyboards need this map */
	/*98*/	0,
	/*99*/	0,
	/*100*/	121,
	/*101*/	0,
	/*102*/	14,	/* Backspace */
	/*103*/	123,
	/*104*/	0,
	/*105*/	79,	/* Num pad 1 End, (ext) arrow pad End */
	/*106*/	125,
	/*107*/	75,	/* Num pad 4 Left, (ext) arrow pad Left */
	/*108*/	71,	/* Num pad 7 Home, (ext) arrow pad Home */
	/*109*/	0,
	/*110*/	0,
	/*111*/	0,
	/*112*/	82,	/* Num pad 0 Insert, (ext) arrow pad Insert */
	/*113*/	83,	/* Num pad . DEL, (ext) arrow pad DEL */
	/*114*/	80,	/* Num pad 2 Down, (ext) arrow pad Down */
	/*115*/	76,	/* Num pad 5 */
	/*116*/	77,	/* Num pad 6 Right, (ext) arrow pad Right */
	/*117*/	72,	/* Num pad 8 Up, (ext) arrow pad Up */
	/*118*/	1,	/* Escape */
	/*119*/	69,	/* Num Lock */
	/*120*/	87,	/* F11 */
	/*121*/	78,	/* Num pad + */
	/*122*/	81,	/* Num pad 3 PgDn, (ext) arrow pad PgDn */
	/*123*/	74,	/* Num pad - */
	/*124*/	55,	/* PrtScr * */
	/*125*/	73,	/* Num pad 9 PgUp, (ext) arrow pad PgUp */
	/*126*/	70,	/* Scroll Lock */
	/*127*/	84,	/* 84-key SysReq */
	/*128*/	0,
	/*129*/	0,
	/*130*/	0,
	/*131*/	65,	/* extra F7	S003 (ref IBM AT, PS/2-80 Tech Ref) */
	/*132*/	84,	/* 84-key SysReq (ref IBM AT Tech Ref) */
	/*133*/	0,
	/*134*/	0,
	/*135*/	79,	/* Num pad 1 End ??? */
	/*136*/	0,
	/*137*/	0,
	/*138*/	0,
	/*139*/	0,
	/*140*/	53,	/* Extended Num '/' ??? */
};

#ifdef	prep
int     kb_raw_mode = KBM_AT;
#elif	i386
int	kb_raw_mode = KBM_XT;
#else
#error don't know default keyboard mode for this platform
#endif

int
kd_xlate_at2xt(int c)
{
	static int  saw_break=0;

	switch(c) {
	    case KAT_BREAK:
		saw_break = KBD_BREAK;
		return 0;
	    case KAT_EXTEND:	/* AT extend prefix is 0xE0 */
	    case KAT_EXTEND2:	/* Used in "Pause" sequence */
	    case KB_ACK:
		return c;
	}
	if ( c >= (sizeof at2xt)/(sizeof at2xt[0]) ) {
		prom_printf("kd_xlate_at2xt:  can't translate 0x%x\n", c);
		return 0;
	}
	c = at2xt[c] & 0xff | saw_break;
	saw_break = 0;
	return c;
}
