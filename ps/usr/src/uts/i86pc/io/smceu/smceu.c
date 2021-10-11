/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 * All Rights Reserved.
 */

#ident	"@(#)smceu.c 1.1	95/07/18 SMI"

/*
 * smceu -- SMC UMAC driver for SMC Elite Ultra (8232) card
 *
 * This file should be identical in the PS and Realmode source trees.
 */

#define	__8232__
#define	SMC_INCLUDE "../smceu/smceu.h"

#ifdef	REALMODE
#include "../smcg/smcgrm.c"
#else
#include "../smcg/smcg.c"
#endif
