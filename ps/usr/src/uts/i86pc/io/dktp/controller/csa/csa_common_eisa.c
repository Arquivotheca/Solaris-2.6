/*
 * Copyright (c) 1995, Sun Microsystems, Inc.
 * All Rights Reserved.
 */

#pragma ident	"@(#)csa_common_eisa.c	1.3@(#)	1.3	95/06/05 SMI"

#include <sys/eisarom.h>
#include <sys/nvm.h>

#include "csa.h"

int	eisa_nvm(char *data, KEY_MASK key_mask, ...);

/*
 * check if an EISA NVRAM has the right type of board
 */
int
eisa_probe_nvm(
	ushort	slotadr,
	ulong	board_id,
	ulong	rev_mask
)
{
	struct	{
		short	slotnum;
		NVM_SLOTINFO	slot;
		NVM_FUNCINFO	func;
	} buff;
	int		slotnum = (slotadr >> 12);
	ulong		nvm_bid;
	KEY_MASK	key_mask = {0};

	key_mask.slot = TRUE;
	key_mask.function = TRUE;

	/* get the slot record and the first function record */
	if (!eisa_nvm((char *)&buff, key_mask, slotnum, 0)) {
		/* shouldn't happen, no functions */
		return (FALSE);
	}
	if (slotnum != buff.slotnum) {
		/* shouldn't happen, eisa nvram mismatch */
		return (FALSE);
	}
	nvm_bid = *((ulong *)(&buff.slot.boardid[0]));
	return ((nvm_bid & rev_mask) == (board_id & rev_mask));
}



/*
 * check if an EISA slot has the right type of board
 */
int
eisa_probe_slot(
	ushort	slotadr,
	ulong	board_id,
	ulong	rev_mask
)
{
	ulong	 bid;

	outb(slotadr + EISA_CFG0, 0xFF);	/* precharge the register */
	bid = inl(slotadr + EISA_CFG0);

	/* Slot is empty if high bit of low ID byte is set */
	if (bid & 0x80)
		return (FALSE);

	/* Check for match with a known board, ignore the revision number */
	return ((board_id & rev_mask) == (bid & rev_mask));
}



eisa_check_id(
	ushort		 ioaddr,
	pid_spec_t	*idp,
	int		 cnt
)
{
	int	idx;

	for (idx = 0; idx < cnt; idx++, idp++) {
		if (eisa_probe_nvm(ioaddr, idp->id, idp->mask)) {
			return (TRUE);
		}
	}
	return (FALSE);
}



Bool_t
eisa_probe(
	dev_info_t	*dip,
	ushort		 ioaddr
)
{
	pid_spec_t	*prod_idp;	/* ptr to the product id property */
	int		 prod_idlen;	/* length of the array */
	int		 rc;

	rc = ddi_getlongprop(DDI_DEV_T_ANY, dip, DDI_PROP_DONTPASS,
			     "product_id", (caddr_t)&prod_idp, &prod_idlen);

	if (rc != DDI_PROP_SUCCESS) {
		return (FALSE);
	}

	rc = eisa_check_id(ioaddr, prod_idp, prod_idlen / sizeof (pid_spec_t));
	kmem_free((caddr_t)prod_idp, prod_idlen);
	return (rc);
}



/*
 * Check the EISA function records for this slot one at a
 * time until we get the one that defines the irq.
 */
int
eisa_get_irq(
	ushort	 slotadr,
	unchar	*irqp
)
{
	struct	{
		short	slotnum;
		NVM_SLOTINFO	slot;
		NVM_FUNCINFO	func;
	} buff;
	int		function_num;
	int		slotnum = (slotadr >> 12);
	int		rc;
	KEY_MASK	key_mask = {0};

	key_mask.slot = TRUE;
	key_mask.function = TRUE;

	/* check all the functions for this slot until the irq is found */
	for (function_num = 0; ; function_num++) {
		/* get the slot record and the next function record */
		rc = eisa_nvm((char *)&buff, key_mask, slotnum, function_num);

		if (rc == 0) {
			/* end of functions, no irq defined */
			return (FALSE);
		}

		if (slotnum != buff.slotnum) {
			/* shouldn't happen */
			return (FALSE);
		}

		/* check if we got the irq function record */
		if (buff.func.fib.irq)
			break;
	}
	*irqp = buff.func.un.r.irq[0].line;
	return (TRUE);
}
