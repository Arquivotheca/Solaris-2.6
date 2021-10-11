/*
 * Copyright (c) 1995-1996, by Sun Microsystems, Inc.
 * All rights reserved.
 */
#ifndef lint
#pragma ident "@(#)find_mod.h 1.1 96/05/31 SMI"
#endif

#ifndef _FIND_MOD_H
#define	_FIND_MOD_H

#include <pkgstrct.h>

#ifdef __cplusplus
extern "C" {
#endif

int	get_next_contents_entry(FILE *, struct cfent *);
Modinfo		*map_pinfo_to_modinfo(Product *, char *);

#ifdef __cplusplus
}
#endif

#endif	/* _FIND_MOD_H */
