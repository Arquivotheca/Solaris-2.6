/*
 * Copyright (c) 1996 by Sun Microsystems, Inc.
 * All rights reserved.
 */
#pragma	ident	"@(#)doreloc.c	1.8	96/07/12 SMI"

/* LINTLIBRARY */

#if	defined(_KERNEL)
#include	"reloc.h"
#else
#include	"sgs.h"
#include	"machdep.h"
#include	"libld.h"
#include	"reloc.h"
#include	"conv.h"
#include	"msg.h"
#endif


/*
 * This table represents the current relocations that do_reloc() is able to
 * process.  The relocations below that are marked SPECIAL are relocations that
 * take special processing and shouldn't actually ever be passed to do_reloc().
 */
const Rel_entry	reloc_table[R_SPARC_NUM] = {
/* R_SPARC_NONE */	{0x0, 0, 0, 0, 0},
/* R_SPARC_8 */		{0x0, FLG_RE_NOTREL | FLG_RE_VERIFY, 1, 0, 0},
/* R_SPARC_16 */	{0x0, FLG_RE_NOTREL | FLG_RE_VERIFY, 2, 0, 0},
/* R_SPARC_32 */	{0x0, FLG_RE_NOTREL | FLG_RE_VERIFY, 4, 0, 0},
/* R_SPARC_DISP8 */	{0x0, FLG_RE_PCREL | FLG_RE_VERIFY | FLG_RE_SIGN,
				1, 0, 0},
/* R_SPARC_DISP16 */	{0x0, FLG_RE_PCREL | FLG_RE_VERIFY | FLG_RE_SIGN,
				2, 0, 0},
/* R_SPARC_DISP32 */	{0x0, FLG_RE_PCREL | FLG_RE_VERIFY | FLG_RE_SIGN,
				4, 0, 0},
/* R_SPARC_WDISP30 */	{0x0, FLG_RE_PCREL | FLG_RE_VERIFY | FLG_RE_SIGN,
				4, 2, 30},
/* R_SPARC_WDISP22 */	{0x0, FLG_RE_PCREL | FLG_RE_VERIFY | FLG_RE_SIGN,
				4, 2, 22},
/* R_SPARC_HI22 */	{0x0, FLG_RE_NOTREL, 4, 10, 22},
/* R_SPARC_22 */	{0x0, FLG_RE_NOTREL | FLG_RE_VERIFY, 4, 0, 22},
/* R_SPARC_13 */	{0x0, FLG_RE_NOTREL | FLG_RE_VERIFY | FLG_RE_SIGN,
				4, 0, 13},
/* R_SPARC_LO10 */	{0x3ff, FLG_RE_NOTREL | FLG_RE_SIGN, 4, 0, 13},
/* R_SPARC_GOT10 */	{0x3ff, FLG_RE_GOTREL | FLG_RE_SIGN, 4, 0, 13},
/* R_SPARC_GOT13 */	{0x0, FLG_RE_GOTREL | FLG_RE_VERIFY | FLG_RE_SIGN,
				4, 0, 13},
/* R_SPARC_GOT22 */	{0x0, FLG_RE_GOTREL, 4, 10, 22},
/* R_SPARC_PC10 */	{0x3ff, FLG_RE_PCREL | FLG_RE_RELPC | FLG_RE_SIGN,
				4, 0, 13},
/* R_SPARC_PC22 */	{0x0, FLG_RE_PCREL | FLG_RE_RELPC | FLG_RE_SIGN |
				FLG_RE_VERIFY,
				4, 10, 22},
/* R_SPARC_WPLT30 */	{0x0, FLG_RE_PCREL | FLG_RE_PLTREL |
				FLG_RE_VERIFY | FLG_RE_SIGN,
				4, 2, 30},
/* R_SPARC_COPY */	{0x0, 0, 0, 0, 0},		/* SPECIAL */
/* R_SPARC_GLOB_DAT */	{0x0, FLG_RE_NOTREL, 4, 0, 0},
/* R_SPARC_JMP_SLOT */	{0x0, 0, 0, 0, 0},		/* SPECIAL */
/* R_SPARC_RELATIVE */	{0x0, FLG_RE_NOTREL, 4, 0, 0},
/* R_SPARC_UA32 */	{0x0, FLG_RE_NOTREL | FLG_RE_UNALIGN, 4, 0, 0},
/* R_SPARC_PLT32 */	{0x0, FLG_RE_PLTREL | FLG_RE_VERIFY |
				FLG_RE_ADDRELATIVE, 4, 0, 0},
/* R_SPARC_HIPLT22 */	{0x0, FLG_RE_PLTREL, 4, 10, 22},
/* R_SPARC_LOPLT10 */	{0x3ff, FLG_RE_PLTREL, 4, 0, 13},
/* R_SPARC_PCPLT32 */	{0x0, FLG_RE_PLTREL | FLG_RE_PCREL | FLG_RE_VERIFY,
				4, 0, 0},
/* R_SPARC_PCPLT22 */	{0x0, FLG_RE_PLTREL | FLG_RE_PCREL |
				FLG_RE_VERIFY,
				4, 10, 22},
/* R_SPARC_PCPLT10 */	{0x3ff, FLG_RE_PLTREL | FLG_RE_PCREL |
				FLG_RE_VERIFY,
				4, 0, 12},
/* R_SPARC_10 */	{0x0, FLG_RE_NOTREL | FLG_RE_VERIFY | FLG_RE_SIGN,
				4, 0, 10},
/* R_SPARC_11 */	{0x0, FLG_RE_NOTREL | FLG_RE_VERIFY | FLG_RE_SIGN,
				4, 0, 11},
/* R_SPARC_64 */	{0x0, 0, 0, 0, 0},	/* V9 - not implemented */
/* R_SPARC_OLO10 */	{0x0, 0, 0, 0, 0},	/* V9 - not implemented */
/* R_SPARC_HH22 */	{0x0, 0, 0, 0, 0},	/* V9 - not implemented */
/* R_SPARC_HM10 */	{0x0, 0, 0, 0, 0},	/* V9 - not implemented */
/* R_SPARC_LM22 */	{0x0, 0, 0, 0, 0},	/* V9 - not implemented */
/* R_SPARC_PC_HH22 */	{0x0, 0, 0, 0, 0},	/* V9 - not implemented */
/* R_SPARC_PC_HM10 */	{0x0, 0, 0, 0, 0},	/* V9 - not implemented */
/* R_SPARC_PC_LM22 */	{0x0, 0, 0, 0, 0},	/* V9 - not implemented */
/* R_SPARC_WDISP16 */	{0x0, FLG_RE_PCREL | FLG_RE_WDISP16 |
				FLG_RE_VERIFY | FLG_RE_SIGN,
				4, 2, 16},
/* R_SPARC_WDISP19 */	{0x0, FLG_RE_PCREL | FLG_RE_VERIFY | FLG_RE_SIGN,
				4, 2, 19},
/* R_SPARC_GLOB_JMP */	{0x0, 0, 0, 0, 0},	/* V9 - not implemented */
/* R_SPARC_7 */		{0x0, FLG_RE_NOTREL, 4, 0, 7},
/* R_SPARC_5 */		{0x0, FLG_RE_NOTREL, 4, 0, 5},
/* R_SPARC_6 */		{0x0, FLG_RE_NOTREL, 4, 0, 6}
};


/*
 * Write a single relocated value to its reference location.
 * We assume we wish to add the relocation amount, value, to the
 * the value of the address already present in the instruction.
 *
 * NAME			VALUE	FIELD		CALCULATION
 *
 * R_SPARC_NONE		0	none		none
 * R_SPARC_8		1	V-byte8		S + A
 * R_SPARC_16		2	V-half16	S + A
 * R_SPARC_32		3	V-word32	S + A
 * R_SPARC_DISP8	4	V-byte8		S + A - P
 * R_SPARC_DISP16	5	V-half16	S + A - P
 * R_SPARC_DISP32	6	V-word32	S + A - P
 * R_SPARC_WDISP30	7	V-disp30	(S + A - P) >> 2
 * R_SPARC_WDISP22	8	V-disp22	(S + A - P) >> 2
 * R_SPARC_HI22		9	T-imm22		(S + A) >> 10
 * R_SPARC_22		10	V-imm22		S + A
 * R_SPARC_13		11	V-simm13	S + A
 * R_SPARC_LO10		12	T-simm13	(S + A) & 0x3ff
 * R_SPARC_GOT10	13	T-simm13	G & 0x3ff
 * R_SPARC_GOT13	14	V-simm13	G
 * R_SPARC_GOT22	15	T-imm22		G >> 10
 * R_SPARC_PC10		16	T-simm13	(S + A - P) & 0x3ff
 * R_SPARC_PC22		17	V-disp22	(S + A - P) >> 10
 * R_SPARC_WPLT30	18	V-disp30	(L + A - P) >> 2
 * R_SPARC_COPY		19	none		none
 * R_SPARC_GLOB_DAT	20	V-word32	S + A
 * R_SPARC_JMP_SLOT	21	V-plt22		S + A
 * R_SPARC_RELATIVE	22	V-word32	S + A
 * R_SPARC_UA32		23	V-word32	S + A
 * R_SPARC_PLT32	24	V-word32        L + A
 * R_SPARC_HIPLT22	25	T-imm22         (L + A) >> 10
 * R_SPARC_LOPLT10	26	T-simm13        (L + A) & 0x3ff
 * R_SPARC_PCPLT32	27	V-word32        L + A - P
 * R_SPARC_PCPLT22	28	V-disp22        (L + A - P) >> 10
 * R_SPARC_PCPLT10	29	V-simm12        (L + A - P) & 0x3ff
 * R_SPARC_10		30	V-simm10	S + A
 * R_SPARC_11		31	V-simm11	S + A
 * R_SPARC_64		32	V-xword64	S + A
 * R_SPARC_OLO10	33	V-simm13	((S + A) & 0x3ff) + O
 * R_SPARC_HH22		34	V-imm22		(S + A) >> 42
 * R_SPARC_HM10		35	T-simm13	((S + A) >>32) & 0x3ff
 * R_SPARC_LM22		36	T-imm22		(S + A) >> 10
 * R_SPARC_PC_HH22	37	V-imm22		(S + A - P) >> 42
 * R_SPARC_PC_HM10	38	T-simm13	((S + A - P) >> 32) & 0x3ff
 * R_SPARC_PC_LM22	39	T-imm22		(S + A - P) >> 10
 * R_SPARC_WDISP16	40	V-d2/disp14	(S + A - P) >> 2
 * R_SPARC_WDISP19	41	V-disp19	(S + A - P) >> 2
 * R_SPARC_GLOB_JMP	42	V-xword64	S + A
 * R_SPARC_7		43	V-imm7		S + A
 * R_SPARC_5		44	V-imm5		S + A
 * R_SPARC_6		45	V-imm6		S + A
 *
 *	This is Figure 4-20: Relocation Types from the Draft Copy of
 * the ABI, Printed on 11/29/88.
 *
 * NOTE: relocations 24->45 are newly registered relocations to support
 *	 C++ ABI & SPARC V8+ and SPARC V9 architectures (1/9/94)
 *
 * Relocation calculations:
 *
 * The FIELD names indicate whether the relocation type checks for overflow.
 * A calculated relocation value may be larger than the intended field, and
 * the relocation type may verify (V) that the value fits, or truncate (T)
 * the result.
 *
 * CALCULATION uses the following notation:
 *      A       the addend used
 *      B       the base address of the shared object in memory
 *      G       the offset into the global offset table
 *      L       the procedure linkage entry
 *      P       the place of the storage unit being relocated
 *      S       the value of the symbol
 *
 *
 * The calculations in the CALCULATION column are assumed to have been performed
 * before calling this function except for the addition of the addresses in the
 * instructions.
 *
 * Upon successful completion of do_reloc() *value will be set to the
 * 'bit-shifted' value that will be or'ed into memory.
 */
int
do_reloc(unsigned char rtype, unsigned char * off, Word * value,
	const char * sym, const char * file)
{
	long			uvalue = 0;
	Word			basevalue, sigbit_mask, sigfit_mask;
	Word			corevalue = *value;
	unsigned char		bshift;
	int			field_size, re_flags;
	const Rel_entry *	rep;

	rep = &reloc_table[rtype];
	bshift = rep->re_bshift;
	field_size = rep->re_fsize;
	re_flags = rep->re_flags;
	sigbit_mask = S_MASK(rep->re_sigbits);

	if ((re_flags & FLG_RE_SIGN) && sigbit_mask) {
		/*
		 * sigfit_mask takes into account that a value
		 * might be signed and discards the signbit for
		 * comparison.
		 */
		sigfit_mask = S_MASK(rep->re_sigbits - 1);
	} else
		sigfit_mask = sigbit_mask;

	if (field_size == 0) {
		eprintf(ERR_FATAL, MSG_INTL(MSG_REL_UNIMPL), file,
		    (sym ? sym : MSG_INTL(MSG_STR_UNKNOWN)), rtype);
		return (0);
	}

	if (re_flags & FLG_RE_UNALIGN) {
		int		i;
		unsigned char *	dest = (unsigned char *)&basevalue;

		basevalue = 0;
		for (i = field_size - 1; i > 0; i--)
			dest[i] = off[i];
	} else {
		if (((field_size == 2) && ((Word)off & 0x1)) ||
		    ((field_size == 4) && ((Word)off & 0x3))) {
			eprintf(ERR_FATAL, MSG_INTL(MSG_REL_NONALIGN),
			    conv_reloc_SPARC_type_str(rtype), file,
			    (sym ? sym : MSG_INTL(MSG_STR_UNKNOWN)), off);
			return (0);
		}
		switch (field_size) {
		case 1:
			basevalue = (Word)*((unsigned char *)off);
			break;
		case 2:
			/* LINTED */
			basevalue = (Word)*((Half *)off);
			break;
		case 4:
			/* LINTED */
			basevalue = (Word)*((Word *)off);
			break;
		default:
			eprintf(ERR_FATAL, MSG_INTL(MSG_REL_UNNOBITS),
			    conv_reloc_SPARC_type_str(rtype), file,
			    (sym ? sym : MSG_INTL(MSG_STR_UNKNOWN)),
			    rep->re_fsize * 8);
			return (0);
		}
	}

	if (sigbit_mask) {
		/*
		 * The WDISP16 relocation is an unusual one in that it's bits
		 * are not all contiguous.  We have to selectivly pull them out.
		 */
		if (re_flags & FLG_RE_WDISP16) {
			uvalue = ((basevalue & 0x300000) >> 6) |
				(basevalue & 0x3fff);
			basevalue &= ~0x303fff;
		} else {
			uvalue = sigbit_mask & basevalue;
			basevalue &= ~sigbit_mask;
		}
		/*
		 * If value is signed make sure that we signextend the uvalue.
		 */
		if (re_flags & FLG_RE_SIGN) {
			if (uvalue & (~sigbit_mask & sigfit_mask))
				uvalue |= ~sigbit_mask;
		}
	} else
		uvalue = basevalue;

	if (bshift)
		uvalue <<= bshift;

	uvalue += *value;

	if (bshift) {
		/*
		 * This is to check that we are not attempting to
		 * jump to a non-4 byte aligned address.
		 */
		if ((bshift == 2) && (uvalue & 0x3)) {
			eprintf(ERR_FATAL, MSG_INTL(MSG_REL_LOOSEBITS),
			    conv_reloc_SPARC_type_str(rtype), file,
			    (sym ? sym : MSG_INTL(MSG_STR_UNKNOWN)),
			    uvalue, 2, off);
			return (0);
		}
		uvalue >>= bshift;
		corevalue >>= bshift;
	}

	if (rep->re_mask)
		uvalue &= rep->re_mask;

	if ((re_flags & FLG_RE_VERIFY) && sigbit_mask) {
		if (((re_flags & FLG_RE_SIGN) &&
		    (S_INRANGE(uvalue, rep->re_sigbits - 1) == 0)) ||
		    (!(re_flags & FLG_RE_SIGN) &&
		    ((sigbit_mask & uvalue) != uvalue))) {
			eprintf(ERR_FATAL, MSG_INTL(MSG_REL_NOFIT),
			    conv_reloc_SPARC_type_str(rtype), file,
			    (sym ? sym : MSG_INTL(MSG_STR_UNKNOWN)), uvalue);
			return (0);
		}
	}

	if (sigbit_mask) {
		/*
		 * Again the R_SPARC_WDISP16 relocation takes special
		 * processing because of its non-continguous bits.
		 */
		if (re_flags & FLG_RE_WDISP16)
			uvalue = ((uvalue & 0xc000) << 6) |
				(uvalue & 0x3fff);
		else
			uvalue &= sigbit_mask;
		/*
		 * Combine value back with original word
		 */
		uvalue |= basevalue;
	}
	*value = corevalue;

	if (re_flags & FLG_RE_UNALIGN) {
		int		i;
		unsigned char *	src = (unsigned char *)&uvalue;

		for (i = field_size - 1; i > 0; i--)
			off[i] = src[i];
	} else {
		switch (rep->re_fsize) {
		case 1:
			*((unsigned char *)off) = (unsigned char)uvalue;
			break;
		case 2:
			/* LINTED */
			*((Half *)off) = (Half)uvalue;
			break;
		case 4:
			/* LINTED */
			*((unsigned long *)off) = uvalue;
			break;
		}
	}
	return (1);
}
