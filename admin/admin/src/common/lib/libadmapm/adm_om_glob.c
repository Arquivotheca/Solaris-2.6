
/*
 *  Copyright (c) 1995 Sun Microsystems, Inc.  All Rights Reserved.
 * All rights reserved.
 */

/*
 *	adm_om_glob.c
 *
 *	This file contains definitions of common variables to aid in the
 *	creation of shared libraries
 *
 *	Dec 18, 1990		Jeff Parker
 *	Jan 12, 1995		Ken Jones
 */

#pragma	ident	"@(#)adm_om_glob.c	1.8	95/01/13 SMI"

#include "adm_om_impl.h"
#include "adm_cache.h"

char *adm_obj_path;
char *adm_obj_pathlist[MAXOBJPATHS];
char *target;				/* used to hold name for match */

adm_cache adm_om_cache;
int adm_caching_on = -1;	/* -1 = unint: 0 = off; 1 = on and init */
