/*
 * Copyright (c) 1995 Sun Microsystems, Inc.  All Rights Reserved.
 */

#pragma	ident "@(#)at_keyprocess.c	1.9	95/09/11 SMI"

#if	defined(KD_DEBUG)
extern int kd_debug;
#define	DEBUG_KD(f)	if (kd_debug) prom_printf f; else
#else
#define	DEBUG_KD(f)	/* nothing */
#endif

struct	key {
	unsigned char	tabnext;	/* next table to use */
	unsigned char	keynum;		/* key position on keyboard */
};

#define	KEYBAD		0xff		/* should generate an error */
#define	KEYHOLE		0xfe		/* ignore this sequence */

#define	KEY(code)	{	(unsigned char)0,	(code)	}
#define	TABLE(tabnxt)	{	tabnxt,			KEYBAD	}
#define	INVALID		{	(unsigned char)0,	KEYBAD	}
#define	HOLE		{	(unsigned char)0,	KEYHOLE	}


#define	GENOKEYTAB_e0			1
#define	GENOKEYTAB_e1			2
#define	GENOKEYTAB_e1_1d		3

static struct	key	hole = HOLE;
static struct	key	invalid = INVALID;
static struct	key	table[9] = {
		    INVALID,
		    TABLE(GENOKEYTAB_e0),
		    TABLE(GENOKEYTAB_e1),
		    TABLE(GENOKEYTAB_e1_1d),
		};
static struct key key_126[1] = {KEY(126)};

#if 0
static struct	key	*genokeytab_base_fn();
static struct	key	*genokeytab_e0_fn();
static struct	key	*genokeytab_e1_fn();
static struct	key	*genokeytab_e1_1d_fn();
#endif

/*
 *	Parse table for the base keyboard state.
 */
struct	key	genokeytab_base[256] = {
/* scan		state		keycap */
/* 00 */	INVALID,
/* 01 */	KEY(110),	/* Esc */
/* 02 */	KEY(2),		/* 1 */
/* 03 */	KEY(3),		/* 2 */
/* 04 */	KEY(4),		/* 3 */
/* 05 */	KEY(5),		/* 4 */
/* 06 */	KEY(6),		/* 5 */
/* 07 */	KEY(7),		/* 6 */
/* 08 */	KEY(8),		/* 7 */
/* 09 */	KEY(9),		/* 8 */
/* 0a */	KEY(10),	/* 9 */
/* 0b */	KEY(11),	/* 0 */
/* 0c */	KEY(12),	/* - */
/* 0d */	KEY(13),	/* + */
/* 0e */	KEY(15),	/* backspace */
/* 0f */	KEY(16),	/* tab */
/* 10 */	KEY(17),	/* Q */
/* 11 */	KEY(18),	/* W */
/* 12 */	KEY(19),	/* E */
/* 13 */	KEY(20),	/* R */
/* 14 */	KEY(21),	/* T */
/* 15 */	KEY(22),	/* Y */
/* 16 */	KEY(23),	/* U */
/* 17 */	KEY(24),	/* I */
/* 18 */	KEY(25),	/* O */
/* 19 */	KEY(26),	/* P */
/* 1a */	KEY(27),	/* [ */
/* 1b */	KEY(28),	/* ] */
/* 1c */	KEY(43),	/* Enter (main key) */
/* 1d */	KEY(58),	/* (Right) Ctrl */
/* 1e */	KEY(31),	/* A */
/* 1f */	KEY(32),	/* S */
/* 20 */	KEY(33),	/* D */
/* 21 */	KEY(34),	/* F */
/* 22 */	KEY(35),	/* G */
/* 23 */	KEY(36),	/* H */
/* 24 */	KEY(37),	/* J */
/* 25 */	KEY(38),	/* K */
/* 26 */	KEY(39),	/* L */
/* 27 */	KEY(40),	/* ; */
/* 28 */	KEY(41),	/* " */
/* 29 */	KEY(1),		/* ` */
/* 2a */	KEY(44),	/* (Left) Shift */
/* 2b */	KEY(29),	/* \ (101-key only) [key 42, not labeled for 102-key] */
/* 2c */	KEY(46),	/* Z */
/* 2d */	KEY(47),	/* X */
/* 2e */	KEY(48),	/* C */
/* 2f */	KEY(49),	/* V */
/* 30 */	KEY(50),	/* B */
/* 31 */	KEY(51),	/* N */
/* 32 */	KEY(52),	/* M */
/* 33 */	KEY(53),	/* , */
/* 34 */	KEY(54),	/* . */
/* 35 */	KEY(55),	/* / */
/* 36 */	KEY(57),	/* (Right) Shift */
/* 37 */	KEY(100),	/* * (numeric pad) */
/* 38 */	KEY(60),	/* (Left) Alt */
/* 39 */	KEY(61),	/* space */
/* 3a */	KEY(30),	/* Caps Lock */
/* 3b */	KEY(112),	/* F1 */
/* 3c */	KEY(113),	/* F2 */
/* 3d */	KEY(114),	/* F3 */
/* 3e */	KEY(115),	/* F4 */
/* 3f */	KEY(116),	/* F5 */
/* 40 */	KEY(117),	/* F6 */
/* 41 */	KEY(118),	/* F7 */
/* 42 */	KEY(119),	/* F8 */
/* 43 */	KEY(120),	/* F9 */
/* 44 */	KEY(121),	/* F10 */
/* 45 */	KEY(90),	/* Num Lock */
/* 46 */	KEY(125),
/* 47 */	KEY(91),	/* 7 (numeric pad) */
/* 48 */	KEY(96),	/* 8 (numeric pad) */
/* 49 */	KEY(101),	/* 9 (numeric pad) */
/* 4a */	KEY(105),	/* - (numeric pad) */
/* 4b */	KEY(92),	/* 4 (numeric pad) */
/* 4c */	KEY(97),	/* 5 (numeric pad) */
/* 4d */	KEY(102),	/* 6 (numeric pad) */
/* 4e */	KEY(106),	/* + (numeric pad) */
/* 4f */	KEY(93),	/* 1 (numeric pad) */
/* 50 */	KEY(98),	/* 2 (numeric pad) */
/* 51 */	KEY(103),	/* 3 (numeric pad) */
/* 52 */	KEY(99),	/* 0 (numeric pad) */
/* 53 */	KEY(104),	/* . (numeric pad) */
/* 54 */	KEY(124),
/* 55 */	INVALID,
/* 56 */	KEY(45),	/* not labeled (102-key only) */
/* 57 */	KEY(122),	/* F11 */
/* 58 */	KEY(123),	/* F12 */
/* 59 */	INVALID,
/* 5a */	INVALID,
/* 5b */	INVALID,
/* 5c */	INVALID,
/* 5d */	INVALID,
/* 5e */	INVALID,
/* 5f */	INVALID,
/* 60 */	INVALID,
/* 61 */	INVALID,
/* 62 */	INVALID,
/* 63 */	INVALID,
/* 64 */	INVALID,
/* 65 */	INVALID,
/* 66 */	INVALID,
/* 67 */	INVALID,
/* 68 */	INVALID,
/* 69 */	INVALID,
/* 6a */	INVALID,
/* 6b */	INVALID,
/* 6c */	INVALID,
/* 6d */	INVALID,
/* 6e */	INVALID,
/* 6f */	INVALID,
/* 70 */	KEY(133),			/* Japanese 106-key keyboard */
/* 71 */	INVALID,
/* 72 */	INVALID,
/* 73 */	KEY(56),			/* Japanese 106-key keyboard */
/* 74 */	INVALID,
/* 75 */	INVALID,
/* 76 */	INVALID,
/* 77 */	INVALID,
/* 78 */	INVALID,
/* 79 */	KEY(132),			/* Japanese 106-key keyboard */
/* 7a */	INVALID,
/* 7b */	KEY(131),			/* Japanese 106-key keyboard */
/* 7c */	INVALID,
/* 7d */	KEY(14),			/* Japanese 106-key keyboard */
/* 7e */	INVALID,
/* 7f */	INVALID,
/* 80 */	INVALID,
/* 81 */	INVALID,
/* 82 */	INVALID,
/* 83 */	INVALID,
/* 84 */	INVALID,
/* 85 */	INVALID,
/* 86 */	INVALID,
/* 87 */	INVALID,
/* 88 */	INVALID,
/* 89 */	INVALID,
/* 8a */	INVALID,
/* 8b */	INVALID,
/* 8c */	INVALID,
/* 8d */	INVALID,
/* 8e */	INVALID,
/* 8f */	INVALID,
/* 90 */	INVALID,
/* 91 */	INVALID,
/* 92 */	INVALID,
/* 93 */	INVALID,
/* 94 */	INVALID,
/* 95 */	INVALID,
/* 96 */	INVALID,
/* 97 */	INVALID,
/* 98 */	INVALID,
/* 99 */	INVALID,
/* 9a */	INVALID,
/* 9b */	INVALID,
/* 9c */	INVALID,
/* 9d */	INVALID,
/* 9e */	INVALID,
/* 9f */	INVALID,
/* a0 */	INVALID,
/* a1 */	INVALID,
/* a2 */	INVALID,
/* a3 */	INVALID,
/* a4 */	INVALID,
/* a5 */	INVALID,
/* a6 */	INVALID,
/* a7 */	INVALID,
/* a8 */	INVALID,
/* a9 */	INVALID,
/* aa */	INVALID,
/* ab */	INVALID,
/* ac */	INVALID,
/* ad */	INVALID,
/* ae */	INVALID,
/* af */	INVALID,
/* b0 */	INVALID,
/* b1 */	INVALID,
/* b2 */	INVALID,
/* b3 */	INVALID,
/* b4 */	INVALID,
/* b5 */	INVALID,
/* b6 */	INVALID,
/* b7 */	INVALID,
/* b8 */	INVALID,
/* b9 */	INVALID,
/* ba */	INVALID,
/* bb */	INVALID,
/* bc */	INVALID,
/* bd */	INVALID,
/* be */	INVALID,
/* bf */	INVALID,
/* c0 */	INVALID,
/* c1 */	INVALID,
/* c2 */	INVALID,
/* c3 */	INVALID,
/* c4 */	INVALID,
/* c5 */	INVALID,
/* c6 */	INVALID,
/* c7 */	INVALID,
/* c8 */	INVALID,
/* c9 */	INVALID,
/* ca */	INVALID,
/* cb */	INVALID,
/* cc */	INVALID,
/* cd */	INVALID,
/* ce */	INVALID,
/* cf */	INVALID,
/* d0 */	INVALID,
/* d1 */	INVALID,
/* d2 */	INVALID,
/* d3 */	INVALID,
/* d4 */	INVALID,
/* d5 */	INVALID,
/* d6 */	INVALID,
/* d7 */	INVALID,
/* d8 */	INVALID,
/* d9 */	INVALID,
/* da */	INVALID,
/* db */	INVALID,
/* dc */	INVALID,
/* dd */	INVALID,
/* de */	INVALID,
/* df */	INVALID,
/* e0 */	TABLE(GENOKEYTAB_e0),
/* e1 */	TABLE(GENOKEYTAB_e1),
/* e2 */	INVALID,
/* e3 */	INVALID,
/* e4 */	INVALID,
/* e5 */	INVALID,
/* e6 */	INVALID,
/* e7 */	INVALID,
/* e8 */	INVALID,
/* e9 */	INVALID,
/* ea */	INVALID,
/* eb */	INVALID,
/* ec */	INVALID,
/* ed */	INVALID,
/* ee */	INVALID,
/* ef */	INVALID,
/* f0 */	INVALID,
/* f1 */	KEY(150),	/* Korean 103 PC kbd - Hangul toggle key */
/* f2 */	KEY(151),	/* Korean 103 PC kbd - Hangul/English toggle key */
/* f3 */	INVALID,
/* f4 */	INVALID,
/* f5 */	INVALID,
/* f6 */	INVALID,
/* f7 */	INVALID,
/* f8 */	INVALID,
/* f9 */	INVALID,
/* fa */	INVALID,
/* fb */	INVALID,
/* fc */	INVALID,
/* fd */	INVALID,
/* fe */	INVALID,
/* ff */	HOLE,		/* keyboard internal buffer overrun indicator */
};

struct key *
genokeytab_base_fn (unsigned char scan)
{
    return (&genokeytab_base[scan]);
}

struct	key	genokeytab_e0[171] = {
/* 00 */	INVALID,
/* 01 */	INVALID,
/* 02 */	INVALID,
/* 03 */	INVALID,
/* 04 */	INVALID,
/* 05 */	INVALID,
/* 06 */	INVALID,
/* 07 */	INVALID,
/* 08 */	INVALID,
/* 09 */	INVALID,
/* 0a */	INVALID,
/* 0b */	INVALID,
/* 0c */	INVALID,
/* 0d */	INVALID,
/* 0e */	INVALID,
/* 0f */	INVALID,
/* 10 */	INVALID,
/* 11 */	INVALID,
/* 12 */	INVALID,
/* 13 */	INVALID,
/* 14 */	INVALID,
/* 15 */	INVALID,
/* 16 */	INVALID,
/* 17 */	INVALID,
/* 18 */	INVALID,
/* 19 */	INVALID,
/* 1a */	INVALID,
/* 1b */	INVALID,
/* 1c */	KEY(108),	/* Enter (numeric pad) */
/* 1d */	KEY(64),	/* (Right) Ctrl */
/* 1e */	INVALID,
/* 1f */	INVALID,
/* 20 */	INVALID,
/* 21 */	INVALID,
/* 22 */	INVALID,
/* 23 */	INVALID,
/* 24 */	INVALID,
/* 25 */	INVALID,
/* 26 */	INVALID,
/* 27 */	INVALID,
/* 28 */	INVALID,
/* 29 */	INVALID,
/* 2a */	HOLE,		/* e0 2a prefixes num-locked arrow keys */
/* 2b */	INVALID,
/* 2c */	INVALID,
/* 2d */	INVALID,
/* 2e */	INVALID,
/* 2f */	INVALID,
/* 30 */	INVALID,
/* 31 */	INVALID,
/* 32 */	INVALID,
/* 33 */	INVALID,
/* 34 */	INVALID,
/* 35 */	KEY(95),	/* / (numeric pad) */
/* 36 */	HOLE,		/* right shift + arrow pad trailed by e0 36 */
/* 37 */	KEY(124),
/* 38 */	KEY(62),	/* (Right) Alt */
/* 39 */	INVALID,
/* 3a */	INVALID,
/* 3b */	INVALID,
/* 3c */	INVALID,
/* 3d */	INVALID,
/* 3e */	INVALID,
/* 3f */	INVALID,
/* 40 */	INVALID,
/* 41 */	INVALID,
/* 42 */	INVALID,
/* 43 */	INVALID,
/* 44 */	INVALID,
/* 45 */	INVALID,
/* 46 */	KEY(126),
/* 47 */	KEY(80),	/* Home (arrow pad) */
/* 48 */	KEY(83),	/* Up Arrow (arrow pad) */
/* 49 */	KEY(85),	/* PgUp (arrow pad) */
/* 4a */	INVALID,
/* 4b */	KEY(79),	/* Left Arrow (arrow pad) */
/* 4c */	INVALID,
/* 4d */	KEY(89),	/* Right Arrow (arrow pad) */
/* 4e */	INVALID,
/* 4f */	KEY(81),	/* End (arrow pad) */
/* 50 */	KEY(84),	/* Down Arrow (arrow pad) */
/* 51 */	KEY(86),	/* PgDn (arrow pad) */
/* 52 */	KEY(75),	/* Ins (arrow pad) */
/* 53 */	KEY(76),	/* Del (arrow pad) */
/* 54 */	INVALID,
/* 55 */	INVALID,
/* 56 */	INVALID,
/* 57 */	INVALID,
/* 58 */	INVALID,
/* 59 */	INVALID,
/* 5a */	INVALID,
/* 5b */	KEY(59),	/* (Left) Meta MS Natural Keyboard */
/* 5c */	KEY(63),	/* (Right) Meta MS Natural Keyboard */
/* 5d */	KEY(65),	/* Menu - Meta MS Natural Keyboard */
};

/*
 *	Reached via e0.
 */
struct key *
genokeytab_e0_fn (unsigned char scan)
{
    if (scan == 0xaa)
	return(&hole);
    if (scan < 0x5e)
	return(&genokeytab_e0[scan]);
    else
	return(&invalid);
}

/*
 *	Reached via e1.
 */
struct key *
genokeytab_e1_fn (unsigned char scan)
{
    if (scan == 0x1d)
	return(&table[GENOKEYTAB_e1_1d]);
    else
	return(&invalid);
}

/*
 *	Reached via e1 1d.
 */
struct key *
genokeytab_e1_1d_fn (unsigned char scan)
{
    if (scan == 0x45)
	return(&key_126[0]);
    else
	return(&invalid);
}

/*
 *----------------------------------------------------------------------
 *	atKbdGenoConvertScan (scan, keynum, up)
 *
 *	State machine that takes scan codes from the keyboard and resolves
 *	them to key numbers using the above tables.  Returns xTrue if this
 *	scan code completes a scan code sequence, in which case "keynum"
 *	and "isup" will be filled in correctly.
 *----------------------------------------------------------------------
 */
int
atKeyboardConvertScan (scan, keynum, isup)
unsigned char	scan;
int		*keynum,
		*isup;
{
	static	struct	key *(*table)(unsigned char) = genokeytab_base_fn;
	struct	key		*entry;
	int			up = 0;

	DEBUG_KD(("atKeyboardConvertScan XT 0x%x", scan));

	/* find the entry for this key */
#if 0
	entry = &table[scan];
#else
	entry = (*table)(scan);
#endif

again:
	if (entry->tabnext) {	/* follow to next table */
		DEBUG_KD(("-> table %d\n", entry->tabnext));
		switch(entry->tabnext) {
			case	GENOKEYTAB_e0:
				table = genokeytab_e0_fn;
				break;
			case	GENOKEYTAB_e1:
				table = genokeytab_e1_fn;
				break;
			case	GENOKEYTAB_e1_1d:
				table = genokeytab_e1_1d_fn;
				break;
		}
		return 0;	/* not a final keycode */
	}

	if (entry->keynum == KEYHOLE) {		/* not a key, nor an error */
		DEBUG_KD(("-> hole -> ignored\n"));
		table = genokeytab_base_fn;	/* return to base state */
		return 0;			/* also not a final keycode */
	}
	
	if (entry->keynum == KEYBAD) {	/* not part of a legit sequence? */
		DEBUG_KD(("-> bad "));
		/*
		 *	See if this is actually a keyup value.
		 */
		if (scan & 0x80) {
#if 0
			entry = &table[scan & 0x7f];
#else
			entry = (*table)(scan & 0x7f);
#endif
			if (entry->tabnext != 0 || entry->keynum != KEYBAD) {
				/* yes, it's the keyup for a key */
				up = 1;
				goto again;	/* ugh, but it works */
			}
		}

		DEBUG_KD(("-> ignored\n"));
		table = genokeytab_base_fn;	/* reset to base state */
		return (0);		/* and return not a final keycode */
	}

	/*
	 *	If we're here, it's a valid keycode.  Fill in the values
	 *	and return.
	 */
	*keynum = entry->keynum;
	*isup   = up;

	table = genokeytab_base_fn;	/* reset to base state */

	DEBUG_KD(("-> %s keypos %d\n",
		*isup ? "released" : "pressed", *keynum));

	return (1);		/* resolved to a key */
}
