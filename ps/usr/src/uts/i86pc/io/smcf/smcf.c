/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 * All Rights Reserved.
 */

#ident	"@(#)smcf.c 1.1	95/07/18 SMI"

/*
 * smcf -- SMC UMAC driver for SMC Ether100 (9232) card
 *
 * This file should be identical in the PS and Realmode source trees.
 */

#define	__9232__
#define	SMC_INCLUDE "../smcf/smcf.h"

#ifdef	REALMODE
#include "../smcg/smcgrm.c"
#else
#include "../smcg/smcg.c"
#endif
