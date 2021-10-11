/*
 *	Copyright (c) 1995 by Sun Microsystems, Inc.
 *	All rights reserved.
 */
#pragma ident	"@(#)_debug.h	1.14	96/05/01 SMI"

#ifndef		_DEBUG_DOT_H
#define		_DEBUG_DOT_H

#include	"debug.h"
#include	"conv.h"

extern	int	_Dbg_mask;


/*
 * Debugging is enabled by various tokens (see debug.c) that result in an
 * internal bit mask (_Dbg_mask) being initialized.  Each debugging function is
 * appropriate for one or more of the classes specified by the bit mask.  Each
 * debugging function validates whether it is appropriate for the present
 * classes before printing anything.
 */
#define	DBG_NOTCLASS(c)	!(_Dbg_mask & DBG_OMASK & (c))
#define	DBG_NOTDETAIL()	!(_Dbg_mask & DBG_DETAIL)

#define	DBG_ALL		0x7ffff
#define	DBG_OMASK	0x0ffff
#define	DBG_AMASK	0x70000

#define	DBG_DETAIL	0x10000

#define	DBG_ARGS	0x00001
#define	DBG_BASIC	0x00002
#define	DBG_BINDINGS	0x00004
#define	DBG_ENTRY	0x00008
#define	DBG_FILES	0x00010
#define	DBG_HELP	0x00020
#define	DBG_LIBS	0x00040
#define	DBG_MAP		0x00080
#define	DBG_RELOC	0x00100
#define	DBG_SECTIONS	0x00200
#define	DBG_SEGMENTS	0x00400
#define	DBG_SYMBOLS	0x00800
#define	DBG_SUPPORT	0x01000
#define	DBG_VERSIONS	0x02000

typedef struct options {
	const char *	o_name;		/* command line argument name */
	int		o_mask;		/* associated bit mask for this name */
} DBG_options, * DBG_opts;


/*
 * Internal debugging routines.
 */
extern	void		_Dbg_elf_data_in(Os_desc *, Is_desc *);
extern	void		_Dbg_elf_data_out(Os_desc *);
extern	void		_Dbg_ent_entry(Ent_desc * enp);
extern	void		_Dbg_reloc_run(void);
extern	void		_Dbg_seg_desc_entry(int, Sg_desc *);

#endif
