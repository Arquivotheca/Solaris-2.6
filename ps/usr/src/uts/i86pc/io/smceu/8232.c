/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 * All Rights Reserved.
 */

#ident	"@(#)8232.c 1.1	95/07/18 SMI"

/*
 * Solaris LMAC aux functions for SMC Elite Ultra (8232) driver
 *
 * This file should be identical in the PS and Realmode source trees.
 */

#include <sys/types.h>

#ifndef REALMODE
#include <sys/ddi.h>
#include <sys/sunddi.h>
#else
#include "common.h"
#endif

#include "sebm.h"
#include "board_id.h"
#include "smchdw.h"
#include "lmstruct.h"
#include "ebm.h"

#ifndef EBM
#error "EBM must be defined for Solaris"
#endif

int sebm_debug = 0x0;
struct  sebmstat  sebmstats[4]; 	/* board statistics */

#ifndef REALMODE
int
LMB_Get_Addr(Adapter_Struc *pAd)
{
	Get_Node_Address(pAd->io_base, pAd->node_address);
}
#endif /* REALMODE */

void
sebmbcopy(caddr_t from, caddr_t to, short bytecount, short direction)
{
#ifdef REALMODE
	bcopy(to, from, bytecount);	/* Bagbiting realmode bcopy */
#else
	bcopy(from, to, bytecount);
#endif
}

int
LMB_Nextcard(Adapter_Struc *pAd)
{
	static slot = 0;
	ushort ioaddr;

	if (pAd->pc_bus != EISA_BUS)
		return (-1);

again:
	/* EISA card -- slots 1 to 15 legal */
	if (++slot > 15) {
		slot = 0;
		return (-1);
	}

	/*
	 * Do our best to minimize the amount of I/O we do to addresses
	 * we aren't yet sure are ours.
	 *
	 * Read manufacturing ID at I/O address (0zc80 - 0zc83)
	 */

	ioaddr = slot * 0x1000 + 0xc80;
	if (inb(ioaddr) != (unchar)(MANUF_ID >> 8))
		goto again;
	if (inb(ioaddr+1) != (unchar)(MANUF_ID))
		goto again;
	if (inb(ioaddr+2) != (unchar)(EISA_BRD_ID >> 8))
		goto again;
	if (inb(ioaddr+3) != (unchar)(EISA_BRD_ID))
		goto again;
	/*
	 * Now we're pretty sure it's ours -- we'll let LM_GetCnfig() do
	 * the rest of the validation.
	 */

	pAd->slot_num = slot;
	pAd->io_base = slot * 0x1000;

	return (0);
}
