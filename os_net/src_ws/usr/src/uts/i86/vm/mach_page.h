/*
 * Copyright (c) 1996 by Sun Microsystems, Inc.
 * All rights reserved.
 */

#ifndef _MACH_PAGE_H
#define	_MACH_PAGE_H

#pragma ident	"@(#)mach_page.h	1.5	96/05/23 SMI"

/*
 * The file contains the platform specific page structure
 */

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The PSM portion of a page structure
 */
typedef struct machpage {
	page_t	p_paget;		/* PIM portion of page_t */
	struct	hment *p_mapping;	/* hat specific translation info */
	u_int	p_pagenum;		/* physical page number */
	u_int	p_share;		/* number of mappings to this page */
	kcondvar_t p_mlistcv;		/* cond var for mapping list lock */
	u_int	p_inuse: 1,		/* page mapping list in use */
		p_wanted: 1,		/* page mapping list wanted */
		p_impl: 6;		/* hat implementation bits */
	u_char	p_nrm;			/* non-cache, ref, mod readonly bits */
} machpage_t;

/*
 * Each segment of physical memory is described by a memseg struct. Within
 * a segment, memory is considered contiguous. The segments from a linked
 * list to describe all of physical memory.
 */
struct memseg {
	machpage_t *pages, *epages;	/* [from, to] in page array */
	u_int pages_base, pages_end;	/* [from, to] in page numbers */
	struct memseg *next;		/* next segment in list */
};

extern struct memseg *memsegs;		/* list of memory segments */

void build_pfn_hash();

#ifdef __cplusplus
}
#endif

#endif /* _MACH_PAGE_H */
