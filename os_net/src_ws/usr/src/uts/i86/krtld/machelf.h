/*
 *	Copyright (c) 1996 by Sun Microsystems, Inc.
 *	All rights reserved.
 */
#pragma ident	"@(#)machelf.h	1.1	96/06/11 SMI"

#if !defined(MACHELF_DOT_H)
#define	MACHELF_DOT_H

#include	<sys/elf_386.h>


/*
 * Make machine class dependent data types transparent to the common code
 */
#define	Word		Elf32_Word
#define	Sword		Elf32_Sword
#define	Half		Elf32_Half
#define	Addr		Elf32_Addr
#define	Off		Elf32_Off
#define	Byte		unsigned char

#define	Ehdr		Elf32_Ehdr
#define	Shdr		Elf32_Shdr
#define	Sym		Elf32_Sym
#define	Rel		Elf32_Rel
#define	Phdr		Elf32_Phdr
#define	Dyn		Elf32_Dyn
#define	Boot		Elf32_Boot
#define	Verdef		Elf32_Verdef
#define	Verdaux		Elf32_Verdaux
#define	Verneed		Elf32_Verneed
#define	Vernaux		Elf32_Vernaux
#define	Versym		Elf32_Versym



#ifndef	_ASM
/*
 * Structure used to build the reloc_table[]
 */
typedef struct {
	unsigned short	re_flags;	/* relocation attributes */
	unsigned char	re_fsize;	/* filed size (in #bytes) */
} Rel_entry;

#endif

#endif
