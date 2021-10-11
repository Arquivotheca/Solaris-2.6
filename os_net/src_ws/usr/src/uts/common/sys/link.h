/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * Copyright (c) 1995, by Sun Microsystems, Inc.
 * All rights reserved.
 */

#ifndef _SYS_LINK_H
#define	_SYS_LINK_H

#pragma ident	"@(#)link.h	1.27	96/09/30 SMI"	/* SVr4.0 1.9	*/

#ifndef	_ASM
#include <sys/types.h>
#include <sys/elftypes.h>
/* #include <sys/rtld_db.h> */
#endif

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * Communication structures for the run-time linker.
 */

/*
 * The following data structure provides a self-identifying union consisting
 * of a tag from a known list and a value.
 */
#ifndef	_ASM
typedef struct {
	Elf32_Sword d_tag;		/* how to interpret value */
	union {
		Elf32_Word	d_val;
		Elf32_Addr	d_ptr;
		Elf32_Off	d_off;
	} d_un;
} Elf32_Dyn;
#endif

/*
 * Tag values
 */
#define	DT_NULL		0	/* last entry in list */
#define	DT_NEEDED	1	/* a needed object */
#define	DT_PLTRELSZ	2	/* size of relocations for the PLT */
#define	DT_PLTGOT	3	/* addresses used by procedure linkage table */
#define	DT_HASH		4	/* hash table */
#define	DT_STRTAB	5	/* string table */
#define	DT_SYMTAB	6	/* symbol table */
#define	DT_RELA		7	/* addr of relocation entries */
#define	DT_RELASZ	8	/* size of relocation table */
#define	DT_RELAENT	9	/* base size of relocation entry */
#define	DT_STRSZ	10	/* size of string table */
#define	DT_SYMENT	11	/* size of symbol table entry */
#define	DT_INIT		12	/* _init addr */
#define	DT_FINI		13	/* _fini addr */
#define	DT_SONAME	14	/* name of this shared object */
#define	DT_RPATH	15	/* run-time search path */
#define	DT_SYMBOLIC	16	/* shared object linked -Bsymbolic */
#define	DT_REL		17	/* addr of relocation entries */
#define	DT_RELSZ	18	/* size of relocation table */
#define	DT_RELENT	19	/* base size of relocation entry */
#define	DT_PLTREL	20	/* relocation type for PLT entry */
#define	DT_DEBUG	21	/* pointer to r_debug structure */
#define	DT_TEXTREL	22	/* text relocations remain for this object */
#define	DT_JMPREL	23	/* pointer to the PLT relocation entries */

#define	DT_MAXPOSTAGS	24	/* number of positive tags */

#define	DT_FLAGS_1	0x6ffffffb	/* state flags - see DF_1_* defs */
#define	DT_VERDEF	0x6ffffffc	/* version definition table and */
#define	DT_VERDEFNUM	0x6ffffffd	/*	associated no. of entries */
#define	DT_VERNEED	0x6ffffffe	/* version needed table and */
#define	DT_VERNEEDNUM	0x6fffffff	/* 	associated no. of entries */
#define	DT_LOPROC	0x70000000	/* processor specific range */
#define	DT_AUXILIARY	0x7ffffffd	/* shared library auxiliary name */
#define	DT_USED		0x7ffffffe	/* an object that isn't needed yet */
#define	DT_FILTER	0x7fffffff	/* shared library filter name */
#define	DT_HIPROC	0x7fffffff

#define	DF_1_NOW	0x00000001	/* set RTLD_NOW for this object */
#define	DF_1_GLOBAL	0x00000002	/* set RTLD_GLOBAL for this object */
#define	DF_1_GROUP	0x00000004	/* set RTLD_GROUP for this object */
#define	DF_1_NODELETE	0x00000008	/* set RTLD_NODELETE for this object */
#define	DF_1_LOADFLTR	0x00000010	/* trigger filtee loading at runtime */


/*
 * Version structures.  There are three types of version structure:
 *
 *  o	A definition of the versions within the image itself.
 *	Each version definition is assigned a unique index (starting from
 *	VER_NDX_BGNDEF)	which is used to cross-reference symbols associated to
 *	the version.  Each version can have one or more dependencies on other
 *	version definitions within the image.  The version name, and any
 *	dependency names, are specified in the version definition auxiliary
 *	array.  Version definition entries require a version symbol index table.
 *
 *  o	A version requirement on a needed dependency.  Each needed entry
 *	specifies the shared object dependency (as specified in DT_NEEDED).
 *	One or more versions required from this dependency are specified in the
 *	version needed auxiliary array.
 *
 *  o	A version symbol index table.  Each symbol indexes into this array
 *	to determine its version index.  Index values of VER_NDX_BGNDEF or
 *	greater indicate the version definition to which a symbol is associated.
 *	(the size of a symbol index entry is recorded in the sh_info field).
 */
#ifndef	_ASM

typedef struct {			/* Version Definition Structure. */
	Elf32_Half	vd_version;	/* this structures version revision */
	Elf32_Half	vd_flags;	/* version information */
	Elf32_Half	vd_ndx;		/* version index */
	Elf32_Half	vd_cnt;		/* no. of associated aux entries */
	Elf32_Word	vd_hash;	/* version name hash value */
	Elf32_Word	vd_aux;		/* no. of bytes from start of this */
					/*	verdef to verdaux array */
	Elf32_Word	vd_next;	/* no. of bytes from start of this */
} Elf32_Verdef;				/*	verdef to next verdef entry */

typedef struct {			/* Verdef Auxiliary Structure. */
	Elf32_Word	vda_name;	/* first element defines the version */
					/*	name. Additional entries */
					/*	define dependency names. */
	Elf32_Word	vda_next;	/* no. of bytes from start of this */
} Elf32_Verdaux;			/*	verdaux to next verdaux entry */


typedef	struct {			/* Version Requirement Structure. */
	Elf32_Half	vn_version;	/* this structures version revision */
	Elf32_Half	vn_cnt;		/* no. of associated aux entries */
	Elf32_Word	vn_file;	/* name of needed dependency (file) */
	Elf32_Word	vn_aux;		/* no. of bytes from start of this */
					/*	verneed to vernaux array */
	Elf32_Word	vn_next;	/* no. of bytes from start of this */
} Elf32_Verneed;			/*	verneed to next verneed entry */

typedef struct {			/* Verneed Auxiliary Structure. */
	Elf32_Word	vna_hash;	/* version name hash value */
	Elf32_Half	vna_flags;	/* version information */
	Elf32_Half	vna_other;
	Elf32_Word	vna_name;	/* version name */
	Elf32_Word	vna_next;	/* no. of bytes from start of this */
} Elf32_Vernaux;			/*	vernaux to next vernaux entry */

typedef	Elf32_Half 	Elf32_Versym;	/* Version symbol index array */

#endif

/*
 * Versym symbol index values.  Values greated than VER_NDX_GLOBAL associate
 * symbols with user specified version descriptors.
 */
#define	VER_NDX_LOCAL		0	/* symbol is local */
#define	VER_NDX_GLOBAL		1	/* symbol is global and assigned to */
					/*	the base version */

/*
 * Verdef and Verneed (via Veraux) flags values.
 */
#define	VER_FLG_BASE		0x1	/* version definition of file itself */
#define	VER_FLG_WEAK		0x2	/* weak version identifier */

/*
 * Verdef version values.
 */
#define	VER_DEF_NONE		0	/* Ver_def version */
#define	VER_DEF_CURRENT		1
#define	VER_DEF_NUM		2

/*
 * Verneed version values.
 */
#define	VER_NEED_NONE		0	/* Ver_need version */
#define	VER_NEED_CURRENT	1
#define	VER_NEED_NUM		2


/*
 * Public structure defined and maintained within the run-time linker
 */
#ifndef	_ASM

typedef struct link_map	Link_map;

struct link_map {
	unsigned long	l_addr;		/* address at which object is mapped */
	char *		l_name;		/* full name of loaded object */
	Elf32_Dyn *	l_ld;		/* dynamic structure of object */
	Link_map *	l_next;		/* next link object */
	Link_map *	l_prev;		/* previous link object */
	char *		l_refname;	/* filters reference name */
};

typedef enum {
	RT_CONSISTENT,
	RT_ADD,
	RT_DELETE
} r_state_e;

typedef enum {
	RD_FL_NONE = 0,		/* no flags */
	RD_FL_ODBG = (1<<0),	/* old style debugger present */
	RD_FL_DBG = (1<<1)	/* debugging enabled */
} rd_flags_e;



/*
 * Debugging events enabled inside of the run-time linker.  To
 * access these events see the librtld_db interface.
 */
typedef enum {
	RD_NONE = 0,		/* no event */
	RD_PREINIT,		/* the Initial rendezvous before .init */
	RD_POSTINIT,		/* the Second rendezvous after .init */
	RD_DLACTIVITY		/* a dlopen or dlclose has happened */
} rd_event_e;

struct r_debug {
	int		r_version;	/* debugging info version no. */
	Link_map *	r_map;		/* address of link_map */
	unsigned long	r_brk;		/* address of update routine */
	r_state_e	r_state;
	unsigned long	r_ldbase;	/* base addr of ld.so */
	Link_map *	r_ldsomap;	/* address of ld.so.1's link map */
	rd_event_e	r_rdevent;	/* debug event */
	rd_flags_e	r_flags;	/* misc flags. */
};

#define	R_DEBUG_VERSION	2		/* current r_debug version */
#endif	/* _ASM */

/*
 * Attribute/value structures used to bootstrap ELF-based dynamic linker.
 */
#ifndef	_ASM
typedef struct {
	Elf32_Sword eb_tag;		/* what this one is */
	union {				/* possible values */
		Elf32_Word eb_val;
		Elf32_Addr eb_ptr;
		Elf32_Off  eb_off;
	} eb_un;
} Elf32_Boot;
#endif

/*
 * Attributes
 */
#define	EB_NULL		0		/* (void) last entry */
#define	EB_DYNAMIC	1		/* (*) dynamic structure of subject */
#define	EB_LDSO_BASE	2		/* (caddr_t) base address of ld.so */
#define	EB_ARGV		3		/* (caddr_t) argument vector */
#define	EB_ENVP		4		/* (char **) environment strings */
#define	EB_AUXV		5		/* (auxv_t *) auxiliary vector */
#define	EB_DEVZERO	6		/* (int) fd for /dev/zero */
#define	EB_PAGESIZE	7		/* (int) page size */
#define	EB_MAX		8		/* number of "EBs" */


/*
 * Concurrency communication structure for threads library.
 */
#ifndef	_ASM
#ifdef __STDC__
extern void	_ld_concurrency(void *);
#else
extern void	_ld_concurrency();
#endif
#endif

#ifdef	__cplusplus
}
#endif

#endif	/* _SYS_LINK_H */
