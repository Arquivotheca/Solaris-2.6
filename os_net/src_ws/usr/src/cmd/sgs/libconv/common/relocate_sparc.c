/*
 *	Copyright (c) 1996 by Sun Microsystems, Inc.
 *	All Rights Reserved.
 */
#pragma ident	"@(#)relocate_sparc.c	1.8	96/02/26 SMI"

/* LINTLIBRARY */

/*
 * String conversion routine for relocation types.
 */
#include	<stdio.h>
#include	<sys/elf_SPARC.h>
#include	"_conv.h"
#include	"relocate_sparc_msg.h"

/*
 * SPARC specific relocations.
 */
static const int rels[] = {
	MSG_R_SPARC_NONE,	MSG_R_SPARC_8,		MSG_R_SPARC_16,
	MSG_R_SPARC_32,		MSG_R_SPARC_DISP8,	MSG_R_SPARC_DISP16,
	MSG_R_SPARC_DISP32,	MSG_R_SPARC_WDISP30,	MSG_R_SPARC_WDISP22,
	MSG_R_SPARC_HI22,	MSG_R_SPARC_22,		MSG_R_SPARC_13,
	MSG_R_SPARC_LO10,	MSG_R_SPARC_GOT10,	MSG_R_SPARC_GOT13,
	MSG_R_SPARC_GOT22,	MSG_R_SPARC_PC10,	MSG_R_SPARC_PC22,
	MSG_R_SPARC_WPLT30,	MSG_R_SPARC_COPY,	MSG_R_SPARC_GLOB_DAT,
	MSG_R_SPARC_JMP_SLOT,	MSG_R_SPARC_RELATIVE,	MSG_R_SPARC_UA32,
	MSG_R_SPARC_PLT32,	MSG_R_SPARC_HIPLT22,	MSG_R_SPARC_LOPLT10,
	MSG_R_SPARC_PCPLT32,	MSG_R_SPARC_PCPLT22,	MSG_R_SPARC_PCPLT10,
	MSG_R_SPARC_10,		MSG_R_SPARC_11,		MSG_R_SPARC_64,
	MSG_R_SPARC_OLO10,	MSG_R_SPARC_HH22,	MSG_R_SPARC_HM10,
	MSG_R_SPARC_LM22,	MSG_R_SPARC_PC_HH22,	MSG_R_SPARC_PC_HM10,
	MSG_R_SPARC_PC_LM22,	MSG_R_SPARC_WDISP16,	MSG_R_SPARC_WDISP19,
	MSG_R_SPARC_GLOB_JMP,	MSG_R_SPARC_7,		MSG_R_SPARC_5,
	MSG_R_SPARC_6
};

const char *
conv_reloc_SPARC_type_str(Word rel)
{
	static char	string[STRSIZE] = { '\0' };

	if (rel >= R_SPARC_NUM)
		return (conv_invalid_str(string, STRSIZE, (int)rel, 0));
	else
		return (MSG_ORIG(rels[rel]));
}
