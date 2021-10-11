/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 * All Rights Reserved.
 */

#ident	"@(#)9232.c 1.1	95/07/18 SMI"

/*
 * Solaris LMAC aux functions for SMC Ether100 (9232) driver
 *
 * This file should be identical in the PS and Realmode source trees.
 */

#include <sys/types.h>

#ifndef	REALMODE
#include <sys/ddi.h>
#include <sys/sunddi.h>
#else
#include "common.h"
#endif

#include "lft_macr.h"
#include "lmstruct.h"
#include "lft_eq.h"

#ifndef REALMODE
int
LM_Get_Addr(AdapterStructure *pAs)
{
	unsigned char bank;

	/* GetLANAddress switches to bank 0, so save bank before call. */
	bank = inb(pAs->BankSelect);
	GetLANAddress(pAs);
	outb(pAs->BankSelect, bank);
}
#endif /* REALMODE */

int
LM_Nextcard(AdapterStructure *pAd)
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
	if (inb(ioaddr) != (unchar)(EISA_MANUF_ID >> 8))
		goto again;
	if (inb(ioaddr+1) != (unchar)(EISA_MANUF_ID))
		goto again;
	if (inb(ioaddr+2) != (unchar)(EISA_BRD_9232 >> 8))
		goto again;
	if (inb(ioaddr+3) != (unchar)(EISA_BRD_9232))
		goto again;
	/*
	 * Now we're pretty sure it's ours -- we'll let LM_GetCnfig() do
	 * the rest of the validation.
	 */

	pAd->slot_num = slot;
	pAd->io_base = slot * 0x1000;

	return (0);
}
